#include <hlslib/xilinx/DataPack.h>

void TightPacking(hlslib::DataPack<ap_fixed<4, 2>, 32> const *mem_in,
                  hlslib::DataPack<ap_int<4>, 32> *mem_out, int n) {
  for (int i = 0; i < n; ++i) {
    #pragma HLS PIPELINE
    for (int w = 0; w < 32; ++w) {
      #pragma HLS UNROLL
      mem_out[i][w] = mem_in[i][w];
    }
  }
}
