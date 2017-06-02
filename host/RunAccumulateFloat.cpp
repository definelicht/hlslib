/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/SDAccel.h"
#include "AccumulateFloat.h"
#include <iostream>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("AccumulateFloat", "[AccumulateFloat]") {

  std::cout << "Initializing OpenCL context..." << std::endl;
  hlslib::ocl::Context context;
  std::cout << "Done." << std::endl;

  std::cout << "Initializing memory..." << std::flush;
  auto memDevice = context.MakeBuffer<DataPack_t, hlslib::ocl::Access::read>(
      hlslib::ocl::MemoryBank::bank0, 1);
  DataPack_t memHost(static_cast<Data_t>(1));
  memDevice.CopyFromHost(&memHost);
  std::cout << " Done." << std::endl;

  std::cout << "Creating kernel..." << std::flush;
  auto kernel = context.MakeKernel("RunAccumulateFloat.xclbin",
                                   "AccumulateFloat", memDevice, memDevice);
  std::cout << " Done." << std::endl;

  std::cout << "Executing kernel..." << std::flush;
  kernel.ExecuteTask();
  std::cout << " Done." << std::endl;

  std::cout << "Verifying result..." << std::flush;
  memDevice.CopyToHost(&memHost);
  for (int i = 0; i < kDataWidth; ++i) {
    REQUIRE(memHost[i] == kSize); 
  }
  std::cout << " Done." << std::endl;

  std::cout << "Kernel ran successfully." << std::endl;

}
