/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

// Do not include this file directly. It should be included from either the
// Xilinx or Intel versions of hlslib OpenCL.

#pragma once

#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#if !defined(HLSLIB_XILINX_OPENCL_H) && !defined(HLSLIB_INTEL_OPENCL_H)
#error \
    "This header should not be included directly. Include either the Xilinx (hlslib/xilinx/OpenCL.h) or Intel OpenCL (hlslib/intel/OpenCL.h) header."
#endif

#if defined(HLSLIB_XILINX_OPENCL_H) && defined(HLSLIB_INTEL_OPENCL_H)
#error "Cannot target both Xilinx and Intel OpenCL simultaneously."
#endif

namespace hlslib {

namespace ocl {

//#############################################################################
// Enumerations and types
//#############################################################################

/// Enum for type of memory access to device buffers. Will cause the
/// the appropriate binary flags to be set when allocating OpenCL buffers.
enum class Access { read, write, readWrite };

/// Mapping to specific memory banks on the FPGA
enum class MemoryBank { unspecified, bank0, bank1, bank2, bank3 };

/// Enum for storage types on the FPGA
enum class StorageType { DDR, HBM };

/// Wrapper for an argument that should only be passed in simulation mode, but
/// not when setting the arguments of the OpenCL kernel. The use case in mind is
/// inter-kernel streams, where Xilinx OpenCL expects the argument to be left
/// unset, but the simulation mode requires a stream to be passed from the host
/// function.
template <typename T>
struct _SimulationOnly {
  _SimulationOnly(T _simulation) : simulation(_simulation) {
    // Intentionally left empty
  }
  /// The reference or value passed in simulation mode only.
  T simulation;
};
template <typename T>
auto SimulationOnly(T &&simulation) {
  return _SimulationOnly<typename std::conditional<
      std::is_rvalue_reference<T &&>::value,
      typename std::remove_reference<T>::type, T &&>::type>(
      std::forward<T>(simulation));
}

#ifndef HLSLIB_SIMULATE_OPENCL
using Event = cl::Event;
#else
/// Wraps cl::Event so we can also wait on them in simulation mode.
class Event : public cl::Event {
 public:
  Event(std::function<void(void)> const &f) {
    future_ = std::async(std::launch::async, f).share();
  }

  // Because we use a shared_future, we can copy this object
  Event(Event &&) = default;
  Event(Event const &) = default;

  cl_int wait() const {
    future_.wait();
    return CL_SUCCESS;
  }

 private:
  std::shared_future<void> future_;
};
#endif

//#############################################################################
// OpenCL exceptions
//#############################################################################

class ConfigurationError : public std::logic_error {
 public:
  ConfigurationError(std::string const &message) : std::logic_error(message) {
  }

  ConfigurationError(char const *const message) : std::logic_error(message) {
  }
};

class RuntimeError : public std::runtime_error {
 public:
  RuntimeError(std::string const &message) : std::runtime_error(message) {
  }

