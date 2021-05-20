#include "hlslib/xilinx/OpenCL.h"
#include <iostream>

int RunKernel() {
    std::cout << "Running simulation kernel..." << std::endl;

    std::cout << "Initializing OpenCL context..." << std::endl;
    hlslib::ocl::Context context;
    std::cout << "Done." << std::endl;

    std::cout << "Initializing memory..." << std::flush;
    std::array<size_t, 3> buf1Size = {100, 100, 10};
    std::array<size_t, 3> buf2Size = {20, 20, 20};
    size_t buf1Elems = buf1Size[0]*buf1Size[1]*buf1Size[2];
    size_t buf2Elems = buf2Size[0]*buf2Size[1]*buf2Size[2];
    std::vector<double, hlslib::ocl::AlignedAllocator<double, 4096>> memHostBuf1(buf1Elems);
    std::vector<double, hlslib::ocl::AlignedAllocator<double, 4096>> memHostBuf2(buf2Elems);
    std::fill(memHostBuf1.begin(), memHostBuf2.end(), 4);
    std::fill(memHostBuf2.begin(), memHostBuf2.end(), 3);
    
    auto memDevice1 = context.MakeBuffer<double, hlslib::ocl::Access::readWrite>(
        hlslib::ocl::StorageType::HBM, 0, buf1Elems);
    auto memDevice2 = context.MakeBuffer<double, hlslib::ocl::Access::readWrite>(
        hlslib::ocl::StorageType::HBM, 10, memHostBuf2.cbegin(), memHostBuf2.cend());
    std::cout << " Done." << std::endl;

    std::cout << "Copy data around";



    return 0;
}

int main(int argc, char **argv) {
  return RunKernel();
}
