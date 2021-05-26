/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#define HLSLIB_INTEL_OPENCL_H

#include "CL/cl.hpp"

#if !defined(CL_CHANNEL_1_INTELFPGA)
// include this header if channel macros are not defined in cl.hpp (versions >=19.0)
#include "CL/cl_ext_intelfpga.h"
#endif

#define HLSLIB_LEGACY_OPENCL

#ifndef HLSLIB_INTEL
#define HLSLIB_INTEL
#endif

#define HLSLIB_OPENCL_VENDOR_STRING "Intel(R) FPGA SDK for OpenCL(TM)"

namespace hlslib {

namespace ocl {

/*
This was modified to be compatible with the Xilinx OpenCL version.
*/
class DDRBankFlags {
public:
  DDRBankFlags(const std::string &device_name) { DefaultAssignment(); }

  DDRBankFlags() { DefaultAssignment(); }

  inline int kMemoryBank0() const { return memory_bank_0; }

  inline int kMemoryBank1() const { return memory_bank_1; }

  inline int kMemoryBank2() const { return memory_bank_2; }

  inline int kMemoryBank3() const { return memory_bank_3; }

private:
  void DefaultAssignment() {
    memory_bank_0 = CL_CHANNEL_1_INTELFPGA;
    memory_bank_1 = CL_CHANNEL_2_INTELFPGA;
    memory_bank_2 = CL_CHANNEL_3_INTELFPGA;
    memory_bank_3 = CL_CHANNEL_4_INTELFPGA;
  }

  int memory_bank_0;
  int memory_bank_1;
  int memory_bank_2;
  int memory_bank_3;
};

constexpr cl_command_queue_properties kCommandQueueFlags =
    CL_QUEUE_PROFILING_ENABLE;

}  // End namespace ocl

}  // End namespace hlslib

#include "../common/OpenCL.h"
