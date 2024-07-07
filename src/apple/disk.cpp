// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#include <cstdint>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <hwinfo/disk.h>

#include <string>

namespace hwinfo {

/**
  Converts a CFStringRef to a std::string
 */
std::string cf_to_std(CFStringRef cfString) {
  if (cfString == nullptr) {
    return "<unknown>";
  }

  CFIndex length = CFStringGetLength(cfString);
  CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

  // Initialize std::string with maxSize and fill with null characters
  auto out = std::string(maxSize, '\0');

  // Fill std::string with the actual string
  auto success = CFStringGetCString(cfString, const_cast<char*>(out.data()), maxSize, kCFStringEncodingUTF8);

  if (!success) {
    return "<unknown>";
  }

  // Resize the string to the actual length
  out.resize(strlen(out.c_str()));
  return out;
}

/**
  Converts a CFNumberRef to a number of type ReturnType
 */
template <typename NumberType>
NumberType cf_to_std(CFNumberRef raw, CFNumberType cfNumberEnum) {
  if (raw == nullptr) {
    return NumberType();
  }

  NumberType out;
  CFNumberGetValue(raw, cfNumberEnum, &out);

  return out;
}

int64_t cf_to_std(CFNumberRef raw) { return cf_to_std<int64_t>(raw, kCFNumberSInt64Type); }

template <typename ReturnType, typename CFType>
ReturnType getIORegistryProperty(io_object_t service, CFStringRef key) {
  // Get the property from I/O Registry
  auto raw = static_cast<CFType>(IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0));

  // Convert the property to a output type
  ReturnType out = cf_to_std(raw);

  // Release the property
  if (raw) {
    CFRelease(raw);
  };

  return out;
}

// Retrieves disk information using I/O Kit
std::vector<Disk> getAllDisks() {
  auto disks = std::vector<Disk>();

  CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOMediaClass);
  CFDictionaryAddValue(matchingDict, CFSTR(kIOMediaWholeKey), kCFBooleanTrue);

  io_iterator_t iter;
  if (IOServiceGetMatchingServices(0, matchingDict, &iter) == KERN_SUCCESS) {
    int i_disk = 0;
    while (true) {
      auto service = IOIteratorNext(iter);
      if (service == 0) {
        break;
      }

      auto disk = Disk();

      disk._id = i_disk;

      // get the name of the IO service
      std::string model;
      model.resize(128);
      if (IORegistryEntryGetName(service, const_cast<char*>(model.data())) != KERN_SUCCESS) {
        model = "<unknown>";
      }
      model.resize(strlen(model.c_str()));
      disk._model = model;

      // guess the vendor from the model
      if (model.find("APPLE") != std::string::npos || model.find("Apple") != std::string::npos) {
        disk._vendor = "Apple";
      } else {
        disk._vendor = "<unknown>";
      }

      disk._serialNumber = getIORegistryProperty<std::string, CFStringRef>(service, CFSTR(kIOMediaUUIDKey));

      disk._size_Bytes = getIORegistryProperty<int64_t, CFNumberRef>(service, CFSTR(kIOMediaSizeKey));

      disks.push_back(std::move(disk));

      IOObjectRelease(service);

      i_disk++;
    }
    IOObjectRelease(iter);
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
