/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      July 2018
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#ifndef HLSLIB_SYNTHESIS
#include <stdexcept>
#endif
#include <iostream>

namespace hlslib {

template <int i, typename T>
struct Increment {
  static void Apply(T *index, T const *ranges) {
    #pragma HLS INLINE
    if (index[i] == ranges[i * 3 + 1] - ranges[i * 3 + 2]) {
      index[i] = ranges[i * 3];
      Increment<i - 1, T>::Apply(index, ranges);
    } else {
      index[i] += ranges[i * 3 + 2];
    }
  }
};

template <typename T>
struct Increment<-1, T> {
  static void Apply(T *, T const*) {}
};

template <int i, typename T>
struct Size {
  static T Apply(T const *ranges) {
    #pragma HLS INLINE
    return Size<i - 1, T>::Apply(ranges) *
           ((ranges[i * 3 + 1] - ranges[i * 3] + ranges[i * 3 + 2] - 1) /
            ranges[i * 3 + 2]);
  }
};

template <typename T>
struct Size<-1, T> {
  static T Apply(T const*) { return 1; }
};

template <int i, typename T>
struct Done {
  static bool Apply(T const *index, T const *ranges) {
    #pragma HLS INLINE
    return index[i] == ranges[i * 3] && Done<i - 1, T>::Apply(index, ranges);
  }
};

template <typename T>
struct Done<-1, T> {
  static bool Apply(T const*, T const*) { return true; }
};

template <class... ArgRanges>
class FlattenImpl {

  static constexpr size_t kDims = sizeof...(ArgRanges) / 3;

 public:
  FlattenImpl(ArgRanges &&... ranges) : ranges_{ranges...} {
    #pragma HLS INLINE
    for (int i = 0; i < kDims; ++i) {
      #pragma HLS UNROLL
      i_[i] = ranges_[3 * i];
    }
  }

  int operator[](int i) {
    #pragma HLS INLINE    
    return i_[i];
  }

  void operator++() {
    #pragma HLS INLINE
    Increment<kDims - 1, int>::Apply(i_, ranges_);
  }

  void operator++(int) {
    #pragma HLS INLINE
    ++(*this);
  }

  size_t size() const {
    #pragma HLS INLINE
    return Size<kDims - 1, int>::Apply(ranges_);
  }

  bool done() const {
    #pragma HLS INLINE
    return Done<kDims - 1, int>::Apply(i_, ranges_);
  }

private:
  int i_[kDims];
  const int ranges_[3 * kDims];
};

template <class... ArgRanges>
FlattenImpl<ArgRanges...> Flatten(ArgRanges &&... ranges) {
  return FlattenImpl<ArgRanges...>(std::forward<ArgRanges>(ranges)...);
}

}  // End namespace hlslib
