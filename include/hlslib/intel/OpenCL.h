/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      February 2019
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

constexpr auto kMemoryBank0 = CL_CHANNEL_1_INTELFPGA;
constexpr auto kMemoryBank1 = CL_CHANNEL_2_INTELFPGA;
constexpr auto kMemoryBank2 = CL_CHANNEL_3_INTELFPGA;
constexpr auto kMemoryBank3 = CL_CHANNEL_4_INTELFPGA;

constexpr cl_command_queue_properties kCommandQueueFlags =
    CL_QUEUE_PROFILING_ENABLE;

}  // End namespace ocl

}  // End namespace hlslib

#include "../common/OpenCL.h"
