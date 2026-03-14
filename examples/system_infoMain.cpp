// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <hwinfo/hwinfo.h>
#include <hwinfo/utils/unit.h>

#include <cassert>
#include <vector>

using namespace hwinfo::unit;

int main(int argc, char** argv) {
  fmt::print(
      "hwinfo is an open source, MIT licensed project that implements a platform independent "
      "hardware and system information gathering API for C++.\n\n"
      "If you face any issues, find bugs or if your platform is not supported yet, do not hesitate to create "
      "a ticket at https://github.com/lfreist/hwinfo/issues.\n\n"
      "Thanks for using hwinfo!\n\n");

  fmt::print(
      "Hardware Report:\n"
      "----------------------------------- CPU ------------------------------------\n");
  const auto cpus = hwinfo::getAllCPUs();
  for (const auto& cpu : cpus) {
    fmt::print(
        "Socket {}:\n"
        "  {:<20} {}\n"
        "  {:<20} {}\n"
        "  {:<20} {}\n"
        "  {:<20} {}\n"
        "  {:<20} {}\n"
        "  {:<20} {}\n"
        "  {:<20} L1: {}, L2: {}, L3: {}\n",
        cpu.id(), "vendor:", cpu.vendor(), "model:", cpu.modelName(), "physical cores:", cpu.numPhysicalCores(),
        "logical cores:", cpu.numLogicalCores(), "max frequency:", cpu.maxClockSpeed_MHz(),
        "regular frequency:", cpu.regularClockSpeed_MHz(),
        "cache size [KiB]:", unit_prefix_to(cpu.L1CacheSize_Bytes(), IECPrefix::KIBI),
        unit_prefix_to(cpu.L2CacheSize_Bytes(), IECPrefix::KIBI),
        unit_prefix_to(cpu.L3CacheSize_Bytes(), IECPrefix::KIBI));

    std::vector<double> threads_utility = cpu.threadsUtilisation();
    std::vector<int64_t> threads_speed = cpu.currentClockSpeed_MHz();
    fmt::print("Thread Utilisation:\n");
    for (size_t thread_id = 0; thread_id != threads_utility.size(); ++thread_id) {
      fmt::print("{:<20}: {} MHz ({:.2f}%)\n", fmt::format("  Thread {:>2}", thread_id), threads_speed[thread_id],
                 threads_utility[thread_id] * 100.f);
    }
    // fmt::print("{}\n", cpu.currentTemperature_Celsius());
  }

  hwinfo::OS os;
  fmt::print(
      "----------------------------------- OS ------------------------------------\n"
      "{:<20} {}\n"
      "{:<20} {}\n"
      "{:<20} {}\n"
      "{:<20} {}\n"
      "{:<20} {}\n",
      "Operating System:", os.name(), "version:", os.version(), "kernel:", os.kernel(),
      "architecture:", os.is32bit() ? "32 bit" : "64 bit",
      "endianess:", os.isLittleEndian() ? "little endian" : "big endian");

  auto gpus = hwinfo::getAllGPUs();
  fmt::print("----------------------------------- GPU -----------------------------------\n");
  for (const auto& gpu : gpus) {
    fmt::print(
        "GPU {}:\n"
        "{:<20} {}\n"
        "{:<20} {}\n"
        "{:<20} {}\n"
        "{:<20} {:.2f}\n"
        "{:<20} {}\n"
        "{:<20} {}\n"
        "{:<20} {}\n"
        "{:<20} {}\n",
        gpu.id(), "vendor:", gpu.vendor(), "model:", gpu.name(), "driverVersion:", gpu.driverVersion(),
        "memory [GiB]:", unit_prefix_to(gpu.memory_Bytes(), IECPrefix::GIBI), "frequency:", gpu.frequency_MHz(),
        "cores:", gpu.num_cores(), "vendor_id:", gpu.vendor_id(), "device_id:", gpu.device_id());
  }

  hwinfo::Memory memory;
  std::int64_t i = memory.total_Bytes();
  fmt::print(
      "----------------------------------- RAM -----------------------------------\n"
      "{:<20} {:.2f}\n"
      "{:<20} {:.2f}\n"
      "{:<20} {:.2f}\n",
      "size [GiB]:", unit_prefix_to(i, IECPrefix::GIBI),
      "free [GiB]:", unit_prefix_to(memory.free_Bytes(), IECPrefix::GIBI),
      "available [GiB]:", unit_prefix_to(memory.available_Bytes(), IECPrefix::GIBI));

  for (const auto& module : memory.modules()) {
    fmt::print(
        "RAM {}:\n"
        "{:<20} {}\n"
        "{:<20} {}\n"
        "{:<20} {}\n"
        "{:<20} {}\n"
        "{:<20} {:.2f}\n",
        module.id, "vendor:", module.vendor, "model:", module.model, "name:", module.name,
        "serial-number:", module.serial_number,
        "Frequency [MHz]:", module.frequency_Hz == -1 ? -1. : unit_prefix_to(module.frequency_Hz, SiPrefix::MEGA));
  }

  hwinfo::MainBoard main_board;
  fmt::print(
      "------------------------------- Main Board --------------------------------\n"
      "{:<20} {}\n"
      "{:<20} {}\n"
      "{:<20} {}\n"
      "{:<20} {}\n",
      "vendor:", main_board.vendor(), "name:", main_board.name(), "version:", main_board.version(),
      "serial-number:", main_board.serialNumber());

  std::vector<hwinfo::Battery> batteries = hwinfo::getAllBatteries();

  fmt::print("------------------------------- Batteries ---------------------------------\n");
  if (!batteries.empty()) {
    int battery_counter = 0;
    for (auto& battery : batteries) {
      fmt::print(
          "Battery {}:\n"
          "{:<20} {}\n"
          "{:<20} {}\n"
          "{:<20} {}\n"
          "{:<20} {}\n"
          "{:<20} {:.2f}\n",
          battery_counter++, "vendor:", battery.vendor(), "model:", battery.model(),
          "serial-number:", battery.serialNumber(), "charging:", battery.charging() ? "yes" : "no",
          "capacity:", battery.capacity());
    }
  } else {
    fmt::print("No Batteries installed or detected\n");
  }

  std::vector<hwinfo::Disk> disks = hwinfo::getAllDisks();
  fmt::print("--------------------------------- Disks -----------------------------------\n");
  if (!disks.empty()) {
    int disk_counter = 0;
    for (const auto& disk : disks) {
      fmt::print(
          "Disk {}:\n"
          "{:<20} {}\n"
          "{:<20} {}\n"
          "{:<20} {}\n"
          "{:<20} {:.2f}\n"
          "{:<20} {:.2f}\n"
          "{:<20} {}\n",
          disk_counter++, "vendor:", disk.vendor(), "model:", disk.model(), "serial-number:", disk.serialNumber(),
          "size [GiB]:", unit_prefix_to(disk.size_Bytes(), IECPrefix::GIBI),
          "free [GiB]:", unit_prefix_to(disk.free_size_Bytes(), IECPrefix::GIBI),
          "volumes:", fmt::join(disk.volumes(), ", "));
    }
  } else {
    fmt::print("No Disks installed or detected\n");
  }

  std::vector<hwinfo::Network> networks = hwinfo::getAllNetworks();
  fmt::print("--------------------------------- Networks -----------------------------------\n");
  if (!networks.empty()) {
    int network_counter = 0;
    for (const auto& network : networks) {
      if (!network.ip4().empty() || !network.ip6().empty()) {
        fmt::print(
            "Network {}:\n"
            "{:<20} {}\n"
            "{:<20} {}\n"
            "{:<20} {}\n"
            "{:<20} {}\n"
            "{:<20} {}\n",
            network_counter++, "description:", network.description(), "interface index:", network.interfaceIndex(),
            "mac:", network.mac(), "ipv4:", network.ip4(), "ipv6:", network.ip6());
      }
    }
  } else {
    fmt::print("No Networks installed or detected\n");
  }
  return EXIT_SUCCESS;
}
