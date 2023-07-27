// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

namespace hwinfo {

#ifdef USE_OCL
struct GPU_CL {
  int id;
  std::string vendor;
  std::string name;
  std::string driver_version;
  int64_t frequency_MHz;
  int num_cores;
  int64_t memory_Bytes;
};
#endif

class GPU {
  friend std::vector<GPU> getAllGPUs();
#ifdef USE_OCL
  friend std::vector<GPU_CL> get_cpu_cl_data();
#endif

 public:
  ~GPU() = default;

  const std::string& vendor() const;
  const std::string& name() const;
  const std::string& driverVersion() const;
  int64_t memory_Bytes() const;
  int64_t frequency_MHz() const;
  int num_cores() const;
  int id() const;

 private:
  GPU() = default;
  std::string _vendor{};
  std::string _name{};
  std::string _driverVersion{};
  int64_t _memory_Bytes{0};
  int64_t _frequency_MHz{0};
  int _num_cores{0};
  int _id{0};
};

std::vector<GPU> getAllGPUs();

std::vector<GPU_CL> get_cpu_cl_data();

}  // namespace hwinfo
