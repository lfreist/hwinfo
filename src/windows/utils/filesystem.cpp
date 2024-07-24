#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <hwinfo/utils/filesystem.h>

#include <string>
#include <vector>

bool hwinfo::filesystem::exists(const std::string& path) {
  // TODO: implement if needed
  return false;
}

std::vector<std::string> hwinfo::filesystem::getDirectoryEntries(const std::string& path) {
  // TODO: implement if needed
  return {};
}

#endif  // HWINFO_WINDOWS
