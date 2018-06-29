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
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/opencl.h>

namespace hlslib {

namespace ocl {

//#############################################################################
// Enumerations
//#############################################################################

/// Enum for type of memory access to device buffers. Will cause the
/// the appropriate binary flags to be set when allocating OpenCL buffers.
enum class Access { read, write, readWrite };

/// Mapping to specific memory banks on the FPGA. Xilinx-specific.
enum class MemoryBank { bank0, bank1, bank2, bank3 };

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

constexpr bool verbose = true;

constexpr size_t kMaxCount = 16;
constexpr size_t kMaxString = 128;

constexpr cl_mem_flags kXilinxMemPointer = 1 << 31;
constexpr unsigned kXilinxBuffer = 1 << 0;
constexpr unsigned kXilinxPipe = 1 << 1;
constexpr unsigned kXilinxBank0 = 1 << 8;
constexpr unsigned kXilinxBank1 = 1 << 9;
constexpr unsigned kXilinxBank2 = 1 << 10;
constexpr unsigned kXilinxBank3 = 1 << 11;

struct ExtendedMemoryPointer {
  unsigned flags;
  void *obj;
  void *param;
};

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

std::vector<cl_platform_id> GetAvailablePlatforms() {
  std::vector<cl_platform_id> platforms(kMaxCount);
  cl_uint platformCount;
  cl_int errorCode =
      clGetPlatformIDs(kMaxCount, platforms.data(), &platformCount);
  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve OpenCL platforms.");
    return {};
  }

  platforms.resize(platformCount);
  return platforms;
}

std::string GetPlatformVendor(cl_platform_id platformId) {
  std::array<char, kMaxString> vendorName;
  auto errorCode = clGetPlatformInfo(platformId, CL_PLATFORM_VENDOR, kMaxString,
                                     vendorName.data(), nullptr);
  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve platform vendor name.");
    return {};
  }
  return std::string(vendorName.data());
}

cl_platform_id FindPlatformByVendor(std::string const &desiredVendor) {
  cl_platform_id platformId{};

  auto available = GetAvailablePlatforms();
  if (available.size() == 0) {
    ThrowConfigurationError("No available OpenCL platforms.");
    return platformId;
  }

  bool foundVendor = false;
  for (auto &p : available) {
    auto vendorName = GetPlatformVendor(p);
    if (std::string(vendorName.data()).find(desiredVendor) !=
        std::string::npos) {
      foundVendor = true;
      platformId = p;
      break;
    }
  }
  if (!foundVendor) {
    std::stringstream ss;
    ss << "Platform \"" << desiredVendor << "\" not found.";
    ThrowConfigurationError(ss.str());
    return platformId;
  }

  return platformId;
}

std::vector<cl_device_id> GetAvailableDevices(
    cl_platform_id const &platformId) {
  std::vector<cl_device_id> devices(kMaxCount);
  cl_uint deviceCount;
  auto errorCode = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_ALL, kMaxCount,
                                  devices.data(), &deviceCount);

  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve device IDs.");
    return {};
  }

  devices.resize(deviceCount);
  return devices;
}

std::string GetDeviceName(cl_device_id const &deviceId) {
  std::array<char, kMaxString> deviceName;
  auto errorCode = clGetDeviceInfo(deviceId, CL_DEVICE_NAME, kMaxString,
                                   deviceName.data(), nullptr);
  if (errorCode != CL_SUCCESS) {
    ThrowConfigurationError("Failed to retrieve device info.");
    return {};
  }
  return std::string(deviceName.data());
}

