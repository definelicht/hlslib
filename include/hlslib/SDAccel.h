/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      April 2016
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Configuration taken from Xilinx' xcl2 utility functions to maximize odds of
// everything working.
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl2.hpp>

namespace hlslib {

namespace ocl {

//#############################################################################
// Enumerations
//#############################################################################

/// Enum for type of memory access to device buffers. Will cause the
/// the appropriate binary flags to be set when allocating OpenCL buffers.
enum class Access { read, write, readWrite };

/// Mapping to specific memory banks on the FPGA. Xilinx-specific.
enum class MemoryBank { unspecified, bank0, bank1, bank2, bank3 };

//#############################################################################
// OpenCL exceptions
//#############################################################################

class ConfigurationError : public std::logic_error {
 public:
  ConfigurationError(std::string const &message) : std::logic_error(message) {}

  ConfigurationError(char const *const message) : std::logic_error(message) {}
};

class RuntimeError : public std::runtime_error {
 public:
  RuntimeError(std::string const &message) : std::runtime_error(message) {}

  RuntimeError(char const *const message) : std::runtime_error(message) {}
};

//#############################################################################
// Internal functionality
//#############################################################################

namespace {

constexpr bool kVerbose = true;

#ifndef HLSLIB_LEGACY_SDX
#warning "HLSLIB_LEGACY_SDX not set: assuming SDAccel 2017.4+. Set to 0 or 1 to silence this warning."
#define HLSLIB_LEGACY_SDX 0
#endif

#if HLSLIB_LEGACY_SDX == 0
constexpr auto kXilinxMemPointer = CL_MEM_EXT_PTR_XILINX;
constexpr auto kXilinxBank0 = XCL_MEM_DDR_BANK0;
constexpr auto kXilinxBank1 = XCL_MEM_DDR_BANK1;
constexpr auto kXilinxBank2 = XCL_MEM_DDR_BANK2;
constexpr auto kXilinxBank3 = XCL_MEM_DDR_BANK3;
using ExtendedMemoryPointer = cl_mem_ext_ptr_t;
#else
// Before 2017.4, these values were only available numerically, and the
// extended memory pointer had to be constructed manually.
constexpr cl_mem_flags kXilinxMemPointer = 1 << 31;
constexpr unsigned kXilinxBank0 = 1 << 8;
constexpr unsigned kXilinxBank1 = 1 << 9;
constexpr unsigned kXilinxBank2 = 1 << 10;
constexpr unsigned kXilinxBank3 = 1 << 11;
struct ExtendedMemoryPointer {
  unsigned flags;
  void *obj;
  void *param;
};
#endif


template <typename IteratorType>
constexpr bool IsRandomAccess() {
  return std::is_base_of<
      std::random_access_iterator_tag,
      typename std::iterator_traits<IteratorType>::iterator_category>::value;
}

template <typename IteratorType, typename T>
constexpr bool IsIteratorOfType() {
  return std::is_same<typename std::iterator_traits<IteratorType>::value_type,
                      T>::value;
}

void ThrowConfigurationError(std::string const &message) {
#ifndef HLSLIB_DISABLE_EXCEPTIONS
  throw ConfigurationError(message);
#else
  std::cerr << "SDAccel [Configuration Error]: " << message << std::endl;
#endif
}

void ThrowRuntimeError(std::string const &message) {
#ifndef HLSLIB_DISABLE_EXCEPTIONS
  throw RuntimeError(message);
#else
  std::cerr << "SDAccel [Runtime Error]: " << message << std::endl;
#endif
}

//-----------------------------------------------------------------------------
// Free functions (for internal use)
//-----------------------------------------------------------------------------

std::vector<cl::Platform> GetAvailablePlatforms() {
  std::vector<cl::Platform> platforms;
  cl_int errorCode = cl::Platform::get(&platforms);
  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve OpenCL platforms.");
    return {};
  }
  return platforms;
}

std::string GetPlatformVendor(cl::Platform platform) {
  cl_int errorCode;
  std::string vendorName = platform.getInfo<CL_PLATFORM_NAME>(&errorCode);
  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve platform vendor name.");
    return {};
  }
  return vendorName;
}

cl::Platform FindPlatformByVendor(std::string const &desiredVendor) {
  cl::Platform platform;

  auto available = GetAvailablePlatforms();
  if (available.size() == 0) {
    ThrowConfigurationError("No available OpenCL platforms.");
    return platform;
  }

  bool foundVendor = false;
  for (auto &p : available) {
    auto vendorName = GetPlatformVendor(p);
    if (std::string(vendorName.data()).find(desiredVendor) !=
        std::string::npos) {
      foundVendor = true;
      platform = p;
      break;
    }
  }
  if (!foundVendor) {
    std::stringstream ss;
    ss << "Platform \"" << desiredVendor << "\" not found.";
    ThrowConfigurationError(ss.str());
    return platform;
  }

  return platform;
}

std::vector<cl::Device> GetAvailableDevices(cl::Platform const &platform) {

  std::vector<cl::Device> devices;
  auto errorCode = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve device IDs.");
    return {};
  }

