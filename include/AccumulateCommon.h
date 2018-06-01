/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include "hlslib/DataPack.h"
#include "hlslib/Stream.h"
#include <vector>

template <typename T>
void Read(T const *memoryIn, hlslib::Stream<T> &streamIn, int iterations) {
  for (int i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    const auto val = memoryIn[i];
    hlslib::WriteBlocking(streamIn, val, 1);
  }
}

template <typename T>
void Write(hlslib::Stream<T> &streamOut, T *memoryOut, int iterations) {
  for (int i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = hlslib::ReadBlocking(streamOut);
    memoryOut[i] = read;
  }
}

template <typename T, class Operator, int size, int iterations>
std::vector<T> NaiveAccumulate(std::vector<T> &vec) {
  std::vector<T> result(iterations);
  for (int i = 0; i < iterations; ++i) {
    T acc = Operator::identity();
    for (int j = 0; j < size; ++j) {
      acc = Operator::Apply(acc, vec[i * size + j]);
    }
    result[i] = acc;
  }
  return result;
}
