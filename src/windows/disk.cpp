// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <winioctl.h>
#include <propsys.h>
#include <shobjidl.h>
// clang-format on
#include <hwinfo/disk.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/strings.h"
#include "internal/windows_com.h"
#include "internal/windows_strings.h"

#ifdef _MSC_VER
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "shell32.lib")
#endif

namespace hwinfo {

namespace {

// Device numbers probed as \\.\PhysicalDriveN.
// Numbers can have gaps (removed devices).
constexpr std::uint32_t max_physical_drives = 64;

// Handle closed on destruction.
class Handle {
 public:
  explicit Handle(HANDLE handle) : _handle(handle) {}
  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;
  ~Handle() {
    if (valid()) {
      CloseHandle(_handle);
    }
  }

  [[nodiscard]] bool valid() const noexcept { return _handle != INVALID_HANDLE_VALUE; }
  [[nodiscard]] HANDLE get() const noexcept { return _handle; }

 private:
  HANDLE _handle;
};

Handle open_device(const std::wstring& path) {
  return Handle(CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
}

DiskBus disk_bus(STORAGE_BUS_TYPE bus) {
  switch (bus) {
    case BusTypeNvme:
      return DiskBus::nvme;
    case BusTypeSata:
    case BusTypeAta:
      return DiskBus::sata;
    case BusTypeScsi:
    case BusTypeSas:
    case BusTypeiScsi:
    case BusTypeFibre:
      return DiskBus::scsi;
    case BusTypeUsb:
      return DiskBus::usb;
    case BusTypeSd:
    case BusTypeMmc:
      return DiskBus::mmc;
    default:
      return DiskBus::unknown;
  }
}

// Null terminated string at `offset` of the descriptor buffer, trimmed.
std::optional<std::string> descriptor_string(const std::vector<char>& buffer, DWORD length, DWORD offset) {
  if (offset == 0 || offset >= length) {
    return std::nullopt;
  }
  const char* begin = buffer.data() + offset;
  return internal::non_empty(std::string_view(begin, strnlen(begin, length - offset)));
}

// Some drivers (NVMe, ATA) report no or a generic vendor: guess it from the model.
std::optional<std::string> vendor_from_model(std::string_view model) {
  constexpr std::array<std::string_view, 12> vendors{"SAMSUNG", "WESTERN DIGITAL", "WDC",      "WD",
                                                     "SEAGATE", "KIOXIA",          "TOSHIBA",  "INTEL",
                                                     "CRUCIAL", "MICRON",          "KINGSTON", "SANDISK"};
  std::string upper(model);
  std::ranges::transform(upper, upper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  for (const auto vendor : vendors) {
    if (upper.starts_with(vendor) || upper.contains(std::format(" {}", vendor))) {
      return std::string(vendor);
    }
  }
  return std::nullopt;
}

// Filesystem of the volume mounted at `root` ("C:\"), e.g. "NTFS", "FAT32", "exFAT", "ReFS".
std::optional<std::string> filesystem_name(const wchar_t* root) {
  std::array<wchar_t, MAX_PATH + 1> name{};
  if (!GetVolumeInformationW(root, nullptr, 0, nullptr, nullptr, nullptr, name.data(),
                             static_cast<DWORD>(name.size()))) {
    return std::nullopt;
  }
  return internal::non_empty(internal::to_utf8(name.data()));
}

// Volume GUID path of the volume mounted at `root`, e.g. "\\?\Volume{4c1b02c1-d990-11dc-99ae-806e6f6e6963}\".
std::optional<std::string> volume_name(const wchar_t* root) {
  std::array<wchar_t, 64> name{};
  if (!GetVolumeNameForVolumeMountPointW(root, name.data(), static_cast<DWORD>(name.size()))) {
    return std::nullopt;
  }
  return internal::non_empty(internal::to_utf8(name.data()));
}

// BitLocker state of the volume mounted at `root` from the shell property System.Volume.BitLockerProtection, which,
// unlike Win32_EncryptableVolume, needs no administrator rights. Its values are not documented; as Explorer shows
// them: 1 on, 2 off, 3 encrypting, 4 decrypting, 5 suspended (encrypted, key stored in the clear), 6 locked. Volumes
// without BitLocker support have no value.
std::optional<bool> bitlocker_encrypted(const wchar_t* root) {
  PROPERTYKEY key{};
  if (FAILED(PSGetPropertyKeyFromName(L"System.Volume.BitLockerProtection", &key))) {
    return std::nullopt;
  }
  internal::ComPtr<IPropertyStore> store;
  if (FAILED(
          SHGetPropertyStoreFromParsingName(root, nullptr, GPS_DEFAULT, __uuidof(IPropertyStore), store.put_void()))) {
    return std::nullopt;
  }
  PROPVARIANT value;
  PropVariantInit(&value);
  if (FAILED(store->GetValue(key, &value))) {
    return std::nullopt;
  }
  std::optional<bool> encrypted;
  if (value.vt == VT_EMPTY) {
    encrypted = false;
  } else if (value.vt == VT_I4 || value.vt == VT_UI4) {
    switch (value.vt == VT_I4 ? value.lVal : static_cast<LONG>(value.ulVal)) {
      case 1:
      case 5:
      case 6:
        encrypted = true;
        break;
      case 0:
      case 2:
        encrypted = false;
        break;
      default:  // in transition (3, 4) or unknown
        break;
    }
  }
  PropVariantClear(&value);
  return encrypted;
}

// Device number -> drive letters ("C:\") of the volumes on that disk.
std::map<DWORD, std::vector<MountPoint>> drive_letters() {
  std::map<DWORD, std::vector<MountPoint>> mapping;
  std::array<wchar_t, 512> drives{};
  const DWORD length = GetLogicalDriveStringsW(static_cast<DWORD>(drives.size()), drives.data());
  if (length == 0 || length > drives.size()) {
    return mapping;
  }
  for (const wchar_t* root = drives.data(); *root != L'\0'; root += std::wcslen(root) + 1) {
    const UINT type = GetDriveTypeW(root);
    if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE) {
      continue;
    }
    const Handle volume = open_device(std::format(L"\\\\.\\{}", std::wstring_view(root, 2)));  // "\\.\C:"
    STORAGE_DEVICE_NUMBER number{};
    DWORD returned = 0;
    if (volume.valid() && DeviceIoControl(volume.get(), IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &number,
                                          sizeof(number), &returned, nullptr)) {
      mapping[number.DeviceNumber].push_back({
          .path = root,
          .filesystem = filesystem_name(root),
          .volume = volume_name(root),
          .encrypted = bitlocker_encrypted(root),
      });
    }
  }
  return mapping;
}

}  // namespace

result<std::vector<Disk>> disks() {
  const internal::ComInit com;  // for the shell property store
  auto mount_points = drive_letters();

  std::vector<Disk> result;
  for (std::uint32_t i = 0; i < max_physical_drives; ++i) {
    const Handle device = open_device(std::format(L"\\\\.\\PhysicalDrive{}", i));
    if (!device.valid()) {
      continue;
    }
    // index: N of \\.\PhysicalDriveN
    Disk disk{.index = i};

    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    std::vector<char> buffer(4096, '\0');
    DWORD returned = 0;
    if (DeviceIoControl(device.get(), IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer.data(),
                        static_cast<DWORD>(buffer.size() - 1), &returned, nullptr) &&
        returned >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
      STORAGE_DEVICE_DESCRIPTOR descriptor{};
      std::memcpy(&descriptor, buffer.data(), sizeof(descriptor));
      disk.bus = disk_bus(descriptor.BusType);
      disk.model = descriptor_string(buffer, returned, descriptor.ProductIdOffset);
      disk.serial_number = descriptor_string(buffer, returned, descriptor.SerialNumberOffset);
      disk.vendor = descriptor_string(buffer, returned, descriptor.VendorIdOffset);
      if ((!disk.vendor || disk.vendor == "ATA" || disk.vendor == "NVMe") && disk.model) {
        disk.vendor = vendor_from_model(*disk.model);
      }
    }

    DISK_GEOMETRY_EX geometry{};
    if (DeviceIoControl(device.get(), IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &geometry, sizeof(geometry),
                        &returned, nullptr)) {
      disk.size = Bytes{static_cast<std::uint64_t>(geometry.DiskSize.QuadPart)};
    }

    if (const auto it = mount_points.find(i); it != mount_points.end()) {
      disk.mount_points = std::move(it->second);
    }
    result.push_back(std::move(disk));
  }
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
