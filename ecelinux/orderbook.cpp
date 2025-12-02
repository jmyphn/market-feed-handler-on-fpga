#include "orderbook.hpp"
#include <cassert>
#include <iostream>

// ===============================================================
// OrderBook Class Method Implementations
// ===============================================================

OrderBook::OrderBook() {
#pragma HLS INLINE
    init();
}

void OrderBook::init() {
#pragma HLS INLINE
    for (int i = 0; i < MAX_ORDERS; ++i) {
#pragma HLS UNROLL
        orders[i].valid = false;
    }
    for (int i = 0; i < MAX_LEVELS; ++i) {
#pragma HLS UNROLL
        bidLevels[i].valid = false;
        askLevels[i].valid = false;
    }
}

idx_t OrderBook::find_free_order_slot() {
#pragma HLS INLINE
    idx_t free_idx = -1;
    for (int i = 0; i < MAX_ORDERS; ++i) {
        if (!orders[i].valid) {
            free_idx = i;
            break;
        }
    }
    return free_idx;
}

idx_t OrderBook::find_order(order_ref_t ref) {
#pragma HLS INLINE
    idx_t found_idx = -1;
    for (int i = 0; i < MAX_ORDERS; ++i) {
        if (orders[i].valid && orders[i].referenceNumber == ref) {
            found_idx = i;
            break;
        }
    }
    return found_idx;
}

Level* OrderBook::level_array(char side) {
#pragma HLS INLINE
    return (side == SIDE_BUY) ? bidLevels : askLevels;
}

const Level* OrderBook::level_array_const(char side) const {
#pragma HLS INLINE
    return (side == SIDE_BUY) ? bidLevels : askLevels;
}

idx_t OrderBook::find_level_idx(char side, price_t price) {
#pragma HLS INLINE
    Level* levels = level_array(side);
    idx_t found_idx = -1;
    for (int i = 0; i < MAX_LEVELS; ++i) {
        if (levels[i].valid && levels[i].price == price) {
            found_idx = i;
            break;
        }
    }
    return found_idx;
}

idx_t OrderBook::allocate_level(char side, price_t price) {
#pragma HLS INLINE
    Level* levels = level_array(side);
    idx_t free_idx = -1;
    for (int i = 0; i < MAX_LEVELS; ++i) {
        if (!levels[i].valid) {
            free_idx = i;
            break;
        }
    }
    if (free_idx != -1) {
        levels[free_idx].valid = true;
        levels[free_idx].price = price;
        levels[free_idx].limitVolume = 0;
        levels[free_idx].firstOrder = -1;
        levels[free_idx].lastOrder = -1;
    }
    return free_idx;
}

idx_t OrderBook::get_or_create_level(char side, price_t price) {
#pragma HLS INLINE
    idx_t lvl_idx = find_level_idx(side, price);
    if (lvl_idx == -1) {
        lvl_idx = allocate_level(side, price);
    }
    return lvl_idx;
}

void OrderBook::maybe_delete_level(char side, idx_t lvl_idx) {
#pragma HLS INLINE
    Level* levels = level_array(side);
    if (lvl_idx != -1 && levels[lvl_idx].firstOrder == -1) {
        levels[lvl_idx].valid = false;
    }
}

void OrderBook::remove_order_from_level(idx_t idx, Level& lvl) {
#pragma HLS INLINE
    Order& order = orders[idx];
    if (order.prev != -1) {
        orders[order.prev].next = order.next;
    } else {
        lvl.firstOrder = order.next;
    }
    if (order.next != -1) {
        orders[order.next].prev = order.prev;
    } else {
        lvl.lastOrder = order.prev;
    }
    lvl.limitVolume -= order.shares;
}

