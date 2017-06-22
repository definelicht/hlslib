/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "AccumulateCommon.h"
#ifdef HLSLIB_COMPILE_ACCUMULATE_FLOAT
#define HLSLIB_ACCUMULATE_KERNEL_NAME AccumulateFloat
#include "AccumulateFloat.h"
#elif defined(HLSLIB_COMPILE_ACCUMULATE_INT)
#define HLSLIB_ACCUMULATE_KERNEL_NAME AccumulateInt
#include "AccumulateInt.h"
#else
#error "Must be compiled for either float or int"
#endif
#include "hlslib/Accumulate.h"
#include "hlslib/Simulation.h"

void ReadAccumulate(DataPack_t const *mapped,
                    hlslib::Stream<DataPack_t> &stream) {
  #pragma HLS INLINE
  Read<DataPack_t, kSize * kIterations>(mapped, stream);
}

void WriteAccumulate(hlslib::Stream<DataPack_t> &stream, DataPack_t *mapped) {
  #pragma HLS INLINE
  Write<DataPack_t, kIterations>(stream, mapped);
}

void HLSLIB_ACCUMULATE_KERNEL_NAME(DataPack_t const *memoryIn,
                                   DataPack_t *memoryOut) {
  #pragma HLS INTERFACE m_axi port=memoryIn  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=memoryOut offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=memoryIn  bundle=control
  #pragma HLS INTERFACE s_axilite port=memoryOut bundle=control
  #pragma HLS INTERFACE s_axilite port=return    bundle=control
  #pragma HLS DATAFLOW
  static hlslib::Stream<DataPack_t> pipeIn("pipeIn");
  static hlslib::Stream<DataPack_t> pipeOut("pipeOut");
  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(ReadAccumulate, memoryIn, pipeIn);
  hlslib::Accumulate<Data_t, kDataWidth, Operator, kLatency, kSize,
                     kIterations>(pipeIn, pipeOut);
  HLSLIB_DATAFLOW_FUNCTION(WriteAccumulate, pipeOut, memoryOut);
  HLSLIB_DATAFLOW_FINALIZE();
}
