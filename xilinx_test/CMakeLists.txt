# User options
set(HLSLIB_PART_NAME "xcvu9p-flgb2104-2-i" CACHE STRING "Part name for HLS.")
set(HLSLIB_DSA_NAME "xilinx_vcu1525_dynamic_5_1" CACHE STRING
    "DSA string for xocc.")

include_directories(${SDAccel_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -DHLSLIB_XILINX")
if(((${SDAccel_MAJOR_VERSION} LESS 2018) AND (${SDAccel_MINOR_VERSION} LESS 3)) OR ${SDAccel_MAJOR_VERSION} LESS 2017)
  message(STATUS "Targeting legacy SDx.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHLSLIB_LEGACY_SDX=1")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHLSLIB_LEGACY_SDX=0")
endif()

# Unit tests 
add_executable(TestSDAccelHostCode test/TestSDAccelHostCode.cpp)
target_link_libraries(TestSDAccelHostCode ${SDAccel_LIBRARIES})
add_test(TestSDAccelHostCode TestSDAccelHostCode)
add_executable(TestDataPack test/TestDataPack.cpp)
target_link_libraries(TestDataPack)
add_test(TestDataPack TestDataPack)
add_executable(TestReduce test/TestReduce.cpp)
target_link_libraries(TestReduce)
add_test(TestReduce TestReduce)
add_executable(TestFlatten test/TestFlatten.cpp)
add_test(TestFlatten TestFlatten)

# Test kernels in software
find_package(Threads)
if(Threads_FOUND)
  add_executable(TestStream test/TestStream.cpp kernels/MultiStageAdd.cpp)
  target_link_libraries(TestStream ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestStream TestStream)
  target_compile_options(TestStream PRIVATE "-DHLSLIB_STREAM_SYNCHRONIZE")
  add_executable(TestAccumulateFloat test/TestAccumulate.cpp kernels/AccumulateFloat.cpp)
  target_compile_options(TestAccumulateFloat PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
  target_link_libraries(TestAccumulateFloat ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestAccumulateFloat TestAccumulateFloat)
  add_executable(TestAccumulateInt test/TestAccumulate.cpp kernels/AccumulateInt.cpp)
  target_compile_options(TestAccumulateInt PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_INT")
  target_link_libraries(TestAccumulateInt ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestAccumulateInt TestAccumulateInt)
  add_executable(TestSimulationForwarding test/TestSimulationForwarding.cpp)
  target_compile_options(TestSimulationForwarding PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_INT")
  target_link_libraries(TestSimulationForwarding ${CMAKE_THREAD_LIBS_INIT})
  add_test(TestSimulationForwarding TestSimulationForwarding)
else()
  message(WARNING "Threads not found. Disabling multi-PE kernel tests.")
endif()

set(SYNTHESIS_FLAGS "-DHLSLIB_SYNTHESIS -DHLSLIB_XILINX -std=c++11 -I${CMAKE_CURRENT_SOURCE_DIR}/include -I.")

function(synthesis_target KERNEL_NAME ADDITIONAL_FLAGS)
  set(HLSLIB_PROJECT_NAME "${KERNEL_NAME}")
  set(HLSLIB_SRC_SYNTHESIS "${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cpp")
  set(HLSLIB_ENTRY_FUNCTION "${KERNEL_NAME}")
  set(HLSLIB_TARGET_CLOCK "200")
  set(HLSLIB_SYNTHESIS_FLAGS ${SYNTHESIS_FLAGS})
  configure_file(scripts/Synthesis.tcl.in Synthesize${KERNEL_NAME}.tcl)
  add_custom_target(synthesize_${KERNEL_NAME}
    COMMAND ${SDAccel_VIVADO_HLS} -f Synthesize${KERNEL_NAME}.tcl) 
endfunction()

function(sdaccel_target KERNEL_NAME ADDITIONAL_FLAGS)
  set(HLSLIB_SYNTHESIS_FLAGS "${SYNTHESIS_FLAGS} ${ADDITIONAL_FLAGS}")
  set(XOCC_COMMAND
    -s
    -O3
    ${MM_XOCC_FLAGS}
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    -I${CMAKE_BINARY_DIR}
    --kernel ${KERNEL_NAME} 
    --platform ${HLSLIB_DSA_NAME}
    --xp prop:kernel.${KERNEL_NAME}.kernel_flags="${HLSLIB_SYNTHESIS_FLAGS}"
    --profile_kernel "data:all:all:all"
    --profile_kernel "stall:all:all"
    --profile_kernel "exec:all:all"
    --max_memory_ports all)
  if(((${SDAccel_MAJOR_VERSION} LESS 2018) AND (${SDAccel_MINOR_VERSION} LESS 3)) OR ${SDAccel_MAJOR_VERSION} LESS 2017)
    add_custom_target(build_${KERNEL_NAME}_hardware
      COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -t hw
      ${XOCC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cpp -o ${KERNEL_NAME}_hw.xclbin) 
    add_custom_target(build_${KERNEL_NAME}_hardware_emulation
      COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -t hw_emu -g
      ${XOCC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cpp -o ${KERNEL_NAME}_hw_emu.xclbin) 
  else()
    add_custom_target(compile_${KERNEL_NAME}_hardware
      COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -c -t hw
      ${XOCC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cpp -o ${KERNEL_NAME}_hw.xo) 
    add_custom_target(build_${KERNEL_NAME}_hardware
      COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -l -t hw
      ${XOCC_COMMAND} ${KERNEL_NAME}_hw.xo -o ${KERNEL_NAME}_hw.xclbin) 
    add_custom_target(compile_${KERNEL_NAME}_hardware_emulation
      COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -c -t hw_emu -g
      ${XOCC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cpp -o ${KERNEL_NAME}_hw_emu.xo) 
    add_custom_target(build_${KERNEL_NAME}_hardware_emulation
      COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -l -t hw_emu -g
      ${XOCC_COMMAND} ${KERNEL_NAME}_hw_emu.xo -o ${KERNEL_NAME}_hw_emu.xclbin) 
  endif()
  add_executable(Run${KERNEL_NAME}.exe host/Run${KERNEL_NAME}.cpp ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cpp)
  target_link_libraries(Run${KERNEL_NAME}.exe ${SDAccel_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  add_custom_target(run_${KERNEL_NAME}_hardware COMMAND ${CMAKE_BINARY_DIR}/Run${KERNEL_NAME}.exe hardware)
  add_custom_target(run_${KERNEL_NAME}_emulation COMMAND ${CMAKE_COMMAND} -E env XILINX_SDX=${SDACCEL_ROOT_DIR} XCL_EMULATION_MODE=hw_emu ${CMAKE_BINARY_DIR}/Run${KERNEL_NAME}.exe emulation)
  add_custom_target(run_${KERNEL_NAME}_simulation COMMAND ${CMAKE_COMMAND} -E env XCL_EMULATION_MODE=sw_emu XILINX_SDX=${SDACCEL_ROOT_DIR} ${CMAKE_BINARY_DIR}/Run${KERNEL_NAME}.exe simulation)
endfunction()

synthesis_target("Reduce" "")
synthesis_target("Flatten" "")
synthesis_target("MultiStageAdd" "")
sdaccel_target("MultiStageAdd" "")
synthesis_target("AccumulateFloat" "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
sdaccel_target("AccumulateFloat" "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
synthesis_target("AccumulateInt" "-DHLSLIB_COMPILE_ACCUMULATE_INT")
sdaccel_target("AccumulateInt" "-DHLSLIB_COMPILE_ACCUMULATE_INT")
