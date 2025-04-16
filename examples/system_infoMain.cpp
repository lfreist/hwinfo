// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <hwinfo/hwinfo.h>
#include <hwinfo/utils/unit.h>

#include <cassert>
#include <vector>

using hwinfo::unit::bytes_to_MiB;

int main(int argc, char** argv) {
  fmt::print(
      "hwinfo is an open source, MIT licensed project that implements a platform independent "
      "hardware and system information gathering API for C++.\n\n"
      "If you face any issues, find bugs or if your platform is not supported yet, do not hesitate to create "
      "a ticket at https://github.com/lfreist/hwinfo/issues.\n\n"
      "Thanks for using hwinfo!\n\n");

  fmt::print(
      "Hardware Report:\n\n"
      "----------------------------------- CPU ------------------------------------\n");
  const auto cpus = hwinfo::getAllCPUs();
  for (const auto& cpu : cpus) {
    // clang-format off
    fmt::print("Socket {}:\n"
               "{:<20} {}\n"
               "{:<20} {}\n"
               "{:<20} {}\n"
               "{:<20} {}\n"
               "{:<20} {}\n"
               "{:<20} {}\n"
               "{:<20} L1: {}, L2: {}, L3: {}\n",
               cpu.id(),
               "vendor:", cpu.vendor(),
               "model:", cpu.modelName(),
               "physical cores:", cpu.numPhysicalCores(),
               "logical cores:", cpu.numLogicalCores(),
               "max frequency:", cpu.maxClockSpeed_MHz(),
               "regular frequency:", cpu.regularClockSpeed_MHz(),
               "cache size:", cpu.L1CacheSize_Bytes(), cpu.L2CacheSize_Bytes(), cpu.L3CacheSize_Bytes());
    // clang-format on

    std::vector<double> threads_utility = cpu.threadsUtilisation();
    std::vector<int64_t> threads_speed = cpu.currentClockSpeed_MHz();
    assert((threads_utility.size() == threads_speed.size()));
    for (size_t thread_id = 0; thread_id != threads_utility.size(); ++thread_id) {
      fmt::print("{:<20} Thread {}: {} MHz ({}%)\n", " ", thread_id, threads_speed[thread_id],
                 threads_utility[thread_id] * 100);
    }
    // fmt::print("{}\n", cpu.currentTemperature_Celsius());
  }

  hwinfo::OS os;
  // clang-format off
  fmt::print("----------------------------------- OS ------------------------------------\n"
             "{:<20} {}\n"
             "{:<20} {}\n"
             "{:<20} {}\n"
             "{:<20} {}\n"
             "{:<20} {}\n",
             "Operating System:", os.name(),
             "version:", os.version(),
             "kernel:", os.kernel(),
             "architecture:", os.is32bit() ? "32 bit" : "64 bit",
             "endianess:", os.isLittleEndian() ? "little endian" : "big endian");
  // clang-format on

  auto gpus = hwinfo::getAllGPUs();
  fmt::print("----------------------------------- GPU -----------------------------------\n");
  for (const auto& gpu : gpus) {
    // clang-format off
    fmt::print("GPU {}:\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n",
                gpu.id(),
                "vendor:", gpu.vendor(),
                "model:", gpu.name(),
                "driverVersion:", gpu.driverVersion(),
                "memory [MiB]:", bytes_to_MiB(gpu.memory_Bytes()),
                "frequency:", gpu.frequency_MHz(),
                "cores:", gpu.num_cores(),
                "vendor_id:", gpu.vendor_id(),
                "device_id:", gpu.device_id());
    // clang-format on
  }

  hwinfo::Memory memory;
  // clang-format off
  fmt::print("----------------------------------- RAM -----------------------------------\n"
             "{:<20} {}\n"
             "{:<20} {}\n"
             "{:<20} {}\n",
             "size [MiB]:", bytes_to_MiB(memory.total_Bytes()),
             "free [MiB]:", bytes_to_MiB(memory.free_Bytes()),
             "available [MiB]:", bytes_to_MiB(memory.available_Bytes()));
  // clang-format on

  for (const auto& module : memory.modules()) {
    // clang-format off
    fmt::print("RAM {}:\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n"
                "{:<20} {}\n",
                module.id,
                "vendor:", module.vendor,
                "model:", module.model,
                "name:", module.name,
                "serial-number:", module.serial_number,
                "Frequency [MHz]:", module.frequency_Hz == -1 ? -1 : static_cast<double>(module.frequency_Hz) / 1e6);
    // clang-format on
  }

  hwinfo::MainBoard main_board;
  // clang-format off
  fmt::print("------------------------------- Main Board --------------------------------\n"
             "{:<20} {}\n"
             "{:<20} {}\n"
             "{:<20} {}\n"
             "{:<20} {}\n",
             "vendor:", main_board.vendor(),
             "name:", main_board.name(),
             "version:", main_board.version(),
             "serial-number:", main_board.serialNumber());
  // clang-format on

  std::vector<hwinfo::Battery> batteries = hwinfo::getAllBatteries();

  fmt::print("------------------------------- Batteries ---------------------------------\n");
  if (!batteries.empty()) {
    int battery_counter = 0;
    for (auto& battery : batteries) {
      // clang-format off
      fmt::print("Battery {}:\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n",
                 battery_counter++,
                 "vendor:", battery.vendor(),
                 "model:", battery.model(),
                 "serial-number:", battery.serialNumber(),
                 "charging:", battery.charging() ? "yes" : "no",
                 "capacity:", battery.capacity());
      // clang-format on
    }
  } else {
    fmt::print("No Batteries installed or detected\n");
  }

  std::vector<hwinfo::Disk> disks = hwinfo::getAllDisks();
  fmt::print("--------------------------------- Disks -----------------------------------\n");
  if (!disks.empty()) {
    int disk_counter = 0;
    for (const auto& disk : disks) {
      // clang-format off
      fmt::print("Disk {}:\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n"
                 "{:<20} {}\n",
                 disk_counter++,
                 "vendor:", disk.vendor(),
                 "model:", disk.model(),
                 "serial-number:", disk.serialNumber(),
                 "size:", disk.size_Bytes(),
                 "free:", disk.free_size_Bytes(),
                 "volumes:", fmt::join(disk.volumes(), ", "));
      // clang-format on
    }
  } else {
    fmt::print("No Disks installed or detected\n");
  }

  std::vector<hwinfo::Network> networks = hwinfo::getAllNetworks();
  fmt::print("--------------------------------- Networks -----------------------------------\n");
  if (!networks.empty()) {
    int network_counter = 0;
    for (const auto& network : networks) {
      // clang-format off
      if (network.ip4().size() > 0 || network.ip6().size() > 0) {
        fmt::print(
            "Network {}:\n"
            "{:<20} {}\n"
            "{:<20} {}\n"
            "{:<20} {}\n"
            "{:<20} {}\n"
            "{:<20} {}\n",
            network_counter++,
            "description:", network.description(),
            "interface index:", network.interfaceIndex(),
            "mac:", network.mac(),
            "ipv4:", network.ip4(),
            "ipv6:", network.ip6());
      }
      // clang-format on
    }
  } else {
    fmt::print("No Networks installed or detected\n");
  }
  return EXIT_SUCCESS;
}
