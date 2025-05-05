// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <IOKit/IOKitLib.h>

#include "hwinfo/mainboard.h"

#ifndef kIOMainPortDefault
#define kIOMainPortDefault kIOMasterPortDefault
#endif

namespace hwinfo {

std::string get_mainboard_property(CFStringRef property_name) {
  auto platformExpert = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
  if (!platformExpert) {
    return "<unknown>";
  }

  auto propertyRef = IORegistryEntryCreateCFProperty(platformExpert, property_name, kCFAllocatorDefault, 0);

  IOObjectRelease(platformExpert);

  if (!propertyRef) {
    return "<unknown>";
  }

  std::string result;
  if (CFGetTypeID(propertyRef) == CFStringGetTypeID()) {
    auto stringRef = static_cast<CFStringRef>(propertyRef);
    auto length = CFStringGetLength(stringRef);
    auto maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

    auto buffer = std::string(maxSize, '\0');
    if (CFStringGetCString(stringRef, buffer.data(), maxSize, kCFStringEncodingUTF8)) {
      result = buffer.c_str();  // Remove trailing nulls
    }
  }

  CFRelease(propertyRef);
  return result.empty() ? "<unknown>" : result;
}

// _____________________________________________________________________________________________________________________
MainBoard::MainBoard() {
  _vendor = "<unknown>";
  _name = "<unknown>";
  _version = "<unknown>";
  _serialNumber = get_mainboard_property(CFSTR(kIOPlatformSerialNumberKey));
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