  return devices;
}

std::string GetDeviceName(cl::Device const &device) {
  cl_int errorCode;
  std::string deviceName = device.getInfo<CL_DEVICE_NAME>(&errorCode);
  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve device info.");
    return {};
  }
  return deviceName;
}

cl::Device FindDeviceByName(cl::Platform const &platform,
                              std::string const &desiredDevice) {
  cl::Device device;
  auto available = GetAvailableDevices(platform);
  if (available.size() == 0) {
    ThrowConfigurationError("No compute devices found.");
    return {};
  }

  bool deviceFound = false;
  for (auto &d : available) {
    auto deviceName = GetDeviceName(d);
    if (deviceName.find(desiredDevice) != std::string::npos) {
      device = d;
      deviceFound = true;
      break;
    }
  }

  if (!deviceFound) {
    std::stringstream ss;
    ss << "Failed to find device \"" << desiredDevice << "\".";
    ThrowConfigurationError(ss.str());
    return {};
  }

  return device;
}

cl::Context CreateComputeContext(std::vector<cl::Device> const &devices) {
  // TODO: support properties and callback functions?
  cl_int errorCode;
  cl::Context context(devices, nullptr, nullptr, nullptr, &errorCode);
  if (errorCode != CL_SUCCESS) {
    ThrowRuntimeError("Failed to create compute context.");
  }
  return context;
}

cl::Context CreateComputeContext(cl::Device const &device) {
  std::vector<cl::Device> devices = {device};
  return CreateComputeContext(devices);
}

cl::CommandQueue CreateCommandQueue(cl::Context const &context,
                                    cl::Device const &device) {
  cl_int errorCode;
  cl::CommandQueue commandQueue(
      context, device,
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE,
      &errorCode);
  if (errorCode != CL_SUCCESS) {
    ThrowRuntimeError("Failed to create command queue.");
  }
  return commandQueue;
}

}  // End anonymous namespace

// Forward declarations for use in MakeProgram and MakeKernel signatures 

template <typename, Access>
class Buffer;

class Kernel;

class Program;

//#############################################################################
// Context
//#############################################################################

class Context {
 public:
  /// Performs initialization of the requested device
  inline Context(std::string const &vendorName, std::string const &deviceName) {
#ifndef HLSLIB_SIMULATE_OPENCL
    // Find requested OpenCL platform
    platformId_ = FindPlatformByVendor(vendorName);

    // Find requested compute device
    device_ = FindDeviceByName(platformId_, deviceName);

    context_ = CreateComputeContext(device_);

    commandQueue_ = CreateCommandQueue(context_, device_);
#endif
  }

