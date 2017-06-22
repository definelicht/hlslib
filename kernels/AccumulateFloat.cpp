/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "AccumulateCommon.h"
#include "AccumulateFloat.h"
#include "hlslib/Accumulate.h"
#include "hlslib/Simulation.h"

void AccumulateFloat(DataPack_t const *memoryIn, DataPack_t *memoryOut) {
  #pragma HLS INTERFACE m_axi port=memoryIn  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=memoryOut offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=memoryIn  bundle=control
  #pragma HLS INTERFACE s_axilite port=memoryOut bundle=control
  #pragma HLS INTERFACE s_axilite port=return    bundle=control
  #pragma HLS DATAFLOW

  hlslib::Stream<DataPack_t> pipeIn("pipeIn");
  hlslib::Stream<DataPack_t> pipeOut("pipeOut");
  hlslib::Stream<DataPack_t> toFeedback("fromFeedback");
  hlslib::Stream<DataPack_t> toReduce("toReduce");

#ifndef HLSLIB_SYNTHESIS
  hlslib::Stream<DataPack_t> fromFeedback("fromFeedback");
  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read<DataPack_t, kIterations * kSize>, memoryIn,
                           pipeIn);
  HLSLIB_DATAFLOW_FUNCTION(
      hlslib::AccumulateIterate<DataPack_t, Operator, kLatency, kSize,
                                kIterations>,
      pipeIn, fromFeedback, toFeedback);
  HLSLIB_DATAFLOW_FUNCTION(
      hlslib::AccumulateFeedback<DataPack_t, Operator, kLatency, kSize,
                                 kIterations>,
      toFeedback, fromFeedback, toReduce);
  HLSLIB_DATAFLOW_FUNCTION(
      hlslib::AccumulateReduce<DataPack_t, Operator, kLatency, kSize,
                               kIterations>,
      toReduce, pipeOut);
  HLSLIB_DATAFLOW_FUNCTION(Write<DataPack_t, kIterations>, pipeOut, memoryOut);
  HLSLIB_DATAFLOW_FINALIZE();
#else
  hls::stream<DataPack_t> fromFeedback("fromFeedback");
  #pragma HLS STREAM variable=fromFeedback depth=kLatency
  Read<DataPack_t, kIterations * kSize>(memoryIn, pipeIn);
  hlslib::AccumulateIterate<DataPack_t, Operator, kLatency, kSize, kIterations>(
      pipeIn, fromFeedback, toFeedback);
  hlslib::AccumulateFeedback<DataPack_t, Operator, kLatency, kSize,
                             kIterations>(toFeedback, fromFeedback, toReduce);
  hlslib::AccumulateReduce<DataPack_t, Operator, kLatency, kSize, kIterations>(
      toReduce, pipeOut);
  Write<DataPack_t, kIterations>(pipeOut, memoryOut);
#endif
}
