/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/xilinx/Flatten.h"
#include "catch.hpp"

TEST_CASE("Flatten") {

  SECTION("Two loops") {

    auto nest = hlslib::Flatten(0, 10, 1, 0, 20, 2);

    for (size_t i = 0; i < nest.size(); ++i, ++nest) {
      REQUIRE(nest[0] == i / (20 / 2));
      REQUIRE(nest[1] == (2 * i) % 20);
    }
    
  }
  
  SECTION("Negative bounds") {
  
    auto nest = hlslib::Flatten(-1, 1, 1, -4, -2, 1, -10, 0, 2);

    for (int i = 0; i < nest.size(); ++i, ++nest) {
      REQUIRE(nest[0] == -1 + i / 10);
      REQUIRE(nest[1] == -4 + (i / 5) % 2);
      REQUIRE(nest[2] == -10 + 2 * (i % 5));
    }

  }

}

TEST_CASE("ConstFlatten") {

  SECTION("Two loops") {
  
    auto nest = hlslib::ConstFlatten<0, 10, 1, 0, 20, 2>();

    for (size_t i = 0; i < nest.size(); ++i, ++nest) {
      REQUIRE(nest[0] == i / (20 / 2));
      REQUIRE(nest[1] == (2 * i) % 20);
      REQUIRE(nest.template get<0>() == i / (20 / 2));
      REQUIRE(nest.template get<1>() == (2 * i) % 20);
    }

  }
  
  SECTION("Negative bounds") {
  
    auto nest = hlslib::ConstFlatten<-1, 1, 1, -4, -2, 1, -10, 0, 2>();

    for (int i = 0; i < nest.size(); ++i, ++nest) {
      REQUIRE(nest[0] == -1 + i / 10);
      REQUIRE(nest[1] == -4 + (i / 5) % 2);
      REQUIRE(nest[2] == -10 + 2 * (i % 5));
    }

  }

}