void OrderBook::add_order(const AddOrderMsg& msg) {
#pragma HLS INLINE
    idx_t order_idx = find_free_order_slot();
    if (order_idx == -1) return; // No space for new order

    idx_t lvl_idx = get_or_create_level(msg.buySellIndicator, msg.price);
    if (lvl_idx == -1) return; // No space for new level

    Level* levels = level_array(msg.buySellIndicator);
    Level& level = levels[lvl_idx];

    Order& new_order = orders[order_idx];
    new_order.referenceNumber = msg.orderReferenceNumber;
    new_order.side = msg.buySellIndicator;
    new_order.shares = msg.shares;
    new_order.price = msg.price;
    new_order.valid = true;
    new_order.next = -1;
    new_order.prev = level.lastOrder;

    if (level.lastOrder != -1) {
        orders[level.lastOrder].next = order_idx;
    } else {
        level.firstOrder = order_idx;
    }
    level.lastOrder = order_idx;
    level.limitVolume += msg.shares;
}

void OrderBook::execute_order(const OrderExecutedMsg& msg) {
#pragma HLS INLINE
    idx_t order_idx = find_order(msg.orderReferenceNumber);
    if (order_idx == -1) return;

    Order& order = orders[order_idx];
    if (msg.executedShares >= order.shares) {
        // Full execution, treat as delete
        delete_order({msg.orderReferenceNumber});
    } else {
        // Partial execution
        order.shares -= msg.executedShares;
        idx_t lvl_idx = find_level_idx(order.side, order.price);
        if (lvl_idx != -1) {
            level_array(order.side)[lvl_idx].limitVolume -= msg.executedShares;
        }
    }
}

void OrderBook::cancel_order(const OrderCancelMsg& msg) {
#pragma HLS INLINE
    idx_t order_idx = find_order(msg.orderReferenceNumber);
    if (order_idx == -1) return;

    Order& order = orders[order_idx];
    if (msg.cancelledShares >= order.shares) {
        // Full cancel, treat as delete
        delete_order({msg.orderReferenceNumber});
    } else {
        // Partial cancel
        order.shares -= msg.cancelledShares;
        idx_t lvl_idx = find_level_idx(order.side, order.price);
        if (lvl_idx != -1) {
            level_array(order.side)[lvl_idx].limitVolume -= msg.cancelledShares;
        }
    }
}

void OrderBook::delete_order(const OrderDeleteMsg& msg) {
#pragma HLS INLINE
    idx_t order_idx = find_order(msg.orderReferenceNumber);
    if (order_idx == -1) return;

    Order& order = orders[order_idx];
    idx_t lvl_idx = find_level_idx(order.side, order.price);
    if (lvl_idx != -1) {
        Level& level = level_array(order.side)[lvl_idx];
        remove_order_from_level(order_idx, level);
        maybe_delete_level(order.side, lvl_idx);
    }
    order.valid = false;
}

void OrderBook::replace_order(const OrderReplaceMsg& msg) {
#pragma HLS INLINE
    // Find original order
    idx_t old_order_idx = find_order(msg.originalOrderReferenceNumber);
    if (old_order_idx == -1) return;

    Order& old_order = orders[old_order_idx];
    char side = old_order.side;

    // Remove old order from its level
    idx_t old_lvl_idx = find_level_idx(side, old_order.price);
    if (old_lvl_idx != -1) {
        Level& old_level = level_array(side)[old_lvl_idx];
        remove_order_from_level(old_order_idx, old_level);
        maybe_delete_level(side, old_lvl_idx);
    }

    // Re-purpose the order slot with new data
    old_order.referenceNumber = msg.newOrderReferenceNumber;
    old_order.shares = msg.shares;
    old_order.price = msg.price;
    // side remains the same

    // Add the 'new' order to the correct level
    idx_t new_lvl_idx = get_or_create_level(side, msg.price);
    if (new_lvl_idx == -1) { // Should not happen if MAX_LEVELS is large enough
        old_order.valid = false; // Could not place, so invalidate
        return;
    }
    Level& new_level = level_array(side)[new_lvl_idx];

    old_order.next = -1;
    old_order.prev = new_level.lastOrder;
    if (new_level.lastOrder != -1) {
        orders[new_level.lastOrder].next = old_order_idx;
    } else {
        new_level.firstOrder = old_order_idx;
    }
    new_level.lastOrder = old_order_idx;
    new_level.limitVolume += msg.shares;
}

