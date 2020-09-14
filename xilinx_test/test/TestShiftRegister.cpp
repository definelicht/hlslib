#include "hlslib/xilinx/ShiftRegister.h"
#include "catch.hpp"

constexpr size_t W = 32;
constexpr size_t H = 32;

TEST_CASE("ShiftRegister") {

    hlslib::ShiftRegister<int, 0, W - 1, W + 1, 2 * W> sw;

  SECTION("Template index") {
    size_t i = 0;

    for (; i < 2 * W; ++i) {
      sw.Shift(i);
    }

    for (; i < W * H; ++i) {
      sw.Shift(i);
      REQUIRE(sw.Get<0>() == i - 2 * W); 
      REQUIRE(sw.Get<W - 1>() == i - (W + 1)); 
      REQUIRE(sw.Get<W + 1>() == i - (W - 1)); 
      REQUIRE(sw.Get<2 * W>() == i); 
    }
  }

  SECTION("Argument index") {
    size_t i = 0;

    for (; i < 2 * W; ++i) {
      sw.Shift(i);
    }

    for (; i < W * H; ++i) {
      sw.Shift(i);
      REQUIRE(sw.Get(0) == i - 2 * W); 
      REQUIRE(sw.Get(W - 1) == i - (W + 1)); 
      REQUIRE(sw.Get(W + 1) == i - (W - 1)); 
      REQUIRE(sw.Get(2 * W) == i); 
    }
  }
}