  RuntimeError(char const *const message) : std::runtime_error(message) {
  }
};

//#############################################################################
// Internal functionality
//#############################################################################

namespace {

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

template <typename IntCollection,
          typename ICIt = decltype(*begin(std::declval<IntCollection>())),
          typename ICIty = std::decay_t<ICIt>>
constexpr bool IsIntCollection() {
  return std::is_convertible<ICIty, int>();
}

void ThrowConfigurationError(std::string const &message) {
#ifndef HLSLIB_DISABLE_EXCEPTIONS
  throw ConfigurationError(message);
#else
  std::cerr << "OpenCL [Configuration Error]: " << message << std::endl;
#endif
}

void ThrowRuntimeError(std::string const &message) {
#ifndef HLSLIB_DISABLE_EXCEPTIONS
  throw RuntimeError(message);
#else
  std::cerr << "OpenCL [Runtime Error]: " << message << std::endl;
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
  cl::CommandQueue commandQueue(context, device, kCommandQueueFlags,
                                &errorCode);
  if (errorCode != CL_SUCCESS) {
    ThrowRuntimeError("Failed to create command queue.");
  }
  return commandQueue;
}

cl_mem_flags BankToFlag(MemoryBank memoryBank, bool failIfUnspecified,
                        DDRBankFlags const &refBankFlags) {
  switch (memoryBank) {
    case MemoryBank::bank0:
      return refBankFlags.memory_bank_0();
    case MemoryBank::bank1:
      return refBankFlags.memory_bank_1();
    case MemoryBank::bank2:
      return refBankFlags.memory_bank_2();
    case MemoryBank::bank3:
      return refBankFlags.memory_bank_3();
    case MemoryBank::unspecified:
      if (failIfUnspecified) {
        ThrowRuntimeError("Memory bank must be specified.");
      }
  }
  return 0;
}

MemoryBank StorageTypeToMemoryBank(StorageType storage, int bank) {
  if (storage != StorageType::DDR) {
    ThrowRuntimeError(
        "Only DDR bank identifiers can be converted to memory bank flags.");
  }
  if (bank < -1 || bank > 3) {
    ThrowRuntimeError(
        "Bank identifier is out of range (must be [0-3] or -1 for "
        "unspecified).");
  }
  switch (bank) {
    case -1:
      return MemoryBank::unspecified;
    case 0:
      return MemoryBank::bank0;
    case 1:
      return MemoryBank::bank1;
    case 2:
      return MemoryBank::bank2;
    case 3:
      return MemoryBank::bank3;
    default:
      ThrowRuntimeError("Unsupported bank identifier.");
  }
  return MemoryBank::unspecified;
}

cl_uint NumEvents(Event const *const eventsBegin,
                  Event const *const eventsEnd) {
  if (eventsBegin != nullptr) {
    if (eventsEnd != nullptr) {
      return std::distance(eventsBegin, eventsEnd);
    }
    // Assume that the address of a single element was passed
    return 1;
  }
  return 0;
}

template <typename EventIterator>
cl_uint NumEvents(EventIterator eventsBegin, EventIterator eventsEnd) {
  return std::distance(eventsBegin, eventsEnd);
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
  /// Performs initialization of the requested vendor and device name.
  inline Context(std::string const &vendorName, std::string const &deviceName) {
#ifndef HLSLIB_SIMULATE_OPENCL
    // Find requested OpenCL platform
    platformId_ = FindPlatformByVendor(vendorName);

    // Find requested compute device
    device_ = FindDeviceByName(platformId_, deviceName);

    DDRFlags_ = DDRBankFlags(deviceName);

    context_ = CreateComputeContext(device_);

    commandQueue_ = CreateCommandQueue(context_, device_);
#endif
  }

  /// Performs initialization of the requested device name.
  inline Context(std::string const &deviceName)
      : Context(HLSLIB_OPENCL_VENDOR_STRING, deviceName) {
  }

  /// Performs initialization of the specified vendor and device index.
  inline Context(std::string const &vendorName, int index) {
#ifndef HLSLIB_SIMULATE_OPENCL
    // Find requested OpenCL platform
    platformId_ = FindPlatformByVendor(vendorName);

    auto devices = GetAvailableDevices(platformId_);
    if (devices.size() == 0) {
      ThrowConfigurationError("No OpenCL devices found for platform.");
      return;
    }
    device_ = devices[index];

    std::string deviceName = DeviceName();
    DDRFlags_ = DDRBankFlags(deviceName);

    context_ = CreateComputeContext(device_);

    commandQueue_ = CreateCommandQueue(context_, device_);
#endif
  }

  /// Performs initialization of the specified device index.
  inline Context(int index) : Context(HLSLIB_OPENCL_VENDOR_STRING, index) {
  }

  /// Performs initialization of first available device.
  inline Context() : Context(0) {
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
  inline cl::Device const &device() const {
    return device_;
  }

  inline std::string DeviceName() const {
#ifndef HLSLIB_SIMULATE_OPENCL
    return GetDeviceName(device_);
#else
    return "Simulation";
#endif
  }

  /// Returns the internal OpenCL execution context.
  inline cl::Context const &context() const {
    return context_;
  }

  /// Returns the internal OpenCL command queue.
  inline cl::CommandQueue const &commandQueue() const {
    return commandQueue_;
  }

  inline cl::CommandQueue &commandQueue() {
    return commandQueue_;
  }

  inline Program CurrentlyLoadedProgram() const;

  template <typename T, Access access, typename... Ts>
  Buffer<T, access> MakeBuffer(Ts &&... args);

 protected:
  friend Program;
  friend Kernel;
  template <typename U, Access access>
  friend class Buffer;

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
  DDRBankFlags DDRFlags_;
};  // End class Context

//#############################################################################
// Buffer
//#############################################################################

template <typename T, Access access>
class Buffer {
 public:
  Buffer() : context_(nullptr), nElements_(0) {
  }

  Buffer(Buffer<T, access> const &other) = delete;

  Buffer(Buffer<T, access> &&other) : Buffer() {
    swap(*this, other);
  }

  /// Allocate and copy to device.
  // First check is used to go on if this might be a call to the HBM constructor
  template <
      typename IteratorType,
      typename = typename std::enable_if<
          !std::is_convertible<IteratorType, int>()>::type,
      typename = typename std::enable_if<IsIteratorOfType<IteratorType, T>() &&
                                         IsRandomAccess<IteratorType>()>::type>
  Buffer(Context &context, MemoryBank memoryBank, IteratorType begin,
         IteratorType end)
      : context_(&context), nElements_(std::distance(begin, end)) {
    AllocateDDR(memoryBank, begin, end);
  }

  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  Buffer(Context &context, IteratorType begin, IteratorType end)
      : Buffer(context, MemoryBank::unspecified, begin, end) {
  }

  /// Allocate but don't perform any transfers
  Buffer(Context &context, MemoryBank memoryBank, size_t nElements)
      : context_(&context), nElements_(nElements) {
    AllocateDDRNoTransfer(memoryBank);
  }

  Buffer(Context &context, size_t nElements)
      : Buffer(context, MemoryBank::unspecified, nElements) {
  }

  /// Allocate DDR or HBM but don't perform any transfers.
  Buffer(Context &context, StorageType storageType, int bankIndex,
         size_t nElements)
      : context_(&context), nElements_(nElements) {
#ifndef HLSLIB_SIMULATE_OPENCL
#ifdef HLSLIB_INTEL
    if (storageType != StorageType::DDR) {
      ThrowRuntimeError("Only DDR memory is supported for Intel FPGA.");
    }
    AllocateDDRNoTransfer(StorageTypeToMemoryBank(storageType, bankIndex));
#endif
#ifdef HLSLIB_XILINX
    void *hostPtr = nullptr;
    ExtendedMemoryPointer extendedHostPointer;
    cl_mem_flags flags = CreateAllocFlags(CL_MEM_ALLOC_HOST_PTR);
    if (storageType != StorageType::DDR || bankIndex != -1) {
      extendedHostPointer = CreateExtendedPointer(nullptr, storageType,
                                                  bankIndex, context.DDRFlags_);
      hostPtr = &extendedHostPointer;
      flags |= kXilinxMemPointer;
    }

    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      devicePtr_ = cl::Buffer(context_->context(), flags,
                              sizeof(T) * nElements_, hostPtr, &errorCode);
    }

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize device memory.");
      return;
    }
#endif
#else
    devicePtr_ = std::make_unique<T[]>(nElements_);
#endif
  }

