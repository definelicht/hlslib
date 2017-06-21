/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "AccumulateFloat.h"
#include "hlslib/Accumulate.h"
#include "hlslib/Simulation.h"

void Read(DataPack_t const *memoryIn, hlslib::Stream<DataPack_t> &streamIn) {
  DataPack_t val;
  for (auto i = 0; i < kIterations * kSize; ++i) {
    #pragma HLS PIPELINE
    val = memoryIn[i];
    hlslib::WriteBlocking(streamIn, val, 1);
  }
}

void Write(hlslib::Stream<DataPack_t> &streamOut, DataPack_t *memoryOut) {
  for (auto i = 0; i < kIterations; ++i) {
    #pragma HLS PIPELINE
    const auto read = hlslib::ReadBlocking(streamOut);
    memoryOut[i] = read;
  }
}

void AccumulateFloat(DataPack_t const *memoryIn, DataPack_t *memoryOut) {
  #pragma HLS INTERFACE m_axi port=memoryIn  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=memoryOut offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=memoryIn  bundle=control
  #pragma HLS INTERFACE s_axilite port=memoryOut bundle=control
  #pragma HLS INTERFACE s_axilite port=return    bundle=control
  #pragma HLS DATAFLOW
  static hlslib::Stream<DataPack_t> pipeIn("pipeIn");
  static hlslib::Stream<DataPack_t> pipeOut("pipeOut");
  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read, memoryIn, pipeIn);
  hlslib::Accumulate<Data_t, kDataWidth, Operator, kLatency, kSize,
                     kIterations>(pipeIn, pipeOut);
  HLSLIB_DATAFLOW_FUNCTION(Write, pipeOut, memoryOut);
  HLSLIB_DATAFLOW_FINALIZE();
}
