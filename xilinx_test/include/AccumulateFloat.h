/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#include "hlslib/xilinx/DataPack.h"
#include "hlslib/xilinx/Operators.h"

using Data_t = float;
constexpr int kDataWidth = 4;
using DataPack_t = hlslib::DataPack<Data_t, kDataWidth>;
constexpr int kLatency = 14;
constexpr int kSize = 10 * kLatency;
constexpr int kIterations = 100;
using Operator =
    hlslib::op::Wide<hlslib::op::Add<Data_t>, DataPack_t, kDataWidth>;
constexpr char const *kKernelName = "AccumulateFloat";
constexpr char const *kKernelFile = "AccumulateFloat.xclbin";

extern "C" {

void AccumulateFloat(DataPack_t const *memoryIn, DataPack_t *memoryOut,
                     int size, int iterations);

}
