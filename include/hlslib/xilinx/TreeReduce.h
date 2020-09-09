/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

// This header implementations reduction using a binary tree, allowing a fully
// pipelined reduction of a static sized array.
// Using the reduction requires passing an operator that implements the
// functions "Reduce" and "identity", as in the examples given below.

namespace hlslib {

namespace { // Internals

template <typename T, class Operator, int width>
struct TreeReduceImplementation {
  template <typename RandomAccessType>
  static T f(RandomAccessType const &arr) {
    #pragma HLS INLINE
    static constexpr int halfWidth = width / 2;
    static constexpr int reducedSize = halfWidth + width % 2;
    T reduced[reducedSize];
    #pragma HLS ARRAY_PARTITION variable=reduced complete
  Reduction:
    for (int i = 0; i < halfWidth; ++i) {
      #pragma HLS UNROLL
      reduced[i] = Operator::Apply(arr[i * 2], arr[i * 2 + 1]);
    }
    if (halfWidth != reducedSize) {
      reduced[reducedSize - 1] = arr[width - 1];
    }
    return TreeReduceImplementation<T, Operator, reducedSize>::f(reduced);
  }
private:
  TreeReduceImplementation() = delete;
  ~TreeReduceImplementation() = delete;
};

template <typename T, class Operator>
struct TreeReduceImplementation<T, Operator, 2> {
  template <typename RandomAccessType>
  static T f(RandomAccessType const &arr) {
    #pragma HLS INLINE
    return Operator::Apply(arr[0], arr[1]);
  }
private:
  TreeReduceImplementation() = delete;
  ~TreeReduceImplementation() = delete;
};

template <typename T, class Operator>
struct TreeReduceImplementation<T, Operator, 1> {
  template <typename RandomAccessType>
  static T f(RandomAccessType const &arr) {
    #pragma HLS INLINE
    return arr[0];
  }
private:
  TreeReduceImplementation() = delete;
  ~TreeReduceImplementation() = delete;
};

template <typename T, class Operator>
struct TreeReduceImplementation<T, Operator, 0> {
  template <typename RandomAccessType>
  static constexpr T f(RandomAccessType const &) { 
    return Operator::identity();
  }
private:
  TreeReduceImplementation() = delete;
  ~TreeReduceImplementation() = delete;
};

} // End anonymous namespace

/// Reduction entry function.
template <typename T, class Operator, int width, typename RandomAccessType>
T TreeReduce(RandomAccessType const &arr) {
  #pragma HLS INLINE 
  return TreeReduceImplementation<T, Operator, width>::f(arr);
}

} // End namespace hlslib
