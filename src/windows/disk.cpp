// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <hwinfo/disk.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace hwinfo {

// -----------------------------------------------------------------------------
// Extracts the actual device path from a WMI reference string
// Example: "Win32_DiskDrive=\"\\\\.\\PHYSICALDRIVE0\"" → "\\\\.\\PHYSICALDRIVE0"
// -----------------------------------------------------------------------------
std::wstring extractDevicePath(const std::wstring& wmiRefString) {
  size_t startPos = wmiRefString.find(L"\"");
  if (startPos == std::wstring::npos) {
    return wmiRefString;
  }
  size_t endPos = wmiRefString.rfind(L"\"");
  if (endPos == std::wstring::npos || endPos <= startPos) {
    return wmiRefString;
  }
  return wmiRefString.substr(startPos + 1, endPos - startPos - 1);
}

// -----------------------------------------------------------------------------
// Normalize backslashes in paths to ensure consistent formatting
// Example: "\\\\.\\PHYSICALDRIVE0" → "\\.\PHYSICALDRIVE0"
// -----------------------------------------------------------------------------
std::wstring normalizeBackslashes(const std::wstring& path) {
  std::wstring result;
  bool lastWasBackslash = false;

  for (wchar_t c : path) {
    if (c == L'\\') {
      if (!lastWasBackslash) {
        result.push_back(c);
      }
      lastWasBackslash = true;
    } else {
      result.push_back(c);
      lastWasBackslash = false;
    }
  }
  return result;
};

// -----------------------------------------------------------------------------
// Computes free space for each physical disk
// Gathers mappings: logical drive → partition → physical disk
// -----------------------------------------------------------------------------
std::unordered_map<std::wstring, uint64_t> computePhysicalFreeSpace() {
  utils::WMI::_WMI wmi;
  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;

  // Maps for tracking relationships
  std::unordered_map<std::wstring, uint64_t> logicalDriveToFree;
  std::unordered_map<std::wstring, std::wstring> partitionToLogical;
  std::unordered_map<std::wstring, std::wstring> partitionToDisk;

  // -------------------------------------------------------------------------
  // Read logical disk free space
  // -------------------------------------------------------------------------
  if (wmi.execute_query(L"SELECT DeviceID, FreeSpace FROM Win32_LogicalDisk")) {
    while (wmi.enumerator) {
      wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
      if (!u_return || obj == nullptr) {
        break;
      }

      VARIANT vtDeviceID, vtFreeSpace;
      VariantInit(&vtDeviceID);
      VariantInit(&vtFreeSpace);

      if (SUCCEEDED(obj->Get(L"DeviceID", 0, &vtDeviceID, nullptr, nullptr)) &&
          SUCCEEDED(obj->Get(L"FreeSpace", 0, &vtFreeSpace, nullptr, nullptr))) {
        if (V_VT(&vtDeviceID) == VT_BSTR && V_VT(&vtFreeSpace) == VT_BSTR) {
          logicalDriveToFree[vtDeviceID.bstrVal] = std::stoll(utils::wstring_to_std_string(vtFreeSpace.bstrVal));
        }
      }
      VariantClear(&vtDeviceID);
      VariantClear(&vtFreeSpace);
      obj->Release();
    }
  }

  // -------------------------------------------------------------------------
  // Map logical drives to partitions
  // -------------------------------------------------------------------------
  if (wmi.execute_query(L"SELECT Antecedent, Dependent FROM Win32_LogicalDiskToPartition")) {
    while (wmi.enumerator) {
      wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
      if (!u_return || obj == nullptr) {
        break;
      }

      VARIANT vtPartition, vtLogical;
      VariantInit(&vtPartition);
      VariantInit(&vtLogical);

      if (SUCCEEDED(obj->Get(L"Antecedent", 0, &vtPartition, nullptr, nullptr)) &&
          SUCCEEDED(obj->Get(L"Dependent", 0, &vtLogical, nullptr, nullptr))) {
        if (V_VT(&vtPartition) == VT_BSTR && V_VT(&vtLogical) == VT_BSTR) {
          std::wstring logicalDrive = vtLogical.bstrVal;
          size_t startPos = logicalDrive.find(L"\"");
          size_t endPos = logicalDrive.rfind(L"\"");

          if (startPos != std::wstring::npos && endPos > startPos) {
            partitionToLogical[vtPartition.bstrVal] = logicalDrive.substr(startPos + 1, endPos - startPos - 1);
          }
        }
      }
      VariantClear(&vtPartition);
      VariantClear(&vtLogical);
      obj->Release();
    }
  }

  // -------------------------------------------------------------------------
  // Map partitions to physical disks
  // -------------------------------------------------------------------------
  if (wmi.execute_query(L"SELECT Antecedent, Dependent FROM Win32_DiskDriveToDiskPartition")) {
    while (wmi.enumerator) {
      wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
      if (!u_return || obj == nullptr) {
        break;
      }

      VARIANT vtDisk, vtPartition;
      VariantInit(&vtDisk);
      VariantInit(&vtPartition);

      if (SUCCEEDED(obj->Get(L"Antecedent", 0, &vtDisk, nullptr, nullptr)) &&
          SUCCEEDED(obj->Get(L"Dependent", 0, &vtPartition, nullptr, nullptr))) {
        if (V_VT(&vtDisk) == VT_BSTR && V_VT(&vtPartition) == VT_BSTR) {
          partitionToDisk[vtPartition.bstrVal] = extractDevicePath(vtDisk.bstrVal);
        }
      }
      VariantClear(&vtDisk);
      VariantClear(&vtPartition);
      obj->Release();
    }
  }

  // -------------------------------------------------------------------------
  // Aggregate free space for each physical disk
  // -------------------------------------------------------------------------
  std::unordered_map<std::wstring, uint64_t> physicalDiskToFree;

  for (const auto& [partitionName, logicalDriveName] : partitionToLogical) {
    auto diskIter = partitionToDisk.find(partitionName);
    if (diskIter != partitionToDisk.end()) {
      auto freeIter = logicalDriveToFree.find(logicalDriveName);
      if (freeIter != logicalDriveToFree.end()) {
        physicalDiskToFree[diskIter->second] += freeIter->second;
      }
    }
  }

  return physicalDiskToFree;
}

// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  utils::WMI::_WMI wmi;
  const std::wstring query_string(L"SELECT Model, Manufacturer, SerialNumber, Size, DeviceID FROM Win32_DiskDrive");

  if (!wmi.execute_query(query_string)) {
    return {};
  }

  std::vector<Disk> disks;
  auto physicalFreeSize = computePhysicalFreeSpace();

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  int disk_id = 0;

  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return || obj == nullptr) {
      break;
    }

    Disk disk;
    disk._id = disk_id++;

    VARIANT vt_prop;
    HRESULT hr;

    hr = obj->Get(L"Model", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_BSTR)) {
      disk._model = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    VariantClear(&vt_prop);

    hr = obj->Get(L"Manufacturer", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_BSTR)) {
      disk._vendor = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    VariantClear(&vt_prop);

    hr = obj->Get(L"SerialNumber", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_BSTR)) {
      disk._serialNumber = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    VariantClear(&vt_prop);

    hr = obj->Get(L"Size", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_BSTR)) {
      disk._size_Bytes = std::stoll(utils::wstring_to_std_string(vt_prop.bstrVal));
    }
    VariantClear(&vt_prop);

    // Match the free space for this device (if found in aggregated map)
    hr = obj->Get(L"DeviceID", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_BSTR)) {
      auto deviceIdW = vt_prop.bstrVal;
      VariantClear(&vt_prop);

      for (const auto& [rawKey, space] : physicalFreeSize) {
        auto normRawKey = normalizeBackslashes(rawKey);
        auto normDeviceIdW = normalizeBackslashes(deviceIdW);

        if (normRawKey == normDeviceIdW) {
          disk._free_size_Bytes = space;
        }
      }
    }

    obj->Release();
    disks.push_back(std::move(disk));
  }

  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
