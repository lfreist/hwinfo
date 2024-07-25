// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#ifndef CL_HPP_TARGET_OPENCL_VERSION
#define CL_HPP_TARGET_OPENCL_VERSION 200
#endif
#define CL_HPP_ENABLE_EXCEPTIONS
#include <cstdint>
#include <iostream>

#include "../../../cmake-build-release/_deps/opencl-src/external/OpenCL-CLHPP/include/CL/opencl.hpp"

namespace opencl_ {

/**
 * @brief Device represents a single OpenCL Device.
 *        It provides instant methods for retrieving common data.
 */
class Device {
  template <unsigned dimension, typename T>
  friend class Memory;
  friend class DeviceManager;
  friend std::ostream& operator<<(std::ostream& os, const Device& device);

 public:
  /**
   * @brief Device can only be constructed using an explicit id, a cl::Device and a cl::Context.
   *
   *        It is highly recommended, not to construct devices by hand but rather by using the DeviceManager class.
   */
  Device(uint32_t id, cl::Device cl_device);
  ~Device();

  /// Copy Constructor
  Device(const Device& device) = delete;
  /// Copy Assignment Operator
  Device& operator=(const Device& device) = delete;

  /// Move Constructor
  Device(Device&& device) noexcept;
  /// Move Assignment Operator
  Device& operator=(Device&& device) noexcept;

  /**
   * @brief A Device can either be a GPU or a CPU.
   */
  enum Type { GPU, CPU };

  /**
   * @brief Returns the id of the Device that was set with the constructor.
   */
  [[nodiscard]] uint32_t get_id() const;

  /**
   * @brief Returns a const reference to the underlying cl::Device.
   */
  [[nodiscard]] const cl::Device& get_cl_device() const;
  /**
   * @brief Returns a reference to the underlying cl::Device.
   * @return
   */
  cl::Device& get_cl_device();

  /**
   * @brief Returns the name of the device.
   */
  [[nodiscard]] std::string name() const;

  /**
   * @brief Returns the vendor of the device.
   */
  [[nodiscard]] std::string vendor() const;

  /**
   * @brief Returns the driver version of the device.
   */
  [[nodiscard]] std::string driver_version() const;

  /**
   * @brief Returns the OpenCL C version of the device.
   */
  [[nodiscard]] std::string opencl_c_version() const;

  /**
   * @brief Returns the size of the memory in Bytes.
   */
  [[nodiscard]] uint64_t memory_Bytes() const;

  /**
   * @brief Returns the amount of memory used in Bytes.
   *
   *        This value is only correct, if the mcl::Memory object was used for memory management and if this Device was
   *        correctly passed to the mcl::Memory instance.
   */
  [[nodiscard]] uint64_t memory_used_Bytes() const;

  /**
   * @brief Returns the size of the global cache in Bytes.
   */
  [[nodiscard]] uint64_t global_cache_Bytes() const;

  /**
   * @brief Returns the size of the local cache in Bytes.
   */
  [[nodiscard]] uint64_t local_cache_Bytes() const;

  /**
   * @brief Returns the size of the global buffer in Bytes.
   */
  [[nodiscard]] uint64_t max_global_buffer_Bytes() const;

  /**
   * @brief Returns the size of the constant buffer in Bytes.
   */
  [[nodiscard]] uint64_t max_constant_buffer_Bytes() const;

  /**
   * @brief Returns the number of compute units of the device.
   *
   *        Note that each compute unit can have different number of cores. In order to get the actual number of cores,
   *        you should call Device::cores().
   */
  [[nodiscard]] uint64_t compute_units() const;

  /**
   * @brief Returns the total number of cores of the device.
   */
  [[nodiscard]] uint64_t cores() const;

  /**
   * @brief Returns the clock frequency in Hz (ticks per second).
   */
  [[nodiscard]] uint64_t clock_frequency_MHz() const;

  /**
   * @brief Returns the type of the device (either GPU or CPU) as Device::Type.
   */
  [[nodiscard]] Type type() const;