  /// Performs initialization of first available device of requested vendor
  inline Context() {
#ifndef HLSLIB_SIMULATE_OPENCL
    // Find requested OpenCL platform
    platformId_ = FindPlatformByVendor("Xilinx");

    auto devices = GetAvailableDevices(platformId_);
    if (devices.size() == 0) {
      ThrowConfigurationError("No OpenCL devices found for platform.");
      return;
    }
    device_ = devices[0];
    if (kVerbose) {
      std::cout << "Using device \"" << GetDeviceName(device_) << "\".\n";
    }

    context_ = CreateComputeContext(device_);

    commandQueue_ = CreateCommandQueue(context_, device_);
#endif
  }

  inline Context(Context const &) = delete;
  inline Context(Context &&) = default;
  inline Context &operator=(Context const &) = delete;
  inline Context &operator=(Context &&) = default;

  inline ~Context() = default;

  /// Create an OpenCL program from the given binary, from which kernels can be
  /// instantiated and executed.
  inline Program MakeProgram(std::string const &path);

  /// Returns the internal OpenCL device id.
  inline cl::Device const &device() const { return device_; }

  inline std::string DeviceName() const {
#ifndef HLSLIB_SIMULATE_OPENCL
    return GetDeviceName(device_);
#else
    return "Simulation";  
#endif
  }

  /// Returns the internal OpenCL execution context.
  inline cl::Context const &context() const { return context_; }

  /// Returns the internal OpenCL command queue.
  inline cl::CommandQueue const &commandQueue() const { return commandQueue_; }

  inline cl::CommandQueue &commandQueue() { return commandQueue_; }

  inline Program CurrentlyLoadedProgram() const;

  template <typename T, Access access, typename... Ts>
  Buffer<T, access> MakeBuffer(Ts &&... args);

 protected:
  friend Program;
  friend Kernel;
  template <typename U, Access access> friend class Buffer;

  void RegisterProgram(
      std::shared_ptr<std::pair<std::string, cl::Program>> program) {
    loadedProgram_ = program;
  }

  std::mutex &memcopyMutex() {
    return memcopyMutex_;
  }

  std::mutex &enqueueMutex() {
    return enqueueMutex_;
  }

  std::mutex &reprogramMutex() {
    return reprogramMutex_;
  }

 private:
  cl::Platform platformId_{};
  cl::Device device_{};
  cl::Context context_{};
  cl::CommandQueue commandQueue_{};
  std::shared_ptr<std::pair<std::string, cl::Program>> loadedProgram_{nullptr};
  std::mutex memcopyMutex_;
  std::mutex enqueueMutex_;
  std::mutex reprogramMutex_;

};  // End class Context

/// Implement a singleton pattern for an SDAccel context.
/// It is NOT recommend to use this if it can be avoided, but might be
/// necssary when linking with external libraries.
inline Context& GlobalContext() {
  static Context singleton;
  return singleton;
}

//#############################################################################
// Buffer
//#############################################################################

template <typename T, Access access>
class Buffer {

 public:
  Buffer() : context_(nullptr), nElements_(0) {}

  Buffer(Buffer<T, access> const &other) = delete;

  Buffer(Buffer<T, access> &&other) : Buffer() { swap(*this, other); }

