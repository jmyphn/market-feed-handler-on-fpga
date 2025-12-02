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

static order_ref_t make_ref(bit32_t hi, bit32_t lo) {
    order_ref_t r = (order_ref_t)hi;
    r <<= 32;
    r |= (order_ref_t)lo;
    return r;
}

void dut(
    hls::stream<bit32_t> &strm_in,    // raw ITCH 32-bit words
    hls::stream<bs_out_t> &strm_out   // packed BS output
) {
#pragma HLS DATAFLOW

    // Internal streams
    static hls::stream<bit32_t> itch_parsed("itch_parsed");
    static hls::stream<OBInput> ob_in("ob_in");
    static hls::stream<OBOutput> ob_out("ob_out");

    static hls::stream<bit32_t> bs_in("bs_in");
    static hls::stream<bit32_t> bs_hw_out("bs_hw_out");

#pragma HLS STREAM variable=itch_parsed depth=32
#pragma HLS STREAM variable=ob_in       depth=32
#pragma HLS STREAM variable=ob_out      depth=32
#pragma HLS STREAM variable=bs_in       depth=4
#pragma HLS STREAM variable=bs_hw_out   depth=4

    // ----------------------------------------------------------
    // Stage 1: ITCH Parser
    // ----------------------------------------------------------
    itch_dut(strm_in, itch_parsed);

    if (!itch_parsed.empty()) {
        bit32_t w0 = itch_parsed.read();
        bit32_t w1 = itch_parsed.read();
        bit32_t w2 = itch_parsed.read();
        bit32_t w3 = itch_parsed.read();
        bit32_t w4 = itch_parsed.read();
        bit32_t w5 = itch_parsed.read();
        bit32_t w6 = itch_parsed.read();

        char msg_type_char = (char)w0(7,0);
        char side_char     = (char)w0(15,8);

        OBInput msg;
        msg.type = 0; // default

        bool valid = true;

        if (msg_type_char == 'A') {
            // Add order
            msg.type = MSG_ADD;
            msg.add.orderReferenceNumber = make_ref(w1, w2);
            msg.add.stockLocate = 0;
            msg.add.timestamp   = 0;
            msg.add.buySellIndicator = (side_char == 'B') ? 'B' : 'S';
            msg.add.shares = (shares_t)((ap_uint<32>)w5);
            msg.add.price  = (price_t)((ap_uint<32>)w6);
        } else if (msg_type_char == 'D') {
            // Delete order
            msg.type = MSG_DELETE;
            msg.del.orderReferenceNumber = make_ref(w1, w2);
        } else if (msg_type_char == 'E') {
            // Execute order
            msg.type = MSG_EXEC;
            msg.exec.orderReferenceNumber = make_ref(w1, w2);
            msg.exec.executedShares       = (shares_t)((ap_uint<32>)w5);
        } else if (msg_type_char == 'X') {
            // Cancel (partial cancel)
            msg.type = MSG_CANCEL;
            msg.cancel.orderReferenceNumber = make_ref(w1, w2);
            msg.cancel.cancelledShares      = (shares_t)((ap_uint<32>)w5);
        } else {
            valid = false;
        }

#ifndef __SYNTHESIS__
        if (valid) {
            std::cout << "[ITCH] type=" << msg_type_char
                      << " side=" << side_char
                      << " price_raw=" << (unsigned)w6
                      << " ";
        }
#endif

        if (valid) {
            ob_in.write(msg);
        }
    }

    // ----------------------------------------------------------
    // Stage 2: Orderbook
    // ----------------------------------------------------------
    if (!ob_in.empty()) {
        orderbook_dut(ob_in, ob_out);
    }

    // ----------------------------------------------------------
    // Stage 3: Black–Scholes
    // ----------------------------------------------------------
    if (!ob_out.empty()) {
        OBOutput ob = ob_out.read();

        // Use best bid as the spot price (in price_t units)
        price_t px_int = ob.bestBid;
        float spot_price = ((float)px_int) / 10000.0f;

#ifndef __SYNTHESIS__
        std::cout << "Spot=" << std::fixed << std::setprecision(4)
                  << spot_price << " ";
#endif

        // Convert spot float → raw bits
        union { float f; uint32_t u; } conv;
        conv.f = spot_price;
        bs_in.write((bit32_t)conv.u);

        // Run BS accelerator
        bs_dut(bs_in, bs_hw_out);

        // Read call/put outputs
        bit32_t call_bits = bs_hw_out.read();
        bit32_t put_bits  = bs_hw_out.read();

        bs_out_t out64;
        out64.range(31,0)  = call_bits;
        out64.range(63,32) = put_bits;

        strm_out.write(out64);
    }
}
