#pragma once

#include <cstdint>

#include "hlslib/xilinx/Stream.h"

extern "C" void FirstKernel(uint64_t const *memory,
                            hlslib::Stream<uint64_t> &stream, int n);

extern "C" void SecondKernel(hlslib::Stream<uint64_t> &stream, uint64_t *memory,
                             int n);
