/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/xilinx/DataPack.h"
#include "hlslib/xilinx/Operators.h"
#include "hlslib/xilinx/TreeReduce.h"
#include "catch.hpp"

TEST_CASE("TreeReduce", "[TreeReduce]") {

  SECTION("Add numbers") {
    int arr[] = {5, 50, 500, 5000};
    int sum = hlslib::TreeReduce<int, hlslib::op::Add<int>, 4>(arr);
    REQUIRE(sum == 5555);
  }

  SECTION("Multiply numbers") {
    float arr[] = {1, 2, 3, 4, 5};
    float prod = hlslib::TreeReduce<float, hlslib::op::Multiply<float>, 5>(arr);
    REQUIRE(prod == 120);
  }

  SECTION("Logical and") {
    {
      bool arr[] = {true, true, true, true, true, true, false};
      bool prod = hlslib::TreeReduce<bool, hlslib::op::And<bool>, 7>(arr);
      REQUIRE(!prod);
    }
    {
      bool arr[] = {true, true, true, true, true, true, true};
      bool prod = hlslib::TreeReduce<bool, hlslib::op::And<bool>, 7>(arr);
      REQUIRE(prod);
    }
  }

  SECTION("DataPack") {
    int arr[] = {5, 50, 500, 5000};
    hlslib::DataPack<int, 4> pack(arr);
    int sum = hlslib::TreeReduce<int, hlslib::op::Add<int>, 4>(pack);
    REQUIRE(sum == 5555);
  }

}
