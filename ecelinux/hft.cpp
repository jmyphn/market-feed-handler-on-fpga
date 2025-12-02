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

// ============================================================================
// STAGE 2: ITCH Message Handler
// Parses ITCH words and constructs orderbook messages
// ============================================================================
void itch_msg_handler(
    hls::stream<bit32_t> &itch_parsed,
    hls::stream<OBInput> &ob_in
) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    // In a DATAFLOW region, this function must block until data is available.
    // The non-blocking check `!stream.empty()` is an anti-pattern here, as it
    // can cause the module to terminate prematurely if data isn't ready on the
    // exact cycle it is checked, leading to dropped messages.
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
    msg.type = 0;
    bool valid = true;

    if (msg_type_char == 'A') {
        msg.type = MSG_ADD;
        msg.add.orderReferenceNumber = make_ref(w1, w2);
        msg.add.stockLocate = 0;
        msg.add.timestamp   = 0;
        msg.add.buySellIndicator = (side_char == 'B') ? 'B' : 'S';
        msg.add.shares = (shares_t)((ap_uint<32>)w5);
        msg.add.price  = (price_t)((ap_uint<32>)w6);
    } else if (msg_type_char == 'C') {
        msg.type = MSG_CANCEL;
        msg.cancel.orderReferenceNumber = make_ref(w1, w2);
        msg.cancel.cancelledShares = (shares_t)((ap_uint<32>)w5);
    } else if (msg_type_char == 'D') {
        msg.type = MSG_DELETE;
        msg.del.orderReferenceNumber = make_ref(w1, w2);
    } else if (msg_type_char == 'E') {
        msg.type = MSG_EXEC;
        msg.exec.orderReferenceNumber = make_ref(w1, w2);
        msg.exec.executedShares = (shares_t)((ap_uint<32>)w5);
    } else if (msg_type_char == 'U') {
        msg.type = MSG_UPDATE;
        msg.update.orderReferenceNumber = make_ref(w1, w2);
        msg.update.newShares = (shares_t)((ap_uint<32>)w5);
        msg.update.newPrice = (price_t)((ap_uint<32>)w6);
    } else if (msg_type_char == 'X') {
        msg.type = MSG_CANCEL;
        msg.cancel.orderReferenceNumber = make_ref(w1, w2);
        msg.cancel.cancelledShares = (shares_t)((ap_uint<32>)w5);
    } else {
        valid = false;
    }

#ifndef __SYNTHESIS__
    if (valid) {
        std::cout << "[ITCH] type=" << msg_type_char
                  << " side=" << side_char
                  << " price_raw=" << (unsigned)w6 << " ";
    }
#endif

    if (valid) {
        ob_in.write(msg);
    }
}

// ============================================================================
// STAGE 4: Black-Scholes Preprocessor
// Calculates mid-price and converts to float
// ============================================================================
void bs_preprocessor(
    hls::stream<OBOutput> &ob_out,
    hls::stream<bit32_t> &bs_in
) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    // In a DATAFLOW region, this function must block until data is available.
    // The non-blocking check `!stream.empty()` is an anti-pattern here, as it
    // can cause the module to terminate prematurely if data isn't ready on the
    // exact cycle it is checked, leading to dropped messages.
    OBOutput ob = ob_out.read();

    price_t bid = ob.bestBid;
    price_t ask = ob.bestAsk;
    price_t mid_int;

        if (bid == 0 && ask == 0) {
            mid_int = 0;
        } else if (bid == 0) {
            mid_int = ask;
        } else if (ask == 0) {
            mid_int = bid;
        } else {
            mid_int = (bid + ask) >> 1;
        }

        float spot_price = ((float)mid_int) / 10000.0f;

#ifndef __SYNTHESIS__
        std::cout << "Spot=" << std::fixed << std::setprecision(4)
                  << spot_price << " ";
#endif

        union { float f; uint32_t u; } conv;
        conv.f = spot_price;
        bs_in.write((bit32_t)conv.u);
}

// ============================================================================
// STAGE 6: Black-Scholes Postprocessor
// Packs call and put results into single 64-bit output
// ============================================================================
void bs_postprocessor(
    hls::stream<bit32_t> &bs_hw_out,
    hls::stream<bs_out_t> &strm_out
) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

    // In a DATAFLOW region, this function must block until data is available.
    // The non-blocking check `!stream.empty()` is an anti-pattern here, as it
    // can cause the module to terminate prematurely if data isn't ready on the
    // exact cycle it is checked, leading to dropped messages.
    bit32_t call_bits = bs_hw_out.read();
    bit32_t put_bits  = bs_hw_out.read();

    bs_out_t out64;
    out64.range(31,0)  = call_bits;
    out64.range(63,32) = put_bits;

    strm_out.write(out64);
}

// ============================================================================
// TOP-LEVEL DUT - CANONICAL DATAFLOW
// ============================================================================
void dut(
    hls::stream<bit32_t> &strm_in,
    hls::stream<bs_out_t> &strm_out,
    int num_msgs
) {
#pragma HLS DATAFLOW

    // Internal streams connecting pipeline stages
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

    // Six-stage pipeline (each function runs concurrently)
    itch_dut(strm_in, itch_parsed, num_msgs);           // Stage 1: ITCH parser
    for (int i = 0; i < num_msgs; ++i) {
#pragma HLS PIPELINE
        itch_msg_handler(itch_parsed, ob_in);     // Stage 2: ITCH message handler
    }
    orderbook_dut(ob_in, ob_out, num_msgs);             // Stage 3: Order book
    for (int i = 0; i < num_msgs; ++i) {
#pragma HLS PIPELINE
        bs_preprocessor(ob_out, bs_in);           // Stage 4: BS preprocessor
        bs_dut(bs_in, bs_hw_out);                 // Stage 5: Black-Scholes engine
        bs_postprocessor(bs_hw_out, strm_out);    // Stage 6: BS postprocessor
    }
}
