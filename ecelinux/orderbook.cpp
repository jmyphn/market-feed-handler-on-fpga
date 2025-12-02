#include "orderbook.hpp"
#include "p_heap.hpp"
#include "hash_tbl.hpp"
#include "itch_common.hpp"
#include "typedefs.h"
#include <iostream>

#define ASSERT true
#if ASSERT
#include <cassert>
#endif

// --- Forward Declarations ---
void balance_and_get_top(PipelinedHeap &p_heap, hash_tbl tbl, ParsedMessage &top_order);
void match_orders(PipelinedHeap &bid_book, PipelinedHeap &ask_book, hash_tbl tbl);

/**
 * @brief Balances the heap by removing stale top entries and returns the current valid top.
 */
void balance_and_get_top(PipelinedHeap &p_heap, hash_tbl tbl, ParsedMessage &top_order) {
#pragma HLS INLINE
    while (p_heap.counter > 0) {
        ParsedMessage current_top = p_heap_get_top(p_heap);
        int idx = hash_tbl_lookup(tbl, current_top.order_id);

        if (idx == -1 || tbl[idx].value == 0) {
            // This order is stale, remove it from the heap
            p_heap_remove_top(p_heap);
        } else {
            // Top is valid
            top_order = current_top;
            return;
        }
    }
    // If loop finishes, the book is empty. Return a dummy order.
    top_order.order_id = 0;
    top_order.price = p_heap.is_max_heap ? 0 : 0xFFFFFFFF;
}

/**
 * @brief Matches aggressive orders against the resting book.
 */
void match_orders(PipelinedHeap &bid_book, PipelinedHeap &ask_book, hash_tbl tbl) {
#pragma HLS INLINE
    ParsedMessage best_bid, best_ask;
    balance_and_get_top(bid_book, tbl, best_bid);
    balance_and_get_top(ask_book, tbl, best_ask);

    while (best_bid.order_id != 0 && best_ask.order_id != 0 && best_bid.price >= best_ask.price) {
        int bid_idx = hash_tbl_lookup(tbl, best_bid.order_id);
        int ask_idx = hash_tbl_lookup(tbl, best_ask.order_id);

#if ASSERT
        assert(bid_idx != -1 && ask_idx != -1);
#endif

        hash_entry &bid_entry = tbl[bid_idx];
        hash_entry &ask_entry = tbl[ask_idx];

        val_type trade_size = (bid_entry.value < ask_entry.value) ? bid_entry.value : ask_entry.value;

        bid_entry.value -= trade_size;
        ask_entry.value -= trade_size;

        // Re-balance and get the new top orders after the trade
        balance_and_get_top(bid_book, tbl, best_bid);
        balance_and_get_top(ask_book, tbl, best_ask);
    }
}

/**
 * @brief Main order book processing logic.
 */
void orderbook(hls::stream<ParsedMessage> &orders,
               hls::stream<bit32_t> &spot_prices) {

    static PipelinedHeap bid_book;
    static PipelinedHeap ask_book;
    static hash_tbl shares_per_order;

    static bool is_initialized = false;
    if (!is_initialized) {
        p_heap_init(bid_book, true);  // true for max-heap (bids)
        p_heap_init(ask_book, false); // false for min-heap (asks)
        hash_tbl_init(shares_per_order);
        is_initialized = true;
    }

    ParsedMessage order = orders.read();

    // Process order based on its type
    switch (order.type) {
    case ITCH::AddOrderMessageType: {
        if (order.side == 'B') {
            p_heap_add(bid_book, order);
        } else {
            p_heap_add(ask_book, order);
        }
        hash_tbl_put(shares_per_order, order.order_id, order.shares);
        break;
    }
    case ITCH::OrderExecutedMessageType:
    case ITCH::OrderExecutedWithPriceMessageType:
    case ITCH::OrderCancelMessageType: {
        int idx = hash_tbl_lookup(shares_per_order, order.order_id);
        if (idx != -1) {
            if (shares_per_order[idx].value > order.shares) {
                shares_per_order[idx].value -= order.shares;
            } else {
                shares_per_order[idx].value = 0;
            }
        }
        break;
    }
    case ITCH::OrderDeleteMessageType: {
        int idx = hash_tbl_lookup(shares_per_order, order.order_id);
        if (idx != -1) {
            shares_per_order[idx].value = 0;
        }
        break;
    }
    case ITCH::OrderReplaceMessageType: {
        // 1. Mark the old order as deleted in the hash table
        int old_idx = hash_tbl_lookup(shares_per_order, order.order_id);
        if (old_idx != -1) {
            shares_per_order[old_idx].value = 0;
        }

        // 2. Add the new order with the new ID
        ParsedMessage new_order = order;
        new_order.order_id = order.new_order_id;
        if (new_order.side == 'B') {
            p_heap_add(bid_book, new_order);
        } else {
            p_heap_add(ask_book, new_order);
        }
        hash_tbl_put(shares_per_order, new_order.order_id, new_order.shares);
        break;
    }
    default:
        break;
    }

    // After any action, match orders and determine the new spot price
    match_orders(bid_book, ask_book, shares_per_order);

    ParsedMessage best_bid, best_ask;
    balance_and_get_top(bid_book, shares_per_order, best_bid);
    balance_and_get_top(ask_book, shares_per_order, best_ask);

    // Output a spot price
    bit32_t spot_price;
    if (best_bid.order_id == 0 || best_ask.order_id == 0) {
        if (best_bid.order_id != 0) spot_price = best_bid.price;
        else if (best_ask.order_id != 0) spot_price = best_ask.price;
        else spot_price = 0; // Book is empty
    } else {
        spot_price = (best_ask.price + best_bid.price) >> 1; // Midpoint
    }
    spot_prices.write(spot_price);
}