  /// Allocate and copy to device.
  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  Buffer(Context &context, MemoryBank memoryBank, IteratorType begin,
         IteratorType end)
      : context_(&context), nElements_(std::distance(begin, end)) {

#ifndef HLSLIB_SIMULATE_OPENCL

    void *hostPtr = const_cast<T *>(&(*begin));

    cl_mem_flags flags = CL_MEM_USE_HOST_PTR;

    switch (access) {
      case Access::read:
        flags |= CL_MEM_READ_ONLY;
        break;
      case Access::write:
        flags |= CL_MEM_WRITE_ONLY;
        break;
      case Access::readWrite:
        flags |= CL_MEM_READ_WRITE;
        break;
    }

    // Allow specifying memory bank
    ExtendedMemoryPointer extendedHostPointer;
    if (memoryBank != MemoryBank::unspecified) {
      extendedHostPointer = CreateExtendedPointer(hostPtr, memoryBank);
      // Replace hostPtr with Xilinx extended pointer
      hostPtr = &extendedHostPointer;
      flags |= kXilinxMemPointer;
    }

    cl_int errorCode;
    // If a memory bank was specified, pass the Xilinx-specific extended memory
    // pointer. Otherwise, pass the host pointer directly.
    devicePtr_ = cl::Buffer(context.context(), flags, sizeof(T) * nElements_,
                            hostPtr, &errorCode);

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize and copy to device memory.");
      return;
    }
#else
    devicePtr_ = std::vector<T>(nElements_);
    std::copy(begin, end, devicePtr_.begin());
#endif
  }

  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  Buffer(Context &context, IteratorType begin, IteratorType end)
      : Buffer(context, MemoryBank::unspecified, begin, end) {}

  /// Allocate device memory but don't perform any transfers.
  Buffer(Context &context, MemoryBank memoryBank, size_t nElements)
      : context_(&context), nElements_(nElements) {
#ifndef HLSLIB_SIMULATE_OPENCL

    cl_mem_flags flags;
    switch (access) {
      case Access::read:
        flags = CL_MEM_READ_ONLY;
        break;
      case Access::write:
        flags = CL_MEM_WRITE_ONLY;
        break;
      case Access::readWrite:
        flags = CL_MEM_READ_WRITE;
        break;
    }

    ExtendedMemoryPointer extendedHostPointer;
    void *hostPtr = nullptr;
    if (memoryBank != MemoryBank::unspecified) {
      extendedHostPointer = CreateExtendedPointer(nullptr, memoryBank);
      // Becomes a pointer to the Xilinx extended memory pointer if a memory
      // bank is specified
      hostPtr = &extendedHostPointer;
      flags |= kXilinxMemPointer;
    }

    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex()); 
      devicePtr_ = cl::Buffer(context_->context(), flags, sizeof(T) * nElements_,
                              hostPtr, &errorCode);
    }

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize device memory.");
      return;
    }
#else
    devicePtr_ = std::vector<T>(nElements_);
#endif
  }

  Buffer(Context &context, size_t nElements)
      : Buffer(context, MemoryBank::unspecified, nElements) {}

  friend void swap(Buffer<T, access> &first, Buffer<T, access> &second) {
    std::swap(first.context_, second.context_);
    std::swap(first.devicePtr_, second.devicePtr_);
    std::swap(first.nElements_, second.nElements_);
  }

  Buffer<T, access> &operator=(Buffer<T, access> const &other) = delete;

  Buffer<T, access> &operator=(Buffer<T, access> &&other) {
    swap(*this, other);
    return *this;
  }

  ~Buffer() = default;

  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  void CopyFromHost(int deviceOffset, int numElements, IteratorType source) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl::Event event;
    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      errorCode = context_->commandQueue().enqueueWriteBuffer(
          devicePtr_, CL_TRUE, sizeof(T) * deviceOffset,
          sizeof(T) * numElements, const_cast<T *>(&(*source)), nullptr,
          &event);
    }
    // Don't need to wait for event because of blocking call (CL_TRUE)
    if (errorCode != CL_SUCCESS) {
      throw std::runtime_error("Failed to copy data to device.");
    }
#else
    std::copy(source, source + numElements, devicePtr_.begin() + deviceOffset);
#endif
  }

  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  void CopyFromHost(IteratorType source) {
    return CopyFromHost(0, nElements_, source);
  }

  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  void CopyToHost(size_t deviceOffset, size_t numElements, IteratorType target) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl::Event event;
    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      errorCode = context_->commandQueue().enqueueReadBuffer(
          devicePtr_, CL_TRUE, sizeof(T) * deviceOffset,
          sizeof(T) * numElements, &(*target), nullptr, &event);
    }
    // Don't need to wait for event because of blocking call (CL_TRUE)
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy back memory from device.");
      return;
    }
#else
    std::copy(devicePtr_.begin() + deviceOffset,
              devicePtr_.begin() + deviceOffset + numElements, target);
