#include <random>

#include "Subflow.h"
#include "catch.hpp"

TEST_CASE("Subflow", "[Subflow]") {
  std::random_device rd;
  std::default_random_engine re(rd());
  std::uniform_int_distribution<Data_t> dist(10, 1);
  std::vector<Data_t> memIn(kSize), memOut(kSize);
  for (unsigned i = 0; i < kSize; ++i) {
    memIn[i] = dist(re);
  }
  Subflow(memIn.data(), memOut.data());
  for (unsigned i = 0; i < kSize; ++i) {
    REQUIRE(memOut[i] == (memIn[i] + 1) * 2);
  }
}
