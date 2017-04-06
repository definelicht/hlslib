#include "hlslib/Stream.h"
#include "MultiStageAdd.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("MultiStageAdd", "[MultiStageAdd]") {

  Data_t memory[kNumElements];
  std::fill(memory, memory + kNumElements, 0);
  MultiStageAdd(memory, memory);
  for (auto &m : memory) {
    REQUIRE(m == kStages); 
  }

}