#endif
  }

  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  void CopyToHost(IteratorType target) {
    return CopyToHost(0, nElements_, target);
  }

  template <Access accessType>
  void CopyToDevice(size_t offsetSource, size_t numElements,
                    Buffer<T, accessType> &other, size_t offsetDestination) {
#ifndef HLSLIB_SIMULATE_OPENCL
    if (offsetSource + numElements > nElements_ ||
        offsetDestination + numElements > other.nElements()) {
      ThrowRuntimeError("Device to device copy interval out of range.");
    }
    cl::Event event;
    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      errorCode = context_->commandQueue().enqueueCopyBuffer(
          devicePtr_, other.devicePtr(), sizeof(T) * offsetSource,
          sizeof(T) * offsetDestination, numElements * sizeof(T), nullptr,
          &event);
    }
    event.wait();
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy from device to device.");
      return;
    }
#else
    std::copy(devicePtr_.begin() + offsetSource,
              devicePtr_.begin() + offsetSource + numElements,
              other.devicePtr_.begin() + offsetDestination);
#endif
  }

  template <Access accessType>
  void CopyToDevice(size_t offsetSource, size_t numElements,
                    Buffer<T, accessType> &other) {
    CopyToDevice(offsetSource, numElements, other, 0);
  }

  template <Access accessType>
  void CopyToDevice(Buffer<T, accessType> &other) {
    if (other.nElements() != nElements_) {
      ThrowRuntimeError(
          "Device to device copy issued for buffers of different size.");
    }
    CopyToDevice(0, nElements_, other, 0);
  }

  
#ifndef HLSLIB_SIMULATE_OPENCL
  cl::Buffer const &devicePtr() const { return devicePtr_; }

  cl::Buffer &devicePtr() { return devicePtr_; }
#else
  T const *devicePtr() const { return devicePtr_.data(); }

  T *devicePtr() { return devicePtr_.data(); }
#endif

  size_t nElements() const { return nElements_; }

 private:
  ExtendedMemoryPointer CreateExtendedPointer(void *hostPtr,
                                              MemoryBank memoryBank) {
    ExtendedMemoryPointer extendedPointer;
    switch (memoryBank) {
      case MemoryBank::bank0:
        extendedPointer.flags = kXilinxBank0;
        break;
      case MemoryBank::bank1:
        extendedPointer.flags = kXilinxBank1;
        break;
      case MemoryBank::bank2:
        extendedPointer.flags = kXilinxBank2;
        break;
      case MemoryBank::bank3:
        extendedPointer.flags = kXilinxBank3;
        break;
      case MemoryBank::unspecified:
        ThrowRuntimeError(
            "Tried to create Xilinx extended memory"
            " pointer for unspecified bank");
        break;
    }
    extendedPointer.obj = hostPtr;
    extendedPointer.param = 0;
    return extendedPointer;
  }

  Context *context_;
#ifndef HLSLIB_SIMULATE_OPENCL
  cl::Buffer devicePtr_{};
#else
  std::vector<T> devicePtr_{};
#endif
  size_t nElements_;

};  // End class Buffer

//#############################################################################
// Program
//#############################################################################

/// Represents an OpenCL program, which can contain multiple kernels. This is
/// relevant for FPGA bitstreams generated from multiple OpenCL kernels, or
/// that use multiple compute units mapped to different outputs, as we need
/// to address different kernels within the same program.
class Program {

 public:

  Program() = delete;
  Program(Program const&) = default;
  Program(Program &&) = default;
  ~Program() = default; 

  // Returns the reference Context object.
  inline Context &context() { return context_; }

  // Returns the reference Context object.
  inline Context const &context() const { return context_; }

  // Returns the internal OpenCL program object.
  inline cl::Program &program() { return program_->second; }

  // Returns the internal OpenCL program object.
  inline cl::Program const &program() const { return program_->second; }

  // Returns the path to the loaded program
  inline std::string const &path() const { return program_->first; }

  /// Create a kernel with the specified name contained in this loaded OpenCL
  /// program, binding the argument to the passed ones.
  template <typename... Ts>
  Kernel MakeKernel(std::string const &kernelName, Ts &&... args);

