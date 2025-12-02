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
    MSG_REPLACE = 4,
    MSG_UPDATE  = 5
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

struct OrderUpdateMsg {
    order_ref_t orderReferenceNumber;
    shares_t    newShares;
    price_t     newPrice;
};

// ---------- Unified input packet to the orderbook ----------
struct OBInput {
    ap_uint<3> type;  // Encodes MsgType (0..5)
    AddOrderMsg      add;
    OrderExecutedMsg exec;
    OrderCancelMsg   cancel;
    OrderDeleteMsg   del;
    OrderReplaceMsg  repl;
    OrderUpdateMsg   update;
};

// ---------- Output packet from the orderbook ----------
struct OBOutput {
    price_t     bestBid;
    price_t     bestAsk;
    ap_uint<16> orderCount;
};

// ===============================================================
// OrderBook internal data structures
// ===============================================================

// Index type for arrays (-1 = none)
typedef ap_int<12> idx_t;

struct Order {
    order_ref_t referenceNumber;
    char        side;     // 'B' / 'S'
    shares_t    shares;
    price_t     price;

    idx_t       prev;
    idx_t       next;

    bool        valid;
};


struct Level {
    price_t price;
    shares_t limitVolume;

    idx_t firstOrder;
    idx_t lastOrder;

    bool valid;
};

#define MAX_ORDERS 768
#define MAX_LEVELS 128
#define SIDE_BUY   'B'
#define SIDE_SELL  'S'

// ===============================================================
// OrderBook Class Declaration
// ===============================================================

class OrderBook {
public:
    OrderBook();

    void init();

    // Core order operations
    void add_order(const AddOrderMsg& msg);
    void execute_order(const OrderExecutedMsg& msg);
    void cancel_order(const OrderCancelMsg& msg);
    void delete_order(const OrderDeleteMsg& msg);
    void replace_order(const OrderReplaceMsg& msg);
    void update_order(const OrderUpdateMsg& msg);

    // Queries
    price_t getBestBid() const;
    price_t getBestAsk() const;
    ap_uint<16> countOrders() const;

private:
    Order orders[MAX_ORDERS];
    Level bidLevels[MAX_LEVELS];
    Level askLevels[MAX_LEVELS];

    // Helper functions
    idx_t find_free_order_slot();
    idx_t find_order(order_ref_t ref);
    Level* level_array(char side);
    const Level* level_array_const(char side) const;
    idx_t find_level_idx(char side, price_t price);
    idx_t allocate_level(char side, price_t price);
    idx_t get_or_create_level(char side, price_t price);
    void maybe_delete_level(char side, idx_t lvl_idx);
    void remove_order_from_level(idx_t idx, Level& lvl);
};


// ---------- Top-level HLS orderbook DUT ----------
void orderbook_dut(hls::stream<OBInput> &in,
                   hls::stream<OBOutput> &out,
                   int num_msgs);