cl_device_id FindDeviceByName(cl_platform_id const &platformId,
                              std::string const &desiredDevice) {
  cl_device_id device;
  auto available = GetAvailableDevices(platformId);
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

cl_context CreateComputeContext(std::vector<cl_device_id> const &deviceIds) {
  // TODO: support properties and callback functions?
  cl_int errorCode;
  auto context = clCreateContext(nullptr, deviceIds.size(), deviceIds.data(),
                                 nullptr, nullptr, &errorCode);
  if (errorCode != CL_SUCCESS) {
    ThrowRuntimeError("Failed to create compute context.");
    return {};
  }
  return context;
}

cl_context CreateComputeContext(cl_device_id const &deviceId) {
  std::vector<cl_device_id> deviceIdVec = {deviceId};
  return CreateComputeContext(deviceIdVec);
}

cl_command_queue CreateCommandQueue(cl_context const &context,
                                    cl_device_id const &deviceId) {
  cl_int errorCode;
  auto commandQueue = clCreateCommandQueue(
      context, deviceId,
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE,
      &errorCode);
  if (errorCode != CL_SUCCESS) {
    ThrowRuntimeError("Failed to create command queue.");
    return {};
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
    deviceId_ = FindDeviceByName(platformId_, deviceName);

    context_ = CreateComputeContext(deviceId_);

    commandQueue_ = CreateCommandQueue(context_, deviceId_);
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
    deviceId_ = devices[0];
    if (verbose) {
      std::cout << "Using device \"" << GetDeviceName(deviceId_) << "\".\n";
    }

    context_ = CreateComputeContext(deviceId_);

    commandQueue_ = CreateCommandQueue(context_, deviceId_);
#endif
  }

  inline Context(Context const &) = delete;
  inline Context(Context &&) = default;
  inline Context &operator=(Context const &) = delete;
  inline Context &operator=(Context &&) = default;

  inline ~Context() {
#ifndef HLSLIB_SIMULATE_OPENCL
    clReleaseContext(context_);
    clReleaseCommandQueue(commandQueue_);
#endif
  }

  /// Create an OpenCL program from the given binary, from which kernels can be
  /// instantiated and executed.
  inline Program MakeProgram(std::string const &path);

  /// Returns the internal OpenCL device id.
  inline cl_device_id const &deviceId() const { return deviceId_; }

  inline std::string DeviceName() const {
#ifndef HLSLIB_SIMULATE_OPENCL
    return GetDeviceName(deviceId_);
#else
    return "Simulation";  
#endif
  }

  /// Returns the internal OpenCL execution context.
  inline cl_context const &context() const { return context_; }

  /// Returns the internal OpenCL command queue.
  inline cl_command_queue const &commandQueue() const { return commandQueue_; }

  template <typename T, Access access>
  Buffer<T, access> MakeBuffer();

  template <typename T, Access access, typename... Ts>
  Buffer<T, access> MakeBuffer(MemoryBank memoryBank, Ts &&... args);

 private:
  cl_platform_id platformId_{};
  cl_device_id deviceId_{};
  cl_context context_{};
  cl_command_queue commandQueue_{};

};  // End class Context

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
  Buffer(Context const &context, MemoryBank memoryBank, IteratorType begin,
         IteratorType end)
      : context_(&context), nElements_(std::distance(begin, end)) {

#ifndef HLSLIB_SIMULATE_OPENCL
    auto extendedPointer = CreateExtendedPointer(begin, memoryBank);

    cl_mem_flags flags = CL_MEM_COPY_HOST_PTR | kXilinxMemPointer;
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

    cl_int errorCode;
    devicePtr_ =
        clCreateBuffer(context_->context(), flags, sizeof(T) * nElements_,
                       &extendedPointer, &errorCode);

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize and copy to device memory.");
      return;
    }
#else
    devicePtr = std::vector<T>(nElements_);
    std::copy(begin, end, devicePtr_.begin());
#endif
  }

  /// Allocate device memory but don't perform any transfers.
  Buffer(Context const &context, MemoryBank memoryBank, size_t nElements)
      : context_(&context), nElements_(nElements) {
#ifndef HLSLIB_SIMULATE_OPENCL
    T *dummy = nullptr;
    auto extendedPointer = CreateExtendedPointer(dummy, memoryBank);

    cl_mem_flags flags = kXilinxMemPointer;
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

    cl_int errorCode;
    devicePtr_ =
        clCreateBuffer(context_->context(), flags, sizeof(T) * nElements_,
                       &extendedPointer, &errorCode);

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize device memory.");
      return;
    }
#else
    devicePtr_ = std::vector<T>(nElements_);
