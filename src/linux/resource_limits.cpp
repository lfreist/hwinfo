// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/resource_limits.h>

#ifdef __linux__
#include <sched.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

namespace {

namespace fs = std::filesystem;

// Directory of the cgroup `cgroup_path` in the hierarchy mounted by `mount`. The mount root is the cgroup that is
// mounted, e.g. the container's own cgroup in a cgroup v1 container without cgroup namespace.
std::optional<fs::path> cgroup_directory(const internal::procfs::MountInfo& mount, std::string_view cgroup_path) {
  std::string_view relative = cgroup_path;
  if (mount.root != "/") {
    if (!relative.starts_with(mount.root) ||
        (relative.size() > mount.root.size() && relative[mount.root.size()] != '/')) {
      return std::nullopt;
    }
    relative.remove_prefix(mount.root.size());
  }
  fs::path directory(mount.mount_point);
  if (const auto r = fs::path(relative).relative_path(); !r.empty()) {
    directory /= r;
  }
  return directory;
}

// The smallest limit set on the cgroup at `directory` and its ancestors up to the mount point: a cgroup can't use more
// than its parent.
template <typename T, typename Read>
std::optional<T> hierarchical_limit(const fs::path& directory, const fs::path& mount_point, Read read) {
  std::optional<T> limit;
  for (auto dir = directory;; dir = dir.parent_path()) {
    if (const std::optional<T> value = read(dir)) {
      limit = limit ? std::min(*limit, *value) : *value;
    }
    if (dir == mount_point || dir == dir.parent_path()) {
      return limit;
    }
  }
}

std::optional<std::string> read_attribute(const fs::path& path) { return internal::read_attribute(path); }

bool has_option(std::string_view options, std::string_view option) {
  for (const auto o : internal::split(options, ',')) {
    if (o == option) {
      return true;
    }
  }
  return false;
}

struct CgroupLimits {
  std::optional<double> cpu_quota{};
  std::optional<Bytes> memory{};
};

CgroupLimits cgroup_limits() {
  const auto membership = internal::procfs::parse_cgroup(internal::read_file("/proc/self/cgroup").value_or(""));
  const auto mounts = internal::procfs::parse_mountinfo(internal::read_file("/proc/self/mountinfo").value_or(""));

  // cgroup v1 hierarchy of `controller`, if mounted
  const auto v1 = [&](std::string_view controller) -> std::optional<std::pair<fs::path, fs::path>> {
    const auto path = membership.controllers.find(controller);
    if (path == membership.controllers.end()) {
      return std::nullopt;
    }
    for (const auto& mount : mounts) {
      if (mount.fs_type == "cgroup" && has_option(mount.super_options, controller)) {
        if (auto directory = cgroup_directory(mount, path->second)) {
          return std::pair{std::move(*directory), fs::path(mount.mount_point)};
        }
      }
    }
    return std::nullopt;
  };
  // cgroup v2 hierarchy, if mounted. In hybrid mode, controllers attached to a v1 hierarchy are not available here.
  const auto v2 = [&]() -> std::optional<std::pair<fs::path, fs::path>> {
    if (!membership.unified) {
      return std::nullopt;
    }
    for (const auto& mount : mounts) {
      if (mount.fs_type == "cgroup2") {
        if (auto directory = cgroup_directory(mount, *membership.unified)) {
          return std::pair{std::move(*directory), fs::path(mount.mount_point)};
        }
      }
    }
    return std::nullopt;
  }();

  CgroupLimits limits;
  if (const auto cpu = v1("cpu")) {
    limits.cpu_quota = hierarchical_limit<double>(cpu->first, cpu->second, [](const fs::path& dir) {
      const auto quota = read_attribute(dir / "cpu.cfs_quota_us");
      const auto period = read_attribute(dir / "cpu.cfs_period_us");
      return quota && period ? internal::procfs::parse_cgroup_cfs_quota(*quota, *period) : std::nullopt;
    });
  } else if (v2) {
    limits.cpu_quota = hierarchical_limit<double>(v2->first, v2->second, [](const fs::path& dir) {
      const auto max = read_attribute(dir / "cpu.max");
      return max ? internal::procfs::parse_cgroup_cpu_max(*max) : std::nullopt;
    });
  }
  if (const auto memory = v1("memory")) {
    limits.memory = hierarchical_limit<Bytes>(memory->first, memory->second, [](const fs::path& dir) {
      const auto limit = read_attribute(dir / "memory.limit_in_bytes");
      return limit ? internal::procfs::parse_cgroup_memory_limit(*limit) : std::nullopt;
    });
  } else if (v2) {
    limits.memory = hierarchical_limit<Bytes>(v2->first, v2->second, [](const fs::path& dir) {
      const auto max = read_attribute(dir / "memory.max");
      return max ? internal::procfs::parse_cgroup_memory_limit(*max) : std::nullopt;
    });
  }
  return limits;
}

std::vector<std::uint32_t> allowed_cpus() {
  std::vector<std::uint32_t> cpus;
#ifdef __linux__
  // the size of the kernel's CPU mask is not known up front: grow until it fits
  for (int count = 1024; count <= (1 << 20); count *= 2) {
    const auto free_set = [](cpu_set_t* set) { CPU_FREE(set); };
    const std::unique_ptr<cpu_set_t, decltype(free_set)> set(CPU_ALLOC(count), free_set);
    if (!set) {
      break;
    }
    const auto size = CPU_ALLOC_SIZE(count);
    CPU_ZERO_S(size, set.get());
    if (sched_getaffinity(0, size, set.get()) == 0) {
      for (int cpu = 0; cpu < count; ++cpu) {
        if (CPU_ISSET_S(cpu, size, set.get())) {
          cpus.push_back(static_cast<std::uint32_t>(cpu));
        }
      }
      break;
    }
    if (errno != EINVAL) {
      break;
    }
  }
#endif
  return cpus;
}

}  // namespace

result<ResourceLimits> resource_limits() {
  auto [cpu_quota, memory] = cgroup_limits();
  return ResourceLimits{.cpu_quota = cpu_quota, .memory = memory, .allowed_cpus = allowed_cpus()};
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
