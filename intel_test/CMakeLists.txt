# User options
set(HLSLIB_BOARD_NAME "a10gx" CACHE STRING "Board name for aoc.")

include_directories(SYSTEM ${IntelFPGAOpenCL_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -DHLSLIB_INTEL")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

function(opencl_target KERNEL_NAME)
  set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cl)
  set(AOC_COMMAND
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    -I${CMAKE_SOURCE_DIR}/include
    -fp-relaxed
    -cl-no-signed-zeros
    -cl-fast-relaxed-math
    -cl-single-precision-constant
    -board=${HLSLIB_BOARD_NAME}
    ${KERNEL_FILE})
  add_custom_target(build_${KERNEL_NAME}_report
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} 
    -rtl -report)
  add_custom_command(
    OUTPUT ${KERNEL_NAME}_emulator.aocx
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -march=emulator
    -emulator-channel-depth-model=strict
    -o ${KERNEL_NAME}_emulator.aocx
    DEPENDS ${KERNEL_FILE})
  add_custom_target(build_${KERNEL_NAME}_emulator ALL DEPENDS
                    ${KERNEL_NAME}_emulator.aocx)
  add_custom_target(build_${KERNEL_NAME}_hardware
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -o ${KERNEL_NAME}_hardware.aocx)
  add_executable(Run${KERNEL_NAME}.exe host/Run${KERNEL_NAME}.cpp)
  target_link_libraries(Run${KERNEL_NAME}.exe ${IntelFPGAOpenCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  add_custom_target(run_${KERNEL_NAME}_hardware COMMAND ${CMAKE_CURRENT_BINARY_DIR}/Run${KERNEL_NAME}.exe hardware)
  add_custom_target(run_${KERNEL_NAME}_emulator COMMAND CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ${CMAKE_CURRENT_BINARY_DIR}/Run${KERNEL_NAME}.exe emulator)
endfunction()

opencl_target("Jacobi2D")
add_custom_target(run_Jacobi2D_hardware_oldapi_copy COMMAND ${CMAKE_CURRENT_BINARY_DIR}/RunJacobi2D.exe hardware oldapi_copy)
add_custom_target(run_Jacobi2D_emulator_oldapi_copy COMMAND CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ${CMAKE_CURRENT_BINARY_DIR}/RunJacobi2D.exe emulator oldapi_copy)
add_custom_target(run_Jacobi2D_hardware_newapi_copy COMMAND ${CMAKE_CURRENT_BINARY_DIR}/RunJacobi2D.exe hardware newapi_copy)
add_custom_target(run_Jacobi2D_emulator_newapi_copy COMMAND CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ${CMAKE_CURRENT_BINARY_DIR}/RunJacobi2D.exe emulator newapi_copy)
add_custom_target(run_Jacobi2D_hardware_newapi_notransfer COMMAND ${CMAKE_CURRENT_BINARY_DIR}/RunJacobi2D.exe hardware newapi_notransfer)
add_custom_target(run_Jacobi2D_emulator_newapi_notransfer COMMAND CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ${CMAKE_CURRENT_BINARY_DIR}/RunJacobi2D.exe emulator newapi_notransfer)
