#include "Jacobi2D.h"

channel float read_stream __attribute__((depth(COLS)));
channel float write_stream __attribute__((depth(COLS)));

__kernel void Read(__global volatile const float memory[]) {
  for (int t = 0; t < TIMESTEPS; ++t) {
    int offset = (t % 2 == 0) ? 0 : ROWS * COLS;
    for (int i = 0; i < ROWS; ++i) {
      for (int j = 0; j < COLS; ++j) {
        DTYPE read = memory[offset + i * COLS + j];
        write_channel_intel(read_stream, read);
      }
    }
  }
}

__kernel void Jacobi2D() {
  for (int t = 0; t < TIMESTEPS; ++t) {
    DTYPE buffer[2 * COLS + 1];
    for (int i = 0; i < ROWS; ++i) {
      for (int j = 0; j < COLS; ++j) {
        // Shift buffer
        #pragma unroll
        for (int b = 0; b < 2 * COLS; ++b) {
          buffer[b] = buffer[b + 1];
        }
        // Read into front
        buffer[2 * COLS] = read_channel_intel(read_stream);
        if (i >= 2 && j >= 1 && j < COLS - 1) {
          DTYPE res = 0.25 * (buffer[2 * COLS] + buffer[COLS - 1] +
                              buffer[COLS + 1] + buffer[0]);
          write_channel_intel(write_stream, res);
        }
      }
    }
  }
}

__kernel void Write(__global volatile float memory[]) {
  for (int t = 0; t < TIMESTEPS; ++t) {
    int offset = (t % 2 == 0) ? ROWS * COLS : 0;
    for (int i = 1; i < ROWS - 1; ++i) {
      for (int j = 1; j < COLS - 1; ++j) {
        DTYPE read = read_channel_intel(write_stream);
        memory[offset + i * COLS + j] = read;
      }
    }
  }
}
