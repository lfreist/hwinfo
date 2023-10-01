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

int64_t hwinfo::filesystem::get_specs_by_file_path(const std::string& path) { return -1; }

#endif  // HWINFO_APPLE
