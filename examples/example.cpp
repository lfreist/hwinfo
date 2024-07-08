// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/hwinfo.h>

#include <iomanip>
#include <iostream>
#include <vector>

#include "hwinfo/utils/PCIMapper.h"

int main(int argc, char** argv) {
  const auto width = 20;

  std::cout << "hwinfo is an open source, MIT licensed project that implements a platform independent "
            << "hardware and system information gathering API for C++.\n\n"
            << "If you face any issues, find bugs or if your platform is not supported yet, do not hesitate to create "
            << "a ticket at https://github.com/lfreist/hwinfo/issues.\n\n"
            << "Thanks for using hwinfo!" << '\n';
  std::cout << '\n' << "Hardware Report:" << '\n' << '\n';
  std::cout << "----------------------------------- CPU -----------------------------------" << '\n';
  auto cpus = hwinfo::getAllCPUs();
  for (const auto& cpu : cpus) {
    std::cout << "Socket " << cpu.id() << ":\n";
    std::cout << std::left << std::setw(width) << " vendor:";
    std::cout << cpu.vendor() << '\n';
    std::cout << std::left << std::setw(width) << " model:";
    std::cout << cpu.modelName() << '\n';
    std::cout << std::left << std::setw(width) << " physical cores:";
    std::cout << cpu.numPhysicalCores() << '\n';
    std::cout << std::left << std::setw(width) << " logical cores:";
    std::cout << cpu.numLogicalCores() << '\n';
    std::cout << std::left << std::setw(width) << " max frequency:";
    std::cout << cpu.maxClockSpeed_MHz() << '\n';
    std::cout << std::left << std::setw(width) << " regular frequency:";
    std::cout << cpu.regularClockSpeed_MHz() << '\n';
    std::cout << std::left << std::setw(width) << " cache size (L1, L2, L3): ";
    std::cout << cpu.L1CacheSize_Bytes() << ", " << cpu.L2CacheSize_Bytes() << ", " << cpu.L3CacheSize_Bytes() << '\n';
    auto threads_utility = cpu.threadsUtilisation();
    auto threads_speed = cpu.currentClockSpeed_MHz();
    for (int thread_id = 0; thread_id < threads_utility.size(); ++thread_id) {
      std::cout << std::left << std::setw(width) << "   Thread " + std::to_string(thread_id) + ": ";
      std::cout << threads_speed[thread_id] << " MHz (" << threads_utility[thread_id] * 100 << "%)" << '\n';
    }
    // std::cout << cpu.currentTemperature_Celsius() << '\n';
  }

  hwinfo::OS os;
  std::cout << "----------------------------------- OS ------------------------------------" << '\n';
  std::cout << std::left << std::setw(width) << "Operating System:";
  std::cout << os.name() << '\n';
  std::cout << std::left << std::setw(width) << "version:";
  std::cout << os.version() << '\n';
  std::cout << std::left << std::setw(width) << "kernel:";
  std::cout << os.kernel() << '\n';
  std::cout << std::left << std::setw(width) << "architecture:";
  std::cout << (os.is32bit() ? "32 bit" : "64 bit") << '\n';
  std::cout << std::left << std::setw(width) << "endianess:";
  std::cout << (os.isLittleEndian() ? "little endian" : "big endian") << '\n';

  auto gpus = hwinfo::getAllGPUs();
  std::cout << "----------------------------------- GPU -----------------------------------" << '\n';
  for (auto& gpu : gpus) {
    std::cout << "GPU " << gpu.id() << ":\n";
    std::cout << std::left << std::setw(width) << "  vendor:";
    std::cout << gpu.vendor() << '\n';
    std::cout << std::left << std::setw(width) << "  model:";
    std::cout << gpu.name() << '\n';
    std::cout << std::left << std::setw(width) << "  driverVersion:";
    std::cout << gpu.driverVersion() << '\n';
    std::cout << std::left << std::setw(width) << "  memory [MiB]:";
    std::cout << static_cast<double>(gpu.memory_Bytes()) / 1024.0 / 1024.0 << '\n';
    std::cout << std::left << std::setw(width) << "  frequency:";
    std::cout << gpu.frequency_MHz() << '\n';
    std::cout << std::left << std::setw(width) << "  cores:";
    std::cout << gpu.num_cores() << '\n';
    std::cout << std::left << std::setw(width) << "  vendor_id:";
    std::cout << gpu.vendor_id() << '\n';
    std::cout << std::left << std::setw(width) << "  device_id:";
    std::cout << gpu.device_id() << '\n';
  }

  hwinfo::Memory memory;
  std::cout << "----------------------------------- RAM -----------------------------------" << '\n';
  std::cout << std::left << std::setw(width) << "size [MiB]:";
  std::cout << memory.total_Bytes() / 1024 / 1024 << '\n';
  std::cout << std::left << std::setw(width) << "free [MiB]:";
  std::cout << memory.free_Bytes() / 1024 / 1024 << '\n';
  std::cout << std::left << std::setw(width) << "available [MiB]:";
  std::cout << memory.available_Bytes() / 1024 / 1024 << '\n';
  for (const auto& module : memory.modules()) {
    std::cout << "RAM " << module.id << ":\n";
    std::cout << std::left << std::setw(width) << "  vendor:";
    std::cout << module.vendor << '\n';
    std::cout << std::left << std::setw(width) << "  model:";
    std::cout << module.model << '\n';
    std::cout << std::left << std::setw(width) << "  name:";
    std::cout << module.name << '\n';
    std::cout << std::left << std::setw(width) << "  serial-number:";
    std::cout << module.serial_number << '\n';
    std::cout << std::left << std::setw(width) << "  Frequency [MHz]:";
    std::cout << module.frequency_Hz / 1000 / 1000 << '\n';
  }

  hwinfo::MainBoard main_board;
  std::cout << "------------------------------- Main Board --------------------------------" << '\n';
  std::cout << std::left << std::setw(width) << "vendor:";
  std::cout << main_board.vendor() << '\n';
  std::cout << std::left << std::setw(width) << "name:";
  std::cout << main_board.name() << '\n';
  std::cout << std::left << std::setw(width) << "version:";
  std::cout << main_board.version() << '\n';
  std::cout << std::left << std::setw(width) << "serial-number:";
  std::cout << main_board.serialNumber() << '\n';

  std::vector<hwinfo::Battery> batteries = hwinfo::getAllBatteries();
  std::cout << "------------------------------- Batteries ---------------------------------" << '\n';
  if (!batteries.empty()) {
    int battery_counter = 0;
    for (auto& battery : batteries) {
      std::cout << "Battery " << battery_counter++ << ":" << '\n';
      std::cout << std::left << std::setw(width) << "  vendor:";
      std::cout << battery.vendor() << '\n';
      std::cout << std::left << std::setw(width) << "  model:";
      std::cout << battery.model() << '\n';
      std::cout << std::left << std::setw(width) << "  serial-number:";
      std::cout << battery.serialNumber() << '\n';
      std::cout << std::left << std::setw(width) << "  charging:";
      std::cout << (battery.charging() ? "yes" : "no") << '\n';
      std::cout << std::left << std::setw(width) << "  capacity:";
      std::cout << battery.capacity() << '\n';
    }
    std::cout << "---------------------------------------------------------------------------" << '\n';
  } else {
    std::cout << "No Batteries installed or detected" << '\n';
  }

  std::vector<hwinfo::Disk> disks = hwinfo::getAllDisks();
  std::cout << "--------------------------------- Disks -----------------------------------" << '\n';
  if (!disks.empty()) {
    int disk_counter = 0;
    for (const auto& disk : disks) {
      std::cout << "Disk " << disk_counter++ << ":" << '\n';
      std::cout << std::left << std::setw(width) << "  vendor:";
      std::cout << disk.vendor() << '\n';
      std::cout << std::left << std::setw(width) << "  model:";
      std::cout << disk.model() << '\n';
      std::cout << std::left << std::setw(width) << "  serial-number:";
      std::cout << disk.serialNumber() << '\n';
      std::cout << std::left << std::setw(width) << "  size:";
      std::cout << disk.size_Bytes() << '\n';
    }
    std::cout << "---------------------------------------------------------------------------" << '\n';
  } else {
    std::cout << "No Disks installed or detected" << '\n';
  }
  return EXIT_SUCCESS;
}
