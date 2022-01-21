# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   Vitis_FOUND - Indicates whether Vitis/SDx/SDAccel was found.
#   Vitis_INCLUDE_DIRS - Include directories for HLS. 
#   Vitis_LIBRARIES - Runtime libraries required for host side code. 
#   Vitis_COMPILER - Path to the compiler executable (v++ or xocc).
#   Vitis_HLS - Path to HLS executable (vitis_hls or vivado_hls). 
#   Vitis_FLOATING_POINT_LIBRARY - Library required for emulation of fp16.
#   Vitis_VERSION - Version of Vitis/SDx/SDAccel installation.
#   Vitis_VERSION_MAJOR - Major version of Vitis/SDx/SDAccel installation.
#   Vitis_VERSION_MINOR - Minor version of Vitis/SDx/SDAccel installation.
#   Vitis_IS_LEGACY - Set if using a pre-Vitis version (i.e., SDx or SDAccel)
#   Vitis_PLATFORMINFO - Path to the utility for extracting information from installed platforms
#
# To specify the location of Vitis, or to force this script to use a specific version, set the variable VITIS_ROOT to
# the root directory of the desired Vitis installation. Similarly, XRT_ROOT can be used to specify the XRT installation
# that should be used.

# Backwards compatibility with SDx and SDAccel (not officially supported)
if(DEFINED SDACCEL_ROOT_DIR AND NOT DEFINED SDACCEL_ROOT)
  set(SDACCEL_ROOT ${SDACCEL_ROOT_DIR})
endif()
if(DEFINED SDX_ROOT_DIR AND NOT DEFINED SDX_ROOT)
  set(SDX_ROOT ${SDX_ROOT_DIR})
endif()
if(DEFINED VITIS_ROOT_DIR AND NOT DEFINED VITIS_ROOT)
  set(VITIS_ROOT ${VITIS_ROOT_DIR})
endif()
if(DEFINED SDX_ROOT AND NOT DEFINED VITIS_ROOT)
  set(VITIS_ROOT ${SDX_ROOT})
endif()
if(DEFINED SDACCEL_ROOT AND NOT DEFINED VITIS_ROOT)
  set(VITIS_ROOT ${SDACCEL_ROOT})
endif()

