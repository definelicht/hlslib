/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "AccumulateCommon.h"
#include "AccumulateFloat.h"
#include "hlslib/xilinx/Accumulate.h"
#include "hlslib/xilinx/Simulation.h"

void AccumulateFloat(DataPack_t const *memoryIn, DataPack_t *memoryOut,
                     int size, int iterations) {

  #pragma HLS INTERFACE m_axi port=memoryIn  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=memoryOut offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=memoryIn  bundle=control
  #pragma HLS INTERFACE s_axilite port=memoryOut bundle=control
  #pragma HLS INTERFACE s_axilite port=size bundle=control
  #pragma HLS INTERFACE s_axilite port=iterations bundle=control
  #pragma HLS INTERFACE s_axilite port=return    bundle=control
  #pragma HLS DATAFLOW

  hlslib::Stream<DataPack_t> pipeIn("pipeIn");
  hlslib::Stream<DataPack_t> pipeOut("pipeOut");
  hlslib::Stream<DataPack_t> toFeedback("fromFeedback");
  hlslib::Stream<DataPack_t> toReduce("toReduce");
  hlslib::Stream<DataPack_t, kLatency> fromFeedback("fromFeedback");

#ifndef HLSLIB_SYNTHESIS
  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read<DataPack_t>, memoryIn, pipeIn,
                           iterations * size);
  HLSLIB_DATAFLOW_FUNCTION(
      hlslib::AccumulateIterate<DataPack_t, Operator, kLatency>, pipeIn,
      fromFeedback, toFeedback, size, iterations);
  HLSLIB_DATAFLOW_FUNCTION(
      hlslib::AccumulateFeedback<DataPack_t, kLatency>,
      toFeedback, fromFeedback, toReduce, size, iterations);
  HLSLIB_DATAFLOW_FUNCTION(
      hlslib::AccumulateReduce<DataPack_t, Operator, kLatency>,
      toReduce, pipeOut, size, iterations);
  HLSLIB_DATAFLOW_FUNCTION(Write<DataPack_t>, pipeOut, memoryOut, iterations);
  HLSLIB_DATAFLOW_FINALIZE();
#else
  Read<DataPack_t>(memoryIn, pipeIn, iterations * size);
  hlslib::AccumulateIterate<DataPack_t, Operator, kLatency>(
      pipeIn, fromFeedback, toFeedback, size, iterations);
  hlslib::AccumulateFeedback<DataPack_t, kLatency>(
      toFeedback, fromFeedback, toReduce, size, iterations);
  hlslib::AccumulateReduce<DataPack_t, Operator, kLatency>(
      toReduce, pipeOut, size, iterations);
  Write<DataPack_t>(pipeOut, memoryOut, iterations);
#endif
}
