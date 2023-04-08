#pragma once

#include <string>

namespace hwinfo {
namespace filesystem {

bool exists(const std::string& path);

std::vector<std::string> getDirectoryEntries(const std::string& path);

}  // namespace filesystem
}  // namespace hwinfo
