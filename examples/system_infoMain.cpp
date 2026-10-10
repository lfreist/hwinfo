// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/hwinfo.h>

#include <format>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>

namespace {

void section(std::string_view title) { std::cout << std::format("{:-^80}", std::format(" {} ", title)) << '\n'; }

void field(std::string_view name, const auto& value) { std::cout << std::format("  {:<22}{}", name, value) << '\n'; }

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
    std::cout << std::format("  error: {}", r.error()) << '\n';
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
  std::cout
      << "hwinfo is an open source, MIT licensed project that implements a platform independent hardware and system "
         "information gathering API for C++.\n\nIf you face any issues, find bugs or if your platform is not supported "
         "yet, do not hesitate to create a ticket at https://github.com/lfreist/hwinfo/issues.\n\n";

  section("CPU");
  if (const auto cpus = hwinfo::cpus(); check(cpus)) {
    for (const auto& cpu : *cpus) {
      std::cout << std::format("Socket {}", cpu.socket) << '\n';
      field("vendor:", cpu.vendor);
      field("model:", cpu.model);
      field("cores:", std::format("{} ({} threads)", cpu.physical_cores, cpu.logical_cores));
      field("flags:", std::format("{} flags", cpu.flags.size()));
      field("core -> logical ids:", join(cpu.cores | std::views::transform([](const hwinfo::Core& core) {
                                           return std::format("{}:[{}]", core.id, join(core.logical_ids, ","));
                                         })));
      if (!cpu.cores.empty()) {
        const auto& core = cpu.cores.front();
        const auto size = [](const std::optional<hwinfo::Bytes>& s) {
          return s ? std::format("{}", *s) : std::string("<unknown>");
        };
        field("L1d / L1i:", std::format("{} / {}", size(core.cache.l1_data), size(core.cache.l1_instruction)));
        field("L2 / L3:", std::format("{} / {}", size(core.cache.l2), size(core.cache.l3)));
        field("base frequency:", core.base_frequency);
        field("max frequency:", core.max_frequency);
      }
    }
  }

  section("Operating System");
  if (const auto os = hwinfo::os(); check(os)) {
    field("family:", os->family);
    field("name:", os->name);
    field("marketing name:", os->marketing_name);
    field("version:", os->version);
    field("kernel:", os->kernel);
    field("architecture:", std::format("{} ({}-bit)", os->architecture, os->bits));
  }

  section("Virtualization");
  if (const auto virtualization = hwinfo::virtualization(); check(virtualization)) {
    field("environment:", *virtualization);
    if (const auto& vm = virtualization->vm) {
      field("hypervisor:", vm->hypervisor);
      field("vendor id:", vm->vendor_id);
    }
    if (const auto& container = virtualization->container) {
      field("container runtime:", container->runtime);
      field("kubernetes:", container->kubernetes ? "yes" : "no");
    }
  }
  if (const auto limits = hwinfo::resource_limits(); check(limits)) {
    field("cpu quota:", limits->cpu_quota ? std::format("{:.2f} cores", *limits->cpu_quota) : "none");
    field("memory limit:", limits->memory ? std::format("{}", *limits->memory) : "none");
    field("allowed cpus:", limits->allowed_cpus.size());
  }

  section("GPU");
  if (const auto gpus = hwinfo::gpus(); check(gpus)) {
    for (const auto& gpu : *gpus) {
      std::cout << std::format("GPU {}", gpu.index) << '\n';
      field("vendor:", gpu.vendor);
      field("model:", gpu.name);
      field("type:", gpu.type);
      field("unified memory:", gpu.unified_memory.transform([](bool unified) { return unified ? "yes" : "no"; }));
      field("uuid:", gpu.uuid);
      field("architecture:", gpu.architecture);
      field("compute capability:", gpu.compute_capability);
      field("compute units:", gpu.compute_units);
      field("cores:", gpu.cores);
      field("driver:", gpu.driver);
      field("driver version:", gpu.driver_version);
      field("vbios version:", gpu.vbios_version);
      field("memory:", gpu.dedicated_memory);
      field("memory type:", gpu.memory_type);
      field("memory bus width:",
            gpu.memory_bus_width.transform([](std::uint32_t bits) { return std::format("{} bit", bits); }));
      field("L2 cache:", gpu.l2_cache);
      field("max frequency:", gpu.max_frequency);
      field("max memory frequency:", gpu.max_memory_frequency);
      field("power limit:", gpu.power_limit);
      field("compute APIs:", join(gpu.compute_apis | std::views::transform([](const hwinfo::GpuApi& api) {
                                    return std::format("{} {}", api.name, api.version);
                                  })));
      if (gpu.pci) {
        field("pci id:", std::format("{:04x}:{:04x}", gpu.pci->vendor_id, gpu.pci->device_id));
        field("pci address:", gpu.pci->address);
        field("pcie link:", gpu.pci->max_link);
      }
    }
  }

  section("Memory");
  if (const auto memory = hwinfo::memory(); check(memory)) {
    field("total:", memory->total);
    for (const auto& module : memory->modules) {
      std::cout << std::format("Module {}", module.index) << '\n';
      field("vendor:", module.vendor);
      field("model:", module.model);
      field("serial number:", module.serial_number);
      field("size:", module.size);
      field("frequency:", module.frequency);
    }
  }

  section("Computer");
  if (const auto computer = hwinfo::computer(); check(computer)) {
    field("vendor:", computer->vendor);
    field("model:", computer->model);
    field("family:", computer->family);
    field("version:", computer->version);
    field("sku:", computer->sku);
    field("serial number:", computer->serial_number);
    field("chassis:", computer->chassis);
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
      std::cout << "No batteries installed or detected\n";
    }
    for (const auto& battery : *batteries) {
      std::cout << std::format("Battery {}", battery.index) << '\n';
      field("vendor:", battery.vendor);
      field("model:", battery.model);
      field("serial number:", battery.serial_number);
      field("technology:", battery.technology);
      field("capacity:", battery.full_charge_capacity);
      if (const auto status = hwinfo::battery_status(battery.index)) {
        field("state:", status->state);
        field("charge:", status->charge.transform([](double c) { return std::format("{:.0f}%", c * 100); }));
      }
    }
  }

  section("Disks");
  if (const auto disks = hwinfo::disks(); check(disks)) {
    for (const auto& disk : *disks) {
      std::cout << std::format("Disk {}", disk.index) << '\n';
      field("vendor:", disk.vendor);
      field("model:", disk.model);
      field("serial number:", disk.serial_number);
      field("bus:", disk.bus);
      field("size:", disk.size);
      field("link speed:", disk.link_speed);
      field("mount points:", join(disk.mount_points | std::views::transform([](const auto& p) { return p.string(); })));
    }
  }

  section("Network");
  if (const auto nics = hwinfo::network_interfaces(); check(nics)) {
    for (const auto& nic : *nics | std::views::filter([](const auto& n) { return !n.is_loopback; })) {
      std::cout << std::format("Interface {}", nic.index) << '\n';
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
