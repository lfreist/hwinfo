#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <dirent.h>
#include <sys/stat.h>

#include <cstring>
#include <vector>
#include <fstream>

#include <sstream>
#include <iterator>

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

void hwinfo::filesystem::get_specs_by_file_path(const std::string& path, int64_t& value) {
  std::string line;
  std::ifstream stream(path);
  
  if (!stream) {
    value = -1;
    return;
  }

  getline(stream, line);
  stream.close();

  try {
    value = static_cast<int64_t>(std::stoll(line)); // MHz ->  / 1000
  } catch (std::invalid_argument& e) {
    value = -1;
  }
}

void hwinfo::filesystem::get_specs_by_file_path(const std::string& path, double& value) {
  std::string line;
  std::ifstream stream(path);
  
  if (!stream) {
    value = -1.0;
    return;
  }

  getline(stream, line);
  stream.close();

  try {
    value = std::stod(line); // Convert to double
  } catch (std::invalid_argument& e) {
    value = -1.0;
  }
}

void hwinfo::filesystem::get_jiffies(int64_t& total, int64_t& working)
{
  //std::string text = "cpu  349585 0 30513 875546 0 935 0 0 0 0";

std::ifstream filestat("/proc/stat");

std::string line;
std::getline(filestat, line);


std::istringstream iss(line);
std::vector<std::string> results(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

const int& jiffies_0 = std::stoi( results[1] );
const int& jiffies_1 = std::stoi( results[2] );
const int& jiffies_2 = std::stoi( results[3] );
const int& jiffies_3 = std::stoi( results[4] );
const int& jiffies_4 = std::stoi( results[5] );
const int& jiffies_5 = std::stoi( results[6] );
const int& jiffies_6 = std::stoi( results[7] );
const int& jiffies_7 = std::stoi( results[8] );
const int& jiffies_8 = std::stoi( results[9] );
const int& jiffies_9 = std::stoi( results[10] );

total = jiffies_0 + jiffies_1 + jiffies_2 + jiffies_3 + jiffies_4 + jiffies_5 + jiffies_6 + jiffies_7 + jiffies_8 + jiffies_9;
working = jiffies_0 + jiffies_1 + jiffies_2;
}

#endif  // HWINFO_UNIX
