/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/xilinx/Simulation.h"
#include "catch.hpp"

void PassByValueAndReference(int value, std::vector<int> &reference) {
  reference[value] = value;
}

TEST_CASE("Forwarding", "[Forwarding]") {

  SECTION("Fill constructor") {
    HLSLIB_DATAFLOW_INIT();
    std::vector<int> vec(32, 0);
    for (int i = 0; i < 32; ++i) {
      HLSLIB_DATAFLOW_FUNCTION(PassByValueAndReference, i, vec); 
    }
    HLSLIB_DATAFLOW_FINALIZE();
    for (int i = 0; i < 32; ++i) {
      REQUIRE(vec[i] == i);
    }
  }

}
