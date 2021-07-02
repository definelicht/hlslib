/// @author    Jannis Widmer (widmerja@ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

constexpr int kDataSize = 1024;

extern "C" {
void HBMandBlockCopy(int *hbm0, int *ddr1, int *ddr0, int *hbm20, int *hbm31,
                     int *ddrX) {
  for (int i = 0; i < kDataSize; i++) {
    #pragma HLS PIPELINE II=1
    ddrX[i] = hbm0[i] + ddr1[i] + ddr0[i] + hbm20[i] + hbm31[i];
  }
}
}
