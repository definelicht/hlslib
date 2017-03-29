#include "hlslib/SDAccel.h"
#include <iostream>

using Data_t = int;
constexpr int kMemSize = 1<<18;

int main() {

  try {

    std::cout << "Creating context..." << std::endl;
    hlslib::ocl::Context context;
    std::cout << "Context created successfully." << std::endl;

    std::cout << "Creating device input buffer..." << std::flush;
    auto mem0Device = context.MakeBuffer<int, hlslib::ocl::Access::read>(
        hlslib::ocl::MemoryBank::bank0, kMemSize);
    std::cout << " Done." << std::endl;

    std::cout << "Initializing host input memory..." << std::flush;
    std::vector<Data_t> mem0Host(kMemSize, 5);
    std::cout << " Done." << std::endl;

    std::cout << "Copying to device..." << std::flush;
    mem0Device.CopyFromHost(mem0Host.cbegin());
    std::cout << " Done." << std::endl;

    std::cout << "Creating device output buffer..." << std::flush;
    auto mem1Device = context.MakeBuffer<int, hlslib::ocl::Access::write>(
        hlslib::ocl::MemoryBank::bank0, kMemSize);
    std::cout << " Done." << std::endl;

    std::cout << "Copying from input to output buffer..." << std::flush;
    mem0Device.CopyToDevice(mem1Device);
    std::cout << " Done." << std::endl;

    std::cout << "Initializing host output memory..." << std::flush;
    std::vector<Data_t> mem1Host(kMemSize, 0);
    std::cout << " Done." << std::endl;

    std::cout << "Copying to host..." << std::flush;
    mem1Device.CopyToHost(mem1Host.begin());
    std::cout << " Done." << std::endl;

    std::cout << "Verifying values..." << std::flush;
    for (auto &e : mem1Host) {
      if (e != 5) {
        std::cerr << "Unexpected value returned from device." << std::endl;
        return 3;
      }
    }
    std::cout << " Done." << std::endl;

  } catch (hlslib::ocl::ConfigurationError const &err) {
    std::cerr << "Configuration failed with error: " << err.what() << std::endl;
    return 1;
  } catch (hlslib::ocl::RuntimeError const &err) {
    std::cerr << "Runtime failed with error: " << err.what()
              << std::endl;
    return 2;
  }

  std::cout << "SDAccel platform successfully verified." << std::endl;

  return 0;

}
