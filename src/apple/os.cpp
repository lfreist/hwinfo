// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/os.h>
#include <sys/utsname.h>

#include <cerrno>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "internal/file.h"
#include "internal/strings.h"
#include "internal/sysctl.h"

namespace hwinfo {

namespace {

// <key>ProductVersion</key> <string>15.1</string>
std::optional<std::string> plist_product_version(std::string_view plist) {
  const auto key = plist.find("<key>ProductVersion</key>");
  if (key == std::string_view::npos) {
    return std::nullopt;
  }
  constexpr std::string_view open_tag = "<string>";
  const auto begin = plist.find(open_tag, key);
  if (begin == std::string_view::npos) {
    return std::nullopt;
  }
  const auto end = plist.find("</string>", begin);
  if (end == std::string_view::npos) {
    return std::nullopt;
  }
  return internal::non_empty(plist.substr(begin + open_tag.size(), end - begin - open_tag.size()));
}

std::optional<std::string> product_version() {
  if (const auto plist = internal::read_file("/System/Library/CoreServices/SystemVersion.plist")) {
    if (auto version = plist_product_version(*plist)) {
      return version;
    }
  }
  return internal::sysctl_attribute("kern.osproductversion");
}

std::string_view marketing_name(std::string_view version) {
  const auto parts = internal::split(version, '.') | std::ranges::to<std::vector<std::string_view>>();
  const auto major = internal::parse<int>(parts.empty() ? std::string_view{} : parts[0]);
  if (!major) {
    return {};
  }
  switch (*major) {
    case 26:
      return "Tahoe";
    case 15:
      return "Sequoia";
    case 14:
      return "Sonoma";
    case 13:
      return "Ventura";
    case 12:
      return "Monterey";
    case 11:
      return "Big Sur";
    case 10:
      break;
    default:
      return {};
  }
  const auto minor = internal::parse<int>(parts.size() > 1 ? parts[1] : std::string_view{});
  if (!minor) {
    return {};
  }
  // 10.0 ... 10.15
  constexpr std::string_view names[]{
      "Cheetah",       "Puma",      "Jaguar",   "Panther",    "Tiger",  "Leopard",     "Snow Leopard", "Lion",
      "Mountain Lion", "Mavericks", "Yosemite", "El Capitan", "Sierra", "High Sierra", "Mojave",       "Catalina",
  };
  if (*minor < 0 || *minor >= static_cast<int>(std::size(names))) {
    return {};
  }
  return names[*minor];
}

}  // namespace

result<Os> os() {
  utsname info{};
  if (uname(&info) != 0) {
    return std::unexpected(error{std::error_code(errno, std::generic_category()), "uname"});
  }

  // e.g. "15.1 Sequoia (24B83)"
  std::string version = product_version().value_or(std::string{});
  if (const auto name = marketing_name(version); !name.empty()) {
    version += std::format(" {}", name);
  }
  if (const auto build = internal::sysctl_attribute("kern.osversion")) {
    version += version.empty() ? *build : std::format(" ({})", *build);
  }

  return Os{
      .name = "macOS",
      .version = std::move(version),
      .kernel = std::format("{} {}", std::string_view(info.sysname), std::string_view(info.release)),
      .architecture = info.machine,  // "arm64", "x86_64"
#if defined(__x86_64__) || defined(__aarch64__) || defined(__arm64__) || defined(__ppc64__)
      .bits = 64,
#else
      .bits = 32,
#endif
  };
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
