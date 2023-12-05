#include "hwinfo/bios.h"
#include <string>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string BIOS::name() const { return _name; }

// _____________________________________________________________________________________________________________________
std::string BIOS::serialNumber() const { return _serialNumber; }

}