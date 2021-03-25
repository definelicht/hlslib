/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "AccumulateCommon.h"
#include "AccumulateInt.h"
#include "hlslib/xilinx/Accumulate.h"
#include "hlslib/xilinx/Simulation.h"

void AccumulateInt(DataPack_t const *memoryIn, DataPack_t *memoryOut, int size,
                   int iterations) {
  #pragma HLS INTERFACE m_axi port=memoryIn  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=memoryOut offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=memoryIn   bundle=control
  #pragma HLS INTERFACE s_axilite port=memoryOut  bundle=control
  #pragma HLS INTERFACE s_axilite port=size       bundle=control
  #pragma HLS INTERFACE s_axilite port=iterations bundle=control
  #pragma HLS INTERFACE s_axilite port=return     bundle=control
  #pragma HLS DATAFLOW
  static hlslib::Stream<DataPack_t> pipeIn("pipeIn");
  static hlslib::Stream<DataPack_t> pipeOut("pipeOut");
  // The preprocessor macro used in HLSLIB_DATAFLOW_FUNCTION doesn't support
  // functions with multiple template arguments
#ifndef HLSLIB_SYNTHESIS
  HLSLIB_DATAFLOW_INIT();
  __hlslib_dataflow_context.AddFunction(Read<DataPack_t>, memoryIn, pipeIn,
                                        iterations * size);
  __hlslib_dataflow_context.AddFunction(
      hlslib::AccumulateSimple<DataPack_t, Operator>, pipeIn, pipeOut,
      iterations, size);
  __hlslib_dataflow_context.AddFunction(Write<DataPack_t>, pipeOut, memoryOut,
                                        iterations);
  HLSLIB_DATAFLOW_FINALIZE();
#else
  Read<DataPack_t>(memoryIn, pipeIn, iterations * size);
  hlslib::AccumulateSimple<DataPack_t, Operator>(pipeIn, pipeOut, size,
                                                 iterations);
  Write<DataPack_t>(pipeOut, memoryOut, iterations);
#endif
}