void OrderBook::update_order(const OrderUpdateMsg& msg) {
#pragma HLS INLINE
    idx_t order_idx = find_order(msg.orderReferenceNumber);
    if (order_idx == -1) return;

    Order& order = orders[order_idx];
    char side = order.side;
    price_t old_price = order.price;
    shares_t old_shares = order.shares;

    // If price is different, it's a move.
    if (msg.newPrice != old_price) {
        // 1. Remove from old level
        idx_t old_lvl_idx = find_level_idx(side, old_price);
        if (old_lvl_idx != -1) {
            Level& old_level = level_array(side)[old_lvl_idx];
            remove_order_from_level(order_idx, old_level);
            maybe_delete_level(side, old_lvl_idx);
        }

        // 2. Update order details
        order.price = msg.newPrice;
        order.shares = msg.newShares;

        // 3. Add to new level
        idx_t new_lvl_idx = get_or_create_level(side, msg.newPrice);
        if (new_lvl_idx == -1) { // Should not happen
            order.valid = false; // Invalidate if we can't place it
            return;
        }
        Level& new_level = level_array(side)[new_lvl_idx];
        order.next = -1;
        order.prev = new_level.lastOrder;
        if (new_level.lastOrder != -1) {
            orders[new_level.lastOrder].next = order_idx;
        } else {
            new_level.firstOrder = order_idx;
        }
        new_level.lastOrder = order_idx;
        new_level.limitVolume += msg.newShares;

    } else { // Price is the same, just update shares
        idx_t lvl_idx = find_level_idx(side, old_price);
        if (lvl_idx != -1) {
            level_array(side)[lvl_idx].limitVolume -= old_shares;
            level_array(side)[lvl_idx].limitVolume += msg.newShares;
        }
        order.shares = msg.newShares;
    }
}


price_t OrderBook::getBestBid() const {
#pragma HLS INLINE
    price_t best_price = 0;
    for (int i = 0; i < MAX_LEVELS; ++i) {
        if (bidLevels[i].valid && bidLevels[i].price > best_price) {
            best_price = bidLevels[i].price;
        }
    }
    return best_price;
}

price_t OrderBook::getBestAsk() const {
#pragma HLS INLINE
    price_t best_price = 0xFFFFFFFF; // Max price
    for (int i = 0; i < MAX_LEVELS; ++i) {
        if (askLevels[i].valid && askLevels[i].price < best_price) {
            best_price = askLevels[i].price;
        }
    }
    return best_price;
}

ap_uint<16> OrderBook::countOrders() const {
#pragma HLS INLINE
    ap_uint<16> count = 0;
    for (int i = 0; i < MAX_ORDERS; ++i) {
        if (orders[i].valid) {
            count++;
        }
    }
    return count;
}


// ===============================================================
// TOP LEVEL FUNCTION
// ===============================================================
void orderbook_dut(hls::stream<OBInput> &in,
                   hls::stream<OBOutput> &out,
                   int num_msgs) {
#pragma HLS DATAFLOW
    static OrderBook ob;

    for (int i = 0; i < num_msgs; ++i) {
#pragma HLS PIPELINE II=1
        OBInput msg;
        if (in.read_nb(msg)) {
            switch (msg.type) {
                case MSG_ADD:     ob.add_order(msg.add);      break;
                case MSG_EXEC:    ob.execute_order(msg.exec); break;
                case MSG_CANCEL:  ob.cancel_order(msg.cancel);break;
                case MSG_DELETE:  ob.delete_order(msg.del);   break;
                case MSG_REPLACE: ob.replace_order(msg.repl); break;
                case MSG_UPDATE:  ob.update_order(msg.update);break;
            }

            OBOutput response;
            response.bestBid = ob.getBestBid();
            response.bestAsk = ob.getBestAsk();
            response.orderCount = ob.countOrders();
            out.write(response);
        }
    }
}