  /// Allocate on HBM or DDR and copy to device
  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  Buffer(Context &context, StorageType storageType, int bankIndex,
         IteratorType begin, IteratorType end)
      : context_(&context), nElements_(std::distance(begin, end)) {
#ifndef HLSLIB_SIMULATE_OPENCL
#ifdef HLSLIB_INTEL
    if (storageType != StorageType::DDR) {
      ThrowRuntimeError("Only DDR memory is supported for Intel FPGA.");
    }
    AllocateDDR(StorageTypeToMemoryBank(storageType, bankIndex), begin, end);
#endif
#ifdef HLSLIB_XILINX
    void *hostPtr = const_cast<T *>(&(*begin));
    ExtendedMemoryPointer extendedHostPointer;
    cl_mem_flags flags = CreateAllocFlags(CL_MEM_USE_HOST_PTR);
    if (storageType != StorageType::DDR || bankIndex != -1) {
      extendedHostPointer = CreateExtendedPointer(hostPtr, storageType,
                                                  bankIndex, context.DDRFlags_);
      hostPtr = &extendedHostPointer;
      flags |= kXilinxMemPointer;
    }

    cl_int errorCode;
    devicePtr_ = cl::Buffer(context.context(), flags, sizeof(T) * nElements_,
                            hostPtr, &errorCode);

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize and copy to device memory.");
      return;
    }
#endif
#else
    devicePtr_ = std::make_unique<T[]>(nElements_);
    std::copy(begin, end, devicePtr_.get());
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

  ~Buffer() = default;

  template <
      typename DataIterator, typename EventIterator = Event *,
      typename = typename std::enable_if<IsIteratorOfType<DataIterator, T>() &&
                                         IsRandomAccess<DataIterator>()>::type,
      typename =
          typename std::enable_if<IsIteratorOfType<EventIterator, Event>() &&
                                  IsRandomAccess<EventIterator>()>::type>
  void CopyFromHost(int deviceOffset, int numElements, DataIterator source,
                    EventIterator eventsBegin, EventIterator eventsEnd) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_event event;
    cl_int errorCode;
    const auto numEvents = NumEvents(eventsBegin, eventsEnd);
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      static_assert(sizeof(cl_event) == sizeof(cl::Event) &&
                        sizeof(cl::Event) == sizeof(Event),
                    "Reinterpret cast is not safe.");
      errorCode = clEnqueueWriteBuffer(
          context_->commandQueue().get(), devicePtr_.get(), CL_TRUE,
          sizeof(T) * deviceOffset, sizeof(T) * numElements,
          const_cast<T *>(&(*source)), numEvents,
          reinterpret_cast<cl_event const *>(&(*eventsBegin)), &event);
    }
    // Don't need to wait for event because of blocking call (CL_TRUE)
    if (errorCode != CL_SUCCESS) {
      throw std::runtime_error("Failed to copy data to device.");
    }
#else
    for (; eventsBegin != eventsEnd; ++eventsBegin) {
      eventsBegin->wait();
    }
    std::copy(source, source + numElements, devicePtr_.get() + deviceOffset);
#endif
  }

  template <typename DataIterator, typename = typename std::enable_if<
                                       IsIteratorOfType<DataIterator, T>() &&
                                       IsRandomAccess<DataIterator>()>::type>
  void CopyFromHost(int deviceOffset, int numElements, DataIterator source) {
    return CopyFromHost<DataIterator, Event *>(deviceOffset, numElements,
                                               source, nullptr, nullptr);
  }

  template <
      typename DataIterator, typename EventIterator = Event *,
      typename = typename std::enable_if<IsIteratorOfType<DataIterator, T>() &&
                                         IsRandomAccess<DataIterator>()>::type,
      typename =
          typename std::enable_if<IsIteratorOfType<EventIterator, Event>() &&
                                  IsRandomAccess<EventIterator>()>::type>
  void CopyFromHost(DataIterator source, EventIterator eventBegin,
                    EventIterator eventEnd) {
    return CopyFromHost(0, nElements_, source, eventBegin, eventEnd);
  }

  template <typename DataIterator, typename = typename std::enable_if<
                                       IsIteratorOfType<DataIterator, T>() &&
                                       IsRandomAccess<DataIterator>()>::type>
  void CopyFromHost(DataIterator source) {
    return CopyFromHost<DataIterator, Event *>(0, nElements_, source, nullptr,
                                               nullptr);
  }

  template <
      typename DataIterator, typename EventIterator = Event *,
      typename = typename std::enable_if<IsIteratorOfType<DataIterator, T>() &&
                                         IsRandomAccess<DataIterator>()>::type,
      typename =
          typename std::enable_if<IsIteratorOfType<EventIterator, Event>() &&
                                  IsRandomAccess<EventIterator>()>::type>
  void CopyToHost(size_t deviceOffset, size_t numElements, DataIterator target,
                  EventIterator eventsBegin, EventIterator eventsEnd) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_event event;
    cl_int errorCode;
    const auto numEvents = NumEvents(eventsBegin, eventsEnd);
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      static_assert(sizeof(cl_event) == sizeof(cl::Event) &&
                        sizeof(cl::Event) == sizeof(Event),
                    "Reinterpret cast is not safe.");
      errorCode = clEnqueueReadBuffer(
          context_->commandQueue().get(), devicePtr_.get(), CL_TRUE,
          sizeof(T) * deviceOffset, sizeof(T) * numElements, &(*target),
          numEvents, reinterpret_cast<cl_event const *>(&(*eventsBegin)),
          &event);
    }
    // Don't need to wait for event because of blocking call (CL_TRUE)
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy back memory from device.");
      return;
    }
#else
    for (; eventsBegin != eventsEnd; ++eventsBegin) {
      eventsBegin->wait();
    }
    std::copy(devicePtr_.get() + deviceOffset,
              devicePtr_.get() + deviceOffset + numElements, target);
