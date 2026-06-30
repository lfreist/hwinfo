// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <fstream>
#include <sstream>
#include <regex>
#include <string>

#include "hwinfo/os.h"
#include "hwinfo/utils/sysctl.h"

std::string getOSVersionFromPlist() {
    std::ifstream file("/System/Library/CoreServices/SystemVersion.plist");
    if (!file.is_open()) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::smatch match;
    std::regex version_regex("<key>ProductVersion</key>\\s*<string>([^<]+)</string>");
    if (std::regex_search(content, match, version_regex)) {
        return match[1];
    }
    return "";
}

namespace hwinfo {

std::string getMarketingName(const std::string& version) {
  size_t dotPos = version.find('.');
  if (dotPos == std::string::npos) {
    return "";
  }

  int majorVersion = 0;
  try {
    majorVersion = std::stoi(version.substr(0, dotPos));
  } catch (...) {
    return "";
  }

  // map major version to marketing name
  switch (majorVersion) {
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
    case 10: {
      // handle 10.x versions - need to check minor version
      size_t secondDot = version.find('.', dotPos + 1);
      std::string minorStr = (secondDot != std::string::npos) ? version.substr(dotPos + 1, secondDot - dotPos - 1)
                                                              : version.substr(dotPos + 1);
      try {
        switch (std::stoi(minorStr)) {
          case 15:
            return "Catalina";
          case 14:
            return "Mojave";
          case 13:
            return "High Sierra";
          case 12:
            return "Sierra";
          case 11:
            return "El Capitan";
          case 10:
            return "Yosemite";
          case 9:
            return "Mavericks";
          case 8:
            return "Mountain Lion";
          case 7:
            return "Lion";
          case 6:
            return "Snow Leopard";
          case 5:
            return "Leopard";
          case 4:
            return "Tiger";
          case 3:
            return "Panther";
          case 2:
            return "Jaguar";
          case 1:
            return "Puma";
          case 0:
            return "Cheetah";
          default:
            return "";
        }
      } catch (...) {
        return "";
      }
    }
    default:
      return "";
  }
}

// _____________________________________________________________________________________________________________________
OS::OS() {
  _name = "macOS";

  // Get kernel name and version
  _kernel = utils::getSysctlString("kern.ostype", "<unknown name>");
  _kernel += " " + utils::getSysctlString("kern.osrelease", "<unknown version>");

  // get OS name and build version
  _version = getOSVersionFromPlist();
  if (_version.empty()) {
    _version = utils::getSysctlString("kern.osproductversion", "<unknown>");
  }
  std::string build = utils::getSysctlString("kern.osversion", "<unknown build>");
  _version = _version + " (" + build + ")";
  // add marketing name if we can determine it
  if (std::string marketingName = getMarketingName(_version); !marketingName.empty()) {
    _name = _name + " " + marketingName;
  }
  // determine endianess
  const int byteorder = utils::getSysctlValue("hw.byteorder", 0);
  _bigEndian = (byteorder == 4321);
  _littleEndian = (byteorder == 1234);

#if defined(__x86_64__) || defined(__aarch64__) || defined(__ppc64__)
  _64bit = true;
#else
  _64bit = false;
#endif
  _32bit = !_64bit;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
