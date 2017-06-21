/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include "hlslib/DataPack.h"
#include "hlslib/Stream.h"
#include "hlslib/Operators.h"
#include <vector>

using Data_t = float;
constexpr int kDataWidth = 4;
using DataPack_t = hlslib::DataPack<Data_t, kDataWidth>; 
constexpr int kLatency = 14;
constexpr int kSize = 10 * kLatency;
constexpr int kIterations = 100;
using Operator = hlslib::Add<Data_t>;

extern "C" {

void AccumulateFloat(DataPack_t const *memoryIn, DataPack_t *memoryOut);

}

inline std::vector<DataPack_t> NaiveAccumulate(std::vector<DataPack_t> &vec) {
  std::vector<DataPack_t> result(kIterations);
  for (int i = 0; i < kIterations; ++i) {
    DataPack_t acc = Operator::identity();
    for (int j = 0; j < kSize; ++j) {
      for (int w = 0; w < kDataWidth; ++w) {
        acc[w] = Operator::Apply(acc[w], vec[i * kSize + j][w]);
      }
    }
    result[i] = acc;
  }
  return result;
}
