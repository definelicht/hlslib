/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/TreeReduce.h"
#include "hlslib/DataPack.h"
#include "hlslib/Stream.h"
#include "hlslib/Simulation.h"

constexpr int kIterations = 2048;
constexpr int kFloatWidth = 8;
constexpr int kBoolWidth = 7;
using Float_t = hlslib::DataPack<float, kFloatWidth>;
using Bool_t = hlslib::DataPack<bool, kBoolWidth>;

void FloatSum(hlslib::Stream<Float_t> &in, hlslib::Stream<float> &out) {
FloatSum:
  for (int i = 0; i < kIterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = hlslib::ReadBlocking(in);
    float arr[kFloatWidth];
    read >> arr;
    const auto result =
        hlslib::TreeReduce<float, hlslib::Add<float>, kFloatWidth>(arr);
    hlslib::WriteBlocking(out, result, 1);
  }
}

void BoolAll(hlslib::Stream<Bool_t> &in, hlslib::Stream<bool> &out) {
BoolAll:
  for (int i = 0; i < kIterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = hlslib::ReadBlocking(in);
    bool arr[kBoolWidth];
    read >> arr;
    const auto result =
        hlslib::TreeReduce<bool, hlslib::And<bool>, kBoolWidth>(arr);
    hlslib::WriteBlocking(out, result, 1);
  }
}

void Reduce(hlslib::Stream<Float_t> &floatIn,
            hlslib::Stream<float> &floatOut,
            hlslib::Stream<Bool_t> &boolIn,
            hlslib::Stream<bool> &boolOut) {
  #pragma HLS INTERFACE axis port=floatIn
  #pragma HLS INTERFACE axis port=floatOut
  #pragma HLS INTERFACE axis port=boolIn
  #pragma HLS INTERFACE axis port=boolOut
  #pragma HLS DATAFLOW
  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(FloatSum, floatIn, floatOut);
  HLSLIB_DATAFLOW_FUNCTION(BoolAll, boolIn, boolOut);
  HLSLIB_DATAFLOW_FINALIZE();
}
