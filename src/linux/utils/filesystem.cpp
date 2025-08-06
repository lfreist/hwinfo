#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <dirent.h>
#include <hwinfo/cpu.h>
#include <hwinfo/utils/filesystem.h>
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

Jiffies get_jiffies(int index) {
  std::ifstream filestat("/proc/stat");
  if (!filestat.is_open()) {
    return {};
  }

  for (int i = 0; i < index; ++i) {
    if (!filestat.ignore(std::numeric_limits<std::streamsize>::max(), '\n')) {
      break;
    }
  }
  std::string line;
  std::getline(filestat, line);

  std::istringstream iss(line);
  std::vector<std::string> results(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

  const int64_t jiffies_0 = std::stol(results[1]);
  const int64_t jiffies_1 = std::stol(results[2]);
  const int64_t jiffies_2 = std::stol(results[3]);
  const int64_t jiffies_3 = std::stol(results[4]);
  const int64_t jiffies_4 = std::stol(results[5]);
  const int64_t jiffies_5 = std::stol(results[6]);
  const int64_t jiffies_6 = std::stol(results[7]);
  const int64_t jiffies_7 = std::stol(results[8]);
  const int64_t jiffies_8 = std::stol(results[9]);
  const int64_t jiffies_9 = std::stol(results[10]);

  int64_t all = jiffies_0 + jiffies_1 + jiffies_2 + jiffies_3 + jiffies_4 + jiffies_5 + jiffies_6 + jiffies_7 +
                jiffies_8 + jiffies_9;
  int64_t working = jiffies_0 + jiffies_1 + jiffies_2;

  return {all, working};
}

}  // namespace filesystem
}  // namespace hwinfo

#endif  // HWINFO_UNIX
