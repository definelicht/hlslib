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

/*
The xilinx alveo u280 fpga has different flags for the DRAM banks, then the existing 
fpga's. Therefore for Xilinx the DDR-flags are now initialized and stored as part of the
Contex-class.
*/
class DDRBankFlags {
public:
  DDRBankFlags(const std::string &device_name) {
    if (device_name.find("xilinx_u280") != std::string::npos) {
      memory_bank_0 = XCL_MEM_TOPOLOGY | 32;
      memory_bank_1 = XCL_MEM_TOPOLOGY | 33;
      memory_bank_2 = -1;
      memory_bank_3 = -1;
    } else {
      DefaultAssignment();
    }
  }

  DDRBankFlags() { DefaultAssignment(); }

  inline int kMemoryBank0() const { return memory_bank_0; }

  inline int kMemoryBank1() const { return memory_bank_1; }

  inline int kMemoryBank2() const { return memory_bank_2; }

  inline int kMemoryBank3() const { return memory_bank_3; }

private:
  void DefaultAssignment() {
    memory_bank_0 = XCL_MEM_DDR_BANK0;
    memory_bank_1 = XCL_MEM_DDR_BANK1;
    memory_bank_2 = XCL_MEM_DDR_BANK2;
    memory_bank_3 = XCL_MEM_DDR_BANK3;
  }

  int memory_bank_0;
  int memory_bank_1;
  int memory_bank_2;
  int memory_bank_3;
};

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
