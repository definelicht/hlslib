/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include <type_traits>

#include "ap_fixed.h"
#include "ap_int.h"
#include "catch.hpp"
#include "hlslib/xilinx/DataPack.h"

constexpr int kWidth = 4;

static_assert(hlslib::DataPack<ap_int<5>, 3>::kBits == 5, "Invalid bit size.");
static_assert(std::is_same<hlslib::DataPack<ap_int<5>, 3>::Internal_t,
                           ap_uint<15>>::value,
              "Invalid internal type.");
static_assert(hlslib::DataPack<ap_fixed<9, 7>, 8>::kBits == 9,
              "Invalid bit size.");
static_assert(std::is_same<hlslib::DataPack<ap_fixed<9, 7>, 8>::Internal_t,
                           ap_uint<72>>::value,
              "Invalid internal type.");
static_assert(hlslib::DataPack<ap_ufixed<23, 5>, 8>::kBits == 23,
              "Invalid bit size.");
static_assert(std::is_same<hlslib::DataPack<ap_ufixed<23, 5>, 8>::Internal_t,
                           ap_uint<184>>::value,
              "Invalid internal type.");

TEMPLATE_TEST_CASE("DataPack", "[DataPack][template]", int, ap_int<5>,
                   ap_uint<33>, (ap_fixed<9, 4>), (ap_ufixed<19, 5>)) {
  const TestType kFillVal = 5;
  using DataPack = hlslib::DataPack<TestType, kWidth>;

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
    TestType arr[kWidth];
    std::fill(arr, arr + kWidth, kFillVal);
    const DataPack pack(arr);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(pack[i] == kFillVal);
    }
  }

  SECTION("Assignment copy operator") {
    DataPack lhs(TestType(0));
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs.Get(i) == 0);
    }
    DataPack rhs(kFillVal);
    lhs = rhs;
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs.Get(i) == kFillVal);
    }
  }

  SECTION("Assignment move operator") {
    DataPack lhs(TestType(0));
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs.Get(i) == 0);
    }
    DataPack rhs(kFillVal);
    lhs = std::move(rhs);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs.Get(i) == kFillVal);
    }
  }

  SECTION("Index-wise assignment") {
    DataPack lhs(TestType(0));
    DataPack rhs(kFillVal);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs.Get(i) == 0);
    }
    for (int i = 0; i < kWidth; ++i) {
      lhs[i] = rhs[i];
    }
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(lhs.Get(i) == kFillVal);
    }
  }

  SECTION("Shift operation") {
    DataPack first(kFillVal);
    DataPack second(TestType(0));
    first.template ShiftTo<0, kWidth / 2, kWidth / 2>(second);
    for (int i = 0; i < kWidth / 2; ++i) {
      REQUIRE(second.Get(i) == 0);
    }
    for (int i = kWidth / 2; i < kWidth; ++i) {
      REQUIRE(second.Get(i) == kFillVal);
    }
  }

  SECTION("Pack and unpack") {
    DataPack pack(TestType(0));
    TestType arr0[kWidth];
    TestType arr1[kWidth];
    std::fill(arr0, arr0 + kWidth, kFillVal);
    std::fill(arr1, arr1 + kWidth, 0);
    pack.Pack(arr0);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(pack.Get(i) == arr0[i]);
    }
    pack.Unpack(arr1);
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(arr1[i] == arr0[i]);
    }
    std::fill(arr0, arr0 + kWidth, kFillVal);
    std::fill(arr1, arr1 + kWidth, 0);
    pack << arr0;
    for (int i = 0; i < kWidth; ++i) {
      REQUIRE(pack.Get(i) == arr0[i]);
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
