/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      July 2018
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/Flatten.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"


TEST_CASE("Flatten", "[Flatten]") {

  SECTION("Iteration space") {

    auto nest = hlslib::Flatten(0, 10, 1, 0, 20, 2); 

    for (size_t i = 0; i < nest.size(); ++i, ++nest) {
      REQUIRE(nest[0] == i / (20 / 2));
      REQUIRE(nest[1] == (2 * i) % 20);
    }

  }

}
