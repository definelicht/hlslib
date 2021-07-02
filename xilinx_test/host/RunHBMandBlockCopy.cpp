/// @author    Jannis Widmer (widmerja@ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include "../../include/hlslib/xilinx/OpenCL.h"
#include <algorithm>
#include <assert.h>
#include <iostream>

constexpr int kDataSize = 1024;

template <typename T>
bool CheckBlockHasValue(const std::array<size_t, 3> blockOffsetSource,
                        const std::array<size_t, 3> copyBlockSize,
                        const std::array<size_t, 3> blockSizeSource,
                        const T *source, T checkValue) {
  size_t sourceSliceJmp =
      blockSizeSource[0] - copyBlockSize[0] +
      (blockSizeSource[1] - copyBlockSize[1]) * blockSizeSource[0];
  size_t srcindex =
      blockOffsetSource[0] + blockOffsetSource[1] * blockSizeSource[0] +
      blockOffsetSource[2] * blockSizeSource[1] * blockSizeSource[0];

  for (size_t sliceCounter = 0; sliceCounter < copyBlockSize[2];
       sliceCounter++) {
    size_t nextaddsource = 0;
    for (size_t rowCounter = 0; rowCounter < copyBlockSize[1]; rowCounter++) {
      srcindex += nextaddsource;
      for (size_t i = 0; i < copyBlockSize[0]; i++) {
        if (*(source + srcindex + i) != checkValue) {
          return false;
        }
      }
      nextaddsource = blockSizeSource[0];
    }
    srcindex += sourceSliceJmp + copyBlockSize[0];
  }

  return true;
}

