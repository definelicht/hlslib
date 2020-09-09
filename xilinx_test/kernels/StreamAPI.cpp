#include "hlslib/xilinx/Stream.h"

void StreamAPI(hlslib::Stream<int> &in, hlslib::Stream<int> &out) {
  #pragma HLS INTERFACE axis port=in
  #pragma HLS INTERFACE axis port=out
  while (true) {
    #pragma HLS PIPELINE II=1
    if (!in.IsEmpty() && !out.IsFull()) {
      out.Push(in.Pop()); 
    }
  }
}

int main() {
  hlslib::Stream<int, 10> strm;
  bool empty = strm.IsEmpty();
}
