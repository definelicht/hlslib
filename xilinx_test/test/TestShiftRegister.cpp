#include "hlslib/xilinx/ShiftRegister.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

constexpr size_t W = 32;
constexpr size_t H = 32;

TEST_CASE("ShiftRegister", "[ShiftRegister]") {

  SECTION("Push and read") {
    hlslib::ShiftRegister<int, 0, W - 1, W + 1, 2 * W> sw;

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
}
