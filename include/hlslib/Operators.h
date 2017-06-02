/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#include <limits>

namespace hlslib {

template <typename T>
struct Add {
  static T Apply(T const &a, T const &b) { return a + b; }
  static constexpr T identity() { return 0; }
private:
  Add() = delete;
  ~Add() = delete;
};

template <typename T>
struct Multiply {
  static T Apply(T const &a, T const &b) { return a * b; }
  static constexpr T identity() { return 1; }
private:
  Multiply() = delete;
  ~Multiply() = delete;
};

template <typename T>
struct And {
  static T Apply(T const &a, T const &b) { return a && b; }
  static constexpr T identity() { return true; }
private:
  And() = delete;
  ~And() = delete;
};

template <typename T>
struct Min {
  static T Apply(T const &a, T const &b) { return (a < b) ? a : b; }
  static constexpr T identity() { return std::numeric_limits<T>::max(); }
private:
  Min() = delete;
  ~Min() = delete;
};

template <typename T>
struct Max {
  static T Apply(T const &a, T const &b) { return (a > b) ? a : b; }
  static constexpr T identity() { return std::numeric_limits<T>::min(); }
private:
  Max() = delete;
  ~Max() = delete;
};

} // End namespace hlslib
