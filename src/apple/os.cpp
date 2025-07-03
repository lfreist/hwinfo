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
