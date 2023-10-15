// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <fstream>

#include "hwinfo/disk.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  // TODO: implement
  std::vector<Disk> disks;
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE