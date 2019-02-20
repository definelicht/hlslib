#include "hlslib/xilinx/Flatten.h"
#include "hlslib/xilinx/Stream.h"

void RunConstFlatten(hlslib::Stream<float> &in, hlslib::Stream<float> &out) {
  {
    auto loops = hlslib::ConstFlatten<0, 10, 1, 0, 100, 10>();
    for (size_t i = 0; i < loops.size(); ++i, ++loops) {
      #pragma HLS PIPELINE II=1
      if (loops[0] >= 5) {
        out.Push(in.Pop() + 1);
      } else {
        out.Push(in.Pop() / 2);
      }
    }
  }
  {
    auto loops_const = hlslib::ConstFlatten<-1, 1, 1, -4, -2, 1, -10, 0, 2>();
    for (size_t i = 0; i < loops_const.size(); ++i, ++loops_const) {
      #pragma HLS PIPELINE II=1
      if (loops_const[1] == -2) {
        out.Push(in.Pop() + 1);
      } else {
        out.Push(in.Pop() / 2);
      }
    }
  }
}

void RunFlatten(hlslib::Stream<float> &in, hlslib::Stream<float> &out) {
  {
    auto loops = hlslib::Flatten(0, 10, 1, 0, 100, 10);
    for (int i = 0; i < loops.size(); ++i, ++loops) {
      #pragma HLS PIPELINE II=1
      if (loops[0] >= 5) {
        out.Push(in.Pop() + 1);
      } else {
        out.Push(in.Pop() / 2);
      }
    }
  }
  {
    auto loops_const = hlslib::Flatten(-1, 1, 1, -4, -2, 1, -10, 0, 2);
    for (int i = 0; i < loops_const.size(); ++i, ++loops_const) {
      #pragma HLS PIPELINE II=1
      if (loops_const[1] == -2) {
        out.Push(in.Pop() + 1);
      } else {
        out.Push(in.Pop() / 2);
      }
    }
  }
}

extern "C" void Flatten(hlslib::Stream<float> &in, hlslib::Stream<float> &out) {
  #pragma HLS INTERFACE axis port=in
  #pragma HLS INTERFACE axis port=out
  #pragma HLS INTERFACE s_axilite port=return bundle=control
  RunFlatten(in, out);
  RunConstFlatten(in, out);
}
