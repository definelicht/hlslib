/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/xilinx/Stream.h"
#include "MultiStageAdd.h"
#include "catch.hpp"

TEST_CASE("MultiStageAdd", "[MultiStageAdd]") {

  Data_t memory[kNumElements];
  std::fill(memory, memory + kNumElements, 0);
  MultiStageAdd(memory, memory);
  for (auto &m : memory) {
    REQUIRE(m == kStages); 
  }

}
