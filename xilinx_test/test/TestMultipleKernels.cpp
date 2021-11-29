#include <cstdlib>

#include "MultipleKernels.h"
#include "catch.hpp"
#include "hlslib/xilinx/OpenCL.h"

constexpr int kTestSize = 16;

#ifndef HLSLIB_SIMULATE_OPENCL
TEST_CASE("MultipleKernels Hardware Emulation") {
  setenv("XCL_EMULATION_MODE", "hw_emu", true);
  hlslib::ocl::Context context;
  auto program = context.MakeProgram("MultipleKernels_hw_emu.xclbin");
#else
TEST_CASE("MultipleKernels Simulation") {
  hlslib::ocl::Context context;
  auto program = context.MakeProgram("");
#endif
  std::vector<uint64_t> memory_host(kTestSize, 1);
  auto memory = context.MakeBuffer<uint64_t, hlslib::ocl::Access::readWrite>(
      memory_host.cbegin(), memory_host.cend());
  hlslib::Stream<uint64_t> pipe;
  auto first_kernel =
      program.MakeKernel(FirstKernel, "FirstKernel", memory,
                         hlslib::ocl::SimulationOnly(pipe), kTestSize);
  auto second_kernel =
      program.MakeKernel(SecondKernel, "SecondKernel",
                         hlslib::ocl::SimulationOnly(pipe), memory, kTestSize);
  auto first_future = first_kernel.ExecuteTaskAsync();
  auto second_future = second_kernel.ExecuteTaskAsync();
  first_future.wait();
  second_future.wait();
  memory.CopyToHost(memory_host.begin());
  for (int i = 0; i < kTestSize; ++i) {
    REQUIRE(memory_host[i] == 4);
  }
}
