#include "itch.hpp"
#include "itch_common.hpp"
#include "orderbook.hpp"
#include "blackscholes.hpp"

#include <hls_stream.h>


void hft(
    hls::stream<bit32_t> &itch_in,
    hls::stream<bit32_t> &call_out,
    hls::stream<bit32_t> &put_out
) {
#pragma HLS DATAFLOW

    // Internal streams
    static hls::stream<bit32_t> itch_parsed("itch_parsed");
    static hls::stream<ParsedMessage> msg_stream("msg_stream");
    static hls::stream<bit32_t> spot_stream("spot_stream");
    static hls::stream<bit32_t> bs_out("bs_out");

#pragma HLS STREAM variable=itch_parsed depth=16
#pragma HLS STREAM variable=msg_stream depth=16
#pragma HLS STREAM variable=spot_stream depth=16
#pragma HLS STREAM variable=bs_out depth=16


    // ITCH Parser
    itch_dut(itch_in, itch_parsed);

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
        std::cout << "Type " << (char)p.type << " | ";
        p.side = (ap_uint<8>)w0(15,8);
        p.order_id     = ((ap_uint<64>)((uint32_t)w1), (uint32_t)w2);
        p.new_order_id = ((ap_uint<64>)((uint32_t)w3), (uint32_t)w4);
        p.shares = (uint32_t)w5;
        p.price  = (uint32_t)w6;

        msg_stream.write(p);
    }

    // Orderbook
    if (!msg_stream.empty()) {
        orderbook(msg_stream, spot_stream);
    }

    // Black-Scholes
    if (!spot_stream.empty()) {
        bit32_t spot_bits = spot_stream.read();

        // Convert fixed-point to float
        float spot_price = (float)spot_bits / 10000.0f;
        std::cout << std::fixed << std::setprecision(4)
                    << "Spot Price=" << spot_price << " | ";

        // Convert float -> IEEE754 bits for DUT
        union { float f; uint32_t u; } conv;
        conv.f = spot_price;

        hls::stream<bit32_t> bs_in;
        bs_in.write((bit32_t)conv.u);

        hls::stream<bit32_t> bs_hw_out;
        dut(bs_in, bs_hw_out);   

        bit32_t call_bits = bs_hw_out.read();
        bit32_t put_bits  = bs_hw_out.read();

        call_out.write(call_bits);
        put_out.write(put_bits);
    }

}
