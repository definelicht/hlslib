/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      April 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#ifndef HLSLIB_SYNTHESIS
#include <thread>
#include <vector>
#endif

// This header provides functionality to simulate dataflow functions that
// include loops in conjunction with Stream.h.
//
// Dataflow functions must be wrapped in the HLSLIB_DATAFLOW_FUNCTION macro,
// which will launch them as a C++ thread when running simulation, but will
// fall back on normal function calls when running synthesis.
//
// The macro HLSLIB_DATAFLOW_FINALIZE must be called before returning from the
// top level function to join the dataflow threads.
// HLSLIB_DATAFLOW_INIT currently has no purpose, but is included for symmetry.

namespace hlslib {

#ifdef HLSLIB_SYNTHESIS
#define HLSLIB_DATAFLOW_INIT()
#define HLSLIB_DATAFLOW_FUNCTION(func, ...) func(__VA_ARGS__)
#define HLSLIB_DATAFLOW_FINALIZE()
#else
#include <vector>
#include <thread>
namespace {
class _Dataflow {
private:
  inline _Dataflow() {}
  inline ~_Dataflow() { this->Join(); }
public:
  inline static _Dataflow& Get() { // Singleton pattern
    static _Dataflow df;
    return df; 
  }
  template <class Function, typename... Args>
  void AddFunction(Function &&func, Args&&... args) {
    threads_.emplace_back([&](){ func(args...); });
  }
  inline void Join() {
    for (auto &t : threads_) {
      t.join();
    }
    threads_.clear();
  }
private:
  std::vector<std::thread> threads_{};
};
#define HLSLIB_DATAFLOW_INIT() 
#define HLSLIB_DATAFLOW_FUNCTION(func, ...) \
  ::hlslib::_Dataflow::Get().AddFunction(func, __VA_ARGS__)
#define HLSLIB_DATAFLOW_FINALIZE() ::hlslib::_Dataflow::Get().Join();
}
#endif

} // End namespace hlslib