#endif
  }

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

  ~Buffer() {
#ifndef HLSLIB_SIMULATE_OPENCL
    if (nElements_ > 0) {
      clReleaseMemObject(devicePtr_);
    }
#endif
  }

  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  void CopyFromHost(int deviceOffset, int numElements, IteratorType source) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_event event;
    auto errorCode =
        clEnqueueWriteBuffer(context_->commandQueue(), devicePtr_, CL_TRUE,
                             deviceOffset, sizeof(T) * numElements,
                             const_cast<T *>(&(*source)), 0, nullptr, &event);
    if (errorCode != CL_SUCCESS) {
      throw std::runtime_error("Failed to copy data to device.");
    }
    clWaitForEvents(1, &event);
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
  void CopyToHost(int deviceOffset, int numElements, IteratorType target) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_event event;
    auto errorCode = clEnqueueReadBuffer(
        context_->commandQueue(), devicePtr_, CL_TRUE, deviceOffset,
        sizeof(T) * numElements, &(*target), 0, nullptr, &event);
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy back memory from device.");
      return;
    }
    clWaitForEvents(1, &event);
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
    cl_event event;
    auto errorCode = clEnqueueCopyBuffer(
        context_->commandQueue(), devicePtr_, other.devicePtr(), offsetSource,
        offsetDestination, numElements * sizeof(T), 0, nullptr, &event);
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy from device to device.");
      return;
    }
    clWaitForEvents(1, &event);
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
  cl_mem const &devicePtr() const { return devicePtr_; }

  cl_mem &devicePtr() { return devicePtr_; }
#else
  T const *devicePtr() const { return devicePtr_.data(); }

  T *devicePtr() { return devicePtr_.data(); }
#endif

  size_t nElements() const { return nElements_; }

 private:
  template <typename IteratorType>
  ExtendedMemoryPointer CreateExtendedPointer(IteratorType begin,
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
    }
    // The target address will not be changed, but OpenCL only accepts
    // non-const, so do a const_cast here
    extendedPointer.obj =
        const_cast<typename std::iterator_traits<IteratorType>::value_type *>(
            &(*begin));
    extendedPointer.param = nullptr;
    return extendedPointer;
  }

  Context const *context_;
#ifndef HLSLIB_SIMULATE_OPENCL
  cl_mem devicePtr_{};
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
  /// Load program from binary file
  inline Program(Context const &context, std::string const &path)
      : context_(context), path_(path) {
#ifndef HLSLIB_SIMULATE_OPENCL
    std::ifstream input(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
      std::stringstream ss;
      ss << "Failed to open program file \"" << path << "\".";
      ThrowConfigurationError(ss.str());
      return;
    }
    // Determine size of file in bytes
    size_t fileSize = input.tellg();
    input.seekg(0);

    // Load content of binary file to memory
    std::string fileContent;
    {
      std::ostringstream fileStream;
      fileStream << input.rdbuf();
      fileContent = fileStream.str();
    }

    cl_int errorCode;

    // Create OpenCL program
    cl_int binaryStatus;
    // Since this is just binary data the reinterpret_cast *should* be safe
    const unsigned char *binaryData =
        reinterpret_cast<const unsigned char *>(fileContent.data());
    program_ = clCreateProgramWithBinary(
        context.context(), 1, &context.deviceId(), &fileSize, &binaryData,
        &binaryStatus, &errorCode);
    if (binaryStatus != CL_SUCCESS || errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to create OpenCL program from binary file \"" << path
         << "\":";
      if (binaryStatus != CL_SUCCESS) {
        ss << " binary status: " << binaryStatus << ".";
      }
      if (errorCode != CL_SUCCESS) {
        ss << " error code: " << errorCode << ".";
      }
      ThrowConfigurationError(ss.str());
      return;
    }

    // Build OpenCL program
    errorCode = clBuildProgram(program_, 1, &context.deviceId(), nullptr,
                               nullptr, nullptr);
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to build OpenCL program from binary file \"" << path
         << "\".";
      ThrowConfigurationError(ss.str());
      return;
    }
#endif
  }

  inline ~Program() {
#ifndef HLSLIB_SIMULATE_OPENCL
    clReleaseProgram(program_);
#endif    
  }

  // Returns the reference Context object.
  inline Context const &context() const { return context_; }

  // Returns the internal OpenCL program object.
  inline cl_program const &program() const { return program_; }

  // Returns the path to the loaded program
  inline std::string const &path() const { return path_; }

  /// Create a kernel with the specified name contained in this loaded OpenCL
  /// program, binding the argument to the passed ones.
  template <typename... Ts>
  Kernel MakeKernel(std::string const &kernelName, Ts &&... args);

  /// Additionally allows passing a function pointer to a host implementation of
  /// the kernel function for simulation and verification purposes.
  template <class F, typename... Ts>
  Kernel MakeKernel(F &&hostFunction, std::string const &kernelName,
                    Ts &&... args);

 private:
  Context const &context_;
  cl_program program_{};
  std::string path_;
};

