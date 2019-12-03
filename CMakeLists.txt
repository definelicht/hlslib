# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)
project(hlslib)

# Include custom Find<Module>.cmake scripts to enable searching for
# Vitis/SDAccel and Intel FPGA OpenCL.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Without this variable set, CMake will build tests when running install
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)

# Generate Doxygen if available
find_package(Doxygen)
if(Doxygen_FOUND)
  configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in Doxyfile)
  add_custom_target(doxygen ALL
      COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile 
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# hlslib target
add_library(hlslib INTERFACE)
target_include_directories(hlslib INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

enable_testing()

# Find Xilinx and Intel OpenCL software for enabling test targets 
find_package(Threads)
find_package(Vitis)
find_package(IntelFPGAOpenCL)

# Xilinx example setup and testing
if(Vitis_FOUND AND Threads_FOUND)
  add_subdirectory(xilinx_test)
endif()
if(IntelFPGAOpenCL_FOUND AND Threads_FOUND)
  add_subdirectory(intel_test)
endif()

# Installation
install(TARGETS hlslib
    EXPORT hlslibConfig
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    RUNTIME DESTINATION bin COMPONENT Runtime
    PUBLIC_HEADER DESTINATION include COMPONENT Development
    BUNDLE DESTINATION bin COMPONENT Runtime)
install(EXPORT hlslibConfig DESTINATION cmake)
export(TARGETS hlslib FILE hlslibConfig.cmake)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/hlslib DESTINATION include)
install(FILES ${PROJECT_SOURCE_DIR}/cmake/FindVitis.cmake DESTINATION cmake)
install(FILES ${PROJECT_SOURCE_DIR}/cmake/FindSDAccel.cmake DESTINATION cmake)
install(FILES ${PROJECT_SOURCE_DIR}/cmake/FindIntelFPGAOpenCL.cmake DESTINATION cmake)
