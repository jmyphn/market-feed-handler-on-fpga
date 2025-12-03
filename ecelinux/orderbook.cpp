#include "orderbook.hpp"

// ===============================================================
// OrderBook internal data structures
// ===============================================================

#define OPT

// Index type for arrays (-1 = none)
typedef ap_int<16> idx_t;

struct Order {
    order_ref_t referenceNumber;
    shares_t    shares;
    price_t     price;
    char        side;     // 'B' / 'S'
    bool        valid;
};


struct Level {
    price_t price;
    shares_t limitVolume;
    bool valid;
};

#define MAX_ORDERS 256
#define MAX_LEVELS 128
#define SIDE_BUY   'B'
#define SIDE_SELL  'S'

// ===============================================================
// OrderBook Class
// ===============================================================

class OrderBook {
public:
    Order orders[MAX_ORDERS];
    Level bidLevels[MAX_LEVELS];
    Level askLevels[MAX_LEVELS];

    OrderBook() {
    #pragma HLS INLINE
        init();
    }

    void init() {
    #pragma HLS INLINE
        // Initialize orders
        for (int i = 0; i < MAX_ORDERS; i++) {
        #ifdef OPT
        #pragma HLS PIPELINE II=1
        #endif
            orders[i].valid = false;
        }

        // Initialize bid levels
        for (int i = 0; i < MAX_LEVELS; i++) {
        #ifdef OPT
        #pragma HLS PIPELINE II=1
        #endif
            bidLevels[i].valid = false;
            bidLevels[i].limitVolume = 0;
        }

        // Initialize ask levels
        for (int i = 0; i < MAX_LEVELS; i++) {
        #ifdef OPT
        #pragma HLS PIPELINE II=1
        #endif
            askLevels[i].valid = false;
            askLevels[i].limitVolume = 0;
        }
    }

    // -----------------------------------------------------------
    // Helper functions
    // -----------------------------------------------------------

    idx_t find_free_order_slot() {
    #pragma HLS INLINE off
        for (int i = 0; i < MAX_ORDERS; i++) {
        // #pragma HLS PIPELINE II=1
          #ifdef OPT
          #pragma HLS UNROLL factor=4
          #endif
            if (!orders[i].valid)
                return i;
        }
        return -1;
    }

    idx_t find_order(order_ref_t ref) {
    #pragma HLS INLINE off
        for (int i = 0; i < MAX_ORDERS; i++) {
        // #pragma HLS UNROLL factor=4
            if (orders[i].valid && orders[i].referenceNumber == ref)
                return i;
        }
        return -1;
    }

    Level* level_array(char side) {
        return (side == SIDE_BUY) ? bidLevels : askLevels;
    }

    const Level* level_array_const(char side) const {
        return (side == SIDE_BUY) ? bidLevels : askLevels;
    }

    idx_t find_level_idx(char side, price_t price) {
    #ifdef OPT
    #pragma HLS INLINE off
    #endif
        Level* levels = level_array(side);
        for (int i = 0; i < MAX_LEVELS; i++) {
        #pragma HLS PIPELINE II=1
            if (levels[i].valid && levels[i].price == price)
                return i;
        }
        return -1;
    }

    idx_t allocate_level(char side, price_t price) {
    #ifdef OPT
    #pragma HLS INLINE off
    #endif
        Level* levels = level_array(side);
        for (int i = 0; i < MAX_LEVELS; i++) {
        #ifdef OPT
        #pragma HLS PIPELINE II=1
        #endif
            if (!levels[i].valid) {
                levels[i].valid = true;
                levels[i].price = price;
                levels[i].limitVolume = 0;
                return i;
            }
        }
        return -1;
    }

    idx_t get_or_create_level(char side, price_t price) {
    #ifdef OPT
    #pragma HLS INLINE
    #endif
        idx_t idx = find_level_idx(side, price);
        if (idx != -1) return idx;
        return allocate_level(side, price);
    }

    void maybe_delete_level(char side, idx_t lvl_idx) {
    #ifdef OPT
    #pragma HLS INLINE
    #endif
        Level* lvls = level_array(side);
        Level& lvl = lvls[lvl_idx];

        if (!lvl.valid) return;
        if (lvl.limitVolume == 0)
            lvl.valid = false;
    }

    // -----------------------------------------------------------
    // Core order operations
    // -----------------------------------------------------------

    void add_order(const ParsedMessage& msg) {
    #pragma HLS INLINE off
        if (find_order(msg.order_id) != -1)
            return;

        idx_t slot = find_free_order_slot();
        if (slot == -1) return;

        Order& o = orders[slot];
        o.referenceNumber = msg.order_id;
        o.side  = msg.side;
        o.shares = msg.shares;
        o.price  = msg.price;
        o.valid = true;

        char side = o.side;
        idx_t lvl_idx = get_or_create_level(side, o.price);
        if (lvl_idx == -1) { o.valid = false; return; }

        Level* levels = level_array(side);
        Level& lvl = levels[lvl_idx];

        lvl.limitVolume += o.shares;
    }

    void remove_order_from_level(idx_t idx, Level& lvl) {
    #ifdef OPT
    #pragma HLS INLINE
    #endif
        Order& o = orders[idx];
    }

    void execute_order(const ParsedMessage& msg) {
    #pragma HLS INLINE
        idx_t idx = find_order(msg.order_id);
        if (idx == -1) return;

        Order& o = orders[idx];
        char side = o.side;

        idx_t lvl_idx = find_level_idx(side, o.price);
        if (lvl_idx == -1) return;

        Level* levels = level_array(side);
        Level& lvl = levels[lvl_idx];

        shares_t exec = msg.shares;
        if (exec > o.shares) exec = o.shares;

        o.shares -= exec;
        lvl.limitVolume -= exec;

        if (o.shares == 0) {
            remove_order_from_level(idx, lvl);
            o.valid = false;
            maybe_delete_level(side, lvl_idx);
        }
    }

