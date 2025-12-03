#include "hft.hpp"

#include <hls_stream.h>
#include <cassert>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#endif

void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
    // ------------------------------------------------------
    // Input processing
    // ------------------------------------------------------
    bit32_t hdr    = strm_in.read();
    uint16_t msg_len = (uint16_t)hdr(15, 0);
    // bit4_t msg_len = (bit4_t)hdr(3, 0);
    // std::cerr << "hdr: " << (uint16_t)hdr(15, 0) << std::endl;
    // assert((uint16_t)hdr(15, 0) <= 15);
    assert(msg_len == (uint16_t)hdr(15, 0));

    char in_buffer[MAX_MESSAGE_SIZE];   // 36 bytes is enough for our ITCH msgs
    int  idx   = 0;
    bit4_t words = (msg_len + 3) >> 2;    // # of 32-bit words = ceil(msg_len/4)
    // assert(words <= 15);
    assert((msg_len + 3) >> 2 == (uint16_t)words);

    for (int w = 0; w < words; ++w) {
    // #pragma HLS PIPELINE II=1
        bit32_t word = strm_in.read();

        if (idx < msg_len){
            in_buffer[idx++] = (char)word(31,24);
            in_buffer[idx++] = (char)word(23,16);
            in_buffer[idx++] = (char)word(15, 8);
            in_buffer[idx++] = (char)word( 7, 0);
        } 
    }

    // ------------------------------------------------------
    // Call functions
    // ------------------------------------------------------
    ParsedMessage parsed = parser(in_buffer);

    bit32_t spot_price_ticks = orderbook(&parsed);

    //
    float S_f = (float)spot_price_ticks / 10000.0f;
    union { float f; int i; } u_in;
    u_in.f = S_f;
    bit32_t spot_price_bits = (bit32_t)u_in.i;
    result_type result = bs(spot_price_bits);
    // result_type result = bs(spot_price);
    //
    
    // Printing here is not synthesizable
    // double price_display = spot_price / 10000.0;
    // std::cout << std::fixed << std::setprecision(4)
    //          << "Spot_Price=" << std::setw(8) << price_display << " | ";

    

    // ------------------------------------------------------
    // Output processing
    // ------------------------------------------------------
    // Convert results back to 32-bit words
    union { float fval; int ival; } ucall;
    union { float fval; int ival; } uput;

    ucall.fval = result.call;
    uput.fval  = result.put;

    bit32_t icall = static_cast<bit32_t>(ucall.ival);
    bit32_t iput  = static_cast<bit32_t>(uput.ival);

    // Write output to stream (call, put)
    strm_out.write(icall);
    strm_out.write(iput);
}