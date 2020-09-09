/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include "hlslib/xilinx/DataPack.h"
#include "hlslib/xilinx/Operators.h"
#include "hlslib/xilinx/Simulation.h"
#include "hlslib/xilinx/Stream.h"
#include "hlslib/xilinx/TreeReduce.h"

constexpr int kIterations = 2048;
constexpr int kFloatWidth = 8;
constexpr int kBoolWidth = 7;
using Float_t = hlslib::DataPack<float, kFloatWidth>;
using Bool_t = hlslib::DataPack<bool, kBoolWidth>;

void FloatSum(hlslib::Stream<Float_t> &in, hlslib::Stream<float> &out) {
FloatSum:
  for (int i = 0; i < kIterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = in.Pop();
    float arr[kFloatWidth];
    read >> arr;
    const auto result =
        hlslib::TreeReduce<float, hlslib::op::Add<float>, kFloatWidth>(arr);
    out.Push(result);
  }
}

void BoolAll(hlslib::Stream<Bool_t> &in, hlslib::Stream<bool> &out) {
BoolAll:
  for (int i = 0; i < kIterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = in.Pop();
    bool arr[kBoolWidth];
    read >> arr;
    const auto result =
        hlslib::TreeReduce<bool, hlslib::op::And<bool>, kBoolWidth>(arr);
    out.Push(result);
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
