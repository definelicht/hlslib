#include "hlslib/intel/OpenCL.h"
#include <iostream>

int main() {
  std::cout << hlslib::ocl::GlobalContext(0).DeviceName();
  return 0;
}
