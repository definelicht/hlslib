/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      April 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/DataPack.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using Test_t = int;
constexpr int kWidth = 4;
constexpr int kFillVal = 5;
using DataPack = hlslib::DataPack<Test_t, kWidth>;

TEST_CASE("DataPack", "[DataPack]") {

  SECTION("Fill constructor") {
    const DataPack pack(kFillVal);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(pack[i] == kFillVal); 
    }
  }

  SECTION("Copy constructor") {
    const DataPack pack(kFillVal);
    const DataPack copy(pack);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(copy[i] == kFillVal); 
    }
  }

  SECTION("Move constructor") {
    const DataPack pack(kFillVal);
    const DataPack move(std::move(pack));
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(move[i] == kFillVal); 
    }
  }

  SECTION("Array constructor") {
    Test_t arr[kWidth];
    std::fill(arr, arr + kWidth, kFillVal);
    const DataPack pack(arr);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(pack[i] == kFillVal); 
    }
  }

  SECTION("Assignment copy operator") {
    DataPack lhs(0);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs[i] == 0);
    }
    DataPack rhs(kFillVal);
    lhs = rhs;
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs[i] == kFillVal);
    }
  }

  SECTION("Assignment move operator") {
    DataPack lhs(0);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs[i] == 0);
    }
    DataPack rhs(kFillVal);
    lhs = std::move(rhs);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs[i] == kFillVal);
    }
  }

  SECTION("Index-wise assignment") {
    DataPack lhs(0);
    DataPack rhs(kFillVal);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs[i] == 0);
    }
    for (int i = 0; i < kWidth; ++i) {
      lhs[i] = rhs[i];
    }
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs[i] == kFillVal);
    }
  }

  SECTION("Shift operation") {
    DataPack first(kFillVal);
    DataPack second(0);
    first.ShiftTo<0, kWidth/2, kWidth/2>(second);
    for (int i = 0; i < kWidth/2; ++i) {
      REQUIRE(second[i] == 0);
    }
    for (int i = kWidth/2; i < kWidth; ++i) {
      REQUIRE(second[i] == kFillVal);
    }
  }

  SECTION("Pack and unpack") {
    DataPack pack(0);
    Test_t arr0[kWidth];
    Test_t arr1[kWidth];
    std::fill(arr0, arr0 + kWidth, kFillVal);
    std::fill(arr1, arr1 + kWidth, 0);
    pack.Pack(arr0);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(pack[i] == arr0[i]);
    }
    pack.Unpack(arr1);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(arr1[i] == arr0[i]);
    }
    std::fill(arr0, arr0 + kWidth, kFillVal);
    std::fill(arr1, arr1 + kWidth, 0);
    pack << arr0;
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(pack[i] == arr0[i]);
    }
    pack >> arr1;
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(arr1[i] == arr0[i]);
    }
  }

  SECTION("String stream operator") {
    const char arr[5] = {'a', 'b', 'c', 'd', 'e'};
    const hlslib::DataPack<char, 5> pack(arr);
    std::stringstream ss;
    ss << pack;
    REQUIRE(ss.str() == "{a, b, c, d, e}");
  }

}