//#############################################################################
// Kernel
//#############################################################################

class Kernel {
 private:
  template <typename T, Access access>
  void SetKernelArguments(size_t index, Buffer<T, access> &arg) {
    auto devicePtr = arg.devicePtr();
    auto errorCode =
        clSetKernelArg(kernel_, index, sizeof(cl_mem), devicePtr);
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to set kernel argument " << index << ".";
      ThrowConfigurationError(ss.str());
      return;
    }
  }

  template <typename T>
  void SetKernelArguments(size_t index, T arg) {
    auto errorCode = clSetKernelArg(kernel_, index, sizeof(T), &arg);
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to set kernel argument " << index << ".";
      ThrowConfigurationError(ss.str());
      return;
    }
  }

  template <typename T, typename... Ts>
  void SetKernelArguments(size_t index, T &arg, Ts &... args) {
    SetKernelArguments(index, arg);
    SetKernelArguments(index + 1, args...);
  }

  template <typename T>
  T&& passed_by(T&& t, std::false_type) {
    return std::forward<T>(t);
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
    kernel_ = clCreateKernel(program_.program(), &kernelName[0], &errorCode);
    if (errorCode != CL_SUCCESS) {
      std::stringstream ss;
      ss << "Failed to create kernel with name \"" << kernelName
         << "\" from program \"" << program_.path() << "\".";
      ThrowConfigurationError(ss.str());
      return;
    }

    // Pass kernel arguments
    SetKernelArguments(0, kernelArgs...);
#endif
  }

  inline ~Kernel() {
#ifndef HLSLIB_SIMULATE_OPENCL
    clReleaseKernel(kernel_);
#endif   
  }

  inline Program const &program() const { return program_; }

  inline cl_kernel kernel() const { return kernel_; }

  /// Execute the kernel as an OpenCL task and returns the time elapsed as
  /// reported by SDAccel (first) and as measured manually with chrono (second).
  inline std::pair<double, double> ExecuteTask() {
    return ExecuteRange<1>({{1}}, {{1}});
  }

  /// Execute the kernel as an OpenCL NDRange and returns the time elapsed as
  /// reported by SDAccel (first) and as measured manually with chrono (second).
  template <unsigned dims>
  std::pair<double, double> ExecuteRange(
      std::array<size_t, dims> const &globalSize,
      std::array<size_t, dims> const &localSize) {
    cl_event event;
    static std::array<size_t, dims> offsets = {};
    const auto start = std::chrono::high_resolution_clock::now();
#ifndef HLSLIB_SIMULATE_OPENCL
    auto errorCode = clEnqueueNDRangeKernel(
        program_.context().commandQueue(), kernel_, dims, &offsets[0],
        &globalSize[0], &localSize[0], 0, nullptr, &event);
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to execute kernel.");
      return {};
    }
    clWaitForEvents(1, &event);
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
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                            sizeof(timeStart), &timeStart, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(timeEnd),
                            &timeEnd, nullptr);
    const double elapsedSDAccel = 1e-9 * (timeEnd - timeStart);
    return {elapsedSDAccel, elapsedChrono};
#else
    return {elapsedChrono, elapsedChrono};
#endif
  }

 private:
  Program &program_;
  cl_kernel kernel_{};
  /// Host version of the kernel function. Can be used to check that the
  /// passed signature is correct, and for simulation purposes.
  std::function<void(void)> hostFunction_{}; 

};  // End class Kernel

//#############################################################################
// Implementations
//#############################################################################

template <typename T, Access access>
Buffer<T, access> Context::MakeBuffer() {
  return Buffer<T, access>();
}

template <typename T, Access access, typename... Ts>
Buffer<T, access> Context::MakeBuffer(MemoryBank memoryBank, Ts &&... args) {
  return Buffer<T, access>(*this, memoryBank, args...);
}

Program Context::MakeProgram(std::string const &path) {
  return Program(*this, path);
}

template <typename F, typename... Ts>
Kernel Program::MakeKernel(F &&hostFunction, std::string const &kernelName,
                           Ts &&... args) {
  return Kernel(*this, std::forward<F>(hostFunction), kernelName,
                std::forward<Ts>(args)...);
}

}  // End namespace ocl

}  // End namespace hlslib
