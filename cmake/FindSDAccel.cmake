# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   SDAccel_FOUND - Indicates whether SDAccel was found.
#   SDAccel_INCLUDE_DIRS - Include directories for HLS. 
#   SDAccel_LIBRARIES - Runtime libraries required for host side code. 
#   SDAccel_FLOATING_POINT_LIBRARY - Library required for emulation of fp16.
#   SDAccel_XOCC - Path to the xocc executable.
#   SDAccel_VIVADO_HLS - Path to the Vivado HLS executable shipped with SDAccel. 
#   SDAccel_VERSION_MAJOR - Major version of SDAccel installation
#   SDAccel_VERSION_MINOR - Minor version of SDAccel installation
#
# To specify the location of SDAccel or to force this script to use a specific
# version, set the variable SDACCEL_ROOT_DIR to the root directory of the
# desired SDAccel installation.
# For SDAccel 2018.3 or newer, SDAccel relies on a separate Xilinx OpenCL
# runtime (XRT). This script will search in the default installation location,
# but a path can be specified with the XRT_ROOT_DIR variable.

message(WARNING "Xilinx has rebranded SDAccel as a component of the Vitis Unified Software Platform. Please use use the Vitis package instead (FindVitis.cmake included with hlslib).")

if(NOT DEFINED SDACCEL_ROOT_DIR)

  find_path(SDACCEL_SEARCH_PATH xocc 
            PATHS ENV XILINX_OPENCL ENV XILINX_SDACCEL
						PATH_SUFFIXES bin)
  get_filename_component(SDACCEL_ROOT_DIR ${SDACCEL_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(SDACCEL_SEARCH_PATH)

else()

  message(STATUS "Using user defined SDAccel directory: ${SDACCEL_ROOT_DIR}")

endif()

# Check if all the necessary components are present. We want to ensure that we
# use the tools bundled together, so we don't use find_path again. 

find_program(SDAccel_XOCC xocc PATHS ${SDACCEL_ROOT_DIR}/bin NO_DEFAULT_PATH)

# Get version number string
get_filename_component(SDAccel_VERSION "${SDACCEL_ROOT_DIR}" NAME)
string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" SDAccel_MAJOR_VERSION "${SDAccel_VERSION}")
string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" SDAccel_MINOR_VERSION "${SDAccel_VERSION}")

find_program(SDAccel_VIVADO_HLS vivado_hls
             PATHS ${SDACCEL_ROOT_DIR}/Vivado_HLS/bin
             PATHS ${SDACCEL_ROOT_DIR}/../../Vivado/${SDAccel_VERSION}/bin NO_DEFAULT_PATH)

find_program(SDAccel_VIVADO vivado
             PATHS ${SDACCEL_ROOT_DIR}/Vivado/bin
             PATHS ${SDACCEL_ROOT_DIR}/../../Vivado/${SDAccel_VERSION}/bin
             NO_DEFAULT_PATH)

find_path(SDAccel_HLS_INCLUDE_DIR hls_stream.h
          PATHS ${SDACCEL_ROOT_DIR}/Vivado_HLS/include
          ${SDACCEL_ROOT_DIR}/../../Vivado/${SDAccel_VERSION}/include
          NO_DEFAULT_PATH)
mark_as_advanced(SDAccel_HLS_INCLUDE_DIR)

if(SDAccel_MAJOR_VERSION GREATER 2018 OR
   (SDAccel_MAJOR_VERSION EQUAL 2018 AND SDAccel_MINOR_VERSION GREATER 2))
  set(SDAccel_USE_XRT ON)
endif()

# Currently only x86 support

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")

  # Floating point library

  find_library(SDAccel_FLOATING_POINT_LIBRARY Ip_floating_point_v7_0_bitacc_cmodel
               PATHS ${SDACCEL_ROOT_DIR}/Vivado_HLS/lnx64/tools/fpo_v7_0
               ${SDACCEL_ROOT_DIR}/../../Vivado/${SDAccel_VERSION}/lnx64/tools/fpo_v7_0)
  mark_as_advanced(SDAccel_FLOATING_POINT_LIBRARY)

  get_filename_component(SDACCEL_FP_DIR ${SDAccel_FLOATING_POINT_LIBRARY}
                         DIRECTORY) 
  mark_as_advanced(SDACCEL_FP_DIR)

  set(SDAccel_FLOATING_POINT_LIBRARY ${SDAccel_FLOATING_POINT_LIBRARY} 
      ${SDAccel_FLOATING_POINT_LIBMPFR} ${SDAccel_FLOATING_POINT_LIBGMP})

  find_library(SDAccel_FLOATING_POINT_LIBGMP gmp
               PATHS ${SDACCEL_FP_DIR} NO_DEFAULT_PATH)
  mark_as_advanced(SDAccel_FLOATING_POINT_LIBGMP)

  find_library(SDAccel_FLOATING_POINT_LIBMPFR mpfr
               PATHS ${SDACCEL_FP_DIR} NO_DEFAULT_PATH)
  mark_as_advanced(SDAccel_FLOATING_POINT_LIBMPFR)

  # OpenCL runtime

  mark_as_advanced(SDACCEL_RUNTIME_DIR)

  if(NOT SDAccel_USE_XRT)

    set(SDACCEL_RUNTIME_DIR ${SDACCEL_ROOT_DIR}/runtime)

    # Older versions of SDAccel ship with their own OpenCL headers. Make sure
    # to use them.
    find_path(SDAccel_OPENCL_INCLUDE_DIR opencl.h
              PATHS ${SDACCEL_RUNTIME_DIR}
              ${SDACCEL_RUNTIME_DIR}/include
              ${SDACCEL_RUNTIME_DIR}/x86_64/include
              PATH_SUFFIXES 1_1/CL 1_2/CL 2_0/CL
              NO_DEFAULT_PATH)
    get_filename_component(SDAccel_OPENCL_INCLUDE_DIR
                           ${SDAccel_OPENCL_INCLUDE_DIR} DIRECTORY) 

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
        message(FATAL_ERROR "The Xilinx OpenCL runtime (XRT) was not found. You can specify the XRT directory with the XRT_ROOT_DIR variable.")
      endif()

      message(STATUS "Found SDAccel runtime: ${XRT_ROOT_DIR}")

    else() 

      message(STATUS "Using user defined runtime directory \"${XRT_ROOT_DIR}\".")

    endif()

    set(SDACCEL_RUNTIME_DIR ${XRT_ROOT_DIR})

    # XRT doesn't ship with its own OpenCL headers and standard OpenCL library:
    # use system OpenCL libraries and headers.
    find_package(OpenCL REQUIRED)
    set(SDAccel_OPENCL_INCLUDE_DIR ${OpenCL_INCLUDE_DIRS})

  endif()

  find_library(SDAccel_LIBXILINXOPENCL xilinxopencl
               PATHS ${SDACCEL_RUNTIME_DIR}
                     ${SDACCEL_RUNTIME_DIR}/lib
                     ${SDACCEL_RUNTIME_DIR}/lib/x86_64
               NO_DEFAULT_PATH)
  mark_as_advanced(SDAccel_LIBXILINXOPENCL)

  get_filename_component(SDACCEL_RUNTIME_LIB_FOLDER ${SDAccel_LIBXILINXOPENCL}  
                         DIRECTORY) 
  mark_as_advanced(SDACCEL_RUNTIME_LIB_FOLDER)

  # Only succeed if libraries were found
  if(SDAccel_LIBXILINXOPENCL)
    set(SDAccel_LIBRARIES ${OpenCL_LIBRARIES} ${SDAccel_LIBXILINXOPENCL}
        CACHE STRING "SDAccel runtime libraries.")
  endif()

  # For some reason, the executable finds the floating point library on the
  # RPATH, but not libgmp. TODO: debug further.
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  set(CMAKE_INSTALL_RPATH
      "${CMAKE_INSTALL_RPATH}:${SDACCEL_RUNTIME_LIB_FOLDER}:${SDACCEL_FP_DIR}")

  find_path(SDAccel_OPENCL_EXTENSIONS_INCLUDE_DIR cl_ext.h
            PATHS ${SDACCEL_RUNTIME_DIR}/include
            PATH_SUFFIXES 1_1/CL 1_2/CL 2_0/CL CL
            NO_DEFAULT_PATH)
  get_filename_component(SDAccel_OPENCL_EXTENSIONS_INCLUDE_DIR 
                         ${SDAccel_OPENCL_EXTENSIONS_INCLUDE_DIR} DIRECTORY) 

  # Only succeed if both include paths were found
  if(SDAccel_HLS_INCLUDE_DIR AND SDAccel_OPENCL_INCLUDE_DIR AND
     SDAccel_OPENCL_EXTENSIONS_INCLUDE_DIR)
    set(SDAccel_INCLUDE_DIRS ${SDAccel_HLS_INCLUDE_DIR}
        ${SDAccel_OPENCL_INCLUDE_DIR} ${SDAccel_OPENCL_EXTENSIONS_INCLUDE_DIR} 
        CACHE STRING "SDAccel include directories.")
  endif()

else()

  message(WARNING "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")

endif()

set(SDAccel_EXPORTS
  SDAccel_XOCC SDAccel_VIVADO_HLS
  SDAccel_FLOATING_POINT_LIBRARY SDAccel_INCLUDE_DIRS
  SDAccel_VERSION SDAccel_MAJOR_VERSION SDAccel_MINOR_VERSION
  SDAccel_LIBRARIES)
mark_as_advanced(SDAccel_EXPORTS)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set SDAccelHLS_FOUND to TRUE
# if all listed variables were found.
find_package_handle_standard_args(SDAccel DEFAULT_MSG ${SDAccel_EXPORTS})
