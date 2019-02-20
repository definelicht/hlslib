/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      February 2019
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#define HLSLIB_XILINX_OPENCL_H

#define HLSLIB_OPENCL_VENDOR_STRING "Xilinx"
#ifndef HLSLIB_XILINX
#define HLSLIB_XILINX
#endif

// Configuration taken from Xilinx' xcl2 utility functions to maximize odds of
// everything working.
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl2.hpp>

namespace hlslib {

namespace ocl {

#ifndef HLSLIB_LEGACY_SDX
#warning "HLSLIB_LEGACY_SDX not set: assuming SDAccel 2017.4+. Set to 0 or 1 to silence this warning."
#define HLSLIB_LEGACY_SDX 0
#endif

#if HLSLIB_LEGACY_SDX == 0
constexpr auto kXilinxMemPointer = CL_MEM_EXT_PTR_XILINX;
constexpr auto kMemoryBank0 = XCL_MEM_DDR_BANK0;
constexpr auto kMemoryBank1 = XCL_MEM_DDR_BANK1;
constexpr auto kMemoryBank2 = XCL_MEM_DDR_BANK2;
constexpr auto kMemoryBank3 = XCL_MEM_DDR_BANK3;
using ExtendedMemoryPointer = cl_mem_ext_ptr_t;
#else
// Before 2017.4, these values were only available numerically, and the
// extended memory pointer had to be constructed manually.
constexpr cl_mem_flags kXilinxMemPointer = 1 << 31;
constexpr unsigned kMemoryBank0 = 1 << 8;
constexpr unsigned kMemoryBank1 = 1 << 9;
constexpr unsigned kMemoryBank2 = 1 << 10;
constexpr unsigned kMemoryBank3 = 1 << 11;
struct ExtendedMemoryPointer {
  unsigned flags;
  void *obj;
  void *param;
};
#endif

constexpr cl_command_queue_properties kCommandQueueFlags =
    CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;

}  // End namespace ocl

}  // End namespace hlslib

#include "../common/OpenCL.h"
