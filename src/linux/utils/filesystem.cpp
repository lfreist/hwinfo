#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <dirent.h>
#include <hwinfo/cpu.h>
#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace hwinfo {
namespace filesystem {

bool exists(const std::string& path) {
  struct stat sb {};
  return stat(path.c_str(), &sb) == 0;
}

std::vector<std::string> getDirectoryEntries(const std::string& path) {
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

int64_t get_specs_by_file_path(const std::string& path) {
  std::string line;
  std::ifstream stream(path);

  if (!stream) {
    return -1;
  }

  getline(stream, line);
  stream.close();

  try {
    return static_cast<int64_t>(std::stoll(line));
  } catch (std::invalid_argument& e) {
    return -1;
  }
}


}  // namespace filesystem
}  // namespace hwinfo

#endif  // HWINFO_UNIX
