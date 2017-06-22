/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include "hlslib/DataPack.h"
#include "hlslib/Operators.h"
#include "hlslib/Simulation.h"
#include "hlslib/Stream.h"

// This header includes a module that allows pipelined accumulation of data
// types with a non-zero latency on the operation, such as addition of floating
// point numbers, using a minimal amount of arithmetic units.
// The functionality is split into three functions:
//
//   1) Iterate, which accumulates `latency` different inputs, outputting
//      partial results.
//   2) Feedback, which loops partial results back to Iterate to continue
//      accumulation until done, then emits the result.
//   3) Reduce, which finally collapses the `latency` inputs into a single
//      output. 
//
// If latency is not set high enough, the design will hang, as no new values
// will be available from Feedback when Iterate requires them, and the pipeline
// will stall forever. The latency should be at least the sum of the latencies
// of the Iterate and Feedback modules, plus a few cycles for the overhead of
// the FIFOs between them.
// For example, for single precision floating point addition at 300 MHz, the
// latency of Iterate is 10 and the latency of bounce is 2. A latency of 14 is 
// enough to successfully run accumulation.

namespace hlslib {

namespace {

template <typename T, int width, class Operator, int latency, int size,
          int iterations>
struct AccumulateImpl {
  using DataPack_t = DataPack<T, width>;
  static void AccumulateIterate(Stream<DataPack_t> &input,
                                Stream<DataPack_t> &fromFeedback,
                                Stream<DataPack_t> &toFeedback) {
  AccumulateIterate_Iterations:
    for (int i = 0; i < iterations; ++i) {
    AcumulateIterate_Size:
      for (int j = 0; j < size / latency; ++j) {
      AccumulateIterate_Latency:
        for (int k = 0; k < latency; ++k) {
          #pragma HLS PIPELINE
          #pragma HLS LOOP_FLATTEN
          DataPack_t a, b, result;
          a = hlslib::ReadBlocking(input);
          if (j > 0) {
            b = hlslib::ReadOptimistic(fromFeedback);
          } else {
            b.Fill(Operator::identity());
          }
        AccumulateIterate_SIMD:
          for (int w = 0; w < width; ++w) {
            #pragma HLS UNROLL
            result[w] = Operator::Apply(a[w], b[w]);
          }
          hlslib::WriteBlocking(toFeedback, result, 1);
        }
      }
    }
  }
  static void AccumulateFeedback(Stream<DataPack_t> &fromIterate,
                                 Stream<DataPack_t> &toIterate,
                                 Stream<DataPack_t> &output) {
  AccumulateFeedback_Iterations:
    for (int i = 0; i < iterations; ++i) {
    AccumulateFeedback_Size:
      for (int j = 0; j < size / latency; ++j) {
      AccumulateFeedback_Latency:
        for (int k = 0; k < latency; ++k) {
          #pragma HLS PIPELINE
          #pragma HLS LOOP_FLATTEN
          const auto read = hlslib::ReadBlocking(fromIterate);
          if (j < size / latency - 1) {
            // Feedback back
            hlslib::WriteBlocking(toIterate, read, latency);
          } else {
            // Write to output
            hlslib::WriteBlocking(output, read, 1);
          }
        }
      }
    }
  }
  static void AccumulateReduce(Stream<DataPack_t> &fromFeedback,
                               Stream<DataPack_t> &output) {
  AccumulateReduce_Iterations:
    for (int i = 0; i < iterations; ++i) {
      #pragma HLS PIPELINE II=latency
      DataPack_t result(Operator::identity());
    AccumulateReduce_Latency:
      for (int j = 0; j < latency; ++j) {
        const auto read = hlslib::ReadBlocking(fromFeedback);
      AccumulateReduce_Width:
        for (int w = 0; w < width; ++w) {
          #pragma HLS UNROLL
          result[w] = Operator::Apply(result[w], read[w]);
        }
      }
      hlslib::WriteBlocking(output, result, 1);
    }
  }
  static void AccumulateEntry(Stream<DataPack_t> &in,
                              Stream<DataPack_t> &out) {
    #pragma HLS INLINE
    static_assert(size % latency == 0, "Size must be divisable by latency.");
    static Stream<DataPack_t> toFeedback("toFeedback");
    static Stream<DataPack_t> fromFeedback("fromFeedback");
    #pragma HLS STREAM variable=fromFeedback depth=latency
    static Stream<DataPack_t> toReduce("toReduce");
  #ifndef HLSLIB_SYNTHESIS
    // TODO: why doesn't this work with __VA__ARGS__?
    HLSLIB_DATAFLOW_FUNCTION(AccumulateIterate, in, fromFeedback, toFeedback);
    HLSLIB_DATAFLOW_FUNCTION(AccumulateFeedback, toFeedback, fromFeedback,
                             toReduce);
    HLSLIB_DATAFLOW_FUNCTION(AccumulateReduce, toReduce, out);
  #else
    AccumulateIterate(in, fromFeedback, toFeedback);
    AccumulateFeedback(toFeedback, fromFeedback, toReduce);
    AccumulateReduce(toReduce, out);
  #endif
  }
};

/// Trivial implementation for single cycle latency
template <typename T, int width, class Operator, int size, int iterations>
struct AccumulateImpl<T, width, Operator, 1, size, iterations> {
  using DataPack_t = DataPack<T, width>;
  static void AccumulateEntry(Stream<DataPack_t> &in,
                              Stream<DataPack_t> &out) {
    #pragma HLS INLINE
    HLSLIB_DATAFLOW_FUNCTION(AccumulateSimple, in, out);
  }
  static void AccumulateSimple(Stream<DataPack_t> &in,
                               Stream<DataPack_t> &out) {
    // When adding labels to this function, Vivado HLS 2017.1 fails with:
    // WARNING: [XFORM 203-302] Region 'AccumulateSimple_Size' (include/hlslib/Accumulate.h:145, include/hlslib/Accumulate.h:145) has multiple begin anchors
  // AccumulateSimple_Iterations:
    for (int i = 0; i < iterations; ++i) {
      DataPack_t acc;
    // AccumulateSimple_Size:
      for (int j = 0; j < size; ++j) {
        #pragma HLS LOOP_FLATTEN
        #pragma HLS PIPELINE
        const auto a = hlslib::ReadBlocking(in);
        const auto b = (j > 0) ? acc : DataPack_t(Operator::identity());
        for (int w = 0; w < width; ++w) {
          #pragma HLS UNROLL
          acc[w] = Operator::Apply(a[w], b[w]);
        }
        if (j == size - 1) {
          hlslib::WriteBlocking(out, acc, 1);
        }
      }
    }
  }
};

} // End anonymous namespace

template <typename T, int width, class Operator, int latency, int size,
          int iterations>
void Accumulate(Stream<DataPack<T, width>> &in,
                Stream<DataPack<T, width>> &out) {
  #pragma HLS INLINE
  AccumulateImpl<T, width, Operator, latency, size,
                 iterations>::AccumulateEntry(in, out);
}

} // End namespace hlslib
