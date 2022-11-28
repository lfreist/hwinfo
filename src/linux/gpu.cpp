
// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <regex>
#include <string>
#include <vector>

#include "hwinfo/gpu.h"
#include "hwinfo/utils/subprocess.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string GPU::getVendor() {
  // TODO: piping stderr to /dev/null seems super ugly.
  //  Why am I doing this? -> lshw prints that one should run it as sudo user to stderr...
  std::string command("lshw -c display 2> /dev/null");
  std::regex matcher("vendor:.*");
  std::string output = exec(command);
  std::smatch match;
  std::string vendor;
  if (std::regex_search(output.cbegin(), output.cend(), match, matcher)) {
    char prev = '\0';
    bool add = false;
    std::string tmp = match[0];
    for (auto& c : tmp) {
      if (c == '\n') {
        break;
      }
      if (add) {
        vendor += c;
      }
      if (prev == ':') {
        add = true;
      }
      prev = c;
    }
  }
  return vendor.empty() ? "<unknown>" : vendor;
}

// _____________________________________________________________________________________________________________________
std::string GPU::getName() {
  // TODO: piping stderr to /dev/null seems super ugly.
  //  Why am I doing this? -> lshw prints that one should run it as sudo user to stderr...
  std::string command("lshw -c display 2> /dev/null");
  std::regex matcher("product:.*\\[.*\\]");
  std::string output = exec(command);
  std::smatch match;
  std::string name;
  if (std::regex_search(output.cbegin(), output.cend(), match, matcher)) {
    bool add = false;
    std::string tmp = match[0];
    for (auto& c : tmp) {
      if (c == ']') {
        break;
      }
      if (add) {
        name += c;
      }
      if (c == '[') {
        add = true;
      }
    }
  }
  return name;
}

// _____________________________________________________________________________________________________________________
std::string GPU::getDriverVersion() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
int64_t GPU::getMemory_Bytes() {
  // TODO: implement
  return -1;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX