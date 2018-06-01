# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# Created: March 2017 
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   VivadoLab_FOUND - Indicates whether Vivado was found.
#   VivadoLab_BINARY - Path to the Vivado binary.
#
# To specify the location of Vivado Lab or to force this script to use a
# specific version, set the variable VIVADOLAB_ROOT_DIR to the root directory of
# the desired Vivado Lab installation.

if(NOT DEFINED VIVADOLAB_ROOT_DIR)

  find_path(VIVADOLAB_SEARCH_PATH vivado_lab 
            PATHS ENV XILINX_VIVADOLAB ENV XILINX_VIVADO_LAB
						PATH_SUFFIXES bin)
  get_filename_component(VIVADOLAB_ROOT_DIR ${VIVADOLAB_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(VIVADOLAB_SEARCH_PATH)

else()

  message(STATUS "Using user defined Vivado Lab directory \"${VIVADOLAB_ROOT_DIR}\".")

endif()

find_program(VivadoLab_BINARY vivado_lab 
             PATHS ${VIVADOLAB_ROOT_DIR}
             ENV XILINX_VIVADOLAB ENV XILINX_VIVADO_LAB
             PATH_SUFFIXES bin)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set VivadoLab_FOUND to TRUE
# if all listed variables were found.
find_package_handle_standard_args(VivadoLab DEFAULT_MSG VivadoLab_BINARY)
