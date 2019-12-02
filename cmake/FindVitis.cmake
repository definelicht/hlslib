# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   Vitis_FOUND - Indicates whether Vitis/SDx/SDAccel was found.
#   Vitis_INCLUDE_DIRS - Include directories for HLS. 
#   Vitis_LIBRARIES - Runtime libraries required for host side code. 
#   Vitis_COMPILER - Path to the compiler executable (v++ or xocc).
#   Vitis_HLS - Path to Vivado HLS executable. 
#   Vitis_FLOATING_POINT_LIBRARY - Library required for emulation of fp16.
#   Vitis_VERSION - Version of Vitis/SDx/SDAccel installation.
#   Vitis_VERSION_MAJOR - Major version of Vitis/SDx/SDAccel installation.
#   Vitis_VERSION_MINOR - Minor version of Vitis/SDx/SDAccel installation.
#   Vitis_IS_LEGACY - Set if using a pre-Vitis version (i.e., SDx or SDAccel)
#
# To specify the location of Vitis or SDAccel, or to force this script to use a
# specific version, set the variable VITIS_ROOT_DIR or SDACCEL_ROOT_DIR to the
# root directory of the desired Vitis or SDAccel installation, respectively.
# For SDAccel 2018.3 or newer, Vitis/SDAccel relies on a separate Xilinx Runtime
# (XRT). This script will search in the default installation location, but a
# path can be specified with the XRT_ROOT_DIR variable.

if(DEFINED SDACCEL_ROOT_DIR AND NOT DEFINED VITIS_ROOT_DIR)
  set(VITIS_ROOT_DIR ${SDACCEL_ROOT_DIR})
endif()

