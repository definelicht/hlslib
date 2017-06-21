/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/Stream.h"
#include "AccumulateFloat.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <random>

TEST_CASE("TestAccumulateFloat", "[TestAccumulateFloat]") {

  std::vector<DataPack_t> memory(kSize * kIterations);
  std::random_device rd;
  std::default_random_engine re(rd());
  std::uniform_real_distribution<Data_t> dist(1, 10);
  for (int i = 0; i < kSize * kIterations; ++i) {
    for (int w = 0; w < kDataWidth; ++w) {
      memory[i][w] = dist(re);
    }
  }
  std::vector<DataPack_t> result(kSize);
  AccumulateFloat(memory.data(), result.data());
  const auto reference = NaiveAccumulate(memory);
  for (int i = 0; i < kIterations; ++i) {
    for (int w = 0; w < kDataWidth; ++w) {
      const auto diff = std::abs(result[i][w] - reference[i][w]);
      REQUIRE(diff < 1e-3); 
    }
  }

}
