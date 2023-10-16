// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <hwinfo/disk.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  utils::WMI::_WMI wmi;
  const std::wstring query_string(
      L"SELECT Model, Manufacturer, SerialNumber, Size "
      L"FROM Win32_DiskDrive");
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }
  std::vector<Disk> disks;

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  int disk_id = 0;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    std::cout << "processing new value... ";
    if (!u_return || obj == nullptr) {
      std::cout << "Failed" << std::endl;
      break;
    }
    std::cout << "Succeeded" << std::endl;
    Disk disk;
    disk._id = disk_id++;
    std::cout << "Disk " << disk_id - 1 << std::endl;
    VARIANT vt_prop;
    obj->Get(L"Model", 0, &vt_prop, nullptr, nullptr);
    disk._model = utils::wstring_to_std_string(((bstr_t)vt_prop.bstrVal).copy());
    obj->Get(L"Manufacturer", 0, &vt_prop, nullptr, nullptr);
    disk._vendor = utils::wstring_to_std_string(((bstr_t)vt_prop.bstrVal).copy());
    obj->Get(L"SerialNumber", 0, &vt_prop, nullptr, nullptr);
    disk._serialNumber = utils::wstring_to_std_string(((bstr_t)vt_prop.bstrVal).copy());
    obj->Get(L"Size", 0, &vt_prop, nullptr, nullptr);
    disk._size_Bytes = std::stoll(utils::wstring_to_std_string(((bstr_t)vt_prop.bstrVal).copy()));
    std::cout << "Added Size" << std::endl;
    VariantClear(&vt_prop);
    obj->Release();
    disks.push_back(std::move(disk));
    std::cout << "Disk added!" << std::endl;
  }
  std::cout << disks.size() << " Disks found." << std::endl;
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS