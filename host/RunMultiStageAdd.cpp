/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      April 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/SDAccel.h"
#include "MultiStageAdd.h"
#include <iostream>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("MultiStageAddDevice", "[MultiStageAddDevice]") {

  std::cout << "Initializing OpenCL context..." << std::endl;
  hlslib::ocl::Context context;
  std::cout << "Done." << std::endl;

  std::cout << "Initializing memory..." << std::flush;
  auto memDevice = context.MakeBuffer<int, hlslib::ocl::Access::readWrite>(
      hlslib::ocl::MemoryBank::bank0, kNumElements);
  std::vector<Data_t> memHost(kNumElements, 0);
  memDevice.CopyFromHost(memHost.cbegin());
  std::cout << " Done." << std::endl;

  std::cout << "Creating kernel..." << std::flush;
  auto program = context.MakeProgram("MultiStageAdd.xclbin");
  auto kernel = program.MakeKernel("MultiStageAdd", memDevice, memDevice);
  std::cout << " Done." << std::endl;

  std::cout << "Executing kernel..." << std::flush;
  kernel.ExecuteTask();
  std::cout << " Done." << std::endl;

  std::cout << "Verifying result..." << std::flush;
  memDevice.CopyToHost(memHost.begin());
  for (auto &m : memHost) {
    REQUIRE(m == kStages); 
  }
  std::cout << " Done." << std::endl;

  std::cout << "Kernel ran successfully." << std::endl;

}
