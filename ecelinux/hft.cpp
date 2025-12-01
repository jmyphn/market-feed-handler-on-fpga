#include "itch.hpp"
#include "itch_common.hpp"
#include "orderbook.hpp"
#include "blackscholes.hpp"

#include <hls_stream.h>

#ifndef __SYNTHESIS__
#include <iostream>
#include <iomanip>
#endif

// Combined output type: pack call + put into 64 bits
typedef ap_uint<64> bs_out_t;

void dut(
    hls::stream<bit32_t> &strm_in,   
    hls::stream<bs_out_t> &strm_out  
) {
#pragma HLS DATAFLOW

    // Internal streams
    static hls::stream<bit32_t>       itch_parsed("itch_parsed");
    static hls::stream<ParsedMessage> msg_stream("msg_stream");
    static hls::stream<bit32_t>       spot_stream("spot_stream");

    // Streams for Black-Scholes 
    static hls::stream<bit32_t> bs_in("bs_in");
    static hls::stream<bit32_t> bs_hw_out("bs_hw_out");

#pragma HLS STREAM variable=itch_parsed depth=32
#pragma HLS STREAM variable=msg_stream  depth=32
#pragma HLS STREAM variable=spot_stream depth=32
#pragma HLS STREAM variable=bs_in       depth=4
#pragma HLS STREAM variable=bs_hw_out   depth=4


    // ---------------------------------------------------------------------
    // Stage 1: ITCH Parser
    // ---------------------------------------------------------------------
    itch_dut(strm_in, itch_parsed);

    if (!itch_parsed.empty()) {
        ParsedMessage p;

        bit32_t w0 = itch_parsed.read();
        bit32_t w1 = itch_parsed.read();
        bit32_t w2 = itch_parsed.read();
        bit32_t w3 = itch_parsed.read();
        bit32_t w4 = itch_parsed.read();
        bit32_t w5 = itch_parsed.read();
        bit32_t w6 = itch_parsed.read();

        p.type = (ap_uint<8>)w0(7,0);
        p.side = (ap_uint<8>)w0(15,8);
        p.order_id     = ((ap_uint<64>)((uint32_t)w1), (uint32_t)w2);
        p.new_order_id = ((ap_uint<64>)((uint32_t)w3), (uint32_t)w4);
        p.shares = (uint32_t)w5;
        p.price  = (uint32_t)w6;

#ifndef __SYNTHESIS__
        double price_display = p.price / 10000.0;
        std::cout << "Type " << (char)p.type << " | "
                  << "Msg Price=" << std::fixed << std::setprecision(4)
                  << std::setw(8) << price_display << " | ";
#endif

        msg_stream.write(p);
    }


    // ---------------------------------------------------------------------
    // Stage 2: Orderbook
    // ---------------------------------------------------------------------
    if (!msg_stream.empty()) {
        orderbook(msg_stream, spot_stream);
    }


    // ---------------------------------------------------------------------
    // Stage 3: Black–Scholes
    // ---------------------------------------------------------------------
    if (!spot_stream.empty()) {

        bit32_t spot_bits = spot_stream.read();
        float spot_price = (float)spot_bits / 10000.0f;

#ifndef __SYNTHESIS__
        std::cout << "Spot Price=" << std::fixed << std::setprecision(4)
                  << std::setw(8) << spot_price << " | ";
#endif

        // Convert spot float → raw bits
        union { float f; uint32_t u; } conv;
        conv.f = spot_price;
        bs_in.write((bit32_t)conv.u);

        // Run BS accelerator
        bs_dut(bs_in, bs_hw_out);

        // Read outputs
        bit32_t call_bits = bs_hw_out.read();
        bit32_t put_bits  = bs_hw_out.read();

        // ------------------------------------------------------------------
        // Pack call + put into a single 64-bit output stream
        // ------------------------------------------------------------------
        bs_out_t out64;
        out64.range(31,0)  = call_bits;
        out64.range(63,32) = put_bits;

        // Write packed output to the single Xillybus output stream
        strm_out.write(out64);
    }
}
