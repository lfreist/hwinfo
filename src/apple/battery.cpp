// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>

#include <iostream>

#include "hwinfo/battery.h"

namespace hwinfo {

// =====================================================================================================================
CFDictionaryRef getPowerSource(const int id) {
  const CFTypeRef powerInfo = IOPSCopyPowerSourcesInfo();
  if (!powerInfo) {
    return nullptr;
  }

  const CFArrayRef powerSources = IOPSCopyPowerSourcesList(powerInfo);
  if (!powerSources) {
    CFRelease(powerInfo);
    return nullptr;
  }

  const CFDictionaryRef powerSource =
      IOPSGetPowerSourceDescription(powerInfo, CFArrayGetValueAtIndex(powerSources, id));

  if (!powerSource) {
    CFRelease(powerSources);
    CFRelease(powerInfo);
    return nullptr;
  }

  CFRetain(powerSource);

  CFRelease(powerSources);
  CFRelease(powerInfo);

  return powerSource;
}

// _____________________________________________________________________________________________________________________
std::string Battery::getVendor() const { return "<unknown>"; }

// _____________________________________________________________________________________________________________________
std::string Battery::getModel() const { return "<unknown>"; }

// _____________________________________________________________________________________________________________________
std::string Battery::getSerialNumber() const {
  const CFDictionaryRef powerSource = getPowerSource(_id);
  if (!powerSource) {
    return "<unknown>";
  }

  // this key is recommended. it may be empty
  const auto serialNumber =
      static_cast<CFStringRef>(CFDictionaryGetValue(powerSource, CFSTR(kIOPSHardwareSerialNumberKey)));

  if (!serialNumber) {
    CFRelease(powerSource);
    return "<unknown>";
  }

  char serialNumberStr[256];

  CFStringGetCString(serialNumber, serialNumberStr, sizeof(serialNumberStr), kCFStringEncodingUTF8);
  CFRelease(powerSource);

  return serialNumberStr;
}

// _____________________________________________________________________________________________________________________
std::string Battery::getTechnology() const { return "<unknown>"; }

// _____________________________________________________________________________________________________________________
uint32_t Battery::getEnergyFull() const {
  const CFDictionaryRef powerSource = getPowerSource(_id);
  if (!powerSource) {
    return 0;
  }

  const auto maxCapacityNum = static_cast<CFNumberRef>(CFDictionaryGetValue(powerSource, CFSTR(kIOPSMaxCapacityKey)));
  CFRelease(powerSource);
  if (!maxCapacityNum) {
    return 0;
  }

  uint32_t maxCapacity;
  CFNumberGetValue(maxCapacityNum, kCFNumberIntType, &maxCapacity);

  return maxCapacity;
}

// _____________________________________________________________________________________________________________________
uint32_t Battery::energyNow() const {
  const CFDictionaryRef powerSource = getPowerSource(_id);
  if (!powerSource) {
    return 0;
  }

  const auto currentCapacityNum =
      static_cast<CFNumberRef>(CFDictionaryGetValue(powerSource, CFSTR(kIOPSCurrentCapacityKey)));
  CFRelease(powerSource);
  if (!currentCapacityNum) {
    return 0;
  }

  uint32_t currentCapacity;
  CFNumberGetValue(currentCapacityNum, kCFNumberIntType, &currentCapacity);

  return currentCapacity;
}

// _____________________________________________________________________________________________________________________
bool Battery::charging() const {
  const CFDictionaryRef powerSource = getPowerSource(_id);
  if (!powerSource) {
    return "<unknown>";
  }

  const auto isCharging = static_cast<CFBooleanRef>(CFDictionaryGetValue(powerSource, CFSTR(kIOPSIsChargingKey)));

  return isCharging == kCFBooleanTrue;
}

// _____________________________________________________________________________________________________________________
bool Battery::discharging() const { return !charging(); }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Battery> getAllBatteries() {
  std::vector<Battery> batteries;

  const CFTypeRef powerInfo = IOPSCopyPowerSourcesInfo();
  if (!powerInfo) {
    return batteries;
  }

  const CFArrayRef powerSources = IOPSCopyPowerSourcesList(powerInfo);
  if (!powerSources) {
    CFRelease(powerInfo);
    return batteries;
  }

  const int numSources = static_cast<int>(CFArrayGetCount(powerSources));

  CFRelease(powerSources);
  CFRelease(powerInfo);

  for (int i = 0; i < numSources; ++i) {
    batteries.emplace_back(i);
  }

  return batteries;
}

}  // namespace hwinfo

#endif