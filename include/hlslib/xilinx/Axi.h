/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#pragma once

#include <ap_int.h>

namespace hlslib {

namespace axi {

/// Implements the AXI Stream interface, which is inferred by the name of the
/// variables (data, keep and last).
template <typename T>
struct Stream {

  T data;
  ap_uint<sizeof(T)> keep{~(keep & 0)}; // Set all bits to 1
  ap_uint<1> last;

  Stream() : data(), keep(0), last(1) {}
  Stream(decltype(data) const &_data, decltype(last) const &_last)
      : data(_data), last(_last) {}
  Stream(decltype(data) const &_data) : data(_data), last(1) {}
};

/// Implements the command bus interface for the DataMover IP
/// https://www.xilinx.com/support/documentation/ip_documentation/axi_datamover/v5_1/pg022_axi_datamover.pdf
template <size_t addressWidth, size_t bttWidth>
struct Command {

  // Typically addressWith is in {32, 64} and bttWidth is 23, so the whole bus
  // is 72 or 104 bits.
  ap_uint<bttWidth> length; // BTT (bytes to transfer)
  ap_uint<1> type{1};
  ap_uint<6> dsa{0};
  ap_uint<1> eof{1};
  ap_uint<1> drr{1};
  ap_uint<addressWidth> address;
  ap_uint<4> tag{0};
  ap_uint<4> reserved{0};

  Command(decltype(address) const &_address, decltype(length) const &_length)
      : length(_length), address(_address) {}
  Command() : length(0), address(0) {}
  Command(ap_uint<addressWidth+bttWidth+17> const &cmd)
      : length(cmd(22,0)), type(cmd(23,23)), dsa(cmd(29,24)), eof(cmd(30,30)), drr(cmd(31,31)), address(cmd(95,32)), tag(cmd(99,96)) {}
  operator ap_uint<addressWidth+bttWidth+17>(){
    ap_uint<addressWidth+bttWidth+17> ret;
    ret(bttWidth-1,0) = length;
    ret(bttWidth,bttWidth) = type;
    ret(bttWidth+6,bttWidth+1) = dsa;
    ret(bttWidth+7,bttWidth+7) = eof;
    ret(bttWidth+8,bttWidth+8) = drr;
    ret(addressWidth+bttWidth+8,bttWidth+9) = address;
    ret(addressWidth+bttWidth+12,addressWidth+bttWidth+9) = tag;
    ret(addressWidth+bttWidth+16,addressWidth+bttWidth+13) = 0;
    return ret;
  }
};

/// Implements the status bus interface for the DataMover IP
/// https://www.xilinx.com/support/documentation/ip_documentation/axi_datamover/v5_1/pg022_axi_datamover.pdf
struct Status { // 8 bits

  ap_uint<4> tag{0};
  ap_uint<1> internalError{0};
  ap_uint<1> decodeError{0};
  ap_uint<1> slaveError{0};
  ap_uint<1> okay;
  ap_uint<23> bytesReceived{0};
  ap_uint<1> endOfPacket{0};

	Status(bool _okay) : okay(_okay) {}  
  Status() : okay(true) {}
  Status(ap_uint<32> sts)
     : tag(sts(3,0)), internalError(sts(4,4)), decodeError(sts(5,5)), slaveError(sts(6,6)), 
        okay(sts(7,7)), bytesReceived(sts(30,8)), endOfPacket(sts(31,31)) {}
  operator ap_uint<32>(){
    ap_uint<32> ret;
    ret(3,0) = tag;
    ret(4,4) = internalError;
    ret(5,5) = decodeError;
    ret(5,5) = slaveError;
    ret(7,7) = okay;
    ret(30,8) = bytesReceived;
    ret(31,31) = endOfPacket;
    return ret;
  }
};

} // End namespace axi 

} // End namespace hlslib