#endif
  }

  template <typename DataIterator, typename = typename std::enable_if<
                                       IsIteratorOfType<DataIterator, T>() &&
                                       IsRandomAccess<DataIterator>()>::type>
  void CopyToHost(size_t deviceOffset, size_t numElements,
                  DataIterator target) {
    return CopyToHost<DataIterator, Event *>(deviceOffset, numElements, target,
                                             nullptr, nullptr);
  }

  template <
      typename DataIterator, typename EventIterator = Event *,
      typename = typename std::enable_if<IsIteratorOfType<DataIterator, T>() &&
                                         IsRandomAccess<DataIterator>()>::type,
      typename =
          typename std::enable_if<IsIteratorOfType<EventIterator, Event>() &&
                                  IsRandomAccess<EventIterator>()>::type>
  void CopyToHost(DataIterator target, EventIterator eventsBegin,
                  EventIterator eventsEnd) {
    return CopyToHost(0, nElements_, target, eventsBegin, eventsEnd);
  }

  template <typename DataIterator, typename = typename std::enable_if<
                                       IsIteratorOfType<DataIterator, T>() &&
                                       IsRandomAccess<DataIterator>()>::type>
  void CopyToHost(DataIterator target) {
    return CopyToHost<DataIterator, Event *>(0, nElements_, target, nullptr,
                                             nullptr);
  }

  template <Access accessType, typename EventIterator = Event *,
            typename = typename std::enable_if<
                IsIteratorOfType<EventIterator, Event>() &&
                IsRandomAccess<EventIterator>()>::type>
  void CopyToDevice(size_t offsetSource, size_t numElements,
                    Buffer<T, accessType> &other, size_t offsetDestination,
                    EventIterator eventsBegin, EventIterator eventsEnd) {
#ifndef HLSLIB_SIMULATE_OPENCL
    if (offsetSource + numElements > nElements_ ||
        offsetDestination + numElements > other.nElements()) {
      ThrowRuntimeError("Device to device copy interval out of range.");
    }
    cl_event event;
    cl_int errorCode;
    static_assert(sizeof(cl_event) == sizeof(cl::Event) &&
                      sizeof(cl::Event) == sizeof(Event),
                  "Reinterpret cast is not safe.");
    const auto numEvents = NumEvents(eventsBegin, eventsEnd);
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      errorCode = clEnqueueCopyBuffer(
          context_->commandQueue().get(), devicePtr_.get(),
          other.devicePtr().get(), sizeof(T) * offsetSource,
          sizeof(T) * offsetDestination, numElements * sizeof(T), numEvents,
          reinterpret_cast<cl_event const *>(&(*eventsBegin)), &event);
    }
    clWaitForEvents(1, &event);
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy from device to device.");
      return;
    }
#else
    for (; eventsBegin != eventsEnd; ++eventsBegin) {
      eventsBegin->wait();
    }
    std::copy(devicePtr_.get() + offsetSource,
              devicePtr_.get() + offsetSource + numElements,
              other.devicePtr_.get() + offsetDestination);
#endif
  }

  template <Access accessType>
  void CopyToDevice(size_t offsetSource, size_t numElements,
                    Buffer<T, accessType> &other, size_t offsetDestination) {
    return CopyToDevice<accessType, Event *>(
        offsetSource, numElements, other, offsetDestination, nullptr, nullptr);
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

  /*
  The following functions copy 3D blocks of memory between host and device or
  on the device. xxxOffset -> the offset of destination or target in
  (elements, rows, slices). For 2d arrays xxxOffset[2] should be 0.
  copyBlockSize -> The size of the block that is actually copied in (elements,
  rows, slices). hostBlockSize/deviceBlockSize/sourceBlockSize/destBlockSize:
  The size of the whole host/device/ source/destination Array in (elements,
  rows, slices)

  Note: (elements, rows, slices) means just "default"-indices, like one would
  index a default c++ multidimensional array.

  For hostBlockSize/deviceBlockSize/sourceBlockSize/destBlockSize it should
  hold that their product is smaller or equal to the size of the array they
  reference. Not explicitely checked, because openCL will throw an error if
  violated.
  */

  template <
      typename IntCollection, typename IteratorType,
      typename = typename std::enable_if<IsIteratorOfType<IteratorType, T>() &&
                                         IsRandomAccess<IteratorType>()>::type,
      typename =
          typename std::enable_if<IsIntCollection<IntCollection>()>::type>
  void CopyBlockFromHost(const IntCollection &hostBlockOffset,
                         const IntCollection &deviceBlockOffset,
                         const IntCollection &copyBlockSize,
                         const IntCollection &hostBlockSize,
                         const IntCollection &deviceBlockSize,
                         IteratorType source) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl::Event event;
    std::array<size_t, 3> copyBlockSizePrepared, hostBlockOffsetsPrepared,
        deviceBlockOffsetsPrepared;
    std::array<size_t, 2> hostBlockSizesBytes, deviceBlockSizesBytes;
    BlockOffsetsPreprocess(copyBlockSize, hostBlockSize, deviceBlockSize,
                           hostBlockOffset, deviceBlockOffset,
                           copyBlockSizePrepared, hostBlockSizesBytes,
                           deviceBlockSizesBytes, hostBlockOffsetsPrepared,
                           deviceBlockOffsetsPrepared, sizeof(T));
    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      errorCode = context_->commandQueue().enqueueWriteBufferRect(
          devicePtr_, CL_TRUE, deviceBlockOffsetsPrepared,
          hostBlockOffsetsPrepared, copyBlockSizePrepared,
          deviceBlockSizesBytes[0], deviceBlockSizesBytes[1],
          hostBlockSizesBytes[0], hostBlockSizesBytes[1],
          const_cast<T *>(&(*source)), nullptr, &event);
    }
    if (errorCode != CL_SUCCESS) {
      throw std::runtime_error("Failed to copy data to device.");
    }
#else
    CopyMemoryBlockSimulate(hostBlockOffset, deviceBlockOffset, copyBlockSize,
                            hostBlockSize, deviceBlockSize, source,
                            devicePtr_.get());
