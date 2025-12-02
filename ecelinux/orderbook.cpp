#include "orderbook.hpp"

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
        #pragma HLS PIPELINE II=1
            orders[i].valid = false;
            orders[i].prev  = -1;
            orders[i].next  = -1;
        }

        // Initialize bid levels
        for (int i = 0; i < MAX_LEVELS; i++) {
        #pragma HLS PIPELINE II=1
            bidLevels[i].valid = false;
            bidLevels[i].limitVolume = 0;
            bidLevels[i].firstOrder = -1;
            bidLevels[i].lastOrder = -1;
        }

        // Initialize ask levels
        for (int i = 0; i < MAX_LEVELS; i++) {
        #pragma HLS PIPELINE II=1
            askLevels[i].valid = false;
            askLevels[i].limitVolume = 0;
            askLevels[i].firstOrder = -1;
            askLevels[i].lastOrder = -1;
        }
    }

    // -----------------------------------------------------------
    // Helper functions
    // -----------------------------------------------------------

    idx_t find_free_order_slot() {
    #pragma HLS INLINE off
        for (int i = 0; i < MAX_ORDERS; i++) {
        #pragma HLS PIPELINE II=1
            if (!orders[i].valid)
                return i;
        }
        return -1;
    }

    idx_t find_order(order_ref_t ref) {
    #pragma HLS INLINE off
        for (int i = 0; i < MAX_ORDERS; i++) {
        #pragma HLS PIPELINE II=1
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
    #pragma HLS INLINE off
        Level* levels = level_array(side);
        for (int i = 0; i < MAX_LEVELS; i++) {
        #pragma HLS PIPELINE II=1
            if (levels[i].valid && levels[i].price == price)
                return i;
        }
        return -1;
    }

    idx_t allocate_level(char side, price_t price) {
    #pragma HLS INLINE off
        Level* levels = level_array(side);
        for (int i = 0; i < MAX_LEVELS; i++) {
        #pragma HLS PIPELINE II=1
            if (!levels[i].valid) {
                levels[i].valid = true;
                levels[i].price = price;
                levels[i].limitVolume = 0;
                levels[i].firstOrder = -1;
                levels[i].lastOrder = -1;
                return i;
            }
        }
        return -1;
    }

    idx_t get_or_create_level(char side, price_t price) {
    #pragma HLS INLINE
        idx_t idx = find_level_idx(side, price);
        if (idx != -1) return idx;
        return allocate_level(side, price);
    }

    void maybe_delete_level(char side, idx_t lvl_idx) {
    #pragma HLS INLINE
        Level* lvls = level_array(side);
        Level& lvl = lvls[lvl_idx];

        if (!lvl.valid) return;
        if (lvl.limitVolume == 0 && lvl.firstOrder == -1 && lvl.lastOrder == -1)
            lvl.valid = false;
    }

    // -----------------------------------------------------------
    // Core order operations
    // -----------------------------------------------------------

    void add_order(const AddOrderMsg& msg) {
    #pragma HLS INLINE
        if (find_order(msg.orderReferenceNumber) != -1)
            return;

        idx_t slot = find_free_order_slot();
        if (slot == -1) return;

        Order& o = orders[slot];
        o.referenceNumber = msg.orderReferenceNumber;
        o.side  = msg.buySellIndicator;
        o.shares = msg.shares;
        o.price  = msg.price;
        o.prev = -1;
        o.next = -1;
        o.valid = true;

        char side = o.side;
        idx_t lvl_idx = get_or_create_level(side, o.price);
        if (lvl_idx == -1) { o.valid = false; return; }

        Level* levels = level_array(side);
        Level& lvl = levels[lvl_idx];

        if (lvl.firstOrder == -1) {
            lvl.firstOrder = slot;
            lvl.lastOrder = slot;
        } else {
            idx_t last = lvl.lastOrder;
            orders[last].next = slot;
            o.prev = last;
            lvl.lastOrder = slot;
        }

        lvl.limitVolume += o.shares;
    }

    void remove_order_from_level(idx_t idx, Level& lvl) {
    #pragma HLS INLINE
        Order& o = orders[idx];

        if (o.prev != -1)
            orders[o.prev].next = o.next;
        else
            lvl.firstOrder = o.next;

        idx_t p = o.prev;
        idx_t n = o.next;
        if (p != -1) orders[p].next = n;
        else         lvl.firstOrder = n;
        if (n != -1) orders[n].prev = p;
        else         lvl.lastOrder  = p;

        o.prev = -1;
        o.next = -1;
    }

    void execute_order(const OrderExecutedMsg& msg) {
    #pragma HLS INLINE
        idx_t idx = find_order(msg.orderReferenceNumber);
        if (idx == -1) return;

        Order& o = orders[idx];
        char side = o.side;

        idx_t lvl_idx = find_level_idx(side, o.price);
        if (lvl_idx == -1) return;

        Level* levels = level_array(side);
        Level& lvl = levels[lvl_idx];

        shares_t exec = msg.executedShares;
        if (exec > o.shares) exec = o.shares;

        o.shares -= exec;
        lvl.limitVolume -= exec;

        if (o.shares == 0) {
            remove_order_from_level(idx, lvl);
            o.valid = false;
            maybe_delete_level(side, lvl_idx);
        }
    }

    void cancel_order(const OrderCancelMsg& msg) {
    #pragma HLS INLINE
        idx_t idx = find_order(msg.orderReferenceNumber);
        if (idx == -1) return;

        Order& o = orders[idx];
        char side = o.side;
        idx_t lvl_idx = find_level_idx(side, o.price);
        if (lvl_idx == -1) return;

        Level* lvls = level_array(side);
        Level& lvl = lvls[lvl_idx];

        shares_t canc = msg.cancelledShares;
        if (canc >= o.shares) canc = o.shares - 1;

        o.shares -= canc;
        lvl.limitVolume -= canc;
    }

    void delete_order(const OrderDeleteMsg& msg) {
    #pragma HLS INLINE
        idx_t idx = find_order(msg.orderReferenceNumber);
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

    void replace_order(const OrderReplaceMsg& msg) {
    #pragma HLS INLINE
        idx_t old = find_order(msg.originalOrderReferenceNumber);
        if (old == -1) return;

        Order& o = orders[old];
        char side = o.side;

        idx_t lvl_idx = find_level_idx(side, o.price);
        if (lvl_idx != -1) {
            Level* lvls = level_array(side);
            Level& lvl = lvls[lvl_idx];
            lvl.limitVolume -= o.shares;
            remove_order_from_level(old, lvl);
            maybe_delete_level(side, lvl_idx);
        }

        o.valid = false;

        AddOrderMsg newMsg;
        newMsg.buySellIndicator = side;
        newMsg.shares = msg.shares;
        newMsg.price = msg.price;

        add_order(newMsg);
    }

    // -----------------------------------------------------------
    // Queries
    // -----------------------------------------------------------

    price_t getBestBid() const {
    #pragma HLS INLINE off
        price_t best = 0;
        const Level* lvls = bidLevels;

        for (int i = 0; i < MAX_LEVELS; i++) {
        #pragma HLS PIPELINE II=2
            if (lvls[i].valid && lvls[i].limitVolume > 0) {
                if (lvls[i].price > best)
                    best = lvls[i].price;
            }
        }
        return best;
    }

    price_t getBestAsk() const {
    #pragma HLS INLINE
        bool   found = false;
        price_t best = 0;

        const Level* lvls = askLevels;

        for (int i = 0; i < MAX_LEVELS; i++) {
        #pragma HLS PIPELINE II=2
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
        #pragma HLS PIPELINE II=1
            if (orders[i].valid)
                c++;
        }
        return c;
    }
};

// ===============================================================
// TOP LEVEL FUNCTION
// ===============================================================
void orderbook_dut(hls::stream<OBInput> &in,
                   hls::stream<OBOutput> &out) {
#pragma HLS INTERFACE axis port=in
#pragma HLS INTERFACE axis port=out
#pragma HLS INTERFACE ap_ctrl_none port=return

    static OrderBook ob;
#pragma HLS RESET variable=ob

    if (!in.empty()) {
        OBInput msg = in.read();

        MsgType mt = static_cast<MsgType>(msg.type.to_uint());

        switch (mt) {
            case MSG_ADD:     ob.add_order(msg.add);      break;
            case MSG_EXEC:    ob.execute_order(msg.exec); break;
            case MSG_CANCEL:  ob.cancel_order(msg.cancel);break;
            case MSG_DELETE:  ob.delete_order(msg.del);   break;
            case MSG_REPLACE: ob.replace_order(msg.repl); break;
            default: break;
        }

        OBOutput o;
        o.bestBid    = ob.getBestBid();
        o.bestAsk    = ob.getBestAsk();
        o.orderCount = ob.countOrders();

        out.write(o);
    }
}