  /// Additionally allows passing a function pointer to a host implementation of
  /// the kernel function for simulation and verification purposes.
  template <class F, typename... Ts>
  Kernel MakeKernel(F &&hostFunction, std::string const &kernelName,
                    Ts &&... args);

 protected:

  friend Context;

  inline Program(Context &context,
                 std::shared_ptr<std::pair<std::string, cl::Program>> program)
      : context_(context), program_(program) {}

 private:
  Context &context_;
  std::shared_ptr<std::pair<std::string, cl::Program>> program_;
};

//#############################################################################
// Kernel
//#############################################################################

class Kernel {
 private:
  template <typename T, Access access>
  void SetKernelArguments(size_t index, Buffer<T, access> &arg) {
    cl::Buffer devicePtr = arg.devicePtr();
    auto errorCode = kernel_.setArg(index, devicePtr);
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to set kernel argument " << index << ".";
      ThrowConfigurationError(ss.str());
      return;
    }
  }

  template <typename T>
  void SetKernelArguments(size_t index, T &&arg) {
    auto errorCode = kernel_.setArg(index, arg);
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to set kernel argument " << index << ".";
      ThrowConfigurationError(ss.str());
      return;
    }
  }

  template <typename T, typename... Ts>
  void SetKernelArguments(size_t index, T &&arg, Ts &&... args) {
    SetKernelArguments(index, std::forward<T>(arg));
    SetKernelArguments(index + 1, std::forward<Ts>(args)...);
  }

  template <typename T, Access access>
  static auto UnpackPointers(Buffer<T, access> &buffer) {
#ifndef HLSLIB_SIMULATE_OPENCL
    // If we are not running simulation mode, we just need to verify that the
    // type is correct, so we pass a dummy pointer of the correct type.
    T *ptr = nullptr;
    return ptr;
#else
    // In simulation mode, unpack the buffer to the raw pointer
    return buffer.devicePtr();
#endif
  }

  template <typename T>
  static auto UnpackPointers(T &&arg) {
    return std::forward<T>(arg); 
  }

  template <class F, typename... Ts>
  static std::function<void(void)> Bind(F &&f, Ts &&... args) {
    return std::bind(f, UnpackPointers(std::forward<Ts>(args))...); 
  }

 public:

  /// Also pass the kernel function as a host function signature. This helps to
  /// verify that the arguments are correct, and allows calling the host
  /// function in simulation mode.
  template <typename F, typename... Ts>
  Kernel(Program &program, F &&hostFunction, std::string const &kernelName,
         Ts &&... kernelArgs)
      : Kernel(program, kernelName, std::forward<Ts>(kernelArgs)...) {
    hostFunction_ =
        Bind(std::forward<F>(hostFunction), std::forward<Ts>(kernelArgs)...);
  }

  /// Load kernel from binary file
  template <typename... Ts>
  Kernel(Program &program, std::string const &kernelName,
         Ts &&... kernelArgs)
      : program_(program) {
#ifndef HLSLIB_SIMULATE_OPENCL
    // Create executable compute kernel
    cl_int errorCode;
    kernel_ = cl::Kernel(program.program(), kernelName.c_str(), &errorCode);
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to create kernel with name \"" << kernelName
         << "\" from program \"" << program_.path() << "\".";
      ThrowConfigurationError(ss.str());
      return;
    }

    // Pass kernel arguments
    SetKernelArguments(0, std::forward<Ts>(kernelArgs)...);
#endif
  }

  inline ~Kernel() = default;

  inline Program const &program() const { return program_; }

  inline cl::Kernel const &kernel() const { return kernel_; }

  /// Execute the kernel as an OpenCL task and returns the time elapsed as
  /// reported by SDAccel (first) and as measured manually with chrono (second).
  inline std::pair<double, double> ExecuteTask() {
    cl::Event event;
    const auto start = std::chrono::high_resolution_clock::now();
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(program_.context().enqueueMutex());
      errorCode = program_.context().commandQueue().enqueueTask(
          kernel_, nullptr, &event);
    }
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to execute kernel.");
      return {};
    }
    event.wait();
