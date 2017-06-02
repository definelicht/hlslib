/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include "hlslib/DataPack.h"
#include "hlslib/Operators.h"
#include "hlslib/Simulation.h"
#include "hlslib/Stream.h"

namespace hlslib {

namespace {

template <typename T, int width, class Operator, int latency, int size,
          int iterations>
struct AccumulateImpl {
  static void AccumulateIterate(Stream<DataPack<T, width>> &input,
                                Stream<DataPack<T, width>> &fromBounce,
                                Stream<DataPack<T, width>> &toBounce) {
  AccumulateIterateIterations:
    for (auto i = 0; i < iterations; ++i) {
    AcumulateIterateSize:
      for (auto j = 0; j < size / latency; ++j) {
      AccumulateIterateLatency:
        for (auto k = 0; k < latency; ++k) {
          #pragma HLS PIPELINE
          #pragma HLS LOOP_FLATTEN
          DataPack<T, width> a, b, result;
          a = hlslib::ReadBlocking(input);
          if (j > 0) {
            b = hlslib::ReadOptimistic(fromBounce);
          } else {
            b.Fill(Operator::identity());
          }
        AccumulateIterateSIMD:
          for (int w = 0; w < width; ++w) {
            #pragma HLS UNROLL
            result[w] = Operator::Apply(a[w], b[w]);
          }
          hlslib::WriteBlocking(toBounce, result, 1);
        }
      }
    }
  }
  static void AccumulateBounce(Stream<DataPack<T, width>> &fromIterate,
                               Stream<DataPack<T, width>> &toIterate,
                               Stream<DataPack<T, width>> &output) {
  AccumulateBounceIterations:
    for (auto i = 0; i < iterations; ++i) {
    AccumulateBounceSize:
      for (auto j = 0; j < size / latency; ++j) {
      AccumulateBounceLatency:
        for (auto k = 0; k < latency; ++k) {
          #pragma HLS PIPELINE
          #pragma HLS LOOP_FLATTEN
          const auto read = hlslib::ReadBlocking(fromIterate);
          if (j < size / latency - 1) {
            // Bounce back
            hlslib::WriteBlocking(toIterate, read, latency);
          } else {
            // Write to output
            hlslib::WriteBlocking(output, read, 1);
          }
        }
      }
    }
  }
  static void AccumulateReduce(Stream<DataPack<T, width>> &fromBounce,
                               Stream<DataPack<T, width>> &output) {
  AccumulateReduceIterations:
    for (auto i = 0; i < iterations; ++i) {
      #pragma HLS PIPELINE II=size
      DataPack<T, width> result(Operator::identity());
    AccumulateReduceLatency:
      for (auto j = 0; j < latency; ++j) {
        const auto read = hlslib::ReadBlocking(fromBounce);
      AccumulateReduceWidth:
        for (auto w = 0; w < width; ++w) {
          result[w] = Operator::Apply(result[w], read[w]);
        }
      }
      hlslib::WriteBlocking(output, result, 1);
    }
  }
};

} // End anonymous namespace

template <typename T, int width, class Operator, int latency, int size,
          int iterations>
void Accumulate(Stream<DataPack<T, width>> &in,
                Stream<DataPack<T, width>> &out) {
  #pragma HLS INLINE
  static_assert(size % latency == 0, "Size must be divisable by latency.");
  static Stream<DataPack<T, width>> toBounce("toBounce");
  static Stream<DataPack<T, width>> fromBounce("fromBounce");
  #pragma HLS STREAM variable=fromBounce depth=latency
  static Stream<DataPack<T, width>> toReduce("toReduce");
  #pragma HLS STREAM variable=toReduce depth=latency
#ifndef HLSLIB_SYNTHESIS
  // TODO: why doesn't this work with __VA__ARGS__?
  HLSLIB_DATAFLOW_FUNCTION(AccumulateImpl<T, width, Operator, latency, size,
                                          iterations>::AccumulateIterate,
                           in, fromBounce, toBounce);
  HLSLIB_DATAFLOW_FUNCTION(AccumulateImpl<T, width, Operator, latency, size,
                                          iterations>::AccumulateBounce,
                           toBounce, fromBounce, toReduce);
  HLSLIB_DATAFLOW_FUNCTION(AccumulateImpl<T, width, Operator, latency, size,
                                          iterations>::AccumulateReduce,
                           toReduce, out);
#else
  AccumulateImpl<T, width, Operator, latency, size,
                 iterations>::AccumulateIterate(in, fromBounce, toBounce);
  AccumulateImpl<T, width, Operator, latency, size,
                 iterations>::AccumulateBounce(toBounce, fromBounce, toReduce);
  AccumulateImpl<T, width, Operator, latency, size,
                 iterations>::AccumulateReduce(toReduce, out);
#endif
}

} // End namespace hlslib
