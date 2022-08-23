// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <iostream>

#include "hwinfo/hwinfo.h"

int main(int argc, char** argv) {
  std::cout << std::endl << "Hardware Report:" << std::endl << std::endl;
  hwinfo::CPU cpu;
  std::cout << "----------------------------------- CPU -----------------------------------" << std::endl;
  std::cout << "CPU model:\t\t\t" << cpu.modelName() << std::endl;
  std::cout << "CPU vendor:\t\t\t" << cpu.vendor() << std::endl;
  std::cout << "CPU physical cores:\t" << cpu.numCores() << std::endl;
  std::cout << "CPU logical cores:\t" << cpu.numLogicalCores() << std::endl;
  std::cout << "CPU max clock s.:\t" << cpu.maxClockSpeedMHz() << std::endl;
  std::cout << "CPU clock speed:\t"<< cpu.regularClockSpeedMHz() << std::endl;
  std::cout << "CPU cache size:\t\t" << cpu.cacheSize() << std::endl;

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
  std::cout << "GPU model:\t\t\t" << gpu.name() << std::endl;
  std::cout << "GPU driverVersion:\t" << gpu.driverVersion() << std::endl;
  std::cout << "GPU memory [MiB]:\t" << gpu.memoryMiB() << std::endl;

  hwinfo::RAM ram;
  std::cout << "----------------------------------- RAM -----------------------------------" << std::endl;
  std::cout << "RAM manufacturer:\t" << ram.manufacturer() << std::endl;
  std::cout << "RAM model:\t\t\t" << ram.model() << std::endl;
  std::cout << "RAM name:\t\t\t" << ram.name() << std::endl;
  std::cout << "RAM serial-number:\t" << ram.serialNumber() << std::endl;
  std::cout << "RAM memory [MiB]:\t" << static_cast<double>(ram.totalSizeBytes()) / 1000.0 / 1000.0 << std::endl;
  std::cout << "---------------------------------------------------------------------------" << std::endl;
}