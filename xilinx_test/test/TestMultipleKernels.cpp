#include "MultipleKernels.h"
#include "catch.hpp"
#include "hlslib/xilinx/OpenCL.h"

TEST_CASE("MultipleKernels") {
  using namespace hlslib::ocl;
  Context context;
  auto program = context.MakeProgram("MultipleKernels_hw_emu.xclbin");
  std::vector<uint64_t> memory_host(128, 1);
  auto memory = context.MakeBuffer<uint64_t, Access::readWrite>(
      memory_host.cbegin(), memory_host.cend());
  hlslib::Stream<uint64_t> pipe;
  auto first_kernel = program.MakeKernel("FirstKernel");
  auto second_kernel = program.MakeKernel("SecondKernel");
  auto first_future = first_kernel.ExecuteTaskAsync();
  auto second_future = second_kernel.ExecuteTaskAsync();
  first_future.wait();
  second_future.wait();
  memory.CopyToHost(memory_host.begin());
  for (int i = 0; i < 128; ++i) {
    REQUIRE(memory_host[i] == 4);
  }
}
