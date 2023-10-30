// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/disk.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::string& Disk::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
const std::string& Disk::model() const { return _model; }

// _____________________________________________________________________________________________________________________
const std::string& Disk::serialNumber() const { return _serialNumber; }

// _____________________________________________________________________________________________________________________
int64_t Disk::size_Bytes() const { return _size_Bytes; }

// _____________________________________________________________________________________________________________________
int Disk::id() const { return _id; }

// _____________________________________________________________________________________________________________________
bool Disk::empty() const { return (_vendor == "<unknown>" && _model == "<unknown>" && _serialNumber == "<unknown>" &&
                                   _size_Bytes == -1 && _id == -1); }

}  // namespace hwinfo