    void cancel_order(const ParsedMessage& msg) {
        idx_t idx = find_order(msg.order_id);
        if (idx == -1) return;

        Order& o = orders[idx];
        char side = o.side;

        idx_t lvl_idx = find_level_idx(side, o.price);
        if (lvl_idx == -1) return;

        Level* lvls = level_array(side);
        Level& lvl = lvls[lvl_idx];

        // Correct cancellation amount
        shares_t canc = msg.shares;
        if (canc > o.shares) canc = o.shares;

        o.shares -= canc;
        lvl.limitVolume -= canc;

        // If order is fully canceled, remove it
        if (o.shares == 0) {
            o.valid = false;
            remove_order_from_level(idx, lvl);
            maybe_delete_level(side, lvl_idx);
        }
    }


    void delete_order(const ParsedMessage& msg) {
    #pragma HLS INLINE
        idx_t idx = find_order(msg.order_id);
        if (idx == -1) return;

        Order& o = orders[idx];
        char side = o.side;

        idx_t lvl_idx = find_level_idx(side, o.price);
        if (lvl_idx != -1) {
            Level* lvls = level_array(side);
            Level& lvl = lvls[lvl_idx];
            lvl.limitVolume -= o.shares;
            remove_order_from_level(idx, lvl);
            maybe_delete_level(side, lvl_idx);
        }

        o.valid = false;
    }

    void replace_order(const ParsedMessage& msg) {
        idx_t old = find_order(msg.order_id);
        if (old == -1) return;

        Order& o = orders[old];
        char side = o.side;

        // Remove old order from its level
        idx_t lvl_idx = find_level_idx(side, o.price);
        if (lvl_idx != -1) {
            Level* lvls = level_array(side);
            Level& lvl = lvls[lvl_idx];

            lvl.limitVolume -= o.shares;
            remove_order_from_level(old, lvl);
            maybe_delete_level(side, lvl_idx);
        }

        // Invalidate the old order
        o.valid = false;

        // Build new order correctly — MUST use msg.new_order_id
        ParsedMessage newMsg;
        newMsg.type        = msg.type;
        newMsg.order_id    = msg.new_order_id;
        newMsg.side        = side;
        newMsg.shares      = msg.shares;
        newMsg.price       = msg.price;

        add_order(newMsg);
    }


    // -----------------------------------------------------------
    // Queries
    // -----------------------------------------------------------

    price_t getBestBid() const {
    // #pragma HLS INLINE off
        price_t best = 0;
        const Level* lvls = bidLevels;

        for (int i = 0; i < MAX_LEVELS; i++) {
        #ifdef OPT
        // #pragma HLS PIPELINE II=1
        // #pragma HLS UNROLL factor=4
        #endif
            if (lvls[i].valid && lvls[i].limitVolume > 0) {
                if (lvls[i].price > best)
                    best = lvls[i].price;
            }
        }
        return best;
    }

    price_t getBestAsk() const {
        const Level* lvls = askLevels;
        bool found = false;
        price_t best = 0;

        for (int i = 0; i < MAX_LEVELS; i++) {
            if (lvls[i].valid && lvls[i].limitVolume > 0) {
                if (!found || lvls[i].price < best) {
                    best = lvls[i].price;
                    found = true;
                }
            }
        }
        return found ? best : price_t(0);
    }


    ap_uint<16> countOrders() const {
    #pragma HLS INLINE
        ap_uint<16> c = 0;
        for (int i = 0; i < MAX_ORDERS; i++) {
        // #pragma HLS PIPELINE II=1
            if (orders[i].valid)
                c++;
        }
        return c;
    }
};

void orderbook_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
    #pragma HLS INTERFACE axis port=in
    #pragma HLS INTERFACE axis port=out
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // ------------------------------------------------------
    // Input processing
    // ------------------------------------------------------
    static OrderBook ob;
    #pragma HLS RESET variable=ob

    ParsedMessage parsed;

    for (int i = 0; i < 7; i++) {
    // #pragma HLS PIPELINE II=1
        bit32_t in_word = strm_in.read();

        switch (i) {
            case '0': parsed.type = in_word(7,0); parsed.side = in_word(15,8); break;
            case '1': parsed.order_id.range(63,32) = in_word; break;
            case '2': parsed.order_id.range(31, 0) = in_word; break;
            case '3': parsed.new_order_id.range(63,32) = in_word; break;
            case '4': parsed.new_order_id.range(31, 0) = in_word; break;
            case '5': parsed.shares = in_word; break;
            case '6': parsed.price = in_word; break;
            default: break;
        }
    }

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

bit32_t orderbook(ParsedMessage* msg) {
    static OrderBook ob;
    char msgType = msg->type;

    switch (msgType) {
        case 'A': ob.add_order      (*msg); break;
        case 'E': ob.execute_order  (*msg); break;
        case 'C': ob.execute_order  (*msg); break;
        case 'X': ob.cancel_order   (*msg); break;
        case 'D': ob.delete_order   (*msg); break;
        case 'U': ob.replace_order  (*msg); break;
        default: break;
    }

    bit32_t best_bid = ob.getBestBid();
    bit32_t best_ask = ob.getBestAsk();
    bit32_t avg = (best_bid + best_ask) >> 1;  // divide by 2 using shift

    // // ---- PRINTING HERE IS NOT SYNTHESIZABLE ----
    // double price_display = avg / 10000.0;
    // std::cout << std::fixed << std::setprecision(4)
    //          << "Spot_Price=" << std::setw(8) << price_display << " | "; 
    
    return avg;
}
