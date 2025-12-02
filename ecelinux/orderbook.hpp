#pragma once
#include "ap_int.h"
#include "hls_stream.h"

// ---------- Basic types ----------
typedef ap_uint<64> order_ref_t;
typedef ap_uint<16> stock_loc_t;
typedef ap_uint<64> timestamp_t;
typedef ap_uint<32> price_t;
typedef ap_uint<32> shares_t;

// ---------- Message type enum (CPU-side semantics) ----------
enum MsgType {
    MSG_ADD     = 0,
    MSG_EXEC    = 1,
    MSG_CANCEL  = 2,
    MSG_DELETE  = 3,
    MSG_REPLACE = 4
};

// ---------- Message payloads ----------
struct AddOrderMsg {
    order_ref_t orderReferenceNumber;
    stock_loc_t stockLocate;
    timestamp_t timestamp;
    char        buySellIndicator; // 'B' or 'S'
    shares_t    shares;
    price_t     price;
};

struct OrderExecutedMsg {
    order_ref_t orderReferenceNumber;
    shares_t    executedShares;
};

struct OrderCancelMsg {
    order_ref_t orderReferenceNumber;
    shares_t    cancelledShares;
};

struct OrderDeleteMsg {
    order_ref_t orderReferenceNumber;
};

struct OrderReplaceMsg {
    order_ref_t originalOrderReferenceNumber;
    order_ref_t newOrderReferenceNumber;
    timestamp_t timestamp;
    shares_t    shares;
    price_t     price;
};

// ---------- Unified input packet to the orderbook ----------
struct OBInput {
    ap_uint<3> type;  // Encodes MsgType (0..4)
    AddOrderMsg      add;
    OrderExecutedMsg exec;
    OrderCancelMsg   cancel;
    OrderDeleteMsg   del;
    OrderReplaceMsg  repl;
};

// ---------- Output packet from the orderbook ----------
struct OBOutput {
    price_t     bestBid;
    price_t     bestAsk;
    ap_uint<16> orderCount;
};

// ---------- Top-level HLS orderbook DUT ----------
void orderbook_dut(hls::stream<OBInput> &in,
                   hls::stream<OBOutput> &out);
