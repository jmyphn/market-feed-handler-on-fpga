#include "hft.hpp"

void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
    // ------------------------------------------------------
    // Input processing
    // ------------------------------------------------------
    bit32_t hdr    = strm_in.read();
    uint16_t msg_len = (uint16_t)hdr(15, 0);
    assert(msg_len == (uint16_t)hdr(15, 0));

    char in_buffer[36];   // 36 bytes is enough for our ITCH msgs
    int  idx   = 0;
    
    // Handle variable msg length
    bit4_t words = (msg_len + 3) >> 2;    // # of 32-bit words = ceil(msg_len/4)
    assert((msg_len + 3) >> 2 == (bit16_t)words);
    for (int w = 0; w < words; ++w) {
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

    float S_f = (float)spot_price_ticks / 10000.0f;
    union { float f; int i; } u_in;
    u_in.f = S_f;
    bit32_t spot_price_bits = (bit32_t)u_in.i;
    
    result_type result = bs(spot_price_bits);

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