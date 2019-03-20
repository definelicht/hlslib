#include "Jacobi2D.h"

channel float read_stream __attribute__((depth(W)));
channel float write_stream __attribute__((depth(W)));

__kernel void Read(__global volatile const float memory[]) {
  for (int t = 0; t < T; ++t) {
    int offset = (t % 2 == 0) ? 0 : H * W;
    for (int i = 0; i < H; ++i) {
      for (int j = 0; j < W; ++j) {
        DTYPE read = memory[offset + i*W + j];
        write_channel_intel(read_stream, read);
      }
    }
  }
}

__kernel void Jacobi2D() {
  for (int t = 0; t < T; ++t) {
    DTYPE buffer[2*W + 1];
    for (int i = 0; i < H; ++i) {
      for (int j = 0; j < W; ++j) {
        // Shift buffer
        #pragma unroll
        for (int b = 0; b < 2*W; ++b) {
          buffer[b] = buffer[b + 1];
        }
        // Read into front
        buffer[2*W] = read_channel_intel(read_stream);
        if (i >= 2 && j >= 1 && j < W - 1) {
          DTYPE res = 0.25 * (buffer[2*W] + buffer[W - 1] +
                              buffer[W + 1] + buffer[0]);
          write_channel_intel(write_stream, res);
        }
      }
    }
  }
}

__kernel void Write(__global volatile float memory[]) {
  for (int t = 0; t < T; ++t) {
    int offset = (t % 2 == 0) ? H * W : 0;
    for (int i = 1; i < H - 1; ++i) {
      for (int j = 1; j < W - 1; ++j) {
        DTYPE read = read_channel_intel(write_stream);
        memory[offset + i*W + j] = read;
      }
    }
  }
}
