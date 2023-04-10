#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <dirent.h>
#include <sys/stat.h>

#include <cstring>
#include <vector>

#include "hwinfo/utils/filesystem.h"

bool hwinfo::filesystem::exists(const std::string& path) {
  struct stat sb {};
  return stat(path.c_str(), &sb) == 0;
}

std::vector<std::string> hwinfo::filesystem::getDirectoryEntries(const std::string& path) {
  std::vector<std::string> children;
  struct dirent* entry = nullptr;
  DIR* dp = nullptr;

  dp = opendir(path.c_str());
  if (dp != nullptr) {
    while ((entry = readdir(dp))) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
      children.emplace_back(entry->d_name);
    }
    closedir(dp);
  }

  return children;
}

#endif  // HWINFO_UNIX
