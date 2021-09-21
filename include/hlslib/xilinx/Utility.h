/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include <cmath>
#include <string>
#include <stdexcept>

namespace hlslib {

// Constexpr methods for compile-time computations

constexpr unsigned char ConstLog2(unsigned long val) {
  return val <= 1 ? 0 : 1 + ConstLog2(val >> 1);
}

// Integer division with ceil instead of floor
template <typename T>
constexpr T CeilDivide(T a, T b) {
  return (a + b - 1) / b;
}

template <typename T>
constexpr T &min(T &a, T &b) { return (a < b) ? a : b; } 

template <typename T>
constexpr T const &min(T const &a, const T &b) { return (a < b) ? a : b; } 

template <typename T>
constexpr T &max(T &a, T &b) { return (a > b) ? a : b; } 

template <typename T>
constexpr T const &max(T const &a, const T &b) { return (a > b) ? a : b; }

constexpr signed long abs(const signed long a) { return (a < 0) ? -a : a; }

#ifndef HLSLIB_SYNTHESIS

inline void SetEnvironmentVariable(std::string const &key,
                                   std::string const &val) {
  const auto ret = setenv(key.c_str(), val.c_str(), 1);
  if (ret != 0) {
    throw std::runtime_error("Failed to set environment variable " + key);
  }
}

inline void UnsetEnvironmentVariable(std::string const &key) {
  unsetenv(key.c_str());
}

#endif

} // End namespace hlslib
