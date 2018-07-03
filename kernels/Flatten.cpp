#include "hlslib/Flatten.h"
#include "hlslib/Stream.h"

extern "C" void Flatten(hlslib::Stream<float> &in, hlslib::Stream<float> &out) {
  #pragma HLS INTERFACE axis port=in
  #pragma HLS INTERFACE axis port=out
  #pragma HLS INTERFACE s_axilite port=return bundle=control
  auto loops = hlslib::Flatten(0, 10, 1, 0, 100, 10);
  for (size_t i = 0; i < loops.size(); ++i, ++loops) {
    #pragma HLS PIPELINE II=1
    if (loops[0] >= 5) {
      out.Push(in.Pop() + 1);
    } else {
      out.Push(in.Pop() / 2);
    }
  }
}
