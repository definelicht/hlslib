# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)
project(hlslib)

option(HLSLIB_ENABLE_TESTING
       "Enable CTest when built as a standalone CMake project." ON)
option(HLSLIB_BUILD_DOCUMENTATION
       "Build Doxygen documentation when built as a standalone CMake project." OFF)

# Include custom Find<Module>.cmake scripts to enable searching for
# Vitis/SDx/SDAccel and Intel FPGA OpenCL.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# Find Xilinx and Intel OpenCL software for enabling test targets 
find_package(Threads)
find_package(Vitis)
find_package(IntelFPGAOpenCL)

# hlslib target
add_library(hlslib INTERFACE)
target_include_directories(hlslib INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${Vitis_INCLUDE_DIRS}
    ${IntelFPGAOpenCL_INCLUDE_DIRS})
target_link_libraries(hlslib INTERFACE
    ${CMAKE_THREAD_LIBS_INIT}
    ${Vitis_LIBRARIES}
    ${IntelFPGAOpenCL_LIBRARIES})

# Check if hlslib is being built standalone or as a subproject
if(NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(HLSLIB_IS_SUBPROJECT TRUE)
endif()

# Only build tests and documentation if hlslib is built standalone
if(NOT HLSLIB_IS_SUBPROJECT)

  if(HLSLIB_ENABLE_TESTING)

    # Without this variable set, CMake will build tests when running install
    set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)

    enable_testing()

    # Xilinx example setup and testing
    if(Vitis_FOUND AND Threads_FOUND)
      add_subdirectory(xilinx_test)
    endif()
    if(IntelFPGAOpenCL_FOUND AND Threads_FOUND)
      add_subdirectory(intel_test)
    endif()

  endif()

  if(HLSLIB_BUILD_DOCUMENTATION)

    # Generate Doxygen if available
    find_package(Doxygen)
    if(Doxygen_FOUND)
      configure_file(${PROJECT_SOURCE_DIR}/Doxyfile.in Doxyfile)
      add_custom_target(doxygen ALL
          COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile 
          WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()

  endif()

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
install(FILES ${PROJECT_SOURCE_DIR}/cmake/FindIntelFPGAOpenCL.cmake DESTINATION cmake)
