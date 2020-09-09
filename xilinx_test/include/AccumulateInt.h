/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include "hlslib/xilinx/DataPack.h"
#include "hlslib/xilinx/Operators.h"

using Data_t = int;
constexpr int kDataWidth = 8;
using DataPack_t = hlslib::DataPack<Data_t, kDataWidth>; 
constexpr int kLatency = 1;
constexpr int kSize = 100;
constexpr int kIterations = 100;
using Operator =
    hlslib::op::Wide<hlslib::op::Add<Data_t>, DataPack_t, kDataWidth>;
constexpr char const *kKernelName = "AccumulateInt";
constexpr char const *kKernelFile = "AccumulateInt.xclbin";

extern "C" {

void AccumulateInt(DataPack_t const *memoryIn, DataPack_t *memoryOut, int size,
                   int iterations);
}