#endif
  }

  template <
      typename IteratorType, typename IntCollection,
      typename = typename std::enable_if<IsIteratorOfType<IteratorType, T>() &&
                                         IsRandomAccess<IteratorType>()>::type,
      typename =
          typename std::enable_if<IsIntCollection<IntCollection>()>::type>
  void CopyBlockToHost(const IntCollection &hostBlockOffset,
                       const IntCollection &deviceBlockOffset,
                       const IntCollection &copyBlockSize,
                       const IntCollection &hostBlockSize,
                       const IntCollection &deviceBlockSize,
                       IteratorType target) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl::Event event;
    std::array<size_t, 3> copyBlockSizePrepared, hostBlockOffsetsPrepared,
        deviceBlockOffsetsPrepared;
    std::array<size_t, 2> hostBlockSizesBytes, deviceBlockSizesBytes;
    BlockOffsetsPreprocess(copyBlockSize, hostBlockSize, deviceBlockSize,
                           hostBlockOffset, deviceBlockOffset,
                           copyBlockSizePrepared, hostBlockSizesBytes,
                           deviceBlockSizesBytes, hostBlockOffsetsPrepared,
                           deviceBlockOffsetsPrepared, sizeof(T));
    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      errorCode = context_->commandQueue().enqueueReadBufferRect(
          devicePtr_, CL_TRUE, deviceBlockOffsetsPrepared,
          hostBlockOffsetsPrepared, copyBlockSizePrepared,
          deviceBlockSizesBytes[0], deviceBlockSizesBytes[1],
          hostBlockSizesBytes[0], hostBlockSizesBytes[1],
          const_cast<T *>(&(*target)), nullptr, &event);
    }
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy back memory from device.");
      return;
    }
#else
    CopyMemoryBlockSimulate(deviceBlockOffset, hostBlockOffset, copyBlockSize,
                            deviceBlockSize, hostBlockSize, devicePtr_.get(),
                            target);
#endif
  }

  template <Access accessType, typename IntCollection,
            typename =
                typename std::enable_if<IsIntCollection<IntCollection>()>::type>
  void CopyBlockToDevice(const IntCollection &sourceBlockOffset,
                         const IntCollection &destBlockOffset,
                         const IntCollection &copyBlockSize,
                         const IntCollection &sourceBlockSize,
                         const IntCollection &destBlockSize,
                         Buffer<T, accessType> &other) {
#ifndef HLSLIB_SIMULATE_OPENCL
    cl::Event event;
    std::array<size_t, 3> copyBlockSizePrepared, sourceBlockOffsetsPrepared,
        destBlockOffsetsPrepared;
    std::array<size_t, 2> sourceBlockSizesBytes, destBlockSizesBytes;
    BlockOffsetsPreprocess(copyBlockSize, sourceBlockSize, destBlockSize,
                           sourceBlockOffset, destBlockOffset,
                           copyBlockSizePrepared, sourceBlockSizesBytes,
                           destBlockSizesBytes, sourceBlockOffsetsPrepared,
                           destBlockOffsetsPrepared, sizeof(T));
    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      errorCode = context_->commandQueue().enqueueCopyBufferRect(
          devicePtr_, other.devicePtr(), sourceBlockOffsetsPrepared,
          destBlockOffsetsPrepared, copyBlockSizePrepared,
          sourceBlockSizesBytes[0], sourceBlockSizesBytes[1],
          destBlockSizesBytes[0], destBlockSizesBytes[1], nullptr, &event);
    }
    event.wait();
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to copy from device to device.");
      return;
    }
#else
    CopyMemoryBlockSimulate(sourceBlockOffset, destBlockOffset, copyBlockSize,
                            sourceBlockSize, destBlockSize, devicePtr_.get(),
                            other.devicePtr_.get());
#endif
  }

#ifndef HLSLIB_SIMULATE_OPENCL
  cl::Buffer const &devicePtr() const {
    return devicePtr_;
  }

  cl::Buffer &devicePtr() {
    return devicePtr_;
  }
#else
  T const *devicePtr() const {
    return devicePtr_.get();
  }

  T *devicePtr() {
    return devicePtr_.get();
  }
#endif

  size_t nElements() const {
    return nElements_;
  }

 private:
#ifdef HLSLIB_XILINX
  ExtendedMemoryPointer CreateExtendedPointer(
      void *hostPtr, MemoryBank memoryBank, DDRBankFlags const &refBankFlags) {
    ExtendedMemoryPointer extendedPointer;
    extendedPointer.flags = BankToFlag(memoryBank, true, refBankFlags);
    extendedPointer.obj = hostPtr;
    extendedPointer.param = 0;
    return extendedPointer;
  }

  ExtendedMemoryPointer CreateExtendedPointer(
      void *hostPtr, StorageType storageType, int bankIndex,
      DDRBankFlags const &refBankFlags) {
    ExtendedMemoryPointer extendedPointer;
    extendedPointer.obj = hostPtr;
    extendedPointer.param = 0;

    switch (storageType) {
      case StorageType::HBM:
        if (bankIndex >= 32 || bankIndex < 0)
          ThrowRuntimeError(
              "HBM bank index out of range. The bank index must be below "
              "32.");
        extendedPointer.flags = bankIndex | kHBMStorageMagicNumber;
        break;
      case StorageType::DDR:
        if (bankIndex >= 4 || bankIndex < 0) {
          ThrowRuntimeError(
              "DDR bank index out of range. The bank index must be "
              "in the range [0,3].");
        }
        switch (bankIndex) {
          case 0:
            extendedPointer.flags = refBankFlags.memory_bank_0();
            break;
          case 1:
            extendedPointer.flags = refBankFlags.memory_bank_1();
            break;
          case 2:
            extendedPointer.flags = refBankFlags.memory_bank_2();
            break;
          case 3:
            extendedPointer.flags = refBankFlags.memory_bank_3();
            break;
          default:
            ThrowRuntimeError(
                "DDR bank index out of range. The bank index must be below "
                "4.");
        }
    }
    return extendedPointer;
  }

  cl_mem_flags CreateAllocFlags(cl_mem_flags initialflags) {
    switch (access) {
      case Access::read:
        initialflags |= CL_MEM_READ_ONLY;
        break;
      case Access::write:
        initialflags |= CL_MEM_WRITE_ONLY;
        break;
      case Access::readWrite:
        initialflags |= CL_MEM_READ_WRITE;
        break;
    }

    return initialflags;
  }