if(NOT DEFINED VITIS_ROOT_DIR)

  find_path(VITIS_SEARCH_PATH v++ xocc 
            PATHS ENV XILINX_OPENCL ENV XILINX_VITIS ENV XILINX_SDACCEL
						PATH_SUFFIXES bin)
  get_filename_component(VITIS_ROOT_DIR ${VITIS_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(VITIS_SEARCH_PATH)

else()

  message(STATUS "Using user defined directory: ${VITIS_ROOT_DIR}")

endif()

# Check if all the necessary components are present. We want to ensure that we
# use the tools bundled together, so we restrict all further finds to only look
# in paths relative to the determined installation. 

find_program(Vitis_XOCC xocc PATHS ${VITIS_ROOT_DIR}/bin NO_DEFAULT_PATH)
find_program(Vitis_VPP v++ PATHS ${VITIS_ROOT_DIR}/bin NO_DEFAULT_PATH)
mark_as_advanced(Vitis_XOCC)
mark_as_advanced(Vitis_VPP)
if(Vitis_XOCC)
  set(VITIS_COMPILER ${Vitis_XOCC})
  set(VITIS_IS_LEGACY TRUE)
endif()
# Prefer v++ over xocc executable 
if(Vitis_VPP)
  set(VITIS_COMPILER ${Vitis_VPP})
  set(VITIS_IS_LEGACY FALSE)
endif()
set(Vitis_COMPILER ${VITIS_COMPILER} CACHE STRING "Compiler used to build FPGA kernels.")
set(Vitis_IS_LEGACY ${VITIS_IS_LEGACY} CACHE STRING "Using legacy version of toolchain (pre-Vitis).")

# Get version number string
get_filename_component(VITIS_VERSION "${VITIS_ROOT_DIR}" NAME)
string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" VITIS_MAJOR_VERSION "${VITIS_VERSION}")
string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" VITIS_MINOR_VERSION "${VITIS_VERSION}")
set(Vitis_VERSION ${VITIS_VERSION} CACHE STRING "Version of Vitis found")
set(Vitis_MAJOR_VERSION ${VITIS_MAJOR_VERSION} CACHE STRING "Major version of Vitis found")
set(Vitis_MINOR_VERSION ${VITIS_MINOR_VERSION} CACHE STRING "Minor version of Vitis found")

# vitis_hls is still in beta as of 2019.2 and breaks some functionality, so
# prefer vivado_hls for now (subject to change).
find_program(Vitis_HLS NAMES vivado_hls vitis_hls PATHS
             ${VITIS_ROOT_DIR}/bin
             ${VITIS_ROOT_DIR}/../../Vivado/${Vitis_VERSION}/bin
             ${VITIS_ROOT_DIR}/Vivado_HLS/bin NO_DEFAULT_PATH)

find_program(Vitis_VIVADO vivado PATHS
             ${VITIS_ROOT_DIR}/../../Vivado/${Vitis_VERSION}/bin
             ${VITIS_ROOT_DIR}/Vivado/bin NO_DEFAULT_PATH)

find_path(Vitis_HLS_INCLUDE_DIR hls_stream.h PATHS
          ${VITIS_ROOT_DIR}/../../Vivado/${Vitis_VERSION}/include
          ${VITIS_ROOT_DIR}/include
          ${VITIS_ROOT_DIR}/Vivado_HLS/include
          NO_DEFAULT_PATH)
mark_as_advanced(Vitis_HLS_INCLUDE_DIR)

if(Vitis_VPP OR (Vitis_MAJOR_VERSION GREATER 2018) OR
   (Vitis_MAJOR_VERSION EQUAL 2018 AND Vitis_MINOR_VERSION GREATER 2))
  set(VITIS_USE_XRT TRUE)
else()
  set(VITIS_USE_XRT FALSE)
endif()
set(Vitis_USE_XRT ${VITIS_USE_XRT} CACHE STRING "Use XRT as runtime. Otherwise, use SDAccel/SDx OpenCL runtime.")

# Currently only x86 support

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")

  #----------------------------------------------------------------------------
  # Floating point library
  #----------------------------------------------------------------------------

  find_library(Vitis_FLOATING_POINT_LIBRARY Ip_floating_point_v7_0_bitacc_cmodel
               PATHS
               ${VITIS_ROOT_DIR}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT_DIR}/../../Vivado/${Vitis_VERSION}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT_DIR}/Vivado_HLS/lnx64/tools/fpo_v7_0)
  mark_as_advanced(Vitis_FLOATING_POINT_LIBRARY)

  get_filename_component(VITIS_FP_DIR ${Vitis_FLOATING_POINT_LIBRARY}
                         DIRECTORY) 
  mark_as_advanced(VITIS_FP_DIR)

  set(Vitis_FLOATING_POINT_LIBRARY ${Vitis_FLOATING_POINT_LIBRARY} 
      ${Vitis_FLOATING_POINT_LIBMPFR} ${Vitis_FLOATING_POINT_LIBGMP})

  find_library(Vitis_FLOATING_POINT_LIBGMP gmp
               PATHS ${VITIS_FP_DIR} NO_DEFAULT_PATH)
  mark_as_advanced(Vitis_FLOATING_POINT_LIBGMP)

  find_library(Vitis_FLOATING_POINT_LIBMPFR mpfr
               PATHS ${VITIS_FP_DIR} NO_DEFAULT_PATH)
  mark_as_advanced(Vitis_FLOATING_POINT_LIBMPFR)

  #----------------------------------------------------------------------------
  # OpenCL runtime
  #----------------------------------------------------------------------------

  mark_as_advanced(VITIS_RUNTIME_DIR)

  if(NOT Vitis_USE_XRT)

    set(VITIS_RUNTIME_DIR ${VITIS_ROOT_DIR}/runtime)

    # Older versions of SDAccel ship with their own OpenCL headers. Make sure
    # to use them.
    find_path(Vitis_OPENCL_INCLUDE_DIR opencl.h
              PATHS ${VITIS_RUNTIME_DIR}
              ${VITIS_RUNTIME_DIR}/include
              ${VITIS_RUNTIME_DIR}/x86_64/include
              PATH_SUFFIXES 1_1/CL 1_2/CL 2_0/CL
              NO_DEFAULT_PATH)
    get_filename_component(Vitis_OPENCL_INCLUDE_DIR
                           ${Vitis_OPENCL_INCLUDE_DIR} DIRECTORY) 

  else()

    if(NOT DEFINED XRT_ROOT_DIR)

      find_path(XRT_SEARCH_PATH libxilinxopencl.so 
                PATHS /opt/xilinx/xrt /opt/Xilinx/xrt
                      /tools/Xilinx/xrt /tools/xilinx/xrt
                      ENV XILINX_XRT
                PATH_SUFFIXES lib)
      get_filename_component(XRT_ROOT_DIR ${XRT_SEARCH_PATH} DIRECTORY) 
      mark_as_advanced(XRT_SEARCH_PATH)

      if(NOT XRT_SEARCH_PATH)
        message(FATAL_ERROR "The Xilinx Runtime (XRT) was not found. You can specify the XRT directory with the XRT_ROOT_DIR variable.")
      endif()

      message(STATUS "Found Xilinx Runtime (XRT): ${XRT_ROOT_DIR}")

    else() 

      message(STATUS "Using user defined Xilinx Runtime (XRT) directory \"${XRT_ROOT_DIR}\".")

    endif()

    set(VITIS_RUNTIME_DIR ${XRT_ROOT_DIR})

    # XRT doesn't ship with its own OpenCL headers and standard OpenCL library:
    # use system OpenCL libraries and headers.
    find_package(OpenCL REQUIRED)
    set(Vitis_OPENCL_INCLUDE_DIR ${OpenCL_INCLUDE_DIRS})

  endif()

  find_library(Vitis_LIBXILINXOPENCL xilinxopencl
               PATHS ${VITIS_RUNTIME_DIR}
                     ${VITIS_RUNTIME_DIR}/lib
                     ${VITIS_RUNTIME_DIR}/lib/x86_64
               NO_DEFAULT_PATH)
  mark_as_advanced(Vitis_LIBXILINXOPENCL)

  get_filename_component(VITIS_RUNTIME_LIB_FOLDER ${Vitis_LIBXILINXOPENCL}  
                         DIRECTORY) 
  mark_as_advanced(VITIS_RUNTIME_LIB_FOLDER)

  # Only succeed if libraries were found
  if(Vitis_LIBXILINXOPENCL)
    set(Vitis_LIBRARIES ${OpenCL_LIBRARIES} ${Vitis_LIBXILINXOPENCL}
        CACHE STRING "OpenCL runtime libraries.")
  endif()

  # For some reason, the executable finds the floating point library on the
  # RPATH, but not libgmp. TODO: debug further.
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  set(CMAKE_INSTALL_RPATH
      "${CMAKE_INSTALL_RPATH}:${VITIS_RUNTIME_LIB_FOLDER}:${VITIS_FP_DIR}")

  find_path(Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR cl_ext.h
            PATHS ${VITIS_RUNTIME_DIR}/include
            PATH_SUFFIXES 1_1/CL 1_2/CL 2_0/CL CL
            NO_DEFAULT_PATH)
  get_filename_component(Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR 
                         ${Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR} DIRECTORY) 

  # Only succeed if both include paths were found
  if(Vitis_HLS_INCLUDE_DIR AND Vitis_OPENCL_INCLUDE_DIR AND
     Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR)
    set(Vitis_INCLUDE_DIRS ${Vitis_HLS_INCLUDE_DIR}
        ${Vitis_OPENCL_INCLUDE_DIR} ${Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR} 
        CACHE STRING "Vitis include directories.")
  endif()

else()

  message(WARNING "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")

endif()

set(Vitis_EXPORTS
    Vitis_COMPILER
    Vitis_HLS
    Vitis_INCLUDE_DIRS
    Vitis_LIBRARIES
    Vitis_FLOATING_POINT_LIBRARY 
    Vitis_VERSION
    Vitis_MAJOR_VERSION
    Vitis_MINOR_VERSION)
mark_as_advanced(Vitis_EXPORTS)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set Vitis_FOUND to TRUE if all
# listed variables were found.
find_package_handle_standard_args(Vitis DEFAULT_MSG ${Vitis_EXPORTS})
