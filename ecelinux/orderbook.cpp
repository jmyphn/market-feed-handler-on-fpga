#include <iostream>

#include "hash_tbl.hpp"
#include "orderbook.hpp"
#include "priority_queue.hpp"
#include "typedefs.h"

#define ASSERT true

/*
 * If pq is at max size, remove an arbitrary element that's not the biggest.
 * Also deletes its corresponding element from the hashtablej
 * POST: pq is not at full capacity
 */
void keep_slim(priority_queue &pq, hash_tbl tbl) {
#pragma hls inline
  if (pq.size == CAPACITY) {
    --pq.size;
    std::cerr << "Keeping slim" << std::endl;
    ParsedMessage &order = pq.heap[pq.size];
    hash_tbl_remove(tbl, order.order_id);
  }
}

void balance(priority_queue &pq, hash_tbl tbl) {
  while (pq.size > 0) {
    ParsedMessage top_order = pq_top(pq);
    hash_entry *top_entry = hash_tbl_lookup(tbl, top_order.order_id);
#if ASSERT
    assert(top_entry != nullptr);
#endif
    if (top_entry->value >= 0) {
      break;
    } else {
      // remove from priority queue
      top_entry->value = 0;
      top_entry->state = TOMBSTONE;
      pq_pop(pq);
      // TODO: remove from hashtable
    }
  }
}

void remove_shares(priority_queue &pq, hash_tbl tbl, key_type order_id,
                   val_type shares) {
  hash_entry *curr_entry = hash_tbl_lookup(tbl, order_id);
#if ASSERT
  assert(curr_entry != nullptr);
#endif
  if (curr_entry->value > shares) {
    curr_entry->value -= shares;
  } else {
    curr_entry->value = 0;
    balance(pq, tbl);
  }
}

void orderbook(hls::stream<ParsedMessage> &orders,
               hls::stream<bit32_t> &spot_prices) {
  static priority_queue bid_pq;
  static hash_tbl bid_shares;
  static priority_queue ask_pq;
  static hash_tbl ask_shares;

#if ASSERT
  assert(!orders.empty());
#endif

  ParsedMessage order = orders.read();
  priority_queue &curr_pq = (order.side == 'b') ? bid_pq : ask_pq;
  hash_tbl &curr_shares = ((order.side) == 'b') ? bid_shares : ask_shares;

  // Process order depending on type
  switch (order.type) {
  case 'A': // Add Order Message
    keep_slim(curr_pq, curr_shares);
    pq_push(curr_pq, order);
    hash_tbl_put(curr_shares, order.order_id, order.shares);
    break;

  case 'E': // Order Executed Message
    remove_shares(curr_pq, curr_shares, order.order_id, order.shares);

    // TODO: rest of the cases

  default:
#if ASSERT
    // message type is probably not implemented
    assert(false);
#endif
  }

  // Output a spot price
  if (bid_pq.size == 0 && ask_pq.size == 0)
    spot_prices.write(0xdeadbeef); // try to fail
  else if (bid_pq.size == 0)
    spot_prices.write(pq_top(ask_pq).price);
  else if (ask_pq.size == 0)
    spot_prices.write(pq_top(bid_pq).price);
  else
    spot_prices.write((pq_top(ask_pq).price + pq_top(bid_pq).price) << 1);
}
