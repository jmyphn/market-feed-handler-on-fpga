//===========================================================================
// itch.h
//===========================================================================
// @brief: This header file defines the interface for the core functions.

#ifndef ITCH_H
#define ITCH_H
// #include "itch_reader.hpp"
#include "itch_common.hpp"
#include "itch_defs.h"
#include <hls_stream.h>
#include <cstdint>

#define MAX_MESSAGE_SIZE 36

struct ParsedMessage {
    ap_uint<8>  type;
    ap_uint<8>  side;
    ap_uint<64> order_id;
    ap_uint<64> new_order_id;
    ap_uint<32> shares;
    ap_uint<32> price;
    ap_uint<64> timestamp;
};

// Top function for synthesis
void itch_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

// Top function for parser
ParsedMessage parser(char* buffer);

#endif
