/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
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
#define HLSLIB_LEGACY_SDX 0
#endif

#if HLSLIB_LEGACY_SDX == 0
constexpr auto kXilinxMemPointer = CL_MEM_EXT_PTR_XILINX;

//constexpr auto kMemoryBank0 = 32 | XCL_MEM_TOPOLOGY;
//constexpr auto kMemoryBank1 = 33 | XCL_MEM_TOPOLOGY;

class XilinxMemoryBanks {
public:
  static void init(const std::string &device_name) {
    if(device_name == "xilinx_u280_xdma_201920_3") {
      _kMemoryBank0 = XCL_MEM_TOPOLOGY | 32;
      _kMemoryBank1 = XCL_MEM_TOPOLOGY | 33;
    }
    else {
      _kMemoryBank0 = XCL_MEM_DDR_BANK0;
      _kMemoryBank1 = XCL_MEM_DDR_BANK1;
    }
  }

  static inline int kMemoryBank0() {
    return _kMemoryBank0;
  }

  static inline int kMemoryBank1() {
    return _kMemoryBank1;
  }

  static inline int kMemoryBank2() {
    return _kMemoryBank2;
  }

  static inline int kMemoryBank3() {
    return _kMemoryBank3;
  }

private:
  static int _kMemoryBank0;
  static int _kMemoryBank1;
  static constexpr auto _kMemoryBank2 = XCL_MEM_DDR_BANK2;
  static constexpr auto _kMemoryBank3 = XCL_MEM_DDR_BANK3;
};

int XilinxMemoryBanks::_kMemoryBank0 = XCL_MEM_DDR_BANK0;
int XilinxMemoryBanks::_kMemoryBank1 = XCL_MEM_DDR_BANK1;

constexpr auto hbmStorageMagicNumber = XCL_MEM_TOPOLOGY;
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
