//===========================================================================
// hft.hpp
//===========================================================================
// @brief: This header file defines the interface for the core functions.

#ifndef HFT_HPP
#define HFT_HPP

#include "itch.hpp"
#include "orderbook.hpp"
#include "blackscholes.hpp"
#include "typedefs.h"

// Top-Level HLS DUT:
//   - strm_in:  1 x 32-bit word containing float-encoded spot price S
//   - strm_out: 2 x 32-bit words containing float-encoded call, then put
void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

#endif // HFT_HPP