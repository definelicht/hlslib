/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "MultiStageAdd.h"
#include "hlslib/xilinx/Simulation.h"

void MemoryToStream(Data_t const *memory, hlslib::Stream<Data_t> &stream) {
MemoryToStream:
  for (int i = 0; i < kNumElements; ++i) {
    #pragma HLS PIPELINE II=1
    stream.Push(memory[i]);
  }
}

void AddStage(hlslib::Stream<Data_t> &streamIn,
              hlslib::Stream<Data_t> &streamOut) {
AddStage:
  for (int i = 0; i < kNumElements; ++i) {
    #pragma HLS PIPELINE II=1
    const Data_t read = streamIn.Pop();
    const Data_t eval = read + 1;
    streamOut.Push(eval);
  }
}

void StreamToMemory(hlslib::Stream<Data_t> &stream, Data_t *memory) {
StreamToMemory:
  for (int i = 0; i < kNumElements; ++i) {
    #pragma HLS PIPELINE
    memory[i] = stream.Pop();
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
  HLSLIB_DATAFLOW_FUNCTION(MemoryToStream, memoryIn, pipes[0]);
MultiAddStages:
  for (int i = 0; i < kStages; ++i) {
    #pragma HLS UNROLL
    HLSLIB_DATAFLOW_FUNCTION(AddStage, pipes[i], pipes[i + 1]);
  }
  HLSLIB_DATAFLOW_FUNCTION(StreamToMemory, pipes[kStages], memoryOut);
  HLSLIB_DATAFLOW_FINALIZE();
}
