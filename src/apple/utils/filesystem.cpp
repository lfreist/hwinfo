#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <string>
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


void hwinfo::filesystem::get_specs_by_file_path(const std::string& path, int64_t& value) {
  value = -1;
}

void hwinfo::filesystem::get_specs_by_file_path(const std::string& path, double& value) {
  value = -1.0;
}

#endif  // HWINFO_APPLE
