/// @author    Jannis Widmer (widmerja@ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#define DATA_SIZE 1024

extern "C" {
void DDRExplicit(int *ddr0, int *ddr1) {
  for (int i = 0; i < DATA_SIZE; i++) {
#pragma HLS PIPELINE II = 1
    ddr0[i] = ddr1[i];
  }
}
}
