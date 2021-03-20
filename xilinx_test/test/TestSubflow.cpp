#include "Subflow.h"
#include "catch.hpp"

#include <random>


TEST_CASE("Subflow", "[Subflow]") {
  std::random_device rd;
  std::default_random_engine re(rd());
  std::uniform_real_distribution<Data_t> dist(10, 1);
  std::vector<Data_t> memIn(dataSize), memOut(dataSize);
  for (unsigned i=0; i<dataSize; ++i) {
    memIn[i] = dist(re);
  }
  Subflow(memIn.data(), memOut.data());
  for (unsigned i=0; i<dataSize; ++i) {
    REQUIRE(memOut[i] == (memIn[i]+1) * 2);
  }
}
