#include "MultiStageAdd.h"
#include "hlslib/Simulation.h"

void AxiToStream(Data_t const *memory, hlslib::Stream<Data_t> &stream) {
AxiToStream:
  for (int i = 0; i < kNumElements; ++i) {
    #pragma HLS PIPELINE
    hlslib::WriteOptimistic(stream, memory[i], 1);
  }
}

void AddStage(hlslib::Stream<Data_t> &streamIn,
              hlslib::Stream<Data_t> &streamOut) {
AddStage:
  for (int i = 0; i < kNumElements; ++i) {
    #pragma HLS PIPELINE
    const Data_t read = hlslib::ReadOptimistic(streamIn);
    const Data_t eval = read + 1;
    hlslib::WriteOptimistic(streamOut, eval, 1);
  }
}

void StreamToAxi(hlslib::Stream<Data_t> &stream, Data_t *memory) {
StreamToAxi:
  for (int i = 0; i < kNumElements; ++i) {
    #pragma HLS PIPELINE
    memory[i] = hlslib::ReadOptimistic(stream);
  }
}

void MultiStageAdd(Data_t const *memoryIn, Data_t *memoryOut) {
  #pragma HLS INTERFACE m_axi port=memoryIn  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=memoryOut offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=memoryIn  bundle=control
  #pragma HLS INTERFACE s_axilite port=memoryOut bundle=control
  #pragma HLS INTERFACE s_axilite port=return    bundle=control
  #pragma HLS DATAFLOW
  HLSLIB_DATAFLOW_INIT();
  hlslib::Stream<Data_t> pipes[kStages + 1];
  HLSLIB_DATAFLOW_FUNCTION(AxiToStream, memoryIn, pipes[0]);
MultiAddStages:
  for (int i = 0; i < kStages; ++i) {
    #pragma HLS UNROLL
    HLSLIB_DATAFLOW_FUNCTION(AddStage, pipes[i], pipes[i + 1]);
  }
  HLSLIB_DATAFLOW_FUNCTION(StreamToAxi, pipes[kStages], memoryOut);
  HLSLIB_DATAFLOW_FINALIZE();
}
