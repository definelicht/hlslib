/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      April 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include <iostream>
#include "aligned_allocator.h"
#include "hlslib/SDAccel.h"

using Data_t = unsigned long;
constexpr int kMemSize = 1 << 20;

int main() {
  try {
    std::cout << "Creating context..." << std::endl;
    hlslib::ocl::Context context;
    std::cout << "Context created successfully." << std::endl;

    std::cout << "Initializing host memory..." << std::flush;
    std::vector<Data_t, aligned_allocator<Data_t, 4096>> mem0Host(2 * kMemSize,
                                                                  0);
    for (int i = kMemSize; i < 2 * kMemSize; ++i) {
      mem0Host[i] = 5;
    }
    std::vector<Data_t, aligned_allocator<Data_t, 4096>> mem1Host(kMemSize, 0);
    std::cout << " Done." << std::endl;

    std::cout << "Creating device input buffer and copying from host..."
              << std::flush;
    auto mem0Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
        mem0Host.cbegin(), mem0Host.cend());
    std::cout << " Done." << std::endl;

    std::cout << "Creating device output buffer..." << std::flush;
    auto mem1Device =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(2 * kMemSize);
    std::cout << " Done." << std::endl;

    std::cout << "Copying from input to output buffer..." << std::flush;
    mem0Device.CopyToDevice(kMemSize, kMemSize, mem1Device, kMemSize);
    std::cout << " Done." << std::endl;

    std::cout << "Copying to host..." << std::flush;
    mem1Device.CopyToHost(kMemSize, kMemSize, mem1Host.begin());
    std::cout << " Done." << std::endl;

    std::cout << "Verifying values..." << std::flush;
    for (int i = 0; i < kMemSize; ++i) {
      if (mem1Host[i] != 5) {
        std::cerr << " Unexpected value returned from device." << std::endl;
        return 3;
      }
    }
    std::cout << " Done." << std::endl;

  } catch (hlslib::ocl::ConfigurationError const &err) {
    std::cerr << "Configuration failed with error: " << err.what() << std::endl;
    return 1;
  } catch (hlslib::ocl::RuntimeError const &err) {
    std::cerr << "Runtime failed with error: " << err.what() << std::endl;
    return 2;
  }

  std::cout << "SDAccel platform successfully verified." << std::endl;

  return 0;
}
