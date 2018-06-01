# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# Created: March 2017 
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   VivadoHLS_FOUND - Indicates whether Vivado HLS was found.
#   VivadoHLS_BINARY - Path of the Vivado HLS binary.
#   VivadoHLS_INCLUDE_DIRS - Include directories for HLS. 
#
# To specify the location of Vivado HLS or to force this script to use a
# specific version, set the variable VIVADOHLS_ROOT_DIR to the root directory of
# the desired Vivado HLS installation.

if(NOT DEFINED VIVADOHLS_ROOT_DIR)

  find_path(VIVADOHLS_SEARCH_PATH vivado_hls 
            PATHS ENV XILINX_HLS ENV XILINX_VIVADOHLS ENV XILINX_VIVADO_HLS
                  ENV XILINX_SDACCEL ENV XILINX_OPENCL
						PATH_SUFFIXES bin Vivado_HLS/bin)
  get_filename_component(VIVADOHLS_ROOT_DIR ${VIVADOHLS_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(VIVADOHLS_SEARCH_PATH)

else()

  message(STATUS "Using user defined Vivado HLS directory \"${VIVADOHLS_ROOT_DIR}\".")

endif()

find_program(VivadoHLS_BINARY vivado_hls 
             PATHS ${VIVADOHLS_ROOT_DIR} ENV XILINX_HLS ENV XILINX_VIVADOHLS
                   ENV XILINX_VIVADO_HLS ENV XILINX_SDACCEL ENV XILINX_OPENCL
             PATH_SUFFIXES bin Vivado_HLS/bin)

find_path(VivadoHLS_INCLUDE_DIRS hls_stream.h 
          PATHS ${VIVADOHLS_ROOT_DIR} ENV XILINX_HLS ENV XILINX_VIVADOHLS
                ENV XILINX_SDACCEL ENV XILINX_OPENCL
          PATH_SUFFIXES include Vivado_HLS/include)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set VivadoHLS_FOUND to TRUE
# if all listed variables were found.
find_package_handle_standard_args(VivadoHLS DEFAULT_MSG
  VivadoHLS_BINARY VivadoHLS_INCLUDE_DIRS)
