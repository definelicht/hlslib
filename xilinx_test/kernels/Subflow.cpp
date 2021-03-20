#include "Subflow.h"

#include "hlslib/xilinx/Simulation.h"
#include "hlslib/xilinx/Stream.h"


void readIn(const Data_t* memIn, hlslib::Stream<Data_t>& inPipe) {
  for (unsigned i=0; i<dataSize; ++i) {
    #pragma HLS pipeline II=1
    #pragma HLS flatten
    inPipe.Push(memIn[i]);
  }
}

void add_one(
  hlslib::Stream<Data_t>& inPipe, hlslib::Stream<Data_t>& internal
) {
  for (unsigned i=0; i<dataSize; ++i) {
    #pragma HLS pipeline II=1
    #pragma HLS flatten
    internal.Push(inPipe.Pop() + 1);
  }
}

void multiply_by_two(
  hlslib::Stream<Data_t>& internal, hlslib::Stream<Data_t>& outPipe
) {
  for (unsigned i=0; i<dataSize; ++i) {
    #pragma HLS pipeline II=1
    #pragma HLS flatten
    outPipe.Push(internal.Pop() * 2);
  }
}

void subtask(
  hlslib::Stream<Data_t>& inPipe, hlslib::Stream<Data_t>& outPipe
) {
  HLSLIB_SUBFLOW_INIT();
  hlslib::Stream<Data_t> internal;
  HLSLIB_DATAFLOW_FUNCTION(add_one, inPipe, internal);
  HLSLIB_DATAFLOW_FUNCTION(multiply_by_two, internal, outPipe);
  HLSLIB_SUBFLOW_FINALIZE();
}

void writeOut(hlslib::Stream<Data_t>& outPipe, Data_t* memOut) {
  for (unsigned i=0; i<dataSize; ++i) {
    #pragma HLS pipeline II=1
    #pragma HLS flatten
    memOut[i] = outPipe.Pop();
 }
}

extern "C" void Subflow(const Data_t* memIn, Data_t* memOut) {
  #pragma HLS dataflow
  HLSLIB_DATAFLOW_INIT();
  hlslib::Stream<Data_t> inPipe, outPipe;
  HLSLIB_DATAFLOW_FUNCTION(readIn, memIn, inPipe);
  HLSLIB_ADD_SUBFLOW(subtask, inPipe, outPipe);
  HLSLIB_DATAFLOW_FUNCTION(writeOut, outPipe, memOut);
  HLSLIB_DATAFLOW_FINALIZE();
}
