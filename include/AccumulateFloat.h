/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include "hlslib/DataPack.h"
#include "hlslib/Stream.h"

using Data_t = float;
constexpr int kDataWidth = 2;
using DataPack_t = hlslib::DataPack<Data_t, kDataWidth>; 
constexpr int kLatency = 10;
constexpr int kSize = 10000;
#ifdef HLSLIB_SYNTHESIS
constexpr int kIterations = 20000;
#else
// In order to let simulation finish in a reasonable amount of time
constexpr int kIterations = 100;
#endif

extern "C" {

void AccumulateFloat(DataPack_t const *memoryIn, DataPack_t *memoryOut);

}
