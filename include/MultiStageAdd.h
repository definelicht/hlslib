#pragma once

#include "hlslib/Stream.h"
#include <ap_fixed.h>

using Data_t = int;
constexpr int kStages = 8;
constexpr int kNumElements = 1024;

extern "C" {

void MultiStageAdd(Data_t const *memIn, Data_t *memOut);

}
