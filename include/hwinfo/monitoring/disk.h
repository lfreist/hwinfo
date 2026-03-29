#pragma once

#include <cstdint>

#include "hwinfo/disk.h"

namespace hwinfo::monitoring::disk {

std::uint64_t get_free_size(const std::string& mount_point);

std::uint64_t get_free_size(const Disk& disk);

}  // namespace hwinfo::monitoring::disk