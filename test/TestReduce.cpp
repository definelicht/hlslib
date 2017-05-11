/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      April 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/TreeReduce.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("TreeReduce", "[TreeReduce]") {

  SECTION("Add numbers") {
    int arr[] = {5, 50, 500, 5000};
    int sum = hlslib::Reduce<int, hlslib::Add<int>, 4>(arr);
    REQUIRE(sum == 5555);
  }

  SECTION("Multiply numbers") {
    float arr[] = {1, 2, 3, 4, 5};
    float prod = hlslib::Reduce<float, hlslib::Multiply<float>, 5>(arr);
    REQUIRE(prod == 120);
  }

  SECTION("Logical and") {
    bool arr[] = {true, true, true, true, true, true, false};
    bool prod = hlslib::Reduce<bool, hlslib::And<bool>, 7>(arr);
    REQUIRE(!prod);
  }

}
