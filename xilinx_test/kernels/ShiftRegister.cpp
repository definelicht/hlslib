#include "ShiftRegister.h"
#include "hlslib/xilinx/Simulation.h"
#include "hlslib/xilinx/ShiftRegister.h"
#include "hlslib/xilinx/Stream.h"

void Read(const Data_t memory[], hlslib::Stream<Data_t> &s) {
  for (int t = 0; t < T; ++t) {
    int offset = (t % 2 == 0) ? 0 : H * W;
    for (int i = 0; i < H; ++i) {
      for (int j = 0; j < W; ++j) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_FLATTEN
        Data_t read = memory[offset + i * W + j];
        s.Push(read);
      }
    }
  }
}

void Compute(hlslib::Stream<Data_t> &s_in, hlslib::Stream<Data_t> &s_out) {
  for (int t = 0; t < T; ++t) {
    hlslib::ShiftRegister<Data_t, 0, W - 1, W + 1, 2 * W> sw;
    for (int i = 0; i < H; ++i) {
      for (int j = 0; j < W; ++j) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_FLATTEN
        // Shift buffer
        sw.Shift(s_in.Pop());
        if (i >= 2 && j >= 1 && j < W - 1) {
          Data_t res = 0.25 * (sw.Get(2 * W) + sw.Get<W - 1>() +
                               sw.Get(W + 1) + sw.Get<0>());
          s_out.Push(res);
        }
      }
    }
  }
}

void Write(hlslib::Stream<Data_t> &s, Data_t memory[]) {
  for (int t = 0; t < T; ++t) {
    int offset = (t % 2 == 0) ? H * W : 0;
    for (int i = 1; i < H - 1; ++i) {
      for (int j = 1; j < W - 1; ++j) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_FLATTEN
        const Data_t read = s.Pop();
        memory[offset + i * W + j] = read;
      }
    }
  }
}

extern "C" void ShiftRegister(Data_t const *const memory_in,
                              Data_t *const memory_out) {
  #pragma HLS INTERFACE m_axi port=memory_in  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=memory_out offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=memory_in   bundle=control
  #pragma HLS INTERFACE s_axilite port=memory_out  bundle=control
  #pragma HLS INTERFACE s_axilite port=return     bundle=control
  #pragma HLS DATAFLOW
  hlslib::Stream<Data_t> s_in, s_out;
  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read, memory_in, s_in);
  HLSLIB_DATAFLOW_FUNCTION(Compute, s_in, s_out);
  HLSLIB_DATAFLOW_FUNCTION(Write, s_out, memory_out);
  HLSLIB_DATAFLOW_FINALIZE();
} 
