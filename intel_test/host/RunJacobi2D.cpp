#include <cmath>
#include <iostream>

#include "Jacobi2D.h"
#include "hlslib/intel/OpenCL.h"

// Convert from C to C++
using Data_t = DTYPE;
constexpr int kW = COLS;
constexpr int kH = ROWS;
constexpr int kT = TIMESTEPS;
constexpr auto kUsage =
    "Usage: ./RunJacobi2D <[emulator/hardware]> "
    "[<[default/oldapi_copy/newapi_copy/newapi_notransfer]>]\n";

// Reference implementation for checking correctness
void Reference(std::vector<Data_t> &domain) {
  std::vector<Data_t> buffer(domain);
  for (int t = 0; t < kT; ++t) {
    for (int i = 1; i < kH - 1; ++i) {
      for (int j = 1; j < kW - 1; ++j) {
        buffer[i * kW + j] =
            static_cast<Data_t>(0.25) *
            (domain[(i - 1) * kW + j] + domain[(i + 1) * kW + j] +
             domain[i * kW + j - 1] + domain[i * kW + j + 1]);
      }
    }
    domain.swap(buffer);
  }
}

int main(int argc, char **argv) {
  // Handle input arguments
  bool printError = argc < 2 && argc > 3;
  std::string copyMode = "default";
  if (argc == 3) {
    copyMode = std::string(argv[2]);
    if (copyMode != "newapi_copy" && copyMode != "newapi_notransfer" &&
        copyMode != "oldapi_copy" && copyMode != "default") {
      printError = true;
    }
  }

  if (printError) {
    std::cout << kUsage;
    return 1;
  }
  bool emulator = false;
  std::string mode_str(argv[1]);
  std::string kernel_path;
  if (mode_str == "emulator") {
    emulator = true;
    kernel_path = "Jacobi2D_emulator.aocx";
  } else if (mode_str == "hardware") {
    kernel_path = "Jacobi2D_hardware.aocx";
    emulator = false;
  } else {
    std::cout << kUsage;
    return 2;
  }

  std::cout << "Initializing host memory...\n" << std::flush;
  // Set center to 0
  std::vector<Data_t> host_buffer(kW * kH, 0);
  // Set boundaries to 1
  for (int i = 0; i < kW; ++i) {
    host_buffer[i] = 1;
    host_buffer[kW * (kH - 1) + i] = 1;
  }
  for (int i = 0; i < kH; ++i) {
    host_buffer[i * kW] = 1;
    host_buffer[i * kW + kW - 1] = 1;
  }
  std::vector<Data_t> reference(host_buffer);

  // Create OpenCL kernels
  std::cout << "Creating OpenCL context...\n" << std::flush;
  hlslib::ocl::Context context;
  std::cout << "Allocating device memory...\n" << std::flush;
  hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::readWrite> device_buffer;
  // Copy to both sections of device memory, so that the boundary conditions
  // are reflected in both
  if (copyMode == "default") {
    device_buffer =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(2 * kW * kH);
    device_buffer.CopyFromHost(0, kW * kH, host_buffer.cbegin());
    device_buffer.CopyFromHost(kW * kH, kW * kH, host_buffer.cbegin());
  } else if (copyMode == "oldapi_copy") {
    std::vector<Data_t> copy_host_buffer(2 * kW * kH, 0);
    std::copy(host_buffer.begin(), host_buffer.end(), copy_host_buffer.begin());
    std::copy(host_buffer.begin(), host_buffer.end(),
              copy_host_buffer.begin() + kW * kH);
    device_buffer = context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
        hlslib::ocl::MemoryBank::bank0, copy_host_buffer.begin(),
        copy_host_buffer.end());
  } else if (copyMode == "newapi_copy") {
    std::vector<Data_t> copy_host_buffer(2 * kW * kH, 0);
    std::copy(host_buffer.begin(), host_buffer.end(), copy_host_buffer.begin());
    std::copy(host_buffer.begin(), host_buffer.end(),
              copy_host_buffer.begin() + kW * kH);
    device_buffer = context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
        hlslib::ocl::StorageType::DDR, -1, copy_host_buffer.begin(),
        copy_host_buffer.end());
  } else if (copyMode == "newapi_notransfer") {
    device_buffer = context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
        hlslib::ocl::StorageType::DDR, 1, 2 * kW * kH);
    device_buffer.CopyFromHost(0, kW * kH, host_buffer.cbegin());
    device_buffer.CopyFromHost(kW * kH, kW * kH, host_buffer.cbegin());
  }

  std::cout << "Creating program from binary...\n" << std::flush;
  auto program = context.MakeProgram(kernel_path);
  std::cout << "Creating kernels...\n" << std::flush;
  std::vector<hlslib::ocl::Kernel> kernels;
  kernels.emplace_back(program.MakeKernel("Read", device_buffer));
  kernels.emplace_back(program.MakeKernel("Jacobi2D"));
  kernels.emplace_back(program.MakeKernel("Write", device_buffer));
  std::vector<hlslib::ocl::Event> futures;

  // Execute kernel
  std::cout << "Launching kernels...\n" << std::flush;
  for (auto &k : kernels) {
    futures.emplace_back(k.ExecuteTaskAsync());
  }
  std::cout << "Waiting for kernels to finish...\n" << std::flush;
  for (auto &f : futures) {
    f.wait();
  }

  // Copy back result
  std::cout << "Copying back result...\n" << std::flush;
  int offset = (kT % 2 == 0) ? 0 : kH * kW;
  std::vector<Data_t> derp(2 * kH * kW);
  device_buffer.CopyToHost(offset, kH * kW, host_buffer.begin());

  // Run reference implementation
  std::cout << "Running reference implementation...\n" << std::flush;
  Reference(reference);

  // Compare result
  for (int i = 0; i < kH; ++i) {
    for (int j = 0; j < kW; ++j) {
      auto diff = std::abs(host_buffer[i * kW + j] - reference[i * kW + j]);
      if (diff > 1e-4 * reference[i * kW + j]) {
        std::cerr << "Mismatch found at (" << i << ", " << j
                  << "): " << host_buffer[i * kW + j] << " (should be "
                  << reference[i * kW + j] << ").\n";
        return 3;
      }
    }
  }

  std::cout << "Successfully verified result.\n";

  return 0;
}
