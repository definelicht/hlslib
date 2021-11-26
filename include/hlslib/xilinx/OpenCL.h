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

constexpr auto kXilinxMemPointer = CL_MEM_EXT_PTR_XILINX;

/*
The Xilinx Alveo U280 FPGA expects different flags for DRAM banks than previous
DSAs. Therefore for Xilinx these flags are now initialized and stored as part of
the Context class
*/
class DDRBankFlags {
 public:
  DDRBankFlags(const std::string &device_name) {
    if (device_name.find("xilinx_u280") != std::string::npos) {
      memory_bank_0_ = XCL_MEM_TOPOLOGY | 32;
      memory_bank_1_ = XCL_MEM_TOPOLOGY | 33;
      memory_bank_2_ = -1;
      memory_bank_3_ = -1;
    } else {
      DefaultAssignment();
    }
  }

  DDRBankFlags() {
    DefaultAssignment();
  }

  inline int memory_bank_0() const {
    return memory_bank_0_;
  }

  inline int memory_bank_1() const {
    return memory_bank_1_;
  }

  inline int memory_bank_2() const {
    return memory_bank_2_;
  }

  inline int memory_bank_3() const {
    return memory_bank_3_;
  }

 private:
  void DefaultAssignment() {
    memory_bank_0_ = XCL_MEM_DDR_BANK0;
    memory_bank_1_ = XCL_MEM_DDR_BANK1;
    memory_bank_2_ = XCL_MEM_DDR_BANK2;
    memory_bank_3_ = XCL_MEM_DDR_BANK3;
  }

  int memory_bank_0_;
  int memory_bank_1_;
  int memory_bank_2_;
  int memory_bank_3_;
};

// See: Documentation:
// https://www.xilinx.com/support/documentation/sw_manuals/xilinx2020_2/ug1393-vitis-application-acceleration.pdf
// HBM Examples: https://github.com/Xilinx/Vitis_Accel_Examples/tree/master/host
constexpr auto kHBMStorageMagicNumber = XCL_MEM_TOPOLOGY;
using ExtendedMemoryPointer = cl_mem_ext_ptr_t;

constexpr cl_command_queue_properties kCommandQueueFlags =
    CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;

}  // End namespace ocl

}  // End namespace hlslib

#include "../common/OpenCL.h"
