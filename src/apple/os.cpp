// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <string>

#include "hwinfo/os.h"
#include "hwinfo/utils/sysctl.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
OS::OS() {
  _name = "macOS";

  // Get kernel name and version
  _kernel = utils::getSysctlString("kern.ostype", "<unknown name> ");
  _kernel.pop_back();
  _kernel = _kernel + " " + utils::getSysctlString("kern.osrelease", "<unknown version> ");
  _kernel.pop_back();

  // get OS name and build version
  _version = utils::getSysctlString("kern.osproductversion", "<unknown> ");
  _version.pop_back();
  _version = _version + " (" + utils::getSysctlString("kern.osversion", "<unknown build> ");
  _version.pop_back();
  _version = _version + ")";

  // determine endianess
  const int byteorder = utils::getSysctlValue("hw.byteorder", 0);
  _bigEndian = (byteorder == 4321);
  _littleEndian = (byteorder == 1234);

  // TODO: Actually check
  _64bit = true;
  _32bit = !_64bit;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