#endif  // HLSLIB_XILINX

  /// Allocate and copy to device.
  template <typename IteratorType, typename = typename std::enable_if<
                                       IsIteratorOfType<IteratorType, T>() &&
                                       IsRandomAccess<IteratorType>()>::type>
  void AllocateDDR(MemoryBank memoryBank, IteratorType begin,
                   IteratorType end) {
#ifndef HLSLIB_SIMULATE_OPENCL

    void *hostPtr = nullptr;

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

#ifdef HLSLIB_XILINX
    hostPtr = const_cast<T *>(&(*begin));
    flags |= CL_MEM_USE_HOST_PTR;
    // Allow specifying memory bank
    ExtendedMemoryPointer extendedHostPointer;
    if (memoryBank != MemoryBank::unspecified) {
      extendedHostPointer =
          CreateExtendedPointer(hostPtr, memoryBank, context_->DDRFlags_);
      // Replace hostPtr with Xilinx extended pointer
      hostPtr = &extendedHostPointer;
      flags |= kXilinxMemPointer;
    }
#endif

#ifdef HLSLIB_INTEL
    flags |= BankToFlag(memoryBank, false, context_->DDRFlags_);
#endif

    cl_int errorCode;
    devicePtr_ = cl::Buffer(context_->context(), flags, sizeof(T) * nElements_,
                            hostPtr, &errorCode);
#ifdef HLSLIB_INTEL
    CopyFromHost(begin);
#endif

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize and copy to device memory.");
      return;
    }
#else
    devicePtr_ = std::make_unique<T[]>(nElements_);
    std::copy(begin, end, devicePtr_.get());
#endif
  }

  /// Allocate device memory but don't perform any transfers.
  void AllocateDDRNoTransfer(MemoryBank memoryBank) {
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

    void *hostPtr = nullptr;
#ifdef HLSLIB_XILINX
    ExtendedMemoryPointer extendedHostPointer;
    if (memoryBank != MemoryBank::unspecified) {
      extendedHostPointer =
          CreateExtendedPointer(nullptr, memoryBank, context_->DDRFlags_);
      // Becomes a pointer to the Xilinx extended memory pointer if a memory
      // bank is specified
      hostPtr = &extendedHostPointer;
      flags |= kXilinxMemPointer;
    }
#endif
#ifdef HLSLIB_INTEL
    flags |= BankToFlag(memoryBank, false, context_->DDRFlags_);
#endif

    cl_int errorCode;
    {
      std::lock_guard<std::mutex> lock(context_->memcopyMutex());
      devicePtr_ = cl::Buffer(context_->context(), flags,
                              sizeof(T) * nElements_, hostPtr, &errorCode);
    }

    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to initialize device memory.");
      return;
    }
#else
    devicePtr_ = std::make_unique<T[]>(nElements_);
#endif
  }

#ifndef HLSLIB_SIMULATE_OPENCL
  /*
  Transform the inputs of the CopyBlockXXX functions to arguments for the
  clXXXRect functions. The conversion is done because the clXXXRect functions
  have some somewhat unintuitive interface (it does make sense if one thinks
  about how they are probably implemented - still very inconvienient to use)
  */
  inline void BlockOffsetsPreprocess(
      const std::array<size_t, 3> &blockSize,
      const std::array<size_t, 3> &hostBlockSizes,
      const std::array<size_t, 3> &deviceBlockSizes,
      const std::array<size_t, 3> &hostBlockOffsets,
      const std::array<size_t, 3> &deviceBlockOffsets,
      std::array<size_t, 3> &copyBlockSizePrepared,
      std::array<size_t, 2> &hostBlockSizesBytes,
      std::array<size_t, 2> &deviceBlockSizesBytes,
      std::array<size_t, 3> &hostBlockOffsetsPrepared,
      std::array<size_t, 3> &deviceBlockOffsetsPrepared,
      const size_t multiplyBy) {
    copyBlockSizePrepared = {blockSize[0] * multiplyBy, blockSize[1],
                             blockSize[2]};
    hostBlockSizesBytes = {hostBlockSizes[0] * multiplyBy,
                           hostBlockSizes[0] * hostBlockSizes[1] * multiplyBy};
    deviceBlockSizesBytes = {
        deviceBlockSizes[0] * multiplyBy,
        deviceBlockSizes[0] * deviceBlockSizes[1] * multiplyBy};
    hostBlockOffsetsPrepared = {hostBlockOffsets[0] * multiplyBy,
                                hostBlockOffsets[1], hostBlockOffsets[2]};
    deviceBlockOffsetsPrepared = {deviceBlockOffsets[0] * multiplyBy,
                                  deviceBlockOffsets[1], deviceBlockOffsets[2]};
  }

#else
  template <
      typename IteratorType1, typename IteratorType2,
      typename = typename std::enable_if<IsIteratorOfType<IteratorType1, T>() &&
                                         IsRandomAccess<IteratorType1>()>::type,
      typename = typename std::enable_if<IsIteratorOfType<IteratorType2, T>() &&
                                         IsRandomAccess<IteratorType2>()>::type>
  void CopyMemoryBlockSimulate(const std::array<size_t, 3> blockOffsetSource,
                               const std::array<size_t, 3> blockOffsetDest,
                               const std::array<size_t, 3> copyBlockSize,
                               const std::array<size_t, 3> blockSizeSource,
                               const std::array<size_t, 3> blockSizeDest,
                               const IteratorType1 source,
                               const IteratorType2 dst) {
    size_t sourceSliceJmp =
        blockSizeSource[0] - copyBlockSize[0] +
        (blockSizeSource[1] - copyBlockSize[1]) * blockSizeSource[0];
    size_t destSliceJmp =
        blockSizeDest[0] - copyBlockSize[0] +
        (blockSizeDest[1] - copyBlockSize[1]) * blockSizeDest[0];
    size_t srcindex =
        blockOffsetSource[0] + blockOffsetSource[1] * blockSizeSource[0] +
        blockOffsetSource[2] * blockSizeSource[1] * blockSizeSource[0];
    size_t dstindex = blockOffsetDest[0] +
                      blockOffsetDest[1] * blockSizeDest[0] +
                      blockOffsetDest[2] * blockSizeDest[1] * blockSizeDest[0];

    for (size_t sliceCounter = 0; sliceCounter < copyBlockSize[2];
         sliceCounter++) {
      size_t nextaddsource = 0;
      size_t nextadddest = 0;
      for (size_t rowCounter = 0; rowCounter < copyBlockSize[1]; rowCounter++) {
        srcindex += nextaddsource;
        dstindex += nextadddest;
        std::copy(source + srcindex, source + srcindex + copyBlockSize[0],
                  dst + dstindex);
        nextaddsource = blockSizeSource[0];
        nextadddest = blockSizeDest[0];
      }
      srcindex += sourceSliceJmp + copyBlockSize[0];
      dstindex += destSliceJmp + copyBlockSize[0];
    }
  }
#endif

  Context *context_;
#ifndef HLSLIB_SIMULATE_OPENCL
  cl::Buffer devicePtr_{};
#else
  std::unique_ptr<T[]> devicePtr_{};
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
  Program(Program const &) = default;
  Program(Program &&) = default;
  ~Program() = default;

  // Returns the reference Context object.
  inline Context &context() {
    return context_;
  }

  // Returns the reference Context object.
  inline Context const &context() const {
    return context_;
  }

  // Returns the internal OpenCL program object.
  inline cl::Program &program() {
    return program_->second;
  }

  // Returns the internal OpenCL program object.
  inline cl::Program const &program() const {
    return program_->second;
  }

  // Returns the path to the loaded program
  inline std::string const &path() const {
    return program_->first;
  }

  /// Create a kernel with the specified name contained in this loaded OpenCL
  /// program, binding the argument to the passed ones.
  template <typename... Ts>
  Kernel MakeKernel(std::string const &kernelName, Ts &&... args);

  /// Additionally allows passing a function pointer to a host implementation
  /// of the kernel function for simulation and verification purposes.
  template <class F, typename... Ts>
  Kernel MakeKernel(F &&hostFunction, std::string const &kernelName,
                    Ts &&... args);

 protected:
  friend Context;

  inline Program(Context &context,
                 std::shared_ptr<std::pair<std::string, cl::Program>> program)
      : context_(context), program_(program) {
  }

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
  void SetKernelArguments(size_t index, _SimulationOnly<T> const &) {
    // Ignore argument, as this is set internally during linking
  }

  template <typename T>
  void SetKernelArguments(size_t index, _SimulationOnly<T> &&) {
    // Ignore argument, as this is set internally during linking
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

  void SetKernelArguments(size_t) {
    // Bottom out
  }

  void SetKernelArguments() {
    // Bottom out
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
  static typename std::conditional<
      std::is_reference<T>::value,
      std::reference_wrapper<typename std::decay<T>::type>, T>::type
  UnpackPointers(_SimulationOnly<T> const &arg) {
    return arg.simulation;
  }

  template <typename T>
  static typename std::conditional<
      std::is_reference<T>::value,
      std::reference_wrapper<typename std::decay<T>::type>, T>::type
  UnpackPointers(_SimulationOnly<T> &&arg) {
    return arg.simulation;
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
  /// Also pass the kernel function as a host function signature. This helps
  /// to verify that the arguments are correct, and allows calling the host
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
  Kernel(Program &program, std::string const &kernelName, Ts &&... kernelArgs)
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

#ifdef HLSLIB_INTEL  // Every kernel has it's own command queue on Intel
    commandQueue_ = CreateCommandQueue(program_.context().context(),
                                       program_.context().device());
#endif
#endif
  }

  inline ~Kernel() = default;

  inline Program const &program() const {
    return program_;
  }

  inline cl::Kernel const &kernel() const {
    return kernel_;
  }

#ifdef HLSLIB_INTEL
  /// Returns the internal OpenCL command queue (Intel FPGA only).
  inline cl::CommandQueue const &commandQueue() const {
    return commandQueue_;
  }
  /// Returns the internal OpenCL command queue (Intel FPGA only).
  inline cl::CommandQueue &commandQueue() {
    return commandQueue_;
  }
#endif

  /// Execute the kernel as an OpenCL task, wait for it to finish, then return
  /// the time elapsed as reported by OpenCL (first) and as measured with
  /// chrono (second).
  template <typename EventIterator = Event *,
            typename = typename std::enable_if<
                IsIteratorOfType<EventIterator, Event>() &&
                IsRandomAccess<EventIterator>()>::type>
  std::pair<double, double> ExecuteTask(EventIterator eventsBegin,
                                        EventIterator eventsEnd) {
    const auto start = std::chrono::high_resolution_clock::now();
    auto event = ExecuteTaskAsync(eventsBegin, eventsEnd);
    event.wait();
    const auto end = std::chrono::high_resolution_clock::now();
    const double elapsedChrono =
        1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                   .count();
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_ulong timeStart, timeEnd;
    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
    const double elapsedOpenCL = 1e-9 * (timeEnd - timeStart);
    return {elapsedOpenCL, elapsedChrono};
#else
    return {elapsedChrono, elapsedChrono};
#endif
  }

  std::pair<double, double> ExecuteTask() {
    return ExecuteTask<Event *>(nullptr, nullptr);
  }

  /// Returns a future  to the result of a kernel launch, returning immediately
  /// and allows the caller to wait on execution to finish as needed. Useful
  /// for executing multiple concurrent kernels.
  template <typename EventIterator = Event *,
            typename = typename std::enable_if<
                IsIteratorOfType<EventIterator, Event>() &&
                IsRandomAccess<EventIterator>()>::type>
  Event ExecuteTaskAsync(EventIterator eventsBegin, EventIterator eventsEnd) {
    cl_event event;
#ifndef HLSLIB_SIMULATE_OPENCL
    cl_int errorCode;
    const auto numEvents = NumEvents(eventsBegin, eventsEnd);
#ifndef HLSLIB_INTEL
    {
      std::lock_guard<std::mutex> lock(program_.context().enqueueMutex());
      static_assert(sizeof(cl_event) == sizeof(cl::Event) &&
                        sizeof(cl::Event) == sizeof(Event),
                    "Reinterpret cast is not safe.");
      errorCode = clEnqueueTask(
          program_.context().commandQueue().get(), kernel_.get(), numEvents,
          reinterpret_cast<cl_event const *>(&(*eventsBegin)), &event);
    }
#else
    errorCode = clEnqueueTask(
        commandQueue_.get(), kernel_.get(), numEvents,
        reinterpret_cast<cl_event const *>(&(*eventsBegin)), &event);
#endif
    if (errorCode != CL_SUCCESS) {
      ThrowRuntimeError("Failed to execute kernel.");
      return Event(cl::Event());
    }
    return Event(event);
#else

    return Event([this, eventsBegin, eventsEnd]() {
      for (auto i = eventsBegin; i != eventsEnd; ++i) {
        i->wait();
      }
      hostFunction_();
    });  // Simulate by calling host function
#endif
  }

  Event ExecuteTaskAsync() {
    return ExecuteTaskAsync<Event *>(nullptr, nullptr);
  }

 private:
  Program &program_;
  cl::Kernel kernel_;
  /// Host version of the kernel function. Can be used to check that the
  /// passed signature is correct, and for simulation purposes.
  std::function<void(void)> hostFunction_{};
#ifdef HLSLIB_INTEL
  cl::CommandQueue commandQueue_;
#endif

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
  // OpenCL C++ bindings).
  // The constructor of cl::Program accepts a vector of binaries, so stick the
  // binary into a single-element outer vector.
  std::vector<std::vector<unsigned char>> binaryContent;
  binaryContent.emplace_back(fileSize);
  // Since this is just binary data the reinterpret_cast *should* be safe
  input.read(reinterpret_cast<char *>(&binaryContent[0][0]), fileSize);
  const auto binary = binaryContent;

  if (fileSize == 0 || binaryContent[0].size() != fileSize) {
    std::stringstream ss;
    ss << "Failed to read binary file \"" << path << "\".";
    ThrowConfigurationError(ss.str());
  }

  cl_int errorCode;

  // Create OpenCL program
  std::vector<int> binaryStatus(1);
  std::vector<cl::Device> devices;
  devices.emplace_back(device_);
  auto program = std::make_shared<std::pair<std::string, cl::Program>>(
      path, cl::Program(context_, devices, binary, &binaryStatus, &errorCode));
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
    ss << "Failed to build OpenCL program from binary file \"" << path << "\".";
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

/// Analogous to cl::waitForEvents, but compatible with the hlslib wrapper so it
/// works across simulation and hardware environments.
inline cl_int WaitForEvents(std::vector<Event> const &events) {
#ifdef HLSLIB_SIMULATE_OPENCL
  for (auto &e : events) {
    e.wait();
  }
  return 0;
#else
  return cl::WaitForEvents(events);
#endif
}

//#############################################################################
// Aligned allocator, for creating page-aligned vectors to stop OpenCL from
// complaining.
//#############################################################################

// Adapted from https://stackoverflow.com/a/12942652/2949968
// All credit to the original author.
namespace detail {
template <size_t alignment>
void *allocate_aligned_memory(size_t size);
void deallocate_aligned_memory(void *ptr) noexcept;
}  // namespace detail

template <typename T, size_t alignment>
class AlignedAllocator;

template <size_t alignment>
class AlignedAllocator<void, alignment> {
 public:
  typedef void *pointer;
  typedef const void *const_pointer;
  typedef void value_type;

  template <class U>
  struct rebind {
    typedef AlignedAllocator<U, alignment> other;
  };
};

template <typename T, size_t alignment>
class AlignedAllocator {
 public:
  typedef T value_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U>
  struct rebind {
    typedef AlignedAllocator<U, alignment> other;
  };

 public:
  AlignedAllocator() noexcept {
  }

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, alignment> &) noexcept {
  }

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(alignment)) / sizeof(T);
  }

  pointer address(reference x) const noexcept {
    return std::addressof(x);
  }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(
      size_type n,
      typename AlignedAllocator<void, alignment>::const_pointer = 0) {
    void *ptr = detail::allocate_aligned_memory<alignment>(n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args>
  void construct(U *p, Args &&... args) {
    ::new (reinterpret_cast<void *>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) {
    p->~T();
  }
};

template <typename T, size_t alignment>
class AlignedAllocator<const T, alignment> {
 public:
  typedef T value_type;
  typedef const T *pointer;
  typedef const T *const_pointer;
  typedef const T &reference;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U>
  struct rebind {
    typedef AlignedAllocator<U, alignment> other;
  };

 public:
  AlignedAllocator() noexcept {
  }

  template <class U>
  AlignedAllocator(const AlignedAllocator<U, alignment> &) noexcept {
  }

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(alignment)) / sizeof(T);
  }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(
      size_type n,
      typename AlignedAllocator<void, alignment>::const_pointer = 0) {
    void *ptr = detail::allocate_aligned_memory<alignment>(n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args>
  void construct(U *p, Args &&... args) {
    ::new (reinterpret_cast<void *>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) {
    p->~T();
  }
};

template <typename T, size_t TAlignment, typename U, size_t UAlignment>
inline bool operator==(const AlignedAllocator<T, TAlignment> &,
                       const AlignedAllocator<U, UAlignment> &) noexcept {
  return TAlignment == UAlignment;
}

template <typename T, size_t TAlignment, typename U, size_t UAlignment>
inline bool operator!=(const AlignedAllocator<T, TAlignment> &,
                       const AlignedAllocator<U, UAlignment> &) noexcept {
  return TAlignment != UAlignment;
}

template <size_t alignment>
void *detail::allocate_aligned_memory(size_t size) {
  if (size == 0) {
    return nullptr;
  }
  void *ptr = nullptr;
  int rc = posix_memalign(&ptr, alignment, size);
  if (rc != 0) {
    return nullptr;
  }
  return ptr;
}

inline void detail::deallocate_aligned_memory(void *ptr) noexcept {
  return free(ptr);
}

}  // End namespace ocl

}  // namespace hlslib
