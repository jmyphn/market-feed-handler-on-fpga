#include "orderbook.hpp"

// ===============================================================
// OrderBook internal data structures
// ===============================================================

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

#define MAX_ORDERS 4096
#define MAX_LEVELS 2048
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
        init();
    }

    void init() {
        // Initialize orders
        for (int i = 0; i < MAX_ORDERS; i++) {
            orders[i].valid = false;
        }

        // Initialize bid levels
        for (int i = 0; i < MAX_LEVELS; i++) {
            bidLevels[i].valid = false;
            bidLevels[i].limitVolume = 0;
        }

        // Initialize ask levels
        for (int i = 0; i < MAX_LEVELS; i++) {
            askLevels[i].valid = false;
            askLevels[i].limitVolume = 0;
        }
    }

    // -----------------------------------------------------------
    // Helper functions
    // -----------------------------------------------------------

    idx_t find_free_order_slot() {
        for (int i = 0; i < MAX_ORDERS; i++) {
            if (!orders[i].valid)
                return i;
        }
        return -1;
    }

    idx_t find_order(order_ref_t ref) {
        for (int i = 0; i < MAX_ORDERS; i++) {
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
        Level* levels = level_array(side);
        for (int i = 0; i < MAX_LEVELS; i++) {
            if (levels[i].valid && levels[i].price == price)
                return i;
        }
        return -1;
    }

    idx_t allocate_level(char side, price_t price) {
        Level* levels = level_array(side);
        for (int i = 0; i < MAX_LEVELS; i++) {
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
        idx_t idx = find_level_idx(side, price);
        if (idx != -1) return idx;
        return allocate_level(side, price);
    }

    void maybe_delete_level(char side, idx_t lvl_idx) {
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
        Order& o = orders[idx];
    }

    void execute_order(const ParsedMessage& msg) {
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
        price_t best = 0;
        const Level* lvls = bidLevels;

        for (int i = 0; i < MAX_LEVELS; i++) {
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
        ap_uint<16> c = 0;
        for (int i = 0; i < MAX_ORDERS; i++) {
            if (orders[i].valid)
                c++;
        }
        return c;
    }
};

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


void orderbook_dut(hls::stream<bit32_t> &strm_in,
                   hls::stream<bit32_t> &strm_out)
{

    static OrderBook ob;

    // Require 7 words per message
    if (strm_in.size() < 7)
        return;

    // ---------------------------------------------------------
    // Read raw 7-word message
    // ---------------------------------------------------------
    bit32_t w0 = strm_in.read();
    bit32_t w1 = strm_in.read();
    bit32_t w2 = strm_in.read();
    bit32_t w3 = strm_in.read();
    bit32_t w4 = strm_in.read();
    bit32_t w5 = strm_in.read();
    bit32_t w6 = strm_in.read();

    ParsedMessage msg;

    msg.type = (char)w0(7,0); // Msg Type
    msg.side = (char)w0(15,8); // Side

    // Decode order reference number
    msg.order_id = 0;
    msg.order_id.range(63,32) = w1;
    msg.order_id.range(31, 0) = w2;

    // Replace/cancel new order id fields unused
    msg.new_order_id = 0;

    // Decode shares + price
    msg.shares = (shares_t)w5;
    msg.price  = (price_t) w6;

    // Update Orderbook
    switch (msg.type) {
        case 'A': ob.add_order(msg); break;
        case 'E': ob.execute_order(msg); break;
        case 'C': ob.execute_order(msg); break;
        case 'X': ob.cancel_order(msg);  break;
        case 'D': ob.delete_order(msg);  break;
        case 'U': ob.replace_order(msg); break;
        default: break;
    }

    // Calculate spot price
    bit32_t bestBid = ob.getBestBid();
    bit32_t bestAsk = ob.getBestAsk();
    bit32_t spot = (bestBid + bestAsk) >> 1;

    // Output spot price
    strm_out.write(spot);
}




