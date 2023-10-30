// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/hwinfo.h>

#include <iomanip>
#include <iostream>
#include <vector>

#include "hwinfo/utils/PCIMapper.h"

int main(int argc, char** argv) {
  std::cout << "hwinfo is an open source, MIT licensed project that implements a platform independent "
            << "hardware and system information gathering API for C++.\n\n"
            << "If you face any issues, find bugs or if your platform is not supported yet, do not hesitate to create "
            << "a ticket at https://github.com/lfreist/hwinfo/issues.\n\n"
            << "Thanks for using hwinfo!" << std::endl;
  std::cout << std::endl << "Hardware Report:" << std::endl << std::endl;
  std::cout << "----------------------------------- CPU -----------------------------------" << std::endl;
  auto cpus = hwinfo::getAllCPUs();
  for (const auto& cpu : cpus) {
    std::cout << "Socket " << cpu.id() << ":\n";
    std::cout << std::left << std::setw(20) << " vendor:";
    std::cout << cpu.vendor() << std::endl;
    std::cout << std::left << std::setw(20) << " model:";
    std::cout << cpu.modelName() << std::endl;
    std::cout << std::left << std::setw(20) << " physical cores:";
    std::cout << cpu.numPhysicalCores() << std::endl;
    std::cout << std::left << std::setw(20) << " logical cores:";
    std::cout << cpu.numLogicalCores() << std::endl;
    std::cout << std::left << std::setw(20) << " max frequency:";
    std::cout << cpu.maxClockSpeed_MHz() << std::endl;
    std::cout << std::left << std::setw(20) << " regular frequency:";
    std::cout << cpu.regularClockSpeed_MHz() << std::endl;
    std::cout << std::left << std::setw(20) << " cache size (L1, L2, L3): ";
    std::cout << cpu.L1CacheSize_Bytes() << ", " << cpu.L2CacheSize_Bytes() << ", " << cpu.L3CacheSize_Bytes()
              << std::endl;
    auto threads_utility = cpu.threadsUtilisation();
    auto threads_speed = cpu.currentClockSpeed_MHz();
    for (int thread_id = 0; thread_id < threads_utility.size(); ++thread_id) {
      std::cout << std::left << std::setw(20) << "   Thread " + std::to_string(thread_id) + ": ";
      std::cout << threads_speed[thread_id] << " MHz (" << threads_utility[thread_id] * 100 << "%)" << std::endl;
    }
    // std::cout << cpu.currentTemperature_Celsius() << std::endl;
  }

  hwinfo::OS os;
  std::cout << "----------------------------------- OS ------------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "Operating System:";
  std::cout << os.name() << std::endl;
  std::cout << std::left << std::setw(20) << "version:";
  std::cout << os.version() << std::endl;
  std::cout << std::left << std::setw(20) << "kernel:";
  std::cout << os.kernel() << std::endl;
  std::cout << std::left << std::setw(20) << "architecture:";
  std::cout << (os.is32bit() ? "32 bit" : "64 bit") << std::endl;
  std::cout << std::left << std::setw(20) << "endianess:";
  std::cout << (os.isLittleEndian() ? "little endian" : "big endian") << std::endl;

  auto gpus = hwinfo::getAllGPUs();
  std::cout << "----------------------------------- GPU -----------------------------------" << std::endl;
  for (auto& gpu : gpus) {
    std::cout << "GPU " << gpu.id() << ":\n";
    std::cout << std::left << std::setw(20) << "  vendor:";
    std::cout << gpu.vendor() << std::endl;
    std::cout << std::left << std::setw(20) << "  model:";
    std::cout << gpu.name() << std::endl;
    std::cout << std::left << std::setw(20) << "  driverVersion:";
    std::cout << gpu.driverVersion() << std::endl;
    std::cout << std::left << std::setw(20) << "  memory [MiB]:";
    std::cout << static_cast<double>(gpu.memory_Bytes()) / 1024.0 / 1024.0 << std::endl;
    std::cout << std::left << std::setw(20) << "  frequency:";
    std::cout << gpu.frequency_MHz() << std::endl;
    std::cout << std::left << std::setw(20) << "  cores:";
    std::cout << gpu.num_cores() << std::endl;
  }

  hwinfo::Memory memory;
  std::cout << "----------------------------------- RAM -----------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "size [MiB]:";
  std::cout << memory.total_Bytes() / 1024 / 1024 << std::endl;
  std::cout << std::left << std::setw(20) << "free [MiB]:";
  std::cout << memory.free_Bytes() / 1024 / 1024 << std::endl;
  std::cout << std::left << std::setw(20) << "available [MiB]:";
  std::cout << memory.available_Bytes() / 1024 / 1024 << std::endl;
  for (auto& module : memory.modules()) {
    std::cout << "RAM " << module.id << ":\n";
    std::cout << std::left << std::setw(20) << "  vendor:";
    std::cout << module.vendor << std::endl;
    std::cout << std::left << std::setw(20) << "  model:";
    std::cout << module.model << std::endl;
    std::cout << std::left << std::setw(20) << "  name:";
    std::cout << module.name << std::endl;
    std::cout << std::left << std::setw(20) << "  serial-number:";
    std::cout << module.serial_number << std::endl;
    std::cout << std::left << std::setw(20) << "  Frequency [MHz]:";
    std::cout << module.frequency_Hz / 1000 / 1000 << std::endl;
  }

  hwinfo::MainBoard main_board;
  std::cout << "------------------------------- Main Board --------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "vendor:";
  std::cout << main_board.vendor() << std::endl;
  std::cout << std::left << std::setw(20) << "name:";
  std::cout << main_board.name() << std::endl;
  std::cout << std::left << std::setw(20) << "version:";
  std::cout << main_board.version() << std::endl;
  std::cout << std::left << std::setw(20) << "serial-number:";
  std::cout << main_board.serialNumber() << std::endl;

  std::vector<hwinfo::Battery> batteries = hwinfo::getAllBatteries();
  std::cout << "------------------------------- Batteries ---------------------------------" << std::endl;
  if (!batteries.empty()) {
    int battery_counter = 0;
    for (auto& battery : batteries) {
      std::cout << "Battery " << battery_counter++ << ":" << std::endl;
      std::cout << std::left << std::setw(20) << "  vendor:";
      std::cout << battery.vendor() << std::endl;
      std::cout << std::left << std::setw(20) << "  model:";
      std::cout << battery.model() << std::endl;
      std::cout << std::left << std::setw(20) << "  serial-number:";
      std::cout << battery.serialNumber() << std::endl;
      std::cout << std::left << std::setw(20) << "  charging:";
      std::cout << (battery.charging() ? "yes" : "no") << std::endl;
      std::cout << std::left << std::setw(20) << "  capacity:";
      std::cout << battery.capacity() << std::endl;
    }
    std::cout << "---------------------------------------------------------------------------" << std::endl;
  } else {
    std::cout << "No Batteries installed or detected" << std::endl;
  }

  std::vector<hwinfo::Disk> disks = hwinfo::getAllDisks();
  std::cout << "--------------------------------- Disks -----------------------------------" << std::endl;
  if (!disks.empty()) {
    int disk_counter = 0;
    for (const auto& disk : disks) {
      std::cout << "Disk " << disk_counter++ << ":" << std::endl;
      std::cout << std::left << std::setw(20) << "  vendor:";
      std::cout << disk.vendor() << std::endl;
      std::cout << std::left << std::setw(20) << "  model:";
      std::cout << disk.model() << std::endl;
      std::cout << std::left << std::setw(20) << "  serial-number:";
      std::cout << disk.serialNumber() << std::endl;
      std::cout << std::left << std::setw(20) << "  size:";
      std::cout << disk.size_Bytes() << std::endl;
    }
    std::cout << "---------------------------------------------------------------------------" << std::endl;
  } else {
    std::cout << "No Disks installed or detected" << std::endl;
  }
}