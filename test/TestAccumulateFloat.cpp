/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/Stream.h"
#include "AccumulateFloat.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("TestAccumulateFloat", "[TestAccumulateFloat]") {

  DataPack_t memory(static_cast<Data_t>(1));
  AccumulateFloat(&memory, &memory);
  for (int i = 0; i < kDataWidth; ++i) {
    REQUIRE(memory[i] == kSize); 
  }

}