int main(int argc, char **argv) {
  std::cout << "Initializing OpenCL context..." << std::endl;
  hlslib::ocl::Context context; // Whatever device was loaded needs support for HBM, e.g. alveo u280
  std::cout << "Done." << std::endl;

  // Handle input arguments
  std::string kUsage = "./RunHBMKernel [emulation|hardware]";
  if (argc != 2) {
    std::cout << kUsage << std::endl;
    return 1;
  }
  std::string mode_str(argv[1]);
  std::string kernel_path;
  if (mode_str == "emulation") {
    kernel_path = "HBMandBlockCopy_hw_emu.xclbin";
  } else if (mode_str == "hardware") {
    kernel_path = "HBMandBlockCopy_hw.xclbin";
  } else {
    std::cout << kUsage << std::endl;
    return 2;
  }

  std::cout << std::endl << "Loading Kernel" << std::endl;
  auto program = context.MakeProgram(kernel_path);

  std::cout << "Done" << std::endl
            << "Initializing memory..." << std::endl;

  std::array<size_t, 3> buf1Size = {5, 5, 5};
  std::array<size_t, 3> buf2Size = {3, 3, 3};
  size_t buf1Elems = buf1Size[0] * buf1Size[1] * buf1Size[2];
  size_t buf2Elems = buf2Size[0] * buf2Size[1] * buf2Size[2];
  std::vector<double, hlslib::ocl::AlignedAllocator<double, 4096>> memHostBuf1(
      buf1Elems);
  // in sw_emu xilinx seems to just take over the ranges provided to it as
  // device memory (during construction), so don't use memDeviceBuf2
  std::vector<double, hlslib::ocl::AlignedAllocator<double, 4096>>
      memDeviceBuf2(buf2Elems);
  std::vector<double, hlslib::ocl::AlignedAllocator<double, 4096>> memHostBuf2(
      buf2Elems);
  std::fill(memDeviceBuf2.begin(), memDeviceBuf2.end(), 3.0);
  std::fill(memHostBuf1.begin(), memHostBuf1.end(), 1.0);
  std::fill(memHostBuf2.begin(), memHostBuf2.end(), 2.0);

  auto memDevice1 = context.MakeBuffer<double, hlslib::ocl::Access::readWrite>(
      hlslib::ocl::StorageType::HBM, 20, buf1Elems);
  auto memDevice2 = context.MakeBuffer<double, hlslib::ocl::Access::readWrite>(
      hlslib::ocl::StorageType::HBM, 0, memDeviceBuf2.begin(),
      memDeviceBuf2.end());
  std::cout << " Done" << std::endl;

  std::cout << "Copy data around (Block copy test)" << std::endl;
  double *dptr = nullptr;

  std::array<size_t, 3> at1 = {0, 0, 0};
  std::array<size_t, 3> at2 = {0, 0, 0};
  std::array<size_t, 3> at3 = {5, 5, 5};
  memDevice1.CopyBlockFromHost(at1, at2, at3, buf1Size, buf1Size,
                               memHostBuf1.begin());
  memDevice2.CopyBlockToDevice({1, 1, 1}, {1, 1, 1}, {2, 2, 2}, buf2Size,
                               buf1Size, memDevice1);
  memDevice1.CopyBlockToHost({0, 0, 0}, {1, 1, 1}, {4, 4, 4}, buf1Size,
                             buf1Size, memHostBuf1.begin());

  dptr = memHostBuf1.data();
  assert(CheckBlockHasValue({0, 0, 0}, {2, 2, 2}, buf1Size, dptr, 3.0));
  assert(CheckBlockHasValue({0, 0, 2}, {5, 5, 3}, buf1Size, dptr, 1.0));
  assert(CheckBlockHasValue({2, 0, 0}, {3, 5, 5}, buf1Size, dptr, 1.0));
  assert(CheckBlockHasValue({0, 2, 0}, {5, 3, 5}, buf1Size, dptr, 1.0));

  std::vector<double, hlslib::ocl::AlignedAllocator<double, 4096>> tmpHost(
      buf1Elems);

  // Check CopyBlockFromHost
  std::fill(memHostBuf1.begin(), memHostBuf1.end(), 1.0);
  std::fill(tmpHost.begin(), tmpHost.end(), 6.0);
  memDevice1.CopyFromHost(memHostBuf1.begin());
  memDevice1.CopyBlockFromHost({0, 0, 0}, {0, 0, 0}, {4, 4, 4}, buf1Size,
                               buf1Size, tmpHost.begin());
  memDevice1.CopyToHost(memHostBuf1.begin());
  dptr = memHostBuf1.data();
  assert(CheckBlockHasValue({0, 0, 0}, {4, 4, 4}, buf1Size, dptr, 6.0));
  assert(CheckBlockHasValue({0, 0, 4}, {5, 5, 1}, buf1Size, dptr, 1.0));

  std::fill(memHostBuf1.begin(), memHostBuf1.end(), 1.0);
  std::fill(tmpHost.begin(), tmpHost.end(), 6.0);
  memDevice1.CopyFromHost(memHostBuf1.begin());
  tmpHost[5 * 5 + 5 + 1] = 8.0;
  memDevice1.CopyBlockFromHost({1, 1, 1}, {0, 0, 0}, {2, 2, 2}, buf1Size,
                               buf1Size, tmpHost.begin());
  memDevice1.CopyToHost(memHostBuf1.begin());
  dptr = memHostBuf1.data();
  assert(memHostBuf1[0] == 8.0);
  assert(CheckBlockHasValue({1, 0, 0}, {1, 2, 2}, buf1Size, dptr, 6.0));

  // Check CopyBlockToDevice
  std::fill(memHostBuf1.begin(), memHostBuf1.end(), 1.0);
  std::fill(memHostBuf2.begin(), memHostBuf2.end(), 3.0);
  memDevice1.CopyFromHost(memHostBuf1.begin());
  memDevice2.CopyFromHost(memHostBuf2.begin());
  memDevice2.CopyBlockToDevice({0, 1, 1}, {0, 1, 1}, {3, 2, 2}, buf2Size,
                               buf1Size, memDevice1);
  memDevice1.CopyToHost(memHostBuf1.begin());
  dptr = memHostBuf1.data();
  assert(CheckBlockHasValue({0, 1, 1}, {3, 2, 2}, buf1Size, dptr, 3.0));
  assert(CheckBlockHasValue({0, 0, 3}, {5, 5, 2}, buf1Size, dptr, 1.0));

  // Check CopyBlockToHost
  std::fill(memHostBuf1.begin(), memHostBuf1.end(), 11.0);
  // reuse values from previous test
  memDevice1.CopyBlockToHost({0, 0, 0}, {0, 1, 1}, {3, 2, 2}, buf1Size,
                             buf1Size, memHostBuf1.begin());
  memDevice1.CopyBlockToHost({3, 3, 3}, {0, 0, 3}, {2, 2, 2}, buf1Size,
                             buf1Size, memHostBuf1.begin());
  dptr = memHostBuf1.data();
  assert(CheckBlockHasValue({0, 0, 0}, {3, 2, 2}, buf1Size, dptr, 3.0));
  assert(CheckBlockHasValue({3, 3, 3}, {2, 2, 2}, buf1Size, dptr, 1.0));
  assert(CheckBlockHasValue({0, 0, 3}, {2, 2, 2}, buf1Size, dptr, 11.0));

  std::cout << "Done. " << std::endl;
  std::cout << "Executing HBMKernel (HBM and DDR test)" << std::endl;

  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> hbm0mem(kDataSize);
  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> ddr1mem(kDataSize);
  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> ddr0mem(
      kDataSize);
  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> hbm20mem(
      kDataSize);
  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> hbm31mem(
      kDataSize);
  std::vector<int, hlslib::ocl::AlignedAllocator<int, 4096>> ddrXmem(kDataSize);
  std::srand(0);
  auto genfun = []() { return std::rand() / (RAND_MAX / 1000); };
  std::generate(hbm0mem.begin(), hbm0mem.end(), genfun);
  std::generate(ddr1mem.begin(), ddr1mem.end(), genfun);
  std::generate(ddr0mem.begin(), ddr0mem.end(), genfun);
  std::generate(hbm20mem.begin(), hbm20mem.end(), genfun);
  std::generate(hbm31mem.begin(), hbm31mem.end(), genfun);

  auto hbm0device = context.MakeBuffer<int, hlslib::ocl::Access::read>(
      hlslib::ocl::StorageType::HBM, 0, hbm0mem.begin(), hbm0mem.end());
  auto ddr1device = context.MakeBuffer<int, hlslib::ocl::Access::read>(
      hlslib::ocl::StorageType::DDR, 1, ddr1mem.begin(), ddr1mem.end());
  auto ddr0device = context.MakeBuffer<int, hlslib::ocl::Access::read>(
      hlslib::ocl::StorageType::DDR, 0, ddr0mem.begin(), ddr0mem.end());
  auto hbm20device = context.MakeBuffer<int, hlslib::ocl::Access::read>(
      hlslib::ocl::StorageType::HBM, 20, hbm20mem.begin(), hbm20mem.end());
  auto hbm31device = context.MakeBuffer<int, hlslib::ocl::Access::read>(
      hlslib::ocl::StorageType::HBM, 31, (size_t)kDataSize);
  auto ddrXdevice = context.MakeBuffer<int, hlslib::ocl::Access::write>(
      hlslib::ocl::StorageType::DDR, -1, (size_t)kDataSize);

  hbm31device.CopyFromHost(hbm31mem.begin());

  auto kernel =
      program.MakeKernel("HBMandBlockCopy", hbm0device, ddr1device, ddr0device,
                         hbm20device, hbm31device, ddrXdevice);
  kernel.ExecuteTask();
  ddrXdevice.CopyToHost(ddrXmem.begin());

  for (int i = 0; i < kDataSize; i++) {
    assert(ddrXmem[i] ==
           hbm0mem[i] + ddr1mem[i] + ddr0mem[i] + hbm20mem[i] + hbm31mem[i]);
  }

  std::cout << "Done" << std::endl;

  return 0;
}
