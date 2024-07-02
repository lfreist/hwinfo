// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>
#include <hwinfo/disk.h>

#include <string>

namespace hwinfo {

// Helper function to convert CFString to std::string
std::string CFStringToString(CFStringRef cfString) {
  if (!cfString) {
    return "<unknown>";
  };
  CFIndex length = CFStringGetLength(cfString);
  CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
  char* buffer = (char*)malloc(maxSize);
  if (CFStringGetCString(cfString, buffer, maxSize, kCFStringEncodingUTF8)) {
    std::string stdString(buffer);
    free(buffer);
    return stdString;
  }
  free(buffer);
  return "<unknown>";
}

template <typename Key>
CFTypeRef GetIORegistryProperty(io_object_t ioObject, Key key) {
  // Convert key to CFString
  auto keyCF = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);

  // Get property from I/O Registry
  CFTypeRef value = IORegistryEntryCreateCFProperty(ioObject, keyCF, kCFAllocatorDefault, 0);

  if (keyCF) {
    CFRelease(keyCF);
  }

  return value;
}

std::string getDiskVendor(const io_object_t& service) {
  auto vendorRef = (CFStringRef)GetIORegistryProperty(service, "Vendor");
  std::string vendor = CFStringToString(vendorRef);
  if (vendorRef) {
    CFRelease(vendorRef);
  };
  return vendor;
}

std::string getDiskModel(const io_object_t& service) {
  auto modelRef = (CFStringRef)GetIORegistryProperty(service, kIONameMatchKey);
  std::string model = CFStringToString(modelRef);
  if (modelRef) {
    CFRelease(modelRef);
  };
  return model;
}

std::string getDiskSerialNumber(const io_object_t& service) {
  auto serialRef = (CFStringRef)GetIORegistryProperty(service, kIOPlatformSerialNumberKey);
  std::string serial = CFStringToString(serialRef);
  if (serialRef) {
    CFRelease(serialRef);
  };
  return serial;
}

int64_t getDiskSize_Bytes(const io_object_t& service) {
  CFNumberRef sizeRef = (CFNumberRef)GetIORegistryProperty(service, kIOMediaSizeKey);
  if (sizeRef) {
    int64_t size;
    CFNumberGetValue(sizeRef, kCFNumberSInt64Type, &size);
    return size;
  }

  // unknown size
  return -1;
}

// Retrieves disk information using I/O Kit
std::vector<Disk> getAllDisks() {
  auto disks = std::vector<Disk>();

  CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOMediaClass);
  CFDictionaryAddValue(matchingDict, CFSTR(kIOMediaWholeKey), kCFBooleanTrue);

  io_iterator_t iter;
  auto status = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iter);
  if (status == KERN_SUCCESS) {
    io_object_t service;
    while ((service = IOIteratorNext(iter))) {
      auto disk = Disk();
      disk._vendor = getDiskVendor(service);
      disk._model = getDiskModel(service);
      disk._serialNumber = getDiskSerialNumber(service);
      disk._size_Bytes = getDiskSize_Bytes(service);

      disks.push_back(std::move(disk));

      IOObjectRelease(service);
    }
    IOObjectRelease(iter);
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
