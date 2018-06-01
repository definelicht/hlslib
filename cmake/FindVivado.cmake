# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# Created: March 2017 
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   Vivado_FOUND - Indicates whether Vivado was found.
#   Vivado_BINARY - Path to the Vivado binary.
#
# To specify the location of Vivado or to force this script to use a
# specific version, set the variable VIVADO_ROOT_DIR to the root directory of
# the desired Vivado installation.

if(NOT DEFINED VIVADO_ROOT_DIR)

  find_path(VIVADO_SEARCH_PATH vivado 
            PATHS ENV XILINX_VIVADO
                  ENV XILINX_SDACCEL ENV XILINX_OPENCL
						PATH_SUFFIXES bin Vivado/bin)
  get_filename_component(VIVADO_ROOT_DIR ${VIVADO_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(VIVADO_SEARCH_PATH)

else()

  message(STATUS "Using user defined Vivado directory \"${VIVADO_ROOT_DIR}\".")

endif()

find_program(Vivado_BINARY vivado 
             PATHS ${VIVADO_ROOT_DIR} ENV XILINX_VIVADO
                   ENV XILINX_SDACCEL ENV XILINX_OPENCL
             PATH_SUFFIXES bin Vivado/bin)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set Vivado_FOUND to TRUE
# if all listed variables were found.
find_package_handle_standard_args(Vivado DEFAULT_MSG Vivado_BINARY)
