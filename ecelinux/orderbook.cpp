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
    bool        valid;
};

#define MAX_ORDERS 4096
#define SIDE_BUY   'B'
#define SIDE_SELL  'S'

// ===============================================================
// OrderBook Class
// ===============================================================

class OrderBook {
public:
    Order bidOrders[MAX_ORDERS];
    Order askOrders[MAX_ORDERS];

    OrderBook() {
        init();
    }

    void init() {
        for (int i = 0; i < MAX_ORDERS; i++) {
            #pragma HLS UNROLL
            bidOrders[i].valid = 0;
        }
        for (int i = 0; i < MAX_ORDERS; i++) {
            #pragma HLS UNROLL
            askOrders[i].valid = 0;
        }
    }

    idx_t find_order(order_ref_t ref, Order orders[MAX_ORDERS]) {
        #pragma hls inline off
        idx_t result = 0;

        for (int i = 0; i < MAX_ORDERS; i++) {
            #pragma hls unroll
            result |= (orders[i].valid && orders[i].referenceNumber == ref) ? i : 0;
        }
        if (result != 0) return result;
        return -1;
    }

    idx_t find_free_order_slot(Order orders[MAX_ORDERS]) {
        #pragma HLS inline
        for (int i = 0; i < MAX_ORDERS; i++) {
            // #pragma HLS unroll
            if (!orders[i].valid) return i;
        }
        return -1;
    }

    /**
     * Assumes that either bid_slot != -1 or ask_slot != -1
     */
    Order& get_order(idx_t bid_slot, idx_t ask_slot) {
        if (bid_slot != -1) {
            return bidOrders[bid_slot];
        } else {
            return askOrders[ask_slot];
        }
    }

    // -----------------------------------------------------------
    // Core order operations
    // -----------------------------------------------------------

    void add_order_helper(const ParsedMessage& msg, Order orders[MAX_ORDERS]) {
        idx_t slot = find_free_order_slot(orders);
        if (slot == -1) return;
        Order& o = orders[slot];
        o.referenceNumber = msg.order_id;
        o.shares = msg.shares;
        o.price  = msg.price;
        o.valid = true; 
    }

    void add_order(const ParsedMessage& msg) {
        if (msg.side == SIDE_BUY) {
            add_order_helper(msg, bidOrders);
        } else {
            add_order_helper(msg, askOrders);
        }
    }

    /**
     * Removes some shares from an order. If we removed more shares than 
     * available, delete it too.
     */
    void remove_order(const ParsedMessage& msg) {
        idx_t bid_slot = find_order(msg.order_id, bidOrders);
        idx_t ask_slot = find_order(msg.order_id, askOrders);
        if (bid_slot == -1 && ask_slot == -1) return;
        Order& o = get_order(bid_slot, ask_slot);

        shares_t exec = msg.shares;
        if (exec > o.shares) exec = o.shares;
        o.shares -= exec;
        if (o.shares == 0) o.valid = false;
    }

    /**
     * Delete all shares from an order.
     */
    void delete_order(const ParsedMessage& msg) {
        idx_t bid_slot = find_order(msg.order_id, bidOrders);
        idx_t ask_slot = find_order(msg.order_id, askOrders);
        if (bid_slot == -1 && ask_slot == -1) return;
        Order& o = get_order(bid_slot, ask_slot);
        o.valid = false;
    }

    void replace_order(const ParsedMessage& msg) {
        delete_order(msg);
        add_order(msg);
    }


    // -----------------------------------------------------------
    // Queries
    // -----------------------------------------------------------

    price_t getBestBid() const {
    #pragma HLS INLINE off
        price_t best = 0;
        for (int i = 0; i < MAX_ORDERS; i++) {
            // #pragma hls unroll factor=32
            if (bidOrders[i].valid && bidOrders[i].price > best) {
                best = bidOrders[i].price;
            }
        }
        return best;
    }

    price_t getBestAsk() const {
        #pragma hls inline off
        bool found = false;
        price_t best = 0;

        for (int i = 0; i < MAX_ORDERS; i++) {
            // #pragma hls unroll factor=32
            if (askOrders[i].valid) {
                if (!found || askOrders[i].price < best) {
                    best = askOrders[i].price;
                    found = true;
                }
            }
        }
        return found ? best : price_t(0);
    }

    ap_uint<16> countOrders() const {
    #pragma HLS inline
        ap_uint<16> c1 = 0;
        ap_uint<16> c2 = 0;
        for (int i = 0; i < MAX_ORDERS; i++) {
            if (bidOrders[i].valid) c1++;
        }
        for (int i = 0; i < MAX_ORDERS; i++) {
            if (askOrders[i].valid) c2++;
        }
        return c1 + c2;
    }
};


void execute_msg(OrderBook& ob, ParsedMessage &msg) {
    switch (msg.type) {
        case 'A': ob.add_order (msg); break;
        case 'E': ob.remove_order  (msg); break;
        case 'C': ob.remove_order  (msg); break;
        case 'X': ob.remove_order  (msg); break;
        case 'D': ob.delete_order   (msg); break;
        case 'U': ob.replace_order  (msg); break;
        default: break;
    }
}

bit32_t orderbook(ParsedMessage* msg) {
    static OrderBook ob;
    #pragma hls array_partition variable=ob.bidOrders block factor=128
    #pragma hls array_partition variable=ob.askOrders block factor=128

    execute_msg(ob, *msg);

    bit32_t best_bid = ob.getBestBid();
    bit32_t best_ask = ob.getBestAsk();
    // bit32_t best_bid;
    // bit32_t best_ask;
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
    #pragma hls array_partition variable=ob.bidOrders block factor=128
    #pragma hls array_partition variable=ob.askOrders block factor=128

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
    execute_msg(ob, msg);

    // Calculate spot price
    bit32_t bestBid = ob.getBestBid();
    bit32_t bestAsk = ob.getBestAsk();
    // bit32_t bestBid;
    // bit32_t bestAsk;
    bit32_t spot = (bestBid + bestAsk) >> 1;

    // Output spot price
    strm_out.write(spot);
}




