# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# Created: March 2017

cmake_minimum_required(VERSION 2.8)
project(hlslib CXX)

# User options
set(HLSLIB_PART_NAME "xcku115-flvb2104-2-e" CACHE STRING "Part name for HLS.")
set(HLSLIB_DSA_NAME "xilinx:xil-accel-rd-ku115:4ddr-xpr:4.0" CACHE STRING
    "DSA string for xocc.")

# Include custom Find<Module>.cmake scripts to enable searching for SDAccel
# and Vivado HLS
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Each tool is optional individually, but at least one is required
find_package(SDAccel)
find_package(VivadoHLS)
if(NOT SDACCEL_FOUND AND NOT VivadoHLS_FOUND)
  message(FATAL_ERROR "Either SDAccel or Vivado HLS required.")
endif()
include_directories(${VivadoHLS_INCLUDE_DIRS} ${SDAccel_INCLUDE_DIRS})
if (VivadoHLS_FOUND)
  set(HLSLIB_HLS ${VivadoHLS_BINARY})
else()
  set(HLSLIB_HLS ${SDAccel_VIVADO_HLS})
endif()

include_directories(include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

enable_testing()

# SDAccel host side executable test
if(SDAccel_FOUND)
  include_directories(${SDAccel_INCLUDE_DIRS})
  add_executable(TestSDAccelHostCode test/TestSDAccelHostCode.cpp)
  target_link_libraries(TestSDAccelHostCode ${SDAccel_LIBRARIES})
  add_test(TestSDAccelHostCode TestSDAccelHostCode)
endif()

# Unit tests 
add_executable(TestDataPack test/TestDataPack.cpp)
target_link_libraries(TestDataPack)
add_test(TestDataPack TestDataPack)
add_executable(TestReduce test/TestReduce.cpp)
target_link_libraries(TestReduce)
add_test(TestReduce TestReduce)

# Test kernels in software
find_package(Threads)
if(Threads_FOUND)
  add_executable(TestStream test/TestStream.cpp kernels/MultiStageAdd.cpp)
  target_link_libraries(TestStream ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestStream TestStream)
  target_compile_options(TestStream PRIVATE "-DHLSLIB_STREAM_SYNCHRONIZE")
  add_executable(TestAccumulateFloat test/TestAccumulate.cpp kernels/AccumulateFloat.cpp)
  target_compile_options(TestAccumulateFloat PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
  target_link_libraries(TestAccumulateFloat ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestAccumulateFloat TestAccumulateFloat)
  add_executable(TestAccumulateInt test/TestAccumulate.cpp kernels/AccumulateInt.cpp)
  target_compile_options(TestAccumulateInt PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_INT")
  target_link_libraries(TestAccumulateInt ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestAccumulateInt TestAccumulateInt)
  add_executable(TestSimulationForwarding test/TestSimulationForwarding.cpp)
  target_compile_options(TestSimulationForwarding PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_INT")
  target_link_libraries(TestSimulationForwarding ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestSimulationForwarding TestSimulationForwarding)
endif()

# Test synthesis of reduction
set(HLSLIB_PROJECT_NAME "Reduce")
set(HLSLIB_SRC_SYNTHESIS "${CMAKE_SOURCE_DIR}/kernels/Reduce.cpp")
set(HLSLIB_ENTRY_FUNCTION "Reduce")
set(HLSLIB_TARGET_CLOCK "200")
configure_file(scripts/Synthesis.tcl.in SynthesizeReduce.tcl)
add_custom_target(synthesize_reduce
  COMMAND ${HLSLIB_HLS} -f SynthesizeReduce.tcl) 

# Multi stage add to test streams and multi-threaded simulation of loop-based
# dataflow kernels
set(HLSLIB_PROJECT_NAME "MultiStageAdd")
set(HLSLIB_SRC_SYNTHESIS "${CMAKE_SOURCE_DIR}/kernels/MultiStageAdd.cpp")
set(HLSLIB_ENTRY_FUNCTION "MultiStageAdd")
set(HLSLIB_TARGET_TIMING "5")
configure_file(scripts/Synthesis.tcl.in SynthesizeMultiStageAdd.tcl)
add_custom_target(synthesize_multi_stage_add
  COMMAND ${HLSLIB_HLS} -f SynthesizeMultiStageAdd.tcl) 
if(SDAccel_FOUND)
  add_custom_target(kernel_multi_stage_add
    COMMAND ${SDAccel_XOCC} 
    -t hw
    -o MultiStageAdd.xclbin
    -I${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/kernels/MultiStageAdd.cpp
    --kernel MultiStageAdd
    --xdevice ${HLSLIB_DSA_NAME}
    --xp prop:kernel.${HLSLIB_ENTRY_FUNCTION}.kernel_flags="${CMAKE_CXX_FLAGS} -DHLSLIB_SYNTHESIS")
  add_executable(RunMultiStageAdd.exe host/RunMultiStageAdd.cpp)
  target_link_libraries(RunMultiStageAdd.exe ${SDAccel_LIBRARIES}) 
endif()

# Test kernel for pipelined accumulation of floating point numbers 
set(HLSLIB_PROJECT_NAME "AccumulateFloat")
set(HLSLIB_SRC_SYNTHESIS "${CMAKE_SOURCE_DIR}/kernels/AccumulateFloat.cpp")
set(HLSLIB_ENTRY_FUNCTION "AccumulateFloat")
set(HLSLIB_TARGET_TIMING "5")
set(HLSLIB_SYNTHESIS_FLAGS "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
configure_file(scripts/Synthesis.tcl.in SynthesizeAccumulateFloat.tcl)
add_custom_target(synthesize_accumulate_float
  COMMAND ${HLSLIB_HLS} -f SynthesizeAccumulateFloat.tcl) 
if(SDAccel_FOUND)
  add_custom_target(kernel_accumulate_float
    COMMAND ${SDAccel_XOCC} 
    -t hw
    -o AccumulateFloat.xclbin
    -I${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/kernels/AccumulateFloat.cpp
    --kernel AccumulateFloat
    --xdevice ${HLSLIB_DSA_NAME}
    --xp prop:kernel.${HLSLIB_ENTRY_FUNCTION}.kernel_flags="${CMAKE_CXX_FLAGS} -DHLSLIB_SYNTHESIS -DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
  add_executable(RunAccumulateFloat.exe host/RunAccumulate.cpp)
  target_compile_options(RunAccumulateFloat.exe PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
  target_link_libraries(RunAccumulateFloat.exe ${SDAccel_LIBRARIES}) 
endif()

# Test kernel for pipelined accumulation of integer numbers 
set(HLSLIB_PROJECT_NAME "AccumulateInt")
set(HLSLIB_SRC_SYNTHESIS "${CMAKE_SOURCE_DIR}/kernels/AccumulateInt.cpp")
set(HLSLIB_ENTRY_FUNCTION "AccumulateInt")
set(HLSLIB_TARGET_TIMING "5")
set(HLSLIB_SYNTHESIS_FLAGS "-DHLSLIB_COMPILE_ACCUMULATE_INT")
configure_file(scripts/Synthesis.tcl.in SynthesizeAccumulateInt.tcl)
add_custom_target(synthesize_accumulate_int
  COMMAND ${HLSLIB_HLS} -f SynthesizeAccumulateInt.tcl) 
if(SDAccel_FOUND)
  add_custom_target(kernel_accumulate_int
    COMMAND ${SDAccel_XOCC} 
    -t hw
    -o AccumulateInt.xclbin
    -I${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/kernels/AccumulateInt.cpp
    --kernel AccumulateInt
    --xdevice ${HLSLIB_DSA_NAME}
    --xp prop:kernel.${HLSLIB_ENTRY_FUNCTION}.kernel_flags="${CMAKE_CXX_FLAGS} -DHLSLIB_SYNTHESIS -DHLSLIB_COMPILE_ACCUMULATE_INT")
  add_executable(RunAccumulateInt.exe host/RunAccumulate.cpp)
  target_compile_options(RunAccumulateInt.exe PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_INT")
  target_link_libraries(RunAccumulateInt.exe ${SDAccel_LIBRARIES}) 
endif()