  /**
   * @brief Returns the native vector width for double size floating point numbers.
   *
   *        Returns 0 if the device does not support operating on double size floating points.
   */
  [[nodiscard]] uint64_t fp64() const;

  /**
   * @brief Returns the native vector width for floating point numbers.
   */
  [[nodiscard]] uint64_t fp32() const;

  /**
   * @brief Returns the native vector width for half size floating point numbers.
   *
   *        Returns 0 if the device does not support operating on half size floating points.
   */
  [[nodiscard]] uint64_t fp16() const;

  /**
   * @brief Returns the native vector width for 64 bit integers.
   */
  [[nodiscard]] uint64_t int64() const;

  /**
   * @brief Returns the native vector width for 32 bit integers.
   */
  [[nodiscard]] uint64_t int32() const;

  /**
   * @brief Returns the native vector width for 16 bit integers.
   */
  [[nodiscard]] uint64_t int16() const;

  /**
   * @brief Returns the native vector width for 8 bit integers.
   */
  [[nodiscard]] uint64_t int8() const;

  /**
   * @brief Returns the estimated amount of floating point operations per second in FLOPS/second.
   *
   *        This value is estimated using the number of cores, the instructions per cycle and the frequency of the
   *        device. Therefore, the real FLOPS/second value may differ.
   */
  [[nodiscard]] uint64_t estimated_flops() const;

  [[nodiscard]] bool intel_gt_4gb_buffer_required() const;

 private:
  uint64_t _compute_cores();

  /// set by constructor
  cl::Device _cl_device;

  /// set by constructor
  uint32_t _id;
  /// set by constructor
  uint32_t _instructions_per_cycle;
  /// set by constructor via _compute_cores()
  uint64_t _cores;
  /// set by mcl::Memory
  uint64_t _memory_used_Bytes{0};

  /// set by _compute_cores()
  bool _intel_gt_4gb_buffer_required{false};

  /// used by _compute_cores()
  const std::vector<std::string> nvidia_192{"gt 6", "gt 7", "gtx 6", "gtx 7", "quadro k", "tesla k"};
  /// used by _compute_cores()
  const std::vector<std::string> nvidia_64{"p100",    "v100",      "a100",     "a30",     " 16",       " 20",
                                           "titan v", "titan rtx", "quadro t", "tesla t", "quadro rtx"};
};

/**
 * @brief Provide a naive ostream output for a Device.
 *
 * format: "[GPU|CPU]: <name> (<id>, <vendor>)"
 */
std::ostream& operator<<(std::ostream& os, const Device& device);

std::ostream& operator<<(std::ostream& os, const Device::Type& type);

// ===== DeviceManager =================================================================================================
/**
 * @brief enum values used to retrieve specific devices using the mcl::DeviceManager
 */
enum class Filter {
  MAX_MEMORY,  // Device with most memory
  MIN_MEMORY,  // Device with the smallest memory
  MAX_FLOPS,   // Device with most estimated FLOPS
  MIN_FLOPS,   // Device with least estimated FLOPS
  GPU,         // All GPU devices
  CPU,         // ALL CPU devices
  ID,          // Device by ID
  ALL          // All Devices
};

class DeviceManager {
 public:
  /**
   * @brief Used to retrieve one specific device.
   *
   *        This method is implemented for
   *        T = MAX_MEMORY
   *            MIN_MEMORY
   *            MAX_FLOPS
   *            MIN_FLOPS
   */
  template <Filter T>
  static Device* get();

  /**
   * @brief Used to retrieve multiple devices.
   *
   *        This method is implemented for
   *        T = GPU
   *            CPU
   *            ALL
   */
  template <Filter T>
  static std::vector<Device*> get_list();

  /**
   * @brief Used to retrieve one specific device by id (= value).
   *
   *        This method is implemented for
   *        T = ID
   */
  template <Filter T>
  static Device* get(uint32_t value);

 private:
  DeviceManager();
  static DeviceManager& get_instance();

  std::vector<Device> _devices;
};

}  // namespace opencl_