#else
    hostFunction_(); // Simulate by calling host function
#endif
    const auto end = std::chrono::high_resolution_clock::now();
    const double elapsedChrono =
        1e-9 *
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_ulong timeStart, timeEnd;
    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
    const double elapsedSDAccel = 1e-9 * (timeEnd - timeStart);
    return {elapsedSDAccel, elapsedChrono};
#else
    return {elapsedChrono, elapsedChrono};
#endif
  }

 private:
  Program &program_;
  cl::Kernel kernel_;
  /// Host version of the kernel function. Can be used to check that the
  /// passed signature is correct, and for simulation purposes.
  std::function<void(void)> hostFunction_{}; 

};  // End class Kernel

//#############################################################################
// Implementations
//#############################################################################

template <typename T, Access access, typename... Ts>
Buffer<T, access> Context::MakeBuffer(Ts &&... args) {
  return Buffer<T, access>(*this, std::forward<Ts>(args)...);
}

Program Context::MakeProgram(std::string const &path) {

#ifndef HLSLIB_SIMULATE_OPENCL
    std::ifstream input(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
      std::stringstream ss;
      ss << "Failed to open program file \"" << path << "\".";
      ThrowConfigurationError(ss.str());
    }

    // Determine size of file in bytes
    input.seekg(0, input.end);
    const auto fileSize = input.tellg();
    input.seekg(0, input.beg);

    // Load content of binary file into a character vector (required by the
    // OpenCL C++ bindigs).
    // The constructor of cl::Program accepts a vector of binaries, so stick the
    // binary into a single-element outer vector.
    std::vector<std::vector<unsigned char>> binary;
    binary.emplace_back(fileSize);
    // Since this is just binary data the reinterpret_cast *should* be safe
    input.read(reinterpret_cast<char *>(&binary[0][0]), fileSize);

    cl_int errorCode;

    // Create OpenCL program
    std::vector<int> binaryStatus(1);
    std::vector<cl::Device> devices;
    devices.emplace_back(device_);
    auto program = std::make_shared<std::pair<std::string, cl::Program>>(
        path,
        cl::Program(context_, devices, binary, &binaryStatus, &errorCode));
    if (binaryStatus[0] != CL_SUCCESS || errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to create OpenCL program from binary file \"" << path
         << "\":";
      if (binaryStatus[0] != CL_SUCCESS) {
        ss << " binary status: " << binaryStatus[0] << ".";
      }
      if (errorCode != CL_SUCCESS) {
        ss << " error code: " << errorCode << ".";
      }
      ThrowConfigurationError(ss.str());
    }

    // Build OpenCL program
    {
      std::lock_guard<std::mutex> lock(reprogramMutex_);
      errorCode = program->second.build(devices, nullptr, nullptr, nullptr);
    }
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to build OpenCL program from binary file \"" << path
         << "\".";
      ThrowConfigurationError(ss.str());
    }
#else
  decltype(loadedProgram_) program = nullptr;
#endif

  loadedProgram_ = program;
  return Program(*this, loadedProgram_);
}

Program Context::CurrentlyLoadedProgram() const {
#ifndef HLSLIB_SIMULATE_OPENCL
  if (loadedProgram_ == nullptr) {
    ThrowRuntimeError("No program is currently loaded.");
  }
#endif
  return Program(*const_cast<Context *>(this), loadedProgram_);
}

template <typename... Ts>
Kernel Program::MakeKernel(std::string const &kernelName, Ts &&... args) {
  return Kernel(*this, kernelName, std::forward<Ts>(args)...);
}

template <typename F, typename... Ts>
Kernel Program::MakeKernel(F &&hostFunction, std::string const &kernelName,
                           Ts &&... args) {
  return Kernel(*this, std::forward<F>(hostFunction), kernelName,
                std::forward<Ts>(args)...);
}

}  // End namespace ocl

}  // End namespace hlslib
