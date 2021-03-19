/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/xilinx/OpenCL.h"
#include "MultiStageAdd.h"
#include <iostream>

enum class Mode {
  simulation,
  emulation,
  hardware,
};

int RunKernel(Mode mode) {

  if (mode == Mode::hardware) {
    std::cout << "Running hardware kernel..." << std::endl;
  } else if (mode == Mode::emulation) {
    std::cout << "Running emulation kernel..." << std::endl;
  } else {
    std::cout << "Running simulation kernel..." << std::endl;
  }

  std::cout << "Initializing OpenCL context..." << std::endl;
  hlslib::ocl::Context context;
  std::cout << "Done." << std::endl;

  std::cout << "Initializing memory..." << std::flush;
  std::vector<Data_t> memHost(kNumElements, 0);
  auto memDevice = context.MakeBuffer<int, hlslib::ocl::Access::readWrite>(
      hlslib::ocl::MemoryBank::bank0, memHost.cbegin(), memHost.cend());
  std::cout << " Done." << std::endl;

  std::cout << "Creating kernel..." << std::flush;
  auto program =
      (mode == Mode::simulation)
          ? context.MakeProgram("MultiStageAdd_sw_emu.xclbin")
          : ((mode == Mode::hardware)
                 ? context.MakeProgram("MultiStageAdd_hw.xclbin")
                 : context.MakeProgram("MultiStageAdd_hw_emu.xclbin"));
  auto kernel = program.MakeKernel("MultiStageAdd", memDevice, memDevice);
  std::cout << " Done." << std::endl;

  std::cout << "Executing kernel..." << std::flush;
  kernel.ExecuteTask();
  std::cout << " Done." << std::endl;

  std::cout << "Verifying result..." << std::flush;
  memDevice.CopyToHost(memHost.begin());
  for (auto &m : memHost) {
    if (m != kStages) {
      return 3;
    }
  }
  std::cout << " Done." << std::endl;

  std::cout << "Kernel ran successfully." << std::endl;

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Required argument: [emulation/hardware]\n" << std::flush;
    return 1;
  }
  const std::string mode(argv[1]);
  if (mode == "emulation") {
    return RunKernel(Mode::emulation);
  } else if (mode == "hardware") {
    return RunKernel(Mode::hardware);
  } else if (mode == "simulation") {
    return RunKernel(Mode::simulation);
  } else {
    std::cerr << "Unrecognized mode: " << mode << std::endl;
    return 2;
  }
}
