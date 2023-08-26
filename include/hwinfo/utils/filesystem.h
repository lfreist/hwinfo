#pragma once

#include <string>

namespace hwinfo {
namespace filesystem {

bool exists(const std::string& path);

std::vector<std::string> getDirectoryEntries(const std::string& path);

void get_specs_by_file_path(const std::string& path, int64_t& value);
void get_specs_by_file_path(const std::string& path, double& value);

#ifdef HWINFO_UNIX
void get_jiffies(int64_t& total, int64_t& working, const int& index);
#endif

}  // namespace filesystem
}  // namespace hwinfo