if(NOT DEFINED VITIS_ROOT)

  find_path(VITIS_SEARCH_PATH v++ xocc 
            PATHS ENV XILINX_OPENCL ENV XILINX_VITIS ENV XILINX_SDACCEL
                  ENV XILINX_SDX
            PATH_SUFFIXES bin)
  get_filename_component(VITIS_ROOT ${VITIS_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(VITIS_SEARCH_PATH)

else()

  message(STATUS "Using user defined Vitis directory: ${VITIS_ROOT}")

endif()

# Check if all the necessary components are present. We want to ensure that we
# use the tools bundled together, so we restrict all further finds to only look
# in paths relative to the determined installation. 

find_program(Vitis_XOCC xocc PATHS ${VITIS_ROOT}/bin NO_DEFAULT_PATH)
find_program(Vitis_VPP v++ PATHS ${VITIS_ROOT}/bin NO_DEFAULT_PATH)
find_program(Vitis_PLATFORMINFO platforminfo PATHS ${VITIS_ROOT}/bin NO_DEFAULT_PATH)
mark_as_advanced(Vitis_XOCC)
mark_as_advanced(Vitis_VPP)
if(Vitis_XOCC)
  set(VITIS_COMPILER ${Vitis_XOCC})
  set(VITIS_IS_LEGACY TRUE)
  add_definitions(-DVITIS_IS_LEGACY)
endif()
# Prefer v++ over xocc executable 
if(Vitis_VPP)
  set(VITIS_COMPILER ${Vitis_VPP})
  set(VITIS_IS_LEGACY FALSE)
endif()
unset(Vitis_XOCC)
unset(Vitis_VPP)
set(Vitis_COMPILER ${VITIS_COMPILER} CACHE STRING "Compiler used to build FPGA kernels." FORCE)
set(Vitis_IS_LEGACY ${VITIS_IS_LEGACY} CACHE STRING "Using legacy version of toolchain (pre-Vitis)." FORCE)
set(Vitis_PLATFORMINFO ${Vitis_PLATFORMINFO} CACHE STRING "Utility for extracting information from Xilinx platforms." FORCE)

# Get version number string
get_filename_component(VITIS_VERSION "${VITIS_ROOT}" NAME)
string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" VITIS_MAJOR_VERSION "${VITIS_VERSION}")
string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" VITIS_MINOR_VERSION "${VITIS_VERSION}")
set(Vitis_VERSION ${VITIS_VERSION} CACHE STRING "Version of Vitis found" FORCE)
set(Vitis_MAJOR_VERSION ${VITIS_MAJOR_VERSION} CACHE STRING "Major version of Vitis found" FORCE)
set(Vitis_MINOR_VERSION ${VITIS_MINOR_VERSION} CACHE STRING "Minor version of Vitis found" FORCE)
add_definitions(-DVITIS_VERSION=${Vitis_VERSION} -DVITIS_MAJOR_VERSION=${Vitis_MAJOR_VERSION} -DVITIS_MINOR_VERSION=${Vitis_MINOR_VERSION})

find_program(Vitis_VIVADO_HLS NAMES vivado_hls PATHS
             ${VITIS_ROOT}/bin
             ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/bin
             ${VITIS_ROOT}/Vivado_HLS/bin NO_DEFAULT_PATH DOC
             "Vivado HLS compiler associated with this version of the tools.")

# Check if we should use vivado_hls or vitis_hls
if(Vitis_MAJOR_VERSION GREATER 2020 OR Vitis_MAJOR_VERSION EQUAL 2020)
  # vitis_hls is used internally for building kernels starting from 2020.1. 
  set(Vitis_USE_VITIS_HLS ON CACHE BOOL "Use vitis_hls instead of vivado_hls." FORCE)
  find_program(VITIS_HLS NAMES vitis_hls vivado_hls PATHS
               ${VITIS_ROOT}/../../Vitis_HLS/${Vitis_VERSION}/bin
               ${VITIS_ROOT}/bin
               ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/bin
               ${VITIS_ROOT}/Vivado_HLS/bin NO_DEFAULT_PATH)
else()
  if(NOT DEFINED Vitis_USE_VITIS_HLS OR Vitis_USE_VITIS_HLS)
    message(WARNING "vitis_hls used from Vitis 2020.1 and onwards introduces breaking changes to hls::stream. Please pass -D__VITIS_HLS__ or -D__VIVADO_HLS__ in your synthesis script depending on which tool you are using to always use a working version.")
  endif()
  # Prior to 2020.1, vivado_hls from the Vivado installation is used.
  set(Vitis_USE_VITIS_HLS OFF CACHE BOOL "Use vitis_hls instead of vivado_hls." FORCE)
  set(VITIS_HLS ${Vitis_VIVADO_HLS})
endif()
mark_as_advanced(VITIS_HLS)
set(Vitis_HLS ${VITIS_HLS} CACHE STRING "Path to HLS executable." FORCE)

find_program(Vitis_VIVADO vivado PATHS
             ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/bin
             ${VITIS_ROOT}/Vivado/bin NO_DEFAULT_PATH)

find_path(Vitis_HLS_INCLUDE_DIR hls_stream.h PATHS
          ${VITIS_ROOT}/../../Vitis_HLS/${Vitis_VERSION}/include
          ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/include
          ${VITIS_ROOT}/include
          ${VITIS_ROOT}/Vivado_HLS/include
          NO_DEFAULT_PATH)
mark_as_advanced(Vitis_HLS_INCLUDE_DIR)

if(Vitis_VPP OR (Vitis_MAJOR_VERSION GREATER 2018) OR
   (Vitis_MAJOR_VERSION EQUAL 2018 AND Vitis_MINOR_VERSION GREATER 2))
  set(VITIS_USE_XRT TRUE)
else()
  set(VITIS_USE_XRT FALSE)
endif()
set(Vitis_USE_XRT ${VITIS_USE_XRT} CACHE STRING "Use XRT as runtime. Otherwise, use SDAccel/SDx OpenCL runtime." FORCE)

# Currently only x86 support

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")

  #----------------------------------------------------------------------------
  # Floating point library
  #----------------------------------------------------------------------------

  find_library(Vitis_FLOATING_POINT_LIBRARY Ip_floating_point_v7_0_bitacc_cmodel
               PATHS
               ${VITIS_ROOT}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT}/../../Vitis_HLS/${Vitis_VERSION}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT}/Vivado_HLS/lnx64/tools/fpo_v7_0)
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

    set(VITIS_RUNTIME_DIR ${VITIS_ROOT}/runtime)

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

    if(NOT DEFINED XRT_ROOT)

      find_path(XRT_SEARCH_PATH libxilinxopencl.so 
                PATHS ENV XILINX_XRT
                      /opt/xilinx/xrt /opt/Xilinx/xrt
                      /tools/Xilinx/xrt /tools/xilinx/xrt
                PATH_SUFFIXES lib)
      get_filename_component(XRT_ROOT ${XRT_SEARCH_PATH} DIRECTORY) 
      mark_as_advanced(XRT_SEARCH_PATH)

      if(NOT XRT_SEARCH_PATH)
        message(FATAL_ERROR "The Xilinx Runtime (XRT) was not found. You can specify the XRT directory with the XRT_ROOT variable or set the XILINX_XRT environment variable.")
      endif()

      message(STATUS "Found Xilinx Runtime (XRT): ${XRT_ROOT}")

    else() 

      message(STATUS "Using user defined Xilinx Runtime (XRT) directory \"${XRT_ROOT}\".")

    endif()

    set(VITIS_RUNTIME_DIR ${XRT_ROOT})

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
        CACHE STRING "OpenCL runtime libraries." FORCE)
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
        CACHE STRING "Vitis include directories." FORCE)
  endif()

