/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#pragma once

#include <ap_int.h>
#include <algorithm>
#ifndef HLSLIB_SYNTHESIS
#include <cstddef> // std::runtime_error
#endif
#include <stdexcept>
#include "hlslib/xilinx/Utility.h"

namespace hlslib {

namespace {

template <typename T, size_t IDesired, size_t IThis, typename TNext>
T _Get(T const &elem,
       typename std::enable_if<IDesired == IThis, TNext>::type const &) {
  #pragma HLS INLINE
  return elem;
}

template <typename T, size_t IDesired, size_t IThis, typename TNext>
T _Get(T const &,
       typename std::enable_if<IDesired != IThis, TNext>::type const &next) {
  #pragma HLS INLINE
  return next.template Get<IDesired>();
}

template <typename T>
constexpr T _ConstMax(T const &a, T const &b) {
  return (a < b) ? b : a;
}

template <size_t... Is>
struct _MaxImpl {
  static constexpr size_t Max() { return 0; }
};

template <size_t I, size_t... Is>
struct _MaxImpl<I, Is...> {
  static constexpr size_t Max() { return _ConstMax(_MaxImpl<Is...>::Max(), I); }
};

}  // End anonymous namespace

template <typename T, size_t Size>
class _ShiftRegisterStageImpl {
  using Index_t = ap_uint<hlslib::ConstLog2(Size) + 1>;

 public:
  T Shift(T const &next) {
    #pragma HLS INLINE
    #pragma HLS DEPENDENCE variable=buffer_ false
    const auto evicted = buffer_[index_];
    buffer_[index_] = next;
    newest_ = next;
    index_ = (index_ == Size - 1) ? Index_t(0) : Index_t(index_ + 1);
    return evicted;
  }

  T Get() const {
    #pragma HLS INLINE
    return newest_;
  }

 private:
  Index_t index_{1};
  T newest_;
  T buffer_[Size];
};

template <typename T>
class _ShiftRegisterStageImpl<T, 1> {
 public:
  T Shift(T const &next) {
    #pragma HLS INLINE
    #pragma HLS DEPENDENCE variable=buffer_ false
    const auto evicted = buffer_;
    buffer_ = next;
    return evicted;
  }

  T Get() const {
    #pragma HLS INLINE
    #pragma HLS DEPENDENCE variable=buffer_ false
    return buffer_;
  }

 private:
  T buffer_;
};

template <typename T, int... Is>
class _ShiftRegisterStage {
 public:
  T Shift(T const &elem) {
    #pragma HLS INLINE
    return elem;
  }
  template <int I>
  T Get() const {
    static_assert(I != I, "Invalid tap index specified.");
  }
  T Get(size_t) const {
#ifndef HLSLIB_SYNTHESIS
    throw std::runtime_error("Accessed invalid index of shift register.");
#endif
    return T();
  }
};

template <typename T, int IPrev, int IThis, int... Is>
class _ShiftRegisterStage<T, IPrev, IThis, Is...> {
  static_assert(IThis >= 0, "Received negative sliding window offset.");
  static_assert(IPrev < IThis,
                "Tap indices must be given in increasing order.");
  static constexpr size_t kSize = IThis - IPrev;

 public:
  T Shift(T const &elem) {
    #pragma HLS INLINE
    const auto next = next_stage_.Shift(elem);
    const auto evicted = impl_.Shift(next);
    return evicted;
  }

  template <size_t I>
  T Get() const {
    #pragma HLS INLINE
    return _Get<T, I, IThis, decltype(next_stage_)>(impl_.Get(), next_stage_);
  }

  T Get(size_t i) const {
    #pragma HLS INLINE
    if (i == IThis) {
      return impl_.Get();
    } else {
      return next_stage_.Get(i);
    }
  }

 private:
  _ShiftRegisterStage<T, IThis, Is...> next_stage_{};
  _ShiftRegisterStageImpl<T, kSize> impl_{};
};

template <typename T, size_t... Is>
class ShiftRegister {
 public:
  void Shift(T const &front) {
    #pragma HLS INLINE
    impl_.Shift(front);
  }

  template <size_t I>
  T Get() const {
    #pragma HLS INLINE
    return impl_.template Get<I>();
  }

  T Get(const size_t i) const {
    #pragma HLS INLINE
    return impl_.Get(i);
  }

 private:
  static constexpr size_t kSize = 1 + _MaxImpl<Is...>::Max();
  _ShiftRegisterStage<T, -1, Is...> impl_{};
};

}  // End namespace hlslib
