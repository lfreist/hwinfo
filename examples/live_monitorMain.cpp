// Live hardware monitor: refreshes in-place every second using hwinfo::Monitor. Press Ctrl+C to quit.

#include <hwinfo/cpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/gpu.h>
#include <hwinfo/monitoring.h>
#include <hwinfo/ram.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <format>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::chrono_literals;

namespace {

std::atomic<bool> g_running{true};

#ifdef _WIN32
BOOL WINAPI console_ctrl_handler(DWORD event) {
  if (event == CTRL_C_EVENT) {
    g_running = false;
    return TRUE;
  }
  return FALSE;
}
#else
void signal_handler(int) { g_running = false; }
#endif

void enable_ansi_on_windows() {
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(h, &mode)) {
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
#endif
}

std::string bar(double ratio, int width = 20) {
  const int filled = std::clamp(static_cast<int>(ratio * width + 0.5), 0, width);
  return std::format("[{}{}]", std::string(filled, '#'), std::string(width - filled, '.'));
}

struct Snapshot {
  hwinfo::result<hwinfo::CpuLoad> cpu;
  hwinfo::result<std::vector<hwinfo::Hertz>> frequencies;
  hwinfo::result<hwinfo::MemoryUsage> memory;
  std::vector<std::pair<std::filesystem::path, hwinfo::result<hwinfo::DiskSpace>>> disks;
  std::vector<hwinfo::result<hwinfo::GpuStatus>> gpus;
};

template <typename T>
std::string or_dash(const std::optional<T>& value, std::string_view spec = "{}") {
  return value ? std::vformat(spec, std::make_format_args(*value)) : std::string("-");
}

std::string gpu_line(const hwinfo::GpuStatus& s) {
  if (s.suspended) {
    return "suspended (powered down)";
  }
  const std::string utilization =
      s.utilization ? std::format("{} {:5.1f}%", bar(*s.utilization, 14), *s.utilization * 100.0) : "n/a";
  return std::format("{:<23}  mem {:>9} {}  {:>6}  {:>7}", utilization, or_dash(s.memory_used, "{:.2GiB}"),
                     s.memory_total ? std::format("of {:.1GiB}", *s.memory_total) : std::string{},
                     s.temperature ? std::format("{:.0f}°C", *s.temperature) : std::string("-"),
                     or_dash(s.power, "{:.1W}"));
}

}  // namespace

int main() {
  enable_ansi_on_windows();
#ifdef _WIN32
  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
  std::signal(SIGINT, signal_handler);
#endif

  // Static hardware information, gathered once
  const auto cpus = hwinfo::cpus();
  const auto disks = hwinfo::disks();
  const auto memory = hwinfo::memory();
  const auto gpus = hwinfo::gpus();

  std::cout << "=== hwinfo live monitor  (Ctrl+C to quit) ===\n\n";
  for (const auto& cpu : cpus.value_or(std::vector<hwinfo::Cpu>{})) {
    std::cout << std::format("CPU : {}", cpu) << '\n';
  }
  if (memory) {
    std::cout << std::format("RAM : {} total", memory->total) << '\n';
  }
  std::vector<std::filesystem::path> mount_points;
  for (const auto& disk : disks.value_or(std::vector<hwinfo::Disk>{})) {
    std::cout << std::format("Disk: [{}] {}", disk.index, disk) << '\n';
    mount_points.append_range(disk.mount_points);
  }
  std::vector<hwinfo::GpuSampler> gpu_samplers;
  for (const auto& gpu : gpus.value_or(std::vector<hwinfo::Gpu>{})) {
    std::cout << std::format("GPU : [{}] {}", gpu.index, gpu) << '\n';
    gpu_samplers.emplace_back(gpu);
  }
  std::cout << '\n';

  int previous_lines = 0;
  const auto render = [&](const Snapshot& s) {
    std::string out;
    int lines = 0;
    const auto line = [&]<typename... Args>(std::format_string<Args...> fmt, Args&&... args) {
      std::format_to(std::back_inserter(out), fmt, std::forward<Args>(args)...);
      out += "\033[K\n";  // clear rest of line
      ++lines;
    };

    if (s.cpu) {
      line("CPU  avg : {} {:5.1f}%", bar(s.cpu->total), s.cpu->total * 100.0);
      for (std::size_t i = 0; i < s.cpu->per_thread.size(); ++i) {
        const double u = s.cpu->per_thread[i];
        const auto frequency =
            s.frequencies && i < s.frequencies->size() ? std::format("{:>10MHz}", (*s.frequencies)[i]) : std::string{};
        line("  T{:02} : {} {:5.1f}%  {}", i, bar(u, 14), u * 100.0, frequency);
      }
    } else {
      line("CPU      : {}", s.cpu.error());
    }

    if (s.memory) {
      line("RAM  free: {:>10.2GiB}   available: {:>10.2GiB}", s.memory->free, s.memory->available);
    }
    for (const auto& [path, space] : s.disks) {
      if (space) {
        line("Disk [{}]  free: {:>10.2GiB} of {:.2GiB}", path.string(), space->available, space->capacity);
      }
    }

    for (std::size_t i = 0; i < s.gpus.size(); ++i) {
      if (s.gpus[i]) {
        line("GPU [{}]  : {}", i, gpu_line(*s.gpus[i]));
      } else {
        line("GPU [{}]  : {}", i, s.gpus[i].error());
      }
    }

    if (previous_lines > 0) {
      std::cout << std::format("\033[{}A", previous_lines);  // move cursor up to overwrite the previous output
    }
    std::cout << out;
    std::cout.flush();
    previous_lines = lines;
  };

  hwinfo::Monitor monitor{1s,
                          [&mount_points, &gpu_samplers, sampler = hwinfo::CpuSampler{}]() mutable {
                            return Snapshot{
                                .cpu = sampler.sample(),
                                .frequencies = hwinfo::cpu_frequencies(),
                                .memory = hwinfo::memory_usage(),
                                .disks = mount_points | std::views::transform([](const auto& path) {
                                           return std::pair{path, hwinfo::disk_space(path)};
                                         }) |
                                         std::ranges::to<std::vector>(),
                                .gpus = gpu_samplers | std::views::transform([](auto& gpu) { return gpu.sample(); }) |
                                        std::ranges::to<std::vector>(),
                            };
                          },
                          render};

  while (g_running) {
    std::this_thread::sleep_for(100ms);
  }
  monitor.stop();
  std::cout << '\n';
  return 0;
}
