// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <iostream>
#include <iomanip>

#include "hwinfo/hwinfo.h"

int main(int argc, char** argv) {
  std::cout << std::endl << "Hardware Report:" << std::endl << std::endl;
  hwinfo::CPU cpu;
  std::cout << "----------------------------------- CPU -----------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "vendor:";
  std::cout << cpu.vendor() << std::endl;
  std::cout << std::left << std::setw(20) << "model:";
  std::cout << cpu.modelName() << std::endl;
  std::cout << std::left << std::setw(20) << "physical cores:";
  std::cout << cpu.numPhysicalCores() << std::endl;
  std::cout << std::left << std::setw(20) << "logical cores:";
  std::cout << cpu.numLogicalCores() << std::endl;
  std::cout << std::left << std::setw(20) << "max frequency:";
  std::cout << cpu.maxClockSpeed_kHz() << std::endl;
  std::cout << std::left << std::setw(20) << "regular frequency:";
  std::cout << cpu.regularClockSpeed_kHz() << std::endl;
  std::cout << std::left << std::setw(20) << "current frequency:";
  std::cout << hwinfo::CPU::currentClockSpeed_kHz() << std::endl;
  std::cout << std::left << std::setw(20) << "cache size:";
  std::cout << cpu.cacheSize_Bytes() << std::endl;

  hwinfo::OS os;
  std::cout << "----------------------------------- OS ------------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "Operating System:";
  std::cout << os.fullName() <<std::endl;
  std::cout << std::left << std::setw(20) << "short name:";
  std::cout << os.name() <<std::endl;
  std::cout << std::left << std::setw(20) << "version:";
  std::cout << os.version() <<std::endl;
  std::cout << std::left << std::setw(20) << "kernel:";
  std::cout << os.kernel() <<std::endl;
  std::cout << std::left << std::setw(20) << "architecture:";
  std::cout << (os.is32bit() ? "32 bit" : "64 bit") << std::endl;
  std::cout << std::left << std::setw(20) << "endianess:";
  std::cout << (os.isLittleEndian() ? "little endian" : "big endian") << std::endl;

  hwinfo::GPU gpu;
  std::cout << "----------------------------------- GPU -----------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "vendor:";
  std::cout << gpu.vendor() << std::endl;
  std::cout << std::left << std::setw(20) << "model:";
  std::cout << gpu.name() << std::endl;
  std::cout << std::left << std::setw(20) << "driverVersion:";
  std::cout << gpu.driverVersion() << std::endl;
  std::cout << std::left << std::setw(20) << "memory [MiB]:";
  std::cout << static_cast<double>(gpu.memory_Bytes()) / 1024.0 / 1024.0 << std::endl;

  hwinfo::RAM ram;
  std::cout << "----------------------------------- RAM -----------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "vendor:";
  std::cout << ram.vendor() << std::endl;
  std::cout << std::left << std::setw(20) << "model:";
  std::cout << ram.model() << std::endl;
  std::cout << std::left << std::setw(20) << "name:";
  std::cout << ram.name() << std::endl;
  std::cout << std::left << std::setw(20) << "serial-number:";
  std::cout << ram.serialNumber() << std::endl;
  std::cout << std::left << std::setw(20) << "size [MiB]:";
  std::cout << static_cast<double>(ram.totalSize_Bytes()) / 1024.0 / 1024.0 << std::endl;

  hwinfo::MainBoard main_board;
  std::cout << "------------------------------- Main Board --------------------------------" << std::endl;
  std::cout << std::left << std::setw(20) << "vendor:";
  std::cout << main_board.vendor() << std::endl;
  std::cout << std::left << std::setw(20) << "name:";
  std::cout << main_board.name() << std::endl;
  std::cout << std::left << std::setw(20) << "version:";
  std::cout << main_board.version() << std::endl;
  std::cout << std::left << std::setw(20) << "serial-number:";
  std::cout << ram.serialNumber() << std::endl;

  std::vector<hwinfo::Disk> disks = hwinfo::getAllDisks();
  int counter = 0;
  std::cout << "--------------------------------- Disks -----------------------------------" << std::endl;
  for (const auto& disk: disks) {
    std::cout << "Disk " << counter++ << ":" << std::endl;
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

  hwinfo::getAllDisks();
}