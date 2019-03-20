# User options
set(HLSLIB_BOARD_NAME "a10gx" CACHE STRING "Board name for aoc.")

include_directories(SYSTEM ${IntelFPGAOpenCL_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -DHLSLIB_INTEL")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

function(opencl_target KERNEL_NAME)
  set(AOC_COMMAND
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    -I${CMAKE_SOURCE_DIR}/include
    -fp-relaxed
    -cl-no-signed-zeros
    -cl-fast-relaxed-math
    -cl-single-precision-constant
    -board=${HLSLIB_BOARD_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cl)
  add_custom_target(build_${KERNEL_NAME}_report
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} 
    -rtl -report)
  add_custom_target(build_${KERNEL_NAME}_emulator
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -march=emulator
    -emulator-channel-depth-model=strict
    -o ${KERNEL_NAME}_emulator.aocx)
  add_custom_target(build_${KERNEL_NAME}_hardware
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -o ${KERNEL_NAME}_hardware.aocx)
  add_executable(Run${KERNEL_NAME}.exe host/Run${KERNEL_NAME}.cpp)
  target_link_libraries(Run${KERNEL_NAME}.exe ${IntelFPGAOpenCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  add_custom_target(run_${KERNEL_NAME}_hardware COMMAND ${CMAKE_CURRENT_BINARY_DIR}/Run${KERNEL_NAME}.exe hardware)
  add_custom_target(run_${KERNEL_NAME}_emulator COMMAND CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ${CMAKE_CURRENT_BINARY_DIR}/Run${KERNEL_NAME}.exe emulator)
endfunction()

opencl_target("Jacobi2D")