/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "AccumulateCommon.h"
#include <random>
#ifdef HLSLIB_COMPILE_ACCUMULATE_FLOAT
#define HLSLIB_ACCUMULATE_KERNEL AccumulateFloat
#include "AccumulateFloat.h"
using Dist_t = std::uniform_real_distribution<Data_t>;
#elif defined(HLSLIB_COMPILE_ACCUMULATE_INT)
#define HLSLIB_ACCUMULATE_KERNEL AccumulateInt
#include "AccumulateInt.h"
using Dist_t = std::uniform_int_distribution<Data_t>;
#else
#error "Must be compiled for either float or int"
#endif
#include "hlslib/xilinx/Stream.h"
#include "hlslib/xilinx/Simulation.h"
#include "catch.hpp"

TEST_CASE(kKernelName, kKernelName) {

  std::vector<DataPack_t> memory(kSize * kIterations);
  std::random_device rd;
  std::default_random_engine re(rd());
  Dist_t dist(1, 10);
  for (int i = 0; i < kSize * kIterations; ++i) {
    for (int w = 0; w < kDataWidth; ++w) {
      memory[i][w] = dist(re);
    }
  }
  std::vector<DataPack_t> result(kSize);
  HLSLIB_ACCUMULATE_KERNEL(memory.data(), result.data(), kSize, kIterations);
  const auto reference =
      NaiveAccumulate<DataPack_t, Operator, kSize, kIterations>(memory);
  for (int i = 0; i < kIterations; ++i) {
    for (int w = 0; w < kDataWidth; ++w) {
      const auto diff = std::abs(result[i][w] - reference[i][w]);
      REQUIRE(diff <= static_cast<Data_t>(1e-3)); 
    }
  }

}
