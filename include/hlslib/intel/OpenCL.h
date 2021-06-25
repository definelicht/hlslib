/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#define HLSLIB_INTEL_OPENCL_H

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "CL/cl2.hpp"

#if !defined(CL_CHANNEL_1_INTELFPGA)
// include this header if channel macros are not defined in cl.hpp (versions >=19.0)
#include "CL/cl_ext_intelfpga.h"
#endif

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
  DDRBankFlags(const std::string &device_name) { }

  DDRBankFlags() { }

  inline int memory_bank_0() const { return CL_CHANNEL_1_INTELFPGA; }

  inline int memory_bank_1() const { return CL_CHANNEL_2_INTELFPGA; }

  inline int memory_bank_2() const { return CL_CHANNEL_3_INTELFPGA; }

  inline int memory_bank_3() const { return CL_CHANNEL_4_INTELFPGA; }
};

constexpr cl_command_queue_properties kCommandQueueFlags =
    CL_QUEUE_PROFILING_ENABLE;

}  // End namespace ocl

}  // End namespace hlslib

#include "../common/OpenCL.h"
