// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/hwinfo.h>

#include <print>
#include <ranges>
#include <string>
#include <string_view>

namespace {

void section(std::string_view title) { std::println("{:-^80}", std::format(" {} ", title)); }

void field(std::string_view name, const auto& value) { std::println("  {:<22}{}", name, value); }

template <typename T>
void field(std::string_view name, const std::optional<T>& value) {
  if (value) {
    field(name, *value);
  } else {
    field(name, "<unknown>");
  }
}

// Prints the error of a failed query and returns false.
template <typename T>
bool check(const hwinfo::result<T>& r) {
  if (!r) {
    std::println("  error: {}", r.error());
  }
  return r.has_value();
}

std::string join(const auto& range, std::string_view separator = ", ") {
  std::string out;
  for (const auto& value : range) {
    out += std::format("{}{}", out.empty() ? "" : separator, value);
  }
  return out;
}

}  // namespace

int main() {
  std::println(
      "hwinfo is an open source, MIT licensed project that implements a platform independent hardware and system "
      "information gathering API for C++.\n\nIf you face any issues, find bugs or if your platform is not supported "
      "yet, do not hesitate to create a ticket at https://github.com/lfreist/hwinfo/issues.\n");

  section("CPU");
  if (const auto cpus = hwinfo::cpus(); check(cpus)) {
    for (const auto& cpu : *cpus) {
      std::println("Socket {}", cpu.socket);
      field("vendor:", cpu.vendor);
      field("model:", cpu.model);
      field("cores:", std::format("{} ({} threads)", cpu.physical_cores, cpu.logical_cores));
      field("flags:", std::format("{} flags", cpu.flags.size()));
      if (!cpu.cores.empty()) {
        const auto& core = cpu.cores.front();
        field("L1d / L1i:", std::format("{} / {}", core.cache.l1_data.value_or(hwinfo::Bytes{}),
                                        core.cache.l1_instruction.value_or(hwinfo::Bytes{})));
        field("L2 / L3:",
              std::format("{} / {}", core.cache.l2.value_or(hwinfo::Bytes{}), core.cache.l3.value_or(hwinfo::Bytes{})));
        field("base frequency:", core.base_frequency);
        field("max frequency:", core.max_frequency);
      }
    }
  }

  section("Operating System");
  if (const auto os = hwinfo::os(); check(os)) {
    field("name:", os->name);
    field("version:", os->version);
    field("kernel:", os->kernel);
    field("architecture:", std::format("{} ({}-bit)", os->architecture, os->bits));
  }

  section("GPU");
  if (const auto gpus = hwinfo::gpus(); check(gpus)) {
    for (const auto& gpu : *gpus) {
      std::println("GPU {}", gpu.index);
      field("vendor:", gpu.vendor);
      field("model:", gpu.name);
      field("driver:", gpu.driver);
      field("driver version:", gpu.driver_version);
      field("memory:", gpu.dedicated_memory);
      field("frequency:", gpu.frequency);
      field("cores:", gpu.cores);
    }
  }

  section("Memory");
  if (const auto memory = hwinfo::memory(); check(memory)) {
    field("total:", memory->total);
    for (const auto& module : memory->modules) {
      std::println("Module {}", module.index);
      field("vendor:", module.vendor);
      field("model:", module.model);
      field("serial number:", module.serial_number);
      field("size:", module.size);
      field("frequency:", module.frequency);
    }
  }

  section("Mainboard");
  if (const auto board = hwinfo::mainboard(); check(board)) {
    field("vendor:", board->vendor);
    field("name:", board->name);
    field("version:", board->version);
    field("serial number:", board->serial_number);
  }

  section("Batteries");
  if (const auto batteries = hwinfo::batteries(); check(batteries)) {
    if (batteries->empty()) {
      std::println("No batteries installed or detected");
    }
    for (const auto& battery : *batteries) {
      std::println("Battery {}", battery.index);
      field("vendor:", battery.vendor);
      field("model:", battery.model);
      field("serial number:", battery.serial_number);
      field("technology:", battery.technology);
      field("capacity [Wh]:", battery.full_charge_capacity_wh);
      if (const auto status = hwinfo::battery_status(battery.index)) {
        field("state:", status->state);
        field("charge:", status->charge.transform([](double c) { return std::format("{:.0f}%", c * 100); }));
      }
    }
  }

  section("Disks");
  if (const auto disks = hwinfo::disks(); check(disks)) {
    for (const auto& disk : *disks) {
      std::println("Disk {}", disk.index);
      field("vendor:", disk.vendor);
      field("model:", disk.model);
      field("serial number:", disk.serial_number);
      field("bus:", disk.bus);
      field("size:", disk.size);
      field("mount points:", join(disk.mount_points | std::views::transform([](const auto& p) { return p.string(); })));
    }
  }

  section("Network");
  if (const auto nics = hwinfo::network_interfaces(); check(nics)) {
    for (const auto& nic : *nics | std::views::filter([](const auto& n) { return !n.is_loopback; })) {
      std::println("Interface {}", nic.index);
      field("name:", nic.name);
      field("description:", nic.description);
      field("state:", nic.is_up ? "up" : "down");
      field("mac:", nic.mac);
      field("ipv4:", join(nic.ipv4));
      field("ipv6:", join(nic.ipv6));
    }
  }
  return 0;
}