else()

  message(WARNING "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")

endif()

# Function to convert each path in a list to an absolute path, if it isn't already
function(hlslib_make_paths_absolute OUTPUT_FILES)
  string(REPLACE " " ";" OUTPUT_FILES ${OUTPUT_FILES})
  set(_OUTPUT_FILES)
  foreach(KERNEL_FILE_PATH ${ARGN})
    if(NOT IS_ABSOLUTE ${KERNEL_FILE_PATH})
      set(_KERNEL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL_FILE_PATH})
      if(NOT EXISTS ${_KERNEL_FILE_PATH})
        message(FATAL_ERROR "File ${KERNEL_FILE_PATH} not found.")
      endif()
      set(KERNEL_FILE_PATH ${_KERNEL_FILE_PATH})
    endif()
    set(_OUTPUT_FILES ${_OUTPUT_FILES} ${KERNEL_FILE_PATH})
  endforeach()
  set(${OUTPUT_FILES} ${_OUTPUT_FILES} PARENT_SCOPE)
endfunction()

# The name of the kernel is expected to match the target name. If it does not,
# the kernel name can be passed separately with the KERNEL keyword.
function(add_vitis_kernel
         KERNEL_TARGET)

  # Keyword arguments
  cmake_parse_arguments(
      KERNEL
      ""
      "KERNEL"
      "FILES;COMPUTE_UNITS;DEPENDS;INCLUDE_DIRS;HLS_FLAGS;HLS_CONFIG;COMPILE_FLAGS;PORT_MAPPING;SLR_MAPPING"
      ${ARGN})

  # Verify that input is sane
  if(NOT KERNEL_FILES)
    message(FATAL_ERROR "Must pass kernel file(s) to add_vitis_kernel using the FILES keyword.")
  endif()
  hlslib_make_paths_absolute(KERNEL_FILES ${KERNEL_FILES})

  # Convert non-target dependencies to absolute paths
  string(REPLACE " " ";" KERNEL_DEPENDS "${KERNEL_DEPENDS}")
  unset(_KERNEL_DEPENDS)
  foreach(DEP ${KERNEL_DEPENDS})
    if(NOT TARGET ${DEP}) 
      hlslib_make_paths_absolute(DEP ${DEP})
    endif()
    set(_KERNEL_DEPENDS ${_KERNEL_DEPENDS} ${DEP})
  endforeach()
  set(KERNEL_DEPENDS ${KERNEL_FILES} ${_KERNEL_DEPENDS})

  # Create the target that will carry properties. Adding the depends here does not actually work, so we have to store
  # them as a property, retrieve them later, and add them manually to each target
  add_custom_target(${KERNEL_TARGET} DEPENDS ${KERNEL_DEPENDS})

  # Use the target name as the kernel name if the kernel name hasn't been
  # explicitly passed
  if(DEFINED KERNEL_KERNEL)
    set(KERNEL_NAME ${KERNEL_KERNEL})
  else()
    set(KERNEL_NAME ${KERNEL_TARGET})
  endif()

  # Default the number of compute units
  if(NOT KERNEL_COMPUTE_UNITS)
    set(KERNEL_COMPUTE_UNITS 1)
  endif()
  if(${KERNEL_COMPUTE_UNITS} GREATER 1)
    set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.nk ${KERNEL_NAME}:${KERNEL_COMPUTE_UNITS}")
  endif()

  # Specify port mapping
  string(REPLACE " " ";" KERNEL_PORT_MAPPING "${KERNEL_PORT_MAPPING}")
  foreach(MAPPING ${KERNEL_PORT_MAPPING})
    string(REGEX MATCH "[A-Za-z0-9_]+\\.[^: \t\n]+:[^: \t\n]+" HAS_KERNEL_NAME ${MAPPING})
    if(HAS_KERNEL_NAME)
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.sp ${MAPPING}") 
    else()
      string(REGEX MATCH "[^: \t\n]+:[^: \t\n]+" IS_MEMORY_BANK ${MAPPING})
      if(IS_MEMORY_BANK)
        set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.sp ${KERNEL_NAME}_1.${MAPPING}") 
      else()
        message(FATAL_ERROR "Unrecognized port mapping \"${MAPPING}\".")
      endif()
    endif()
  endforeach()

  # Specify SLR mapping
  string(REPLACE " " ";" KERNEL_SLR_MAPPING "${KERNEL_SLR_MAPPING}")
  foreach(MAPPING ${KERNEL_SLR_MAPPING})
    string(REGEX MATCH "[A-Za-z0-9_]+:[^: \t\n]+" HAS_KERNEL_NAME ${MAPPING})
    if(HAS_KERNEL_NAME)
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.slr ${MAPPING}") 
    else()
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.slr ${KERNEL_NAME}_1:${MAPPING}") 
    endif()
  endforeach()

  # Mandatory flags for HLS when building kernels that use hlslib
  string(FIND "${KERNEL_HLS_FLAGS}" "-DHLSLIB_SYNTHESIS" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DHLSLIB_SYNTHESIS")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-DHLSLIB_XILINX" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DHLSLIB_XILINX")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-std=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -std=c++11")
  endif()

  # Pass the Vitis version to HLS
  string(FIND "${KERNEL_HLS_FLAGS}" "-DVITIS_MAJOR_VERSION=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DVITIS_MAJOR_VERSION=${Vitis_MAJOR_VERSION}")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-DVITIS_MINOR_VERSION=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DVITIS_MINOR_VERSION=${Vitis_MINOR_VERSION}")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-DVITIS_VERSION=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DVITIS_VERSION=${Vitis_VERSION}")
  endif()

  # Tell hlslib whether we're using Vitis HLS or Vivado HLS
  if(NOT Vitis_USE_VITIS_HLS)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -D__VIVADO_HLS__")
  else()
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -D__VITIS_HLS__")
  endif()

  # Add additional include directories specified
  string(REPLACE " " ";" KERNEL_INCLUDE_DIRS "${KERNEL_INCLUDE_DIRS}")
  hlslib_make_paths_absolute(KERNEL_INCLUDE_DIRS ${KERNEL_INCLUDE_DIRS})
  foreach(INCLUDE_DIR ${KERNEL_INCLUDE_DIRS})
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -I${INCLUDE_DIR}")
  endforeach()

  # Clean up HLS flags to make sure they use string syntax (not list syntax),
  # and that there are no superfluous spaces.
  string(REGEX REPLACE ";|[ \t\r\n][ \t\r\n]+" " " KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS}")
  string(STRIP "${KERNEL_HLS_FLAGS}" KERNEL_HLS_FLAGS)

  # Pass variables the program target through properties
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_FILES "${KERNEL_FILES}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_COMPUTE_UNITS "${KERNEL_COMPUTE_UNITS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_NAME "${KERNEL_NAME}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES HLS_FLAGS "${KERNEL_HLS_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES LINK_FLAGS "${KERNEL_LINK_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES HLS_CONFIG "${KERNEL_HLS_CONFIG}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES DEPENDS "${KERNEL_DEPENDS}")

endfunction()

function(add_vitis_program
         PROGRAM_TARGET
         PROGRAM_PLATFORM)

  # Keyword arguments
  cmake_parse_arguments(
      PROGRAM
      ""
      "CLOCK;CONFIG;SAVE_TEMPS;DEBUGGING;PROFILING"
      "KERNELS;BUILD_FLAGS;LINK_FLAGS;CONNECTIVITY;DEPENDS"
      ${ARGN})

  # Verify that input is sane
  if(NOT PROGRAM_KERNELS)
    if(TARGET ${PROGRAM_TARGET})
      # If no kernels were specified, try using the same target name
      set(PROGRAM_KERNELS ${PROGRAM_TARGET})
    else()
      # Otherwise we have to give up
      message(FATAL_ERROR "Must pass kernel targets created with add_vitis_kernel to add_vitis_program using the KERNELS keyword.")
    endif()
  endif()
  foreach(KERNEL ${PROGRAM_KERNELS})
    if(NOT TARGET ${KERNEL})
      message(FATAL_ERROR "${KERNEL} is not a valid Vitis kernel. Kernels must have been created with the add_vitis_kernel function.")
    endif()
  endforeach()

  # Convert non-target dependencies to absolute paths
  string(REPLACE " " ";" PROGRAM_DEPENDS "${PROGRAM_DEPENDS}")
  unset(_PROGRAM_DEPENDS)
  foreach(DEP ${PROGRAM_DEPENDS})
    if(NOT TARGET ${DEP}) 
      hlslib_make_paths_absolute(DEP ${DEP})
    endif()
    set(_PROGRAM_DEPENDS ${_PROGRAM_DEPENDS} ${DEP})
  endforeach()
  set(PROGRAM_DEPENDS ${PROGRAM_DEPENDS} ${_PROGRAM_DEPENDS})

  # Recover the part name used by the given platform
  if(NOT PROGRAM_PLATFORM_PART OR NOT "${${PROGRAM_TARGET}_PLATFORM}" STREQUAL "${PROGRAM_PLATFORM}")
    message(STATUS "Querying Vitis platform for ${PROGRAM_TARGET}.")
    execute_process(COMMAND ${Vitis_PLATFORMINFO} --platform ${PROGRAM_PLATFORM} -jhardwarePlatform.board.part
                    OUTPUT_VARIABLE PLATFORM_PART)
    string(STRIP "${PLATFORM_PART}" PLATFORM_PART)
    if(PLATFORM_PART)
      set(PROGRAM_PLATFORM_PART ${PLATFORM_PART} CACHE INTERNAL "")
    endif()
  endif()
  if(NOT PROGRAM_PLATFORM_PART)
    message(WARNING "Xilinx platform ${PROGRAM_PLATFORM} was not found. Please consult \"${Vitis_PLATFORMINFO} -l\" for a list of installed platforms.")
  else()
    # Cache this so we don't have to rerun platforminfo if the platform didn't change
    set(${PROGRAM_TARGET}_PLATFORM ${PROGRAM_PLATFORM} CACHE INTERNAL "")
  endif()

  # Specify the platform
  set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --platform ${PROGRAM_PLATFORM}")

  # Augment with frequency flag if specified
  if(PROGRAM_CLOCK)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --kernel_frequency ${PROGRAM_CLOCK}")
  endif()

  # Pass config file if specified
  if(PROGRAM_CONFIG)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --config ${PROGRAM_CONFIG}")
  endif()

  # Save temporaries if instructed to do so
  if(PROGRAM_SAVE_TEMPS)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --save-temps")
  endif()

  # Default to -O3 if no other optimization flag is passed
  string(FIND "${PROGRAM_BUILD_FLAGS} ${PROGRAM_LINK_FLAGS}" "-O" FOUND_SHORT)
  string(FIND "${PROGRAM_BUILD_FLAGS} ${PROGRAM_LINK_FLAGS}" "--optimize" FOUND_LONG)
  if(FOUND_SHORT EQUAL -1 AND FOUND_LONG EQUAL -1)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} -O3")
  endif()

  # Optional profiling flags
  if(PROGRAM_PROFILING)
    set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} --profile.data all:all:all --profile.exec all:all --profile.stall all:all")
  endif()

  # Optional debugging flags
  if(PROGRAM_DEBUGGING)
    foreach(KERNEL ${PROGRAM_KERNELS})
      # Append _1 to match the Vitis convention (only supports single compute unit kernels)
      get_target_property(KERNEL_NAME ${KERNEL} KERNEL_NAME)
      set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} --debug --debug.chipscope ${KERNEL_NAME}_1")
    endforeach()
  endif()

  # Mapping between streaming interfaces on kernels
  string(REPLACE " " ";" PROGRAM_CONNECTIVITY "${PROGRAM_CONNECTIVITY}")
  foreach(MAPPING ${PROGRAM_CONNECTIVITY})
    string(REGEX MATCH "[^\\.: \t\n]+\\.[^\\.: \t\n]+:[^\\.: \t\n]+\\.[^\\.: \t\n]+" IS_MAPPING ${MAPPING})
    if(IS_MAPPING)
      set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} --connectivity.sc ${MAPPING}") 
    else()
      message(FATAL_ERROR "Unrecognized kernel mapping \"${MAPPING}\".")
    endif()
  endforeach()

  # Clean up build flags by removing superfluous whitespace and convert from
  # string syntax to list syntax,
  string(REGEX REPLACE "[ \t\r\n][ \t\r\n]+" " " PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS}")
  string(STRIP "${PROGRAM_BUILD_FLAGS}" PROGRAM_BUILD_FLAGS)
  string(REGEX REPLACE " " ";" PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS}")

  unset(PROGRAM_XO_FILES_SW_EMU)
  unset(PROGRAM_XO_FILES_HW_EMU)
  unset(PROGRAM_XO_FILES_HW)

  foreach(KERNEL ${PROGRAM_KERNELS})
  
    get_target_property(KERNEL_NAME ${KERNEL} KERNEL_NAME)
    get_target_property(KERNEL_FILES ${KERNEL} KERNEL_FILES)
    get_target_property(KERNEL_HLS_FLAGS ${KERNEL} HLS_FLAGS)
    get_target_property(KERNEL_HLS_CONFIG ${KERNEL} HLS_CONFIG)
    get_target_property(KERNEL_COMPILE_FLAGS ${KERNEL} COMPILE_FLAGS)
    get_target_property(KERNEL_LINK_FLAGS ${KERNEL} LINK_FLAGS)
    get_target_property(KERNEL_DEPENDS ${KERNEL} DEPENDS)

    set(KERNEL_COMPILE_FLAGS "${PROGRAM_COMPILE_FLAGS} ${KERNEL_COMPILE_FLAGS} --advanced.prop kernel.${KERNEL_NAME}.kernel_flags=\"${KERNEL_HLS_FLAGS}\"") 
    set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} ${KERNEL_LINK_FLAGS}")

    # If HLS TCL config commands are provided, generate a file that can be passed
    if(KERNEL_HLS_CONFIG)
      set(HLS_TCL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}_config.tcl)
      file(WRITE ${HLS_TCL_FILE} "${KERNEL_HLS_CONFIG}")
      set(KERNEL_COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS} --hls.pre_tcl ${HLS_TCL_FILE}")
    endif()
    
    # Canonicalize flags
    string(REGEX REPLACE "[ \t\r\n][ \t\r\n]+" " " KERNEL_COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS}")
    string(STRIP "${KERNEL_COMPILE_FLAGS}" KERNEL_COMPILE_FLAGS)
    string(REGEX REPLACE " " ";" KERNEL_COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS}")

    # Software emulation target
    set(KERNEL_XO_FILE_SW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_sw_emu.xo)
    set(PROGRAM_XO_FILES_SW_EMU ${PROGRAM_XO_FILES_SW_EMU} ${KERNEL_XO_FILE_SW_EMU})
    add_custom_command(
      OUTPUT ${KERNEL_XO_FILE_SW_EMU}
      COMMENT "Compiling ${KERNEL} for software emulation."
      COMMAND ${CMAKE_COMMAND} -E env
              XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
              ${Vitis_COMPILER} --compile --target sw_emu
              --kernel ${KERNEL_NAME}
              ${KERNEL_COMPILE_FLAGS} 
              ${PROGRAM_BUILD_FLAGS}
              ${KERNEL_FILES}
              --output ${KERNEL_XO_FILE_SW_EMU}
      DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
    add_custom_target(compile_${KERNEL}_sw_emu DEPENDS
                      ${KERNEL_XO_FILE_SW_EMU})
    if(NOT TARGET compile_sw_emu)
      add_custom_target(compile_sw_emu COMMENT "Compiling software emulation targets."
                        DEPENDS compile_${KERNEL}_sw_emu)
    else()
      add_dependencies(compile_sw_emu compile_${KERNEL}_sw_emu)
    endif()

    # Hardware emulation target
    set(KERNEL_XO_FILE_HW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_hw_emu.xo)
    set(PROGRAM_XO_FILES_HW_EMU ${PROGRAM_XO_FILES_HW_EMU} ${KERNEL_XO_FILE_HW_EMU})
    add_custom_command(
      OUTPUT ${KERNEL_XO_FILE_HW_EMU}
      COMMENT "Compiling ${KERNEL} for hardware emulation."
      COMMAND ${CMAKE_COMMAND} -E env
              XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
              ${Vitis_COMPILER} --compile --target hw_emu
              --kernel ${KERNEL_NAME}
              ${KERNEL_COMPILE_FLAGS} 
              ${PROGRAM_BUILD_FLAGS}
              ${KERNEL_FILES}
              --output ${KERNEL_XO_FILE_HW_EMU}
      DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
    add_custom_target(compile_${KERNEL}_hw_emu DEPENDS
                      ${KERNEL_XO_FILE_HW_EMU})
    if(NOT TARGET compile_hw_emu)
      add_custom_target(compile_hw_emu COMMENT "Compiling hardware emulation targets."
                        DEPENDS compile_${KERNEL}_hw_emu)
    else()
      add_dependencies(compile_hw_emu compile_${KERNEL}_hw_emu)
    endif()

    # Hardware target
    set(KERNEL_XO_FILE_HW ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_hw.xo)
    set(PROGRAM_XO_FILES_HW ${PROGRAM_XO_FILES_HW} ${KERNEL_XO_FILE_HW})
    add_custom_command(
      OUTPUT ${KERNEL_XO_FILE_HW}
      COMMENT "Compiling ${KERNEL_NAME} for hardware."
      COMMAND ${CMAKE_COMMAND} -E env
              XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
              ${Vitis_COMPILER} --compile --target hw
              --kernel ${KERNEL_NAME}
              ${KERNEL_COMPILE_FLAGS}
              ${PROGRAM_BUILD_FLAGS}
              ${KERNEL_FILES}
              --output ${KERNEL_XO_FILE_HW}
      DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
    add_custom_target(compile_${KERNEL}_hw DEPENDS
                      ${KERNEL_XO_FILE_HW})
    if(NOT TARGET compile_hw)
      add_custom_target(compile_hw COMMENT "Compiling hardware targets."
                        DEPENDS compile_${KERNEL}_hw)
    else()
      add_dependencies(compile_hw compile_${KERNEL}_hw)
    endif()

    if(PROGRAM_PLATFORM_PART)
      # Make separate synthesis target, which is faster to run than Vitis compile
      if(PROGRAM_CLOCK)
        set(KERNEL_HLS_TCL_CLOCK "create_clock -period ${PROGRAM_CLOCK}MHz -name default\n")
      endif()
      string(REPLACE ";" " " KERNEL_FILES "${KERNEL_FILES}") 
      file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}_synthesis.tcl
           "\
open_project ${KERNEL} \ 
open_solution -flow_target vitis ${PROGRAM_PLATFORM_PART} \ 
set_part ${PROGRAM_PLATFORM_PART} \ 
add_files -cflags \"${KERNEL_HLS_FLAGS}\" \"${KERNEL_FILES}\" \ 
set_top ${KERNEL_NAME} \ 
${KERNEL_HLS_TCL_CLOCK}\
${KERNEL_HLS_CONFIG} \ 
csynth_design \ 
exit")
      add_custom_command(OUTPUT ${KERNEL}/${PROGRAM_PLATFORM_PART}/${PROGRAM_PLATFORM_PART}.log
                         COMMENT "Running high-level synthesis for ${KERNEL}."
                         COMMAND ${Vitis_HLS} -f ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}_synthesis.tcl
                         # CMake does not seem to rerun the custom target if the target that it depends
                         # on is rerun, so we have to re-add the files as dependencies here.
                         DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
      add_custom_target(synthesize_${KERNEL} DEPENDS  
                        ${KERNEL}/${PROGRAM_PLATFORM_PART}/${PROGRAM_PLATFORM_PART}.log)
      set_property(TARGET synthesize_${KERNEL} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
                   ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL} vitis_hls.log)
      if(NOT TARGET synthesis)
        add_custom_target(synthesis COMMENT "Running high-level synthesis for Vitis kernels."
                          DEPENDS synthesize_${KERNEL})
      else()
        add_dependencies(synthesis synthesize_${KERNEL})
      endif()
    endif()

  endforeach()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/emconfig.json
    COMMENT "Generating emconfig.json file for hardware emulation."
    COMMAND ${VITIS_ROOT}/bin/emconfigutil --platform ${PROGRAM_PLATFORM})
  if(NOT TARGET ${PROGRAM_PLATFORM}_emconfig)
    add_custom_target(${PROGRAM_PLATFORM}_emconfig DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/emconfig.json)
  endif()

  # Canonicalize link flags
  string(REGEX REPLACE "[ \t\r\n][ \t\r\n]+" " " PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS}")
  string(STRIP "${PROGRAM_LINK_FLAGS}" PROGRAM_LINK_FLAGS)
  string(REGEX REPLACE " " ";" PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS}")

  # Software emulation target
  set(PROGRAM_XCLBIN_SW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}_sw_emu.xclbin)
  add_custom_command(
    OUTPUT ${PROGRAM_XCLBIN_SW_EMU}
    COMMENT "Linking ${PROGRAM_TARGET} for software emulation."
    COMMAND ${CMAKE_COMMAND} -E env
            XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
            ${Vitis_COMPILER} --link --target sw_emu
            ${PROGRAM_BUILD_FLAGS}
            ${PROGRAM_LINK_FLAGS}
            ${PROGRAM_XO_FILES_SW_EMU}
            --output ${PROGRAM_XCLBIN_SW_EMU}
    # Back up the binary in case it accidentally gets cleaned when outdated
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                       ${PROGRAM_XCLBIN_SW_EMU} ${PROGRAM_XCLBIN_SW_EMU}.bak
    DEPENDS ${PROGRAM_XO_FILES_SW_EMU}
            ${PROGRAM_PLATFORM}_emconfig
            ${PROGRAM_DEPENDS})
  add_custom_target(link_${PROGRAM_TARGET}_sw_emu DEPENDS ${PROGRAM_XCLBIN_SW_EMU})
  add_custom_target(${PROGRAM_TARGET}_sw_emu DEPENDS ${PROGRAM_XCLBIN_SW_EMU})

  # Hardware emulation target
  set(PROGRAM_XCLBIN_HW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}_hw_emu.xclbin)
  add_custom_command(
    OUTPUT ${PROGRAM_XCLBIN_HW_EMU}
    COMMENT "Linking ${PROGRAM_TARGET} for hardware emulation."
    COMMAND ${CMAKE_COMMAND} -E env
            XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
            ${Vitis_COMPILER} --link --target hw_emu
            ${PROGRAM_BUILD_FLAGS}
            ${PROGRAM_LINK_FLAGS}
            ${PROGRAM_XO_FILES_HW_EMU}
            --output ${PROGRAM_XCLBIN_HW_EMU}
    # Back up the binary in case it accidentally gets cleaned when outdated
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                       ${PROGRAM_XCLBIN_HW_EMU} ${PROGRAM_XCLBIN_HW_EMU}.bak
    DEPENDS ${PROGRAM_XO_FILES_HW_EMU}
            ${PROGRAM_PLATFORM}_emconfig
            ${PROGRAM_DEPENDS})
  add_custom_target(link_${PROGRAM_TARGET}_hw_emu DEPENDS ${PROGRAM_XCLBIN_HW_EMU})
  add_custom_target(${PROGRAM_TARGET}_hw_emu DEPENDS ${PROGRAM_XCLBIN_HW_EMU})

  # Hardware target
  set(PROGRAM_XCLBIN_HW ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}_hw.xclbin)
  add_custom_command(
    OUTPUT ${PROGRAM_XCLBIN_HW}
    COMMENT "Linking ${PROGRAM_TARGET} for hardware."
    COMMAND ${CMAKE_COMMAND} -E env
            XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
            ${Vitis_COMPILER} --link --target hw
            ${PROGRAM_BUILD_FLAGS}
            ${PROGRAM_LINK_FLAGS}
            ${PROGRAM_XO_FILES_HW}
            --output ${PROGRAM_XCLBIN_HW}
    # Back up the binary in case it accidentally gets cleaned when outdated
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                       ${PROGRAM_XCLBIN_HW} ${PROGRAM_XCLBIN_HW}.bak
    DEPENDS ${PROGRAM_XO_FILES_HW}
            ${PROGRAM_DEPENDS})
  add_custom_target(link_${PROGRAM_TARGET}_hw DEPENDS ${PROGRAM_XCLBIN_HW})
  add_custom_target(${PROGRAM_TARGET}_hw DEPENDS link_${PROGRAM_TARGET}_hw)

  # Shorthand to compile kernels, so user can just run "make hw" or "make hw_emu"
  if(NOT TARGET link_sw_emu)
    add_custom_target(link_sw_emu COMMENT "Linking software emulation targets."
                      DEPENDS link_${PROGRAM_TARGET}_sw_emu)
  else()
    add_dependencies(link_sw_emu link_${PROGRAM_TARGET}_sw_emu)
  endif()
  if(NOT TARGET link_hw_emu)
    add_custom_target(link_hw_emu COMMENT "Linking hardware emulation targets."
                      DEPENDS link_${PROGRAM_TARGET}_hw_emu)
  else()
    add_dependencies(link_hw_emu link_${PROGRAM_TARGET}_hw_emu)
  endif()
  if(NOT TARGET link_hw)
    add_custom_target(link_hw COMMENT "Linking hardware targets."
                      DEPENDS link_${PROGRAM_TARGET}_hw)
  else()
    add_dependencies(link_hw link_${PROGRAM_TARGET}_hw)
  endif()
  if(NOT TARGET sw_emu)
    add_custom_target(sw_emu COMMENT "Building software emulation targets."
                      DEPENDS link_sw_emu)
  else()
    add_dependencies(sw_emu ${PROGRAM_TARGET}_sw_emu)
  endif()
  if(NOT TARGET hw_emu)
    add_custom_target(hw_emu COMMENT "Building hardware emulation targets."
                      DEPENDS link_hw_emu)
  else()
    add_dependencies(hw_emu ${PROGRAM_TARGET}_hw_emu)
  endif()
  if(NOT TARGET hw)
    add_custom_target(hw COMMENT "Building hardware targets."
                      DEPENDS link_hw)
  else()
    add_dependencies(hw ${PROGRAM_TARGET}_hw)
  endif()

  # Add Xilinx build directory to clean target
  set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/_x)

endfunction()

set(Vitis_EXPORTS
    Vitis_COMPILER
    Vitis_HLS
    Vitis_INCLUDE_DIRS
    Vitis_LIBRARIES
    Vitis_FLOATING_POINT_LIBRARY 
    Vitis_VERSION
    Vitis_MAJOR_VERSION
    Vitis_MINOR_VERSION
    Vitis_PLATFORMINFO)
mark_as_advanced(Vitis_EXPORTS)

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set Vitis_FOUND to TRUE if all
# listed variables were found.
find_package_handle_standard_args(Vitis DEFAULT_MSG ${Vitis_EXPORTS})
