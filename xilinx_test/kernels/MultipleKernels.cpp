#include "MultipleKernels.h"

#include "hlslib/xilinx/Stream.h"

void FirstKernel(uint64_t const *memory, hlslib::Stream<uint64_t> &stream, int n) {
  #pragma HLS INTERFACE m_axi port=memory offset=slave bundle=gmem0
  #pragma HLS INTERFACE axis port=stream
  for (int i = 0; i < n; ++i) {
    #pragma HLS PIPELINE II=1
    stream.Push(memory[i] + 1);
  }
}

void SecondKernel(hlslib::Stream<uint64_t> &stream, uint64_t *memory, int n) {
  #pragma HLS INTERFACE axis port=stream
  #pragma HLS INTERFACE m_axi port=memory offset=slave bundle=gmem1
  for (int i = 0; i < n; ++i) {
    #pragma HLS PIPELINE II=1
    memory[i] = 2 * stream.Pop();
  }
}
