//===========================================================================
// orderbook.hpp
//===========================================================================
// @brief: This header file defines the interface for the orderbook module.

#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include "typedefs.h"

#include <hls_stream.h>
#include <ap_int.h>

typedef ap_uint<64> order_ref_t;
typedef ap_uint<16> stock_loc_t;
typedef ap_uint<64> timestamp_t;
typedef ap_uint<32> price_t;
typedef ap_uint<32> shares_t;

// Top function
bit32_t orderbook(ParsedMessage* msg);

// Orderbook HLS DUT:
//   - strm_in:  7 x 32-bit words containing extracted info from ITCH msgs
//   - strm_out: 1 x 32-bit word containing float-encoded spot price S
void orderbook_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

#endif // ORDERBOOK_HPP
