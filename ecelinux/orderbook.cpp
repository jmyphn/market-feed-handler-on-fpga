#include <iostream>

#include "hash_tbl.hpp"
#include "itch_common.hpp"
#include "orderbook.hpp"
#include "priority_queue.hpp"
#include "typedefs.h"

#define ASSERT true
#if ASSERT
#include <cassert>
#endif

/*
 * If pq is full, drop last element (not max).
 */
void keep_slim(priority_queue &pq, hash_tbl tbl) {
#pragma HLS INLINE
    if (pq.size == CAPACITY) {
        --pq.size;

#ifndef __SYNTHESIS__
        std::cerr << "Keeping slim\n";
#endif

        ParsedMessage &order = pq.heap[pq.size];
        int idx = hash_tbl_lookup(tbl, order.order_id);

#if ASSERT
        assert(idx != -1);
#endif

        hash_entry &entry = tbl[idx];
        entry.value = 0;
        entry.state = TOMBSTONE;
    }
}

/*
 * Fixed-bound BALANCE loop.
 *
 * Removes up to CAPACITY heads, but exits early.
 */
void balance(priority_queue &pq, hash_tbl tbl) {
#pragma HLS INLINE off

BALANCE_LOOP:
    for (int i = 0; i < CAPACITY; i++) {
// #pragma HLS PIPELINE II=1
        #pragma HLS UNROLL factor=16

        if (pq.size == 0) break;

        ParsedMessage top_order = pq_top(pq);
        int idx = hash_tbl_lookup(tbl, top_order.order_id);

#if ASSERT
        assert(idx != -1);
#endif

        hash_entry &top_entry = tbl[idx];

        if (top_entry.value > 0)
            break;

        top_entry.value = 0;
        top_entry.state = TOMBSTONE;

        pq_pop(pq);
    }
}

/*
 * Remove shares from entry
 */
void remove_shares(hash_tbl tbl, key_type order_id, val_type &shares) {
#pragma HLS INLINE
    int idx = hash_tbl_lookup(tbl, order_id);
    if (idx != -1) {
        hash_entry &curr_entry = tbl[idx];
        if (curr_entry.value > shares)
            curr_entry.value -= shares;
        else
            curr_entry.value = 0;
    }
}

/*
 * Remove all shares for an entry
 */
void remove_all_shares(hash_tbl tbl, key_type order_id) {
#pragma HLS INLINE
    int idx = hash_tbl_lookup(tbl, order_id);
    if (idx != -1) {
        hash_entry &curr_entry = tbl[idx];
        curr_entry.value = 0;
    }
}

/*
 * Fully deterministic orderbook with fixed-bound loops.
 */
void orderbook(hls::stream<ParsedMessage> &orders,
               hls::stream<bit32_t> &spot_prices) {

#pragma HLS INLINE off

    static priority_queue bid_pq;
    static priority_queue ask_pq;
    static hash_tbl shares_per_order;

    ParsedMessage order = orders.read();

    switch (order.type) {

    case ITCH::AddOrderMessageType:
        if (order.side == 'b') {
            keep_slim(bid_pq, shares_per_order);
            pq_push(bid_pq, order);
        } else {
            keep_slim(ask_pq, shares_per_order);
            pq_push(ask_pq, order);
        }
        hash_tbl_put(shares_per_order, order.order_id, order.shares);
        break;

    case ITCH::OrderExecutedMessageType:
    case ITCH::OrderExecutedWithPriceMessageType:
    case ITCH::OrderCancelMessageType:
        remove_shares(shares_per_order, order.order_id, order.shares);
        balance(bid_pq, shares_per_order);
        balance(ask_pq, shares_per_order);
        break;

    case ITCH::OrderDeleteMessageType:
        remove_all_shares(shares_per_order, order.order_id);
        balance(bid_pq, shares_per_order);
        balance(ask_pq, shares_per_order);
        break;

    case ITCH::OrderReplaceMessageType:
        remove_all_shares(shares_per_order, order.order_id);
        balance(bid_pq, shares_per_order);
        balance(ask_pq, shares_per_order);

        if (order.side == 'b') {
            keep_slim(bid_pq, shares_per_order);
            pq_push(bid_pq, order);
        } else {
            keep_slim(ask_pq, shares_per_order);
            pq_push(ask_pq, order);
        }
        hash_tbl_put(shares_per_order, order.order_id, order.shares);
        break;

    default:
        break;
    }

    // Compute spot price
    bit32_t spot_price;

    if (bid_pq.size == 0 && ask_pq.size == 0)
        spot_price = 6767;
    else if (bid_pq.size == 0)
        spot_price = pq_top(ask_pq).price;
    else if (ask_pq.size == 0)
        spot_price = pq_top(bid_pq).price;
    else
        spot_price = (pq_top(ask_pq).price + pq_top(bid_pq).price) << 1;

    spot_prices.write(spot_price);
}
