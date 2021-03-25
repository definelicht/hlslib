#include "Subflow.h"

#include "hlslib/xilinx/Simulation.h"
#include "hlslib/xilinx/Stream.h"

void ReadIn(const Data_t* memIn, hlslib::Stream<Data_t>& inPipe) {
  for (unsigned i = 0; i < kSize; ++i) {
    #pragma HLS PIPELINE II=1
    inPipe.Push(memIn[i]);
  }
}

void AddOne(hlslib::Stream<Data_t>& inPipe, hlslib::Stream<Data_t>& internal) {
  for (unsigned i = 0; i < kSize; ++i) {
    #pragma HLS PIPELINE II=1
    internal.Push(inPipe.Pop() + 1);
  }
}

void MultiplyByTwo(hlslib::Stream<Data_t>& internal,
                   hlslib::Stream<Data_t>& outPipe) {
  for (unsigned i = 0; i < kSize; ++i) {
    #pragma HLS PIPELINE II=1
    outPipe.Push(internal.Pop() * 2);
  }
}

void Subtask(hlslib::Stream<Data_t>& inPipe, hlslib::Stream<Data_t>& outPipe) {
  HLSLIB_DATAFLOW_INIT();
  hlslib::Stream<Data_t> internal;
  HLSLIB_DATAFLOW_FUNCTION(AddOne, inPipe, internal);
  HLSLIB_DATAFLOW_FUNCTION(MultiplyByTwo, internal, outPipe);
  HLSLIB_DATAFLOW_FINALIZE();
}

void WriteOut(hlslib::Stream<Data_t>& outPipe, Data_t* memOut) {
  for (unsigned i = 0; i < kSize; ++i) {
    #pragma HLS PIPELINE II=1
    memOut[i] = outPipe.Pop();
  }
}

void Subflow(const Data_t* memIn, Data_t* memOut) {
  #pragma HLS DATAFLOW
  HLSLIB_DATAFLOW_INIT();
  hlslib::Stream<Data_t> inPipe, outPipe;
  HLSLIB_DATAFLOW_FUNCTION(ReadIn, memIn, inPipe);
  HLSLIB_DATAFLOW_FUNCTION(Subtask, inPipe, outPipe);
  HLSLIB_DATAFLOW_FUNCTION(WriteOut, outPipe, memOut);
  HLSLIB_DATAFLOW_FINALIZE();
}
