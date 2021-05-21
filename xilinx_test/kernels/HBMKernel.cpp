#define DATA_SIZE = (1024*100)

extern "C" {
    void hbmkernel(
        int *hbm0,
        int *ddr3,
        int *hbm13,
        int *hbm20,
        int *hbm31,
        int *ddr0
    ) {
        for(int i = 0; i < DATA_SIZE; i++) {
            #pragma HLS PIPELINE II=1
            ddr0[i] = hbm0[i] + ddr3[i] + hbm13[i] + hbm20[i] + hbm31[i];
        }
    }
}
