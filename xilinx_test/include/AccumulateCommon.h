/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include "hlslib/xilinx/DataPack.h"
#include "hlslib/xilinx/Stream.h"
#include <vector>

template <typename T>
void Read(T const *memoryIn, hlslib::Stream<T> &streamIn, int iterations) {
  for (int i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    const auto val = memoryIn[i];
    streamIn.Push(val);
  }
}

template <typename T>
void Write(hlslib::Stream<T> &streamOut, T *memoryOut, int iterations) {
  for (int i = 0; i < iterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = streamOut.Pop();
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
