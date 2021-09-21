/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#ifndef HLSLIB_SYNTHESIS
#include <stdexcept>
#endif
#include <cstddef>
#include <ap_int.h>
#include "hlslib/xilinx/Utility.h"

namespace hlslib {

namespace {

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
    return (index[i] >= ranges[i * 3 + 1] - ranges[i * 3 + 2]) &&
           Done<i - 1, T>::Apply(index, ranges);
  }
};

template <typename T>
struct Done<-1, T> {
  static bool Apply(T const*, T const*) { return true; }
};

template <class... Ranges>
struct FlattenImpl {

  static constexpr int kDims = sizeof...(Ranges) / 3;

 public:
  FlattenImpl(Ranges &&... ranges) : ranges_{ranges...} {
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

  int size() const {
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

} // End anonymous namespace

template <class... Ranges>
FlattenImpl<Ranges...> Flatten(Ranges &&... ranges) {
  return FlattenImpl<Ranges...>(std::forward<Ranges>(ranges)...);
}

namespace {

template <signed long begin, signed long end>
struct BitsToRepresent {
  static constexpr unsigned kBits =
      1 +
      ((begin < 0 || end < 0)
           ? (ConstLog2(hlslib::max(hlslib::abs(begin), hlslib::abs(end))) + 1)
           : ConstLog2(hlslib::max(begin, end)));
  using type = typename std::conditional<(begin < 0 || end < 0), ap_int<kBits>,
                                         ap_uint<kBits>>::type;
};

template <int... ranges>
struct ConstFlattenImpl;

template <int dimIndex, int rangeBegin, int rangeEnd, int rangeStep>
struct ConstFlattenImpl<dimIndex, rangeBegin, rangeEnd, rangeStep> {

public:

  static constexpr int kSize =
      (rangeEnd - rangeBegin + rangeStep - 1) / rangeStep;

  int operator[](int dim) {
    #pragma HLS INLINE
#ifndef HLSLIB_SYNTHESIS 
    if (dim != dimIndex) {
      throw std::runtime_error("Index out of bounds");
    }
#endif
    return i_;
  }

  template <int>
  int get() {
    #pragma HLS INLINE
    return i_;
  }

  bool Increment() {
    #pragma HLS INLINE
    if (i_ >= rangeEnd - rangeStep) {
      i_ = rangeBegin;
      return true;
    } else {
      i_ += rangeStep;
    }
    return false;  // Whether we wrapped over the end
  }

  bool last() const {
    #pragma HLS INLINE
    return i_ >= rangeEnd - rangeStep;
  }

private:
  typename BitsToRepresent<rangeBegin, rangeEnd>::type i_{rangeBegin};
};

template <int dimIndex, int rangeBegin, int rangeEnd, int rangeStep,
          int... ranges>
struct ConstFlattenImpl<dimIndex, rangeBegin, rangeEnd, rangeStep,
                       ranges...> {

  static constexpr int kSize =
      ConstFlattenImpl<dimIndex, ranges...>::kSize *
      ConstFlattenImpl<dimIndex + 1, rangeBegin, rangeEnd, rangeStep>::kSize;

  int operator[](int i) {
    #pragma HLS INLINE
    if (i == dimIndex) {
      return i_;
    } else {
      return next_[i];
    }
  }

  template <int dim>
  int get() {
    #pragma HLS INLINE
    if (dim == dimIndex) {
      return i_;
    } else {
      return next_.template get<dim>();
    }
  }

  bool Increment() {
    #pragma HLS INLINE
    if (next_.Increment()) {
      if (i_ >= rangeEnd - rangeStep) {
        i_ = rangeBegin;
        return true;
      } else {
        i_ += rangeStep;
      }
    }
    return false; // Whether we wrapped over the end
  }

  bool last() const {
    return (i_ >= rangeEnd - rangeStep) && next_.last();
  }

private:
  ConstFlattenImpl<dimIndex + 1, ranges...> next_{};
  typename BitsToRepresent<rangeBegin, rangeEnd>::type i_{rangeBegin};
};

}

template <int... ranges>
class ConstFlatten {

public:
  static constexpr int kDims = sizeof...(ranges) / 3;
  static constexpr int kSize = ConstFlattenImpl<0, ranges...>::kSize;

  int operator[](int i) {
    #pragma HLS INLINE
    return impl_[i];
  }

  template <int dim>
  int get() {
    #pragma HLS INLINE
    static_assert(dim >= 0 && dim < kDims, "Invalid dimension specified.");
    return impl_.template get<dim>();
  }

  void operator++() {
    #pragma HLS INLINE
    impl_.Increment();
  }

  void operator++(int) {
    #pragma HLS INLINE
    impl_.Increment();
  }

  constexpr int size() const {
    return kSize;
  }

private:
  ConstFlattenImpl<0, ranges...> impl_{};
};

}  // End namespace hlslib
