/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include "hlslib/DataPack.h"
#include "hlslib/Stream.h"
#include <vector>

template <typename T, int iterations>
void Read(T const *memoryIn, hlslib::Stream<T> &streamIn) {
  for (int i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    const auto val = memoryIn[i];
    hlslib::WriteBlocking(streamIn, val, 1);
  }
}

template <typename T, int iterations>
void Write(hlslib::Stream<T> &streamOut, T *memoryOut) {
  for (int i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = hlslib::ReadBlocking(streamOut);
    memoryOut[i] = read;
  }
}

template <typename T, int width, class Operator, int size, int iterations>
std::vector<hlslib::DataPack<T, width>>
NaiveAccumulate(std::vector<hlslib::DataPack<T, width>> &vec) {
  std::vector<hlslib::DataPack<T, width>> result(iterations);
  for (int i = 0; i < iterations; ++i) {
    hlslib::DataPack<T, width> acc = Operator::identity();
    for (int j = 0; j < size; ++j) {
      for (int w = 0; w < width; ++w) {
        acc[w] = Operator::Apply(acc[w], vec[i * size + j][w]);
      }
    }
    result[i] = acc;
  }
  return result;
}
