# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: March 2017

cmake_minimum_required(VERSION 2.8)
project(hlslib CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(SDAccel)

include_directories(include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

enable_testing()

if(SDAccel_FOUND)
  include_directories(${SDAccel_INCLUDE_DIRS})
  add_executable(TestSDAccelHostCode test/TestSDAccelHostCode.cpp)
  target_link_libraries(TestSDAccelHostCode ${SDAccel_LIBRARIES})
  add_test(TestSDAccelHostCode TestSDAccelHostCode)
endif()
