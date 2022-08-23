// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <string>

#ifndef HWINFO_OS_H_
#define HWINFO_OS_H_

namespace hwinfo {

class OS {
 public:
  OS();
  ~OS();

  std::string fullName();
  std::string name();
  std::string version();
  std::string kernel();
  [[nodiscard]] bool is32bit() const;
  [[nodiscard]] bool is64bit() const;
  [[nodiscard]] bool isBigEndian() const;
  [[nodiscard]] bool isLittleEndian() const;

  static std::string getFullName();
  static std::string getName();
  static std::string getVersion();
  static std::string getKernel();
  static bool getIs32bit();
  static bool getIs64bit();
  static bool getIsBigEndian();
  static bool getIsLittleEndian();

 private:
  std::string _fullName;
  std::string _name;
  std::string _version;
  std::string _kernel;
  bool _32bit;
  bool _64bit;
  bool _bigEndian;
  bool _littleEndian;
};

}  // namespace hwinfo

#endif //HWINFO_OS_H_
