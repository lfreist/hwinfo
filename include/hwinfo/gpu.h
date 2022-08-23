// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <string>
#include <vector>

#ifndef HWINFO_GPU_H_
#define HWINFO_SRC_GPU_H_

namespace hwinfo {

class GPU {
 public:
  GPU();
  ~GPU();

  [[nodiscard]] std::string name() const { return _name; }
  [[nodiscard]] std::string driverVersion() const { return _driverVersion; }
  [[nodiscard]] int64_t memoryMiB() const { return _memoryMiB; }

  static std::string getName();
  static std::string getDriverVersion();
  static int64_t getMemoryMiB();

 private:
  std::string _name;
  std::string _driverVersion;
  int64_t _memoryMiB = -1;
  int _xResolution = -1;
  int _yResolution = -1;
  int _refreshRate = -1;
};

}  // namespace hwinfo

#endif //HWINFO_GPU_H_
