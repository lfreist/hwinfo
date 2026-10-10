// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
// clang-format on
#include <hwinfo/monitoring.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "internal/gpu_counters.h"
#include "internal/gpu_monitor.h"
#include "internal/windows_strings.h"

#ifdef _MSC_VER
#pragma comment(lib, "pdh.lib")
#endif

namespace hwinfo::internal::gpu {

namespace {

// All instances of a wildcard counter as (instance name, value).
std::vector<std::pair<std::string, double>> instances(PDH_HCOUNTER counter) {
  DWORD size = 0;
  DWORD count = 0;
  if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, nullptr) !=
          static_cast<PDH_STATUS>(PDH_MORE_DATA) ||
      size == 0) {
    return {};
  }
  std::vector<std::byte> buffer(size);
  auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
  if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, items) != ERROR_SUCCESS) {
    return {};
  }
  std::vector<std::pair<std::string, double>> result;
  result.reserve(count);
  for (DWORD i = 0; i < count; ++i) {
    if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA) {
      result.emplace_back(internal::to_utf8(items[i].szName), items[i].FmtValue.doubleValue);
    }
  }
  return result;
}

// The "GPU Engine" and "GPU Adapter Memory" performance counters (Windows 10 1709+), as shown by Task Manager.
class PdhSource final : public StatusSource {
 public:
  explicit PdhSource(std::uint64_t luid) : _luid(luid) {
    if (PdhOpenQueryW(nullptr, 0, &_query) != ERROR_SUCCESS) {
      _query = nullptr;
      return;
    }
    if (PdhAddEnglishCounterW(_query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &_engines) != ERROR_SUCCESS) {
      _engines = nullptr;
    }
    if (PdhAddEnglishCounterW(_query, L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &_memory) != ERROR_SUCCESS) {
      _memory = nullptr;
    }
    PdhCollectQueryData(_query);  // baseline: utilization is a rate between two collections
  }
  PdhSource(const PdhSource&) = delete;
  PdhSource& operator=(const PdhSource&) = delete;
  ~PdhSource() override {
    if (_query != nullptr) {
      PdhCloseQuery(_query);
    }
  }

  void sample(GpuStatus& status) override {
    if (_query == nullptr || PdhCollectQueryData(_query) != ERROR_SUCCESS) {
      return;
    }
    if (_engines != nullptr) {
      const auto load = gpu_counters::aggregate_engines(instances(_engines), _luid);
      fill(status.utilization, load.utilization);
      fill(status.video_utilization, load.video_utilization);
    }
    if (_memory != nullptr) {
      if (const auto bytes = gpu_counters::adapter_memory(instances(_memory), _luid)) {
        fill(status.memory_used, std::optional(Bytes{*bytes}));
      }
    }
  }

 private:
  std::uint64_t _luid;
  PDH_HQUERY _query = nullptr;
  PDH_HCOUNTER _engines = nullptr;
  PDH_HCOUNTER _memory = nullptr;
};

}  // namespace

std::unique_ptr<StatusSource> os_source(const Gpu& gpu) {
  if (!gpu.luid) {
    return nullptr;
  }
  auto source = std::make_unique<PdhSource>(*gpu.luid);
  return source;
}

bool is_suspended(const Gpu&) { return false; }

}  // namespace hwinfo::internal::gpu

#endif  // HWINFO_WINDOWS
