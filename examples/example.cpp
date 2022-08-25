// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <iostream>

#include "hwinfo/hwinfo.h"

int main(int argc, char** argv) {
  std::cout << std::endl << "Hardware Report:" << std::endl << std::endl;
  hwinfo::CPU cpu;
  std::cout << "----------------------------------- CPU -----------------------------------" << std::endl;
  std::cout << "CPU model:\t\t\t\t" << cpu.modelName() << std::endl;
  std::cout << "CPU vendor:\t\t\t\t" << cpu.vendor() << std::endl;
  std::cout << "CPU physical cores:\t\t" << cpu.numPhysicalCores() << std::endl;
  std::cout << "CPU logical cores:\t\t" << cpu.numLogicalCores() << std::endl;
  std::cout << "CPU max clock s.:\t\t" << cpu.maxClockSpeed_kHz() << std::endl;
  std::cout << "CPU clock speed:\t\t"<< cpu.regularClockSpeed_kHz() << std::endl;
  std::cout << "current clock speed:\t"<< hwinfo::CPU::currentClockSpeed_kHz() << std::endl;
  std::cout << "CPU cache size:\t\t\t" << cpu.cacheSize_Bytes() << std::endl;

  hwinfo::OS os;
  std::cout << "----------------------------------- OS ------------------------------------" << std::endl;
  std::cout << "Operating System:\t" << os.fullName() <<std::endl;
  std::cout << "OS short name:\t\t" << os.name() <<std::endl;
  std::cout << os.version() <<std::endl;
  std::cout << os.kernel() <<std::endl;
  std::cout << "Architecture:\t\t" << (os.is32bit() ? "32 bit" : "64 bit") << std::endl;
  std::cout << "Endianess:\t\t\t" << (os.isLittleEndian() ? "little endian" : "big endian") << std::endl;

  hwinfo::GPU gpu;
  std::cout << "----------------------------------- GPU -----------------------------------" << std::endl;
  std::cout << "GPU vendor:\t\t\t" << gpu.vendor() << std::endl;
  std::cout << "GPU model:\t\t\t" << gpu.name() << std::endl;
  std::cout << "GPU driverVersion:\t" << gpu.driverVersion() << std::endl;
  std::cout << "GPU memory [MiB]:\t" << gpu.memory_Bytes() / 1024.0 / 1024.0 << std::endl;

  hwinfo::RAM ram;
  std::cout << "----------------------------------- RAM -----------------------------------" << std::endl;
  std::cout << "RAM manufacturer:\t" << ram.manufacturer() << std::endl;
  std::cout << "RAM model:\t\t\t" << ram.model() << std::endl;
  std::cout << "RAM name:\t\t\t" << ram.name() << std::endl;
  std::cout << "RAM serial-number:\t" << ram.serialNumber() << std::endl;
  std::cout << "RAM memory [MiB]:\t" << static_cast<double>(ram.totalSize_Bytes()) / 1024.0 / 1024.0 << std::endl;
  std::cout << "---------------------------------------------------------------------------" << std::endl;

  hwinfo::MainBoard main_board;
  std::cout << "------------------------------- Main Board --------------------------------" << std::endl;
  std::cout << "Board manufacturer:\t\t" << main_board.manufacturer() << std::endl;
  std::cout << "Board name:\t\t\t\t" << main_board.name() << std::endl;
  std::cout << "Board version:\t\t\t" << main_board.version() << std::endl;
  std::cout << "Board serial-number:\t" << ram.serialNumber() << std::endl;
  std::cout << "---------------------------------------------------------------------------" << std::endl;

  std::vector<hwinfo::Disk> disks = hwinfo::getAllDisks();
  int counter = 0;
  std::cout << "--------------------------------- Disks -----------------------------------" << std::endl;
  for (const auto& disk: disks) {
    std::cout << "Disk " << counter++ << ":" << std::endl;
    std::cout << "  Disk vendor:\t\t\t" << disk.vendor() << std::endl;
    std::cout << "  Disk model:\t\t\t" << disk.model() << std::endl;
    std::cout << "  Disk serial-number:\t" << disk.serialNumber() << std::endl;
    std::cout << "  Disk size:\t\t\t" << disk.size_Bytes() << std::endl;
  }
  std::cout << "---------------------------------------------------------------------------" << std::endl;

  hwinfo::getAllDisks();
}