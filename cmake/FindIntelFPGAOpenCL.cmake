# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# Created: February 2019 
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   IntelFPGAOpenCL_FOUND - Indicates whether Intel FPGA OpenCL was found.
#   IntelFPGAOpenCL_INCLUDE_DIRS - Include directories for HLS. 
#   IntelFPGAOpenCL_LIBRARIES - Runtime libraries required for host side code. 
#   IntelFPGAOpenCL_RPATH - rpath required for runtime linkage
#   IntelFPGAOpenCL_AOC - Path to the aoc executable.
#   IntelFPGAOpenCL_AOCL - Path to the aocl executable.
#   IntelFPGAOpenCL_VERSION - Version of Intel FPGA OpenCL installation
#   IntelFPGAOpenCL_VERSION_MAJOR - Major version of Intel FPGA OpenCL installation
#   IntelFPGAOpenCL_VERSION_MINOR - Minor version of Intel FPGA OpenCL installation
#
# To specify the location of Intel FPGA OpenCL, or to force this script to use a
# specific version, set the variable INTELFPGAOCL_ROOT_DIR to the root
# directory of the desired Intel FPGA OpenCL installation.

if(NOT DEFINED INTELFPGAOCL_ROOT_DIR)

  find_path(INTELFPGAOCL_SEARCH_PATH aocl 
            PATHS ENV INTELFPGAOCLSDKROOT
						PATH_SUFFIXES bin)
  get_filename_component(INTELFPGAOCL_ROOT_DIR ${INTELFPGAOCL_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(INTELFPGAOCL_SEARCH_PATH)

else()

  message(STATUS "Using user defined Intel FPGA OpenCL directory: ${INTELFPGAOCL_ROOT_DIR}")

endif()

# Check if all the necessary components are present. We want to ensure that we
# use the tools bundled together, so we don't use find_path again. 

find_program(IntelFPGAOpenCL_AOC aoc PATHS ${INTELFPGAOCL_ROOT_DIR}/bin NO_DEFAULT_PATH)

get_filename_component(INTELFPGA_ROOT_DIR "${INTELFPGAOCL_ROOT_DIR}" DIRECTORY) 
mark_as_advanced(INTELFPGA_ROOT_DIR)

# Get version number string
get_filename_component(IntelFPGAOpenCL_VERSION "${INTELFPGA_ROOT_DIR}" NAME)
string(REGEX REPLACE "([0-9]+)\\.[0-9\\.]+" "\\1" IntelFPGAOpenCL_MAJOR_VERSION "${IntelFPGAOpenCL_VERSION}")
string(REGEX REPLACE "[0-9]+\\.([0-9\\.]+)" "\\1" IntelFPGAOpenCL_MINOR_VERSION "${IntelFPGAOpenCL_VERSION}")
set(IntelFPGAOpenCL_VERSION ${IntelFPGAOpenCL_VERSION} CACHE STRING "Intel FPGA OpenCL version.")
set(IntelFPGAOpenCL_MAJOR_VERSION ${IntelFPGAOpenCL_MAJOR_VERSION} CACHE STRING "Intel FPGA OpenCL major version.")
set(IntelFPGAOpenCL_MINOR_VERSION ${IntelFPGAOpenCL_MINOR_VERSION} CACHE STRING "Intel FPGA OpenCL minor version.")

find_program(IntelFPGAOpenCL_AOC aoc 
             PATHS ${INTELFPGAOCL_ROOT_DIR}/bin NO_DEFAULT_PATH)

find_program(IntelFPGAOpenCL_AOCL aocl 
             PATHS ${INTELFPGAOCL_ROOT_DIR}/bin NO_DEFAULT_PATH)

# Extract include directory from aocl compile-config command
execute_process(COMMAND ${IntelFPGAOpenCL_AOCL} compile-config OUTPUT_VARIABLE IntelFPGAOpenCL_INCLUDE_DIRS)
string(REGEX MATCHALL "-I[^ \t]+" IntelFPGAOpenCL_INCLUDE_DIRS "${IntelFPGAOpenCL_INCLUDE_DIRS}")
string(REPLACE "-I" "" IntelFPGAOpenCL_INCLUDE_DIRS "${IntelFPGAOpenCL_INCLUDE_DIRS}")
set(IntelFPGAOpenCL_INCLUDE_DIRS ${IntelFPGAOpenCL_INCLUDE_DIRS} CACHE STRING
    "Intel FPGA OpenCL host code include directories.")

# Extract libraries from aocl link-config command
execute_process(COMMAND ${IntelFPGAOpenCL_AOCL} link-config OUTPUT_VARIABLE INTELFPGAOCL_LINK_FLAGS)
string(REGEX MATCHALL "-L[^ \t\n]+" INTELFPGAOCL_LINK_DIRS "${INTELFPGAOCL_LINK_FLAGS}")
string(REPLACE "-L" "" INTELFPGAOCL_LINK_DIRS "${INTELFPGAOCL_LINK_DIRS}")
string(REGEX MATCHALL "-l[^ \t\n]+" INTELFPGAOCL_LINK_LIBS "${INTELFPGAOCL_LINK_FLAGS}")
string(REPLACE "-l" "" INTELFPGAOCL_LINK_LIBS "${INTELFPGAOCL_LINK_LIBS}")
mark_as_advanced(INTELFPGAOCL_LINK_DIRS INTELFPGAOCL_LINK_LIBS)

# Use find_library with hardcoded paths from aocl link-config
set(IntelFPGAOpenCL_LIBRARIES)
foreach(INTELFPGAOCL_LIB ${INTELFPGAOCL_LINK_LIBS})
  find_library(${INTELFPGAOCL_LIB}_PATH ${INTELFPGAOCL_LIB}  
               PATHS ${INTELFPGAOCL_LINK_DIRS} NO_DEFAULT_PATH)
  set(IntelFPGAOpenCL_LIBRARIES ${IntelFPGAOpenCL_LIBRARIES} ${${INTELFPGAOCL_LIB}_PATH})
  mark_as_advanced(${INTELFPGAOCL_LIB}_PATH)
endforeach()
set(IntelFPGAOpenCL_LIBRARIES ${IntelFPGAOpenCL_LIBRARIES} CACHE STRING
    "Intel FPGA OpenCL host code libraries.")

string(REPLACE " " ":" IntelFPGAOpenCL_RPATH "${INTELFPGAOCL_LINK_DIRS}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH
    "${CMAKE_INSTALL_RPATH}:${INTELFPGAOCL_LINK_DIRS}")
set(IntelFPGAOpenCL_RPATH ${IntelFPGAOpenCL_RPATH} CACHE STRING
    "Intel FPGA OpenCL rpath necessary at runtime to launch OpenCL kernels.")

set(IntelFPGAOpenCL_EXPORTS
    IntelFPGAOpenCL_AOCL
    IntelFPGAOpenCL_AOC
    IntelFPGAOpenCL_INCLUDE_DIRS
    IntelFPGAOpenCL_LIBRARIES
    IntelFPGAOpenCL_RPATH
    IntelFPGAOpenCL_VERSION
    IntelFPGAOpenCL_MAJOR_VERSION
    IntelFPGAOpenCL_MINOR_VERSION)
mark_as_advanced(IntelFPGAOpenCL_EXPORTS)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set IntelFPGAOpenCLHLS_FOUND to
# TRUE if all listed variables were found.
find_package_handle_standard_args(IntelFPGAOpenCL DEFAULT_MSG ${IntelFPGAOpenCL_EXPORTS})
