/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#include "hlslib/xilinx/Resource.h"
#include <limits>

namespace hlslib {

namespace op {

#ifdef HLSLIB_OPERATOR_ADD_RESOURCE
#define HLSLIB_OPERATOR_ADD_RESOURCE_PRAGMA(var)                                 \
  HLSLIB_RESOURCE_PRAGMA(var, HLSLIB_OPERATOR_ADD_RESOURCE)
#else
#define HLSLIB_OPERATOR_ADD_RESOURCE_PRAGMA(var)
#endif

template <typename T>
struct Sum {
  template <typename T0, typename T1>
  static T Apply(T0 &&a, T1 &&b) {
    #pragma HLS INLINE
    const T res = a + b;
    HLSLIB_OPERATOR_ADD_RESOURCE_PRAGMA(res);
    return res;
  }
  static constexpr T identity() { return 0; }
private:
  Sum() = delete;
  ~Sum() = delete;
};

template <typename T>
using Add = Sum<T>;

#ifdef HLSLIB_OPERATOR_MULT_RESOURCE
#define HLSLIB_OPERATOR_MULT_RESOURCE_PRAGMA(var)                                 \
  HLSLIB_RESOURCE_PRAGMA(var, HLSLIB_OPERATOR_MULT_RESOURCE)
#else
#define HLSLIB_OPERATOR_MULT_RESOURCE_PRAGMA(var)
#endif

template <typename T>
struct Product {
  template <typename T0, typename T1>
  static T Apply(T0 &&a, T1 &&b) {
    #pragma HLS INLINE
    const T res = a * b;
    HLSLIB_OPERATOR_MULT_RESOURCE_PRAGMA(res);
    return res;
  }
  static constexpr T identity() { return 1; }
private:
  Product() = delete;
  ~Product() = delete;
};

template <typename T>
using Multiply = Product<T>;

template <typename T>
struct And {
  template <typename T0, typename T1>
  static T Apply(T0 &&a, T1 &&b) {
    #pragma HLS INLINE
    return a && b;
  }
  static constexpr T identity() { return true; }
private:
  And() = delete;
  ~And() = delete;
};

template <typename T>
struct Min {
  template <typename T0, typename T1>
  static T Apply(T0 &&a, T1 &&b) {
    #pragma HLS INLINE
    return (a < b) ? a : b;
  }
  static constexpr T identity() { return std::numeric_limits<T>::max(); }
private:
  Min() = delete;
  ~Min() = delete;
};

template <typename T>
struct Max {
  template <typename T0, typename T1>
  static T Apply(T0 &&a, T1 &&b) {
    #pragma HLS INLINE
    return (a > b) ? a : b;
  }
  static constexpr T identity() { return std::numeric_limits<T>::min(); }
private:
  Max() = delete;
  ~Max() = delete;
};

// TODO: should this be decoupled from DataPack? It could rely on the index
//       operator and have T be the vector class.
template <class Operator, typename T, int width>
struct Wide {
  template <typename T0, typename T1>
  static T Apply(T0 &&a, T1 &&b) {
    #pragma HLS INLINE
    T res;
    for (int w = 0; w < width; ++w) {
      #pragma HLS UNROLL
      res[w] = Operator::Apply(a[w], b[w]);
    }
    return res;
  }
  static T identity() {
    #pragma HLS INLINE
    T result;
    for (int w = 0; w < width; ++w) {
      #pragma HLS UNROLL
      result[w] = Operator::identity();
    }
    return result;
  }
private:
  Wide() = delete;
  ~Wide() = delete;
};

} // End namespace op

} // End namespace hlslib
