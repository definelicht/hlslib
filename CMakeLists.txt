# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: March 2017

cmake_minimum_required(VERSION 2.8)
project(hlslib CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(SDAccel)

include_directories(include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(hls kernels/MultiStageAdd.cpp)

enable_testing()

# SDAccel host side executable test
if(SDAccel_FOUND)
  include_directories(${SDAccel_INCLUDE_DIRS})
  add_executable(TestSDAccelHostCode test/TestSDAccelHostCode.cpp)
  target_link_libraries(TestSDAccelHostCode ${SDAccel_LIBRARIES})
  add_test(TestSDAccelHostCode TestSDAccelHostCode)
endif()

add_executable(TestDataPack test/TestDataPack.cpp)
target_link_libraries(TestDataPack hls)
add_test(TestDataPack TestDataPack)

find_package(Threads)
if(Threads_FOUND)
  add_executable(TestStream test/TestStream.cpp)
  target_link_libraries(TestStream hls ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestStream TestStream)
endif()

# Multi stage add to test streams and multi-threaded simulation of loop-based
# dataflow kernels
set(HLSLIB_PROJECT_NAME "MultiStageAdd")
set(HLSLIB_PART_NAME "xcku115-flvb2104-2-e")
set(HLSLIB_SRC_SYNTHESIS "${CMAKE_SOURCE_DIR}/kernels/MultiStageAdd.cpp")
set(HLSLIB_ENTRY_FUNCTION "MultiStageAdd")
set(HLSLIB_TARGET_TIMING "5")
configure_file(scripts/Synthesis.tcl.in SynthesizeMultiStageAdd.tcl)
add_custom_target(synthesize_multi_stage_add
  COMMAND ${SDAccel_VIVADO_HLS} -f SynthesizeMultiStageAdd.tcl
  DEPENDS kernels/MultiStageAdd.cpp include/MultiStageAdd.h
          include/hlslib/Stream.h include/hlslib/Simulation.h)
