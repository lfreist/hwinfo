// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/disk.h"

#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::string& Disk::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
const std::string& Disk::model() const { return _model; }

// _____________________________________________________________________________________________________________________
const std::string& Disk::serialNumber() const { return _serialNumber; }

// _____________________________________________________________________________________________________________________
int64_t Disk::size_Bytes() const { return _size_Bytes; }

}  // namespace hwinfo
