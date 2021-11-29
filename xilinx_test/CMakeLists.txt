# User options
set(HLSLIB_PART_NAME "xcvu9p-flgb2104-2-i" CACHE STRING "Part name for HLS.")
set(HLSLIB_DSA_NAME "xilinx_u250_gen3x16_xdma_3_1_202020_1" CACHE STRING "DSA string for v++/xocc.")

include_directories(${Vitis_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -DHLSLIB_XILINX")
if(((${Vitis_MAJOR_VERSION} LESS 2018) AND (${Vitis_MINOR_VERSION} LESS 3)) OR ${Vitis_MAJOR_VERSION} LESS 2017)
  message(STATUS "Targeting legacy SDx.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHLSLIB_LEGACY_SDX=1")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHLSLIB_LEGACY_SDX=0")
endif()

# Unit tests 
add_library(catch test/Catch.cpp)
add_executable(TestDataPack test/TestDataPack.cpp)
target_link_libraries(TestDataPack catch)
add_test(TestDataPack TestDataPack)
add_executable(TestReduce test/TestReduce.cpp)
target_link_libraries(TestReduce catch)
add_test(TestReduce TestReduce)
add_executable(TestFlatten test/TestFlatten.cpp)
target_link_libraries(TestFlatten catch)
add_test(TestFlatten TestFlatten)
add_executable(TestHBMandBlockCopySimulation test/TestHBMandBlockCopySimulation.cpp)
target_link_libraries(TestHBMandBlockCopySimulation ${Vitis_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} catch)
add_test(TestHBMandBlockCopySimulation TestHBMandBlockCopySimulation)

function(add_hls_test KERNEL_NAME)
  cmake_parse_arguments(
      KERNEL
      ""
      "CONFIG"
      "HLS_FLAGS"
      ${ARGN})
  # Abuse add_vitis_kernel to generate synthesis target, even if not all of the
  # tests would actually be valid OpenCL kernels
  add_vitis_kernel(${KERNEL_NAME}
                   FILES kernels/${KERNEL_NAME}.cpp
                   INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
                                ${CMAKE_SOURCE_DIR}/include
                   HLS_FLAGS "${KERNEL_HLS_FLAGS}")
  add_vitis_program(${KERNEL_NAME} ${HLSLIB_DSA_NAME}
                    CONFIG "${KERNEL_CONFIG}")
  add_test(NAME Test${KERNEL_NAME}Synthesis COMMAND ${CMAKE_COMMAND}
           --build ${CMAKE_CURRENT_BINARY_DIR} --target synthesize_${KERNEL_NAME}) 
endfunction()

add_hls_test("MultiStageAdd")
add_hls_test("Reduce")
add_hls_test("AccumulateFloat" HLS_FLAGS "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
add_hls_test("AccumulateInt" HLS_FLAGS "-DHLSLIB_COMPILE_ACCUMULATE_INT")
add_hls_test("ShiftRegister")
add_hls_test("StreamAPI")
add_hls_test("Subflow")
add_hls_test("HBMandBlockCopy" CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configs/hbmkernel.cfg)
add_hls_test("TightPacking")

# Test mapping of interfaces to DRAM banks
add_vitis_kernel(DDRMappingKernel
                 FILES kernels/DDRExplicit.cpp
                 KERNEL DDRExplicit
                 INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
                              ${CMAKE_SOURCE_DIR}/include
                 HLS_CONFIG "config_compile -pipeline_style frp"
                 PORT_MAPPING "ddr0:DDR[0]" "ddr1:DDR[1]")
add_vitis_program(DDRMapping ${HLSLIB_DSA_NAME}
                  KERNELS DDRMappingKernel)
add_test(NAME TestDDRMapping COMMAND ${CMAKE_COMMAND}
         --build ${CMAKE_CURRENT_BINARY_DIR} --target DDRMapping_hw_emu)

# Test linking multiple kernels
add_vitis_kernel(FirstKernel
                 FILES kernels/MultipleKernels.cpp
                 INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
                              ${CMAKE_SOURCE_DIR}/include)
add_vitis_kernel(SecondKernel
                 FILES kernels/MultipleKernels.cpp
                 INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
                              ${CMAKE_SOURCE_DIR}/include)
add_vitis_program(MultipleKernels ${HLSLIB_DSA_NAME}
                  KERNELS FirstKernel SecondKernel
                  CONNECTIVITY "FirstKernel_1.stream:SecondKernel_1.stream")

# Test kernels in software
find_package(Threads)
if(Threads_FOUND)
  add_executable(TestStream test/TestStream.cpp kernels/MultiStageAdd.cpp)
  target_link_libraries(TestStream ${CMAKE_THREAD_LIBS_INIT} catch)
  add_test(TestStream TestStream)
  target_compile_options(TestStream PRIVATE "-DHLSLIB_STREAM_SYNCHRONIZE")
  add_executable(TestAccumulateFloat test/TestAccumulate.cpp kernels/AccumulateFloat.cpp)
  target_compile_options(TestAccumulateFloat PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_FLOAT")
  target_link_libraries(TestAccumulateFloat ${CMAKE_THREAD_LIBS_INIT} catch)
  add_test(TestAccumulateFloat TestAccumulateFloat)
  add_executable(TestAccumulateInt test/TestAccumulate.cpp kernels/AccumulateInt.cpp)
  target_compile_options(TestAccumulateInt PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_INT")
  target_link_libraries(TestAccumulateInt ${CMAKE_THREAD_LIBS_INIT} catch)
  add_test(TestAccumulateInt TestAccumulateInt)
  add_executable(TestSimulationForwarding test/TestSimulationForwarding.cpp)
  target_compile_options(TestSimulationForwarding PRIVATE "-DHLSLIB_COMPILE_ACCUMULATE_INT")
  target_link_libraries(TestSimulationForwarding ${CMAKE_THREAD_LIBS_INIT} catch)
  add_test(TestSimulationForwarding TestSimulationForwarding)
  add_executable(TestShiftRegister test/TestShiftRegister.cpp)
  target_link_libraries(TestShiftRegister ${CMAKE_THREAD_LIBS_INIT} catch)
  add_test(TestShiftRegister TestShiftRegister)
  add_executable(TestSubflow test/TestSubflow.cpp kernels/Subflow.cpp)
  target_link_libraries(TestSubflow ${CMAKE_THREAD_LIBS_INIT} catch)
  add_test(TestSubflow TestSubflow)
  add_executable(TestMultipleKernelsHardwareEmulation test/TestMultipleKernels.cpp kernels/MultipleKernels.cpp)
  add_dependencies(TestMultipleKernelsHardwareEmulation MultipleKernels_hw_emu)
  target_link_libraries(TestMultipleKernelsHardwareEmulation ${Vitis_LIBRARIES} catch ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME TestMultipleKernelsHardwareEmulation 
           COMMAND ${CMAKE_COMMAND} -E env
           XCL_EMULATION_MODE="hw_emu"
           ${CMAKE_CURRENT_BINARY_DIR}/TestMultipleKernelsHardwareEmulation)
  add_executable(TestMultipleKernelsSimulation test/TestMultipleKernels.cpp kernels/MultipleKernels.cpp)
  target_compile_options(TestMultipleKernelsSimulation PRIVATE -DHLSLIB_SIMULATE_OPENCL) 
  target_link_libraries(TestMultipleKernelsSimulation ${Vitis_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} catch)
  add_test(TestMultipleKernelsSimulation TestMultipleKernelsSimulation)
else()
  message(WARNING "Threads not found. Disabling multi-PE kernel tests.")
endif()
