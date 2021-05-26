#include "../../include/hlslib/intel/OpenCL.h"
#include <algorithm>
#include <assert.h>
#include <iostream>

#define DATA_SIZE 1024

int main(int argc, char **argv) {
  std::cout << "Initializing OpenCL context..." << std::endl;
  //"xilinx_u280_xdma_201920_3"
  hlslib::ocl::Context context;
  std::cout << "Done." << std::endl << std::flush;

  // Handle input arguments
  std::string kUsage = "./RunDDRExplicit [emulation|hardware]";
  if (argc != 2) {
    std::cout << kUsage << std::flush;
    return 1;
  }
  std::string mode_str(argv[1]);
  std::string kernel_path;
  if (mode_str == "emulation") {
    kernel_path = "DDRExplicit_hw_emu.xclbin";
  } else if (mode_str == "hardware") {
    kernel_path = "DDRExplicit_hw.xclbin";
  } else {
    std::cout << kUsage << std::flush;
    return 2;
  }

  std::cout << std::endl << "Loading Kernel" << std::endl << std::flush;
  auto program = context.MakeProgram(kernel_path);

  std::cout << "Done" << std::endl
            << "Initializing memory..." << std::endl
            << std::flush;
  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> ddr0mem(DATA_SIZE);
  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> ddr1mem(DATA_SIZE);
  std::fill(ddr1mem.begin(), ddr1mem.end(), 15);

  auto memDevice1 = context.MakeBuffer<int, hlslib::ocl::Access::readWrite>(hlslib::ocl::MemoryBank::bank0, DATA_SIZE);
  auto memDevice2 = context.MakeBuffer<int,hlslib::ocl::Access::readWrite>(hlslib::ocl::MemoryBank::bank1, ddr1mem.begin(), ddr1mem.end());

  std::cout << "Done" << std::endl << std::flush;
  std::cout << "Running Kernel" << std::endl << std::flush;

  auto kernel = program.MakeKernel("DDRExplicit", memDevice1, memDevice2);
  kernel.ExecuteTask();
  memDevice1.CopyToHost(ddr0mem.begin());

  for (int i = 0; i < DATA_SIZE; i++) {
    assert(ddr0mem[i] == ddr1mem[i]);
  }

  std::cout << "Done" << std::endl << std::flush;
}