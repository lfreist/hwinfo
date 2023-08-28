#pragma once

#include <string>

namespace hwinfo {
namespace filesystem {

bool exists(const std::string& path);

std::vector<std::string> getDirectoryEntries(const std::string& path);

#if defined(HWINFO_UNIX) || defined(HWINFO_APPLE)
int64_t get_specs_by_file_path(const std::string& path);

#ifdef HWINFO_UNIX
Jiffies get_jiffies(const int& index);
#endif // HWINFO_UNIX

#endif // HWINFO_UNIX || HWINFO_APPLE

}  // namespace filesystem
}  // namespace hwinfo
