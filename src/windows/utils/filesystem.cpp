#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

#include "hwinfo/utils/filesystem.h"

bool hwinfo::filesystem::exists(const std::string& path) {
  // TODO: implement if needed
  return false;
}

std::vector<std::string> hwinfo::filesystem::getDirectoryEntries(const std::string& path) {
  // TODO: implement if needed
  return {};
}

#endif  // HWINFO_UNIX
