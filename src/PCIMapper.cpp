/**
 * Copyright 2023, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of hwinfo.
 */

#include <hwinfo/PCIMapper.h>
#include <hwinfo/utils/stringutils.h>

#include <fstream>
#include <stdexcept>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
PCIDevice::PCIDevice(std::string d_id, std::string d_name)
    : device_id(std::move(d_id)), device_name(std::move(d_name)) {}

// _____________________________________________________________________________________________________________________
PCIVendor::PCIVendor(std::string v_id, std::string v_name)
    : vendor_id(std::move(v_id)), vendor_name(std::move(v_name)) {}

// _____________________________________________________________________________________________________________________
const PCIDevice& PCIVendor::operator[](const std::string& device_id) const {
  std::string d_id =
      utils::starts_with(device_id, "0x") ? std::string(device_id.begin() + 2, device_id.end()) : device_id;
  if (devices.find(d_id) == devices.end()) {
    return _invalid_device;
  }
  return devices.at(d_id);
}

// _____________________________________________________________________________________________________________________
PCIMapper::PCIMapper(const std::string& pci_ids_file) {
  std::ifstream f_pciid(pci_ids_file);
  if (!f_pciid) {
    throw std::runtime_error("ERROR: Could not open file '" + pci_ids_file + "'.\n");
  }
  PCIVendor* current_vendor = nullptr;
  PCIDevice* current_device = nullptr;
  while (true) {
    std::string line;
    if (!std::getline(f_pciid, line)) {
      break;
    }
    if (utils::starts_with(line, "#") || utils::starts_with(line, "\n") || line.empty()) {
      // empty line or comment
      continue;
    }
    if (utils::starts_with(line, "\t\t")) {
      // subsystem entry
      utils::strip(line);
      auto split_line = utils::split(line, "  ");
      if (split_line.size() != 2 || current_device == nullptr) {
        // parse error
        continue;
      }
      current_device->subsystems.insert({split_line[0], split_line[1]});
    } else if (utils::starts_with(line, "\t")) {
      // device
      utils::strip(line);
      auto split_line = utils::split(line, "  ");
      if (split_line.size() != 2 || current_vendor == nullptr) {
        // parse error
        continue;
      }
      current_vendor->devices.insert({split_line[0], {split_line[0], split_line[1]}});
      current_device = &current_vendor->devices.at(split_line[0]);
    } else {
      // vendor
      utils::strip(line);
      auto split_line = utils::split(line, "  ");
      if (split_line.size() != 2) {
        // parse error
        continue;
      }
      _vendors.insert({split_line[0], {split_line[0], split_line[1]}});
      current_vendor = &_vendors.at(split_line[0]);
    }
  }
}

// _____________________________________________________________________________________________________________________
const PCIVendor& PCIMapper::vendor_from_id(const std::string& vendor_id) const {
  std::string v_id =
      utils::starts_with(vendor_id, "0x") ? std::string(vendor_id.begin() + 2, vendor_id.end()) : vendor_id;
  if (_vendors.find(v_id) == _vendors.end()) {
    return _invalid_vendor;
  }
  return _vendors.at(v_id);
}

// _____________________________________________________________________________________________________________________
const PCIVendor& PCIMapper::operator[](const std::string& vendor_id) const { return vendor_from_id(vendor_id); }

// _____________________________________________________________________________________________________________________
PCIMapper PCI::getMapper() {
  static std::string path(std::getenv("HOME"));
  static PCIMapper mapper(path + "/.hwinfo/pci.ids");
  return mapper;
}

}  // namespace hwinfo