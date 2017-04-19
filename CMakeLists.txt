# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: March 2017

cmake_minimum_required(VERSION 2.8)
project(hlslib CXX)

# User options
set(HLSLIB_PART_NAME "xcku115-flvb2104-2-e" CACHE STRING "Part name for HLS.")
set(HLSLIB_DSA_NAME "xilinx:xil-accel-rd-ku115:4ddr-xpr:3.2" CACHE STRING
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DHLSLIB_STREAM_SYNCHRONIZE")

add_library(hls kernels/MultiStageAdd.cpp)

enable_testing()

# SDAccel host side executable test
if(SDAccel_FOUND)
  include_directories(${SDAccel_INCLUDE_DIRS})
  add_executable(TestSDAccelHostCode test/TestSDAccelHostCode.cpp)
  target_link_libraries(TestSDAccelHostCode ${SDAccel_LIBRARIES})
  add_test(TestSDAccelHostCode TestSDAccelHostCode)
endif()

# Unit tests of DataPack 
add_executable(TestDataPack test/TestDataPack.cpp)
target_link_libraries(TestDataPack hls)
add_test(TestDataPack TestDataPack)

# Test dataflow simulation along with Stream class
find_package(Threads)
if(Threads_FOUND)
  add_executable(TestStream test/TestStream.cpp)
  target_link_libraries(TestStream hls ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestStream TestStream)
endif()

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
  target_link_libraries(RunMultiStageAdd.exe hls ${SDAccel_LIBRARIES}) 
endif()
