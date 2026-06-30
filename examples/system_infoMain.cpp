// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/hwinfo.h>
#include <hwinfo/utils/unit.h>
#include <hwinfo/utils/stringutils.h>

#include <iomanip>
#include <iostream>
#include <vector>

using namespace hwinfo::unit;

int main(/*int argc, char** argv*/) {
  std::cout << "hwinfo is an open source, MIT licensed project that implements a platform independent "
               "hardware and system information gathering API for C++.\n\n"
               "If you face any issues, find bugs or if your platform is not supported yet, do not hesitate to create "
               "a ticket at https://github.com/lfreist/hwinfo/issues.\n"
            << std::endl;

  std::cout << "Hardware Report:\n"
            << "----------------------------------- CPU ------------------------------------\n";
  const auto cpus = hwinfo::getAllCPUs();
  for (const auto& cpu : cpus) {
    std::cout << "Socket " << cpu.id() << "\n"
              << std::left << std::setw(20) << " vendor: " << cpu.vendor() << "\n"
              << std::left << std::setw(20) << " model: " << cpu.modelName() << "\n"
              << std::left << std::setw(20) << " physical cores: " << cpu.numPhysicalCores() << "\n"
              << std::left << std::setw(20) << " logical cores: " << cpu.numLogicalCores() << "\n"
              << std::left << std::setw(20) << " cores: " << cpu.numPhysicalCores() << " (" << cpu.numLogicalCores()
              << ")\n";
    for (const auto& core : cpu.cores()) {
      std::cout << std::left << std::setw(20) << ("  core " + std::to_string(core.id) + ":") << "\n"
                << std::left << std::setw(20) << "   SMT: " << (core.smt ? "yes" : "no") << "\n"
                << std::left << std::setw(20) << "   cache:" << "\n"
                << std::left << std::setw(20)
                << "    L1 Data:" << unit_prefix_to(core.cache.l1_data, IECPrefix::KIBI) << " KiB\n"
                << std::left << std::setw(20)
                << "    L1 Inst.:" << unit_prefix_to(core.cache.l1_instruction, IECPrefix::KIBI) << " KiB\n"
                << std::left << std::setw(20) << "    L2:" << unit_prefix_to(core.cache.l2, IECPrefix::KIBI)
                << " KiB\n"
                << std::left << std::setw(20) << "    L3:" << unit_prefix_to(core.cache.l3, IECPrefix::KIBI)
                << " KiB\n"
                << std::left << std::setw(20)
                << "   regular freq.:" << unit_prefix_to(core.regular_frequency_hz, SiPrefix::MEGA) << " MHz\n"
                << std::left << std::setw(20)
                << "   max freq.:" << unit_prefix_to(core.max_frequency_hz, SiPrefix::MEGA) << " MHz\n";
    }
  }

  hwinfo::OS os;
  std::cout << "----------------------------------- OS ------------------------------------\n"
            << std::left << std::setw(20) << "name: " << os.name() << "\n"
            << std::left << std::setw(20) << "version: " << os.version() << "\n"
            << std::left << std::setw(20) << "kernel: " << os.kernel() << "\n"
            << std::left << std::setw(20) << "bitness: " << (os.is32bit() ? "32 bit" : "64 bit") << "\n"
            << std::left << std::setw(20) << "endianess: " << (os.isLittleEndian() ? "little endian" : "big endian")
            << "\n";

  auto gpus = hwinfo::getAllGPUs();
  std::cout << "----------------------------------- GPU -----------------------------------\n";
  for (const auto& gpu : gpus) {
    std::cout << "GPU " << gpu.id() << "\n"
              << std::left << std::setw(20) << " vendor: " << gpu.vendor() << "\n"
              << std::left << std::setw(20) << " model: " << gpu.name() << "\n"
              << std::left << std::setw(20) << " driver version: " << gpu.driverVersion() << "\n"
              << std::left << std::setw(20)
              << " dedicated memory: " << unit_prefix_to(gpu.dedicated_memory_Bytes(), IECPrefix::GIBI) << " GiB\n"
              << std::left << std::setw(20)
              << " shared memory: " << unit_prefix_to(gpu.shared_memory_Bytes(), IECPrefix::GIBI) << " GiB\n"
              << std::left << std::setw(20) << " frequency: " << unit_prefix_to(gpu.frequency_hz(), SiPrefix::MEGA)
              << " MHz\n"
              << std::left << std::setw(20) << " cores: " << gpu.num_cores() << "\n"
              << std::left << std::setw(20) << " vendor_id: " << gpu.vendor_id() << "\n"
              << std::left << std::setw(20) << " device_id: " << gpu.device_id() << "\n";
  }

  hwinfo::Memory memory;
  std::cout << "----------------------------------- RAM -----------------------------------\n"
            << std::left << std::setw(20) << "size: " << unit_prefix_to(memory.size(), IECPrefix::GIBI) << "\n"
            << std::left << std::setw(20) << "free: " << unit_prefix_to(memory.free(), IECPrefix::GIBI) << "\n"
            << std::left << std::setw(20)
            << "available: " << unit_prefix_to(memory.available(), IECPrefix::GIBI) << "\n";
  for (const auto& module : memory.modules()) {
    std::cout << std::left << std::setw(20) << (" RAM " + std::to_string(module.id)) << "\n"
              << std::left << std::setw(20) << "  vendor: " << module.vendor << "\n"
              << std::left << std::setw(20) << "  model: " << module.model << "\n"
              << std::left << std::setw(20) << "  name: " << module.name << "\n"
              << std::left << std::setw(20) << "  serial number: " << module.serial_number << "\n"
              << std::left << std::setw(20) << "  frequency: " << unit_prefix_to(module.frequency_hz, SiPrefix::MEGA)
              << "\n";
  }

  hwinfo::MainBoard main_board;
  std::cout << "------------------------------- Main Board --------------------------------\n"
            << std::left << std::setw(20) << "vendor: " << main_board.vendor() << "\n"
            << std::left << std::setw(20) << "name: " << main_board.name() << "\n"
            << std::left << std::setw(20) << "version: " << main_board.version() << "\n"
            << std::left << std::setw(20) << "serial number: " << main_board.serialNumber() << "\n";

  std::vector<hwinfo::Battery> batteries = hwinfo::getAllBatteries();

  std::cout << "------------------------------- Batteries ---------------------------------\n";
  if (!batteries.empty()) {
    for (auto& battery : batteries) {
      std::cout << std::left << std::setw(20) << ("Battery " + std::to_string(battery.id()) + ":") << "\n"
                << std::left << std::setw(20) << " vendor: " << battery.vendor() << "\n"
                << std::left << std::setw(20) << " model: " << battery.model() << "\n"
                << std::left << std::setw(20) << " serial number: " << battery.serialNumber() << "\n"
                << std::left << std::setw(20) << " state: " << battery.state() << "\n"
                << std::left << std::setw(20) << " capacity: " << battery.capacity() << "\n";
    }
  } else {
    std::cout << "No Batteries installed or detected\n";
  }

  std::vector<hwinfo::Disk> disks = hwinfo::getAllDisks();
  std::cout << "--------------------------------- Disks -----------------------------------\n";
  if (!disks.empty()) {
    for (const auto& disk : disks) {
      std::cout << std::left << std::setw(20) << ("Disk " + std::to_string(disk.id()) + ":") << "\n"
                << std::left << std::setw(20) << " vendor: " << disk.vendor() << "\n"
                << std::left << std::setw(20) << " model: " << disk.model() << "\n"
                << std::left << std::setw(20) << " serial number: " << disk.serial_number() << "\n"
                << std::left << std::setw(20) << " interface: " << disk.disk_interface() << "\n"
                << std::left << std::setw(20) << " size: " << unit_prefix_to(disk.size(), IECPrefix::GIBI) << " GiB\n"
                << std::left << std::setw(20) << " mount points: " << hwinfo::utils::join(disk.mount_points(), ", ") << "\n";
    }
  } else {
    std::cout << "No Disks installed or detected\n";
  }

  std::vector<hwinfo::Network> networks = hwinfo::getAllNetworks();
  std::cout << "--------------------------------- Networks -----------------------------------\n";
  if (!networks.empty()) {
    int network_counter = 0;
    for (const auto& network : networks) {
      if (!network.ip4().empty() || !network.ip6().empty()) {
        std::cout << std::left << std::setw(20) << ("Network " + std::to_string(network_counter++) + ":") << "\n"
                  << std::left << std::setw(20) << " description: " << network.description() << "\n"
                  << std::left << std::setw(20) << " interface index: " << network.interfaceIndex() << "\n"
                  << std::left << std::setw(20) << " mac: " << network.mac() << "\n"
                  << std::left << std::setw(20) << " ipv4 address: " << network.ip4() << "\n"
                  << std::left << std::setw(20) << " ipv6 address: " << network.ip6() << "\n";
      }
    }
  } else {
    std::cout << "No Networks installed or detected\n";
  }
  return 0;
}
