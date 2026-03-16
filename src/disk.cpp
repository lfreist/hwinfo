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
uint64_t Disk::size_bytes() const { return _size_Bytes; }

// _____________________________________________________________________________________________________________________
std::uint32_t Disk::id() const { return _id; }

// _____________________________________________________________________________________________________________________
const std::vector<std::string>& Disk::mount_points() const { return _mount_points; }

// _____________________________________________________________________________________________________________________
Disk::Interface Disk::interface() const { return _interface; }

}  // namespace hwinfo
