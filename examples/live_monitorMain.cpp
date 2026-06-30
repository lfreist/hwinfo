// Live hardware monitor – refreshes in-place every second using the
// hwinfo::monitoring API.  Press Ctrl+C to quit.

#include <hwinfo/cpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/monitoring/cpu.h>
#include <hwinfo/monitoring/disk.h>
#include <hwinfo/monitoring/monitor.h>
#include <hwinfo/monitoring/ram.h>
#include <hwinfo/ram.h>
#include <hwinfo/utils/unit.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace hwinfo::unit;
using namespace std::chrono_literals;

// ---- signal handling --------------------------------------------------------

static std::atomic<bool> g_running{true};

#ifdef _WIN32
static BOOL WINAPI console_ctrl_handler(DWORD event) {
  if (event == CTRL_C_EVENT) {
    g_running = false;
    return TRUE;
  }
  return FALSE;
}
#else
static void signal_handler(int) { g_running = false; }
#endif

// ---- helpers ----------------------------------------------------------------

static void enable_ansi_on_windows() {
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(h, &mode)) {
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
#endif
}

// ASCII progress bar:  [####......]
static std::string bar(double ratio, int width = 20) {
  const int filled = std::max(0, std::min(width, static_cast<int>(ratio * width + 0.5)));
  return '[' + std::string(filled, '#') + std::string(width - filled, '.') + ']';
}

// ---- live snapshot ----------------------------------------------------------

struct Snapshot {
  hwinfo::monitoring::cpu::Data cpu;
  hwinfo::monitoring::ram::Data ram;
  std::vector<hwinfo::monitoring::disk::Data> disks;
};

// ---- main -------------------------------------------------------------------

int main() {
  enable_ansi_on_windows();

#ifdef _WIN32
  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
  std::signal(SIGINT, signal_handler);
#endif

  // Static hardware info gathered once at startup
  const auto cpus = hwinfo::getAllCPUs();
  const auto disks = hwinfo::getAllDisks();
  hwinfo::Memory memory;

  // Collect all mount points across all disks
  std::vector<std::string> mount_points;
  for (const auto& disk : disks) {
    for (const auto& mp : disk.mount_points()) {
      mount_points.push_back(mp);
    }
  }

  std::cout << "=== hwinfo live monitor  (Ctrl+C to quit) ===\n\n";
  for (const auto& cpu : cpus) {
    std::cout << "CPU : " << cpu.vendor() << " " << cpu.modelName() << "  ("
              << cpu.numPhysicalCores() << " physical / " << cpu.numLogicalCores() << " logical)\n";
  }
  std::cout << "RAM : " << std::fixed << std::setprecision(1)
            << unit_prefix_to(memory.size(), IECPrefix::GIBI) << " GiB total\n";
  for (const auto& disk : disks) {
    std::cout << "Disk: [" << disk.id() << "] " << disk.model() << "  "
              << unit_prefix_to(disk.size(), IECPrefix::GIBI) << " GiB  interface: " << disk.disk_interface()
              << "  mounts:";
    for (const auto& mp : disk.mount_points()) std::cout << ' ' << mp;
    std::cout << '\n';
  }
  std::cout << '\n';
  std::cout.flush();

  // --- live rendering ----------------------------------------------------------

  std::mutex render_mtx;
  int prev_lines = 0;  // number of lines written in the last render pass

  auto render = [&](const Snapshot& s) {
    std::ostringstream out;
    int lines = 0;

    // CPU – overall
    out << std::fixed;
    out << "CPU  avg : " << bar(s.cpu.utilization) << ' ' << std::setw(5) << std::setprecision(1)
        << s.cpu.utilization * 100.0 << "%\n";
    ++lines;

    // CPU – per thread
    for (std::size_t i = 0; i < s.cpu.thread_utilization.size(); ++i) {
      const double u = s.cpu.thread_utilization[i];
      out << "  T" << std::setw(2) << std::setfill('0') << i << std::setfill(' ') << " : "
          << bar(u, 14) << ' ' << std::setw(5) << std::setprecision(1) << u * 100.0 << '%';
      if (i < s.cpu.thread_frequency_hz.size() && s.cpu.thread_frequency_hz[i] > 0) {
        out << "  " << std::setw(7) << std::setprecision(0) << unit_prefix_to(s.cpu.thread_frequency_hz[i], SiPrefix::MEGA) << " MHz";
      }
      out << '\n';
      ++lines;
    }

    // RAM
    out << std::setprecision(2);
    out << "RAM  free: " << std::setw(7) << unit_prefix_to(s.ram.free_bytes, IECPrefix::GIBI)
        << " GiB   available: " << std::setw(7)
        << unit_prefix_to(s.ram.available_bytes, IECPrefix::GIBI) << " GiB\n";
    ++lines;

    // Disk – per mount point
    for (const auto& d : s.disks) {
      out << "Disk [" << d.mount_point << "]  free: " << std::setw(7)
          << unit_prefix_to(d.free_bytes, IECPrefix::GIBI) << " GiB\n";
      ++lines;
    }

    // Move cursor up to overwrite the previous live section
    std::lock_guard<std::mutex> lock(render_mtx);
    if (prev_lines > 0) {
      std::cout << "\033[" << prev_lines << 'A';
    }
    std::cout << out.str();
    std::cout.flush();
    prev_lines = lines;
  };

  // Fetch function: CPU blocks for 200 ms to measure utilisation
  auto fetch_all = [&mount_points]() -> Snapshot {
    Snapshot s;
    s.cpu = hwinfo::monitoring::cpu::fetch(200ms);
    s.ram = hwinfo::monitoring::ram::fetch();
    for (const auto& mp : mount_points) {
      s.disks.push_back(hwinfo::monitoring::disk::fetch(mp));
    }
    return s;
  };

  hwinfo::monitoring::Monitor<Snapshot> monitor(fetch_all, render, 1s);
  monitor.start();

  while (g_running) {
    std::this_thread::sleep_for(100ms);
  }

  monitor.stop();
  std::cout << '\n';
  return 0;
}
