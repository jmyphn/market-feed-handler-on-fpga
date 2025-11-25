#include <iostream>

#include "hash_tbl.hpp"
#include "itch_common.hpp"
#include "orderbook.hpp"
#include "priority_queue.hpp"
#include "typedefs.h"

#define ASSERT true

/*
 * If pq is at max size, remove an arbitrary element that's not the biggest.
 * Also deletes its corresponding element from the hashtable
 * POST: pq is not at full capacity
 */
void keep_slim(priority_queue &pq, hash_tbl tbl) {
#pragma hls inline
  if (pq.size == CAPACITY) {
    --pq.size;
    // std::cerr << "Keeping slim" << std::endl;
    ParsedMessage &order = pq.heap[pq.size];
    hash_entry *entry = hash_tbl_lookup(tbl, order.order_id);
#if ASSERT
    assert(entry != nullptr);
#endif
    // std::cerr << "Removing " << entry->key << std::endl;
    entry->value = 0;
    entry->state = TOMBSTONE;
  }
}

/**
 * This function removes the head of `pq` if it has 0 shares, as tracked by
 * `tbl`. It'll keep removing heads until `pq` is empty, or the head has some
 * shares left. Removing a head will also delete its corresponding entry in
 * `tbl`.
 */
void balance(priority_queue &pq, hash_tbl &tbl) {
  while (pq.size > 0) {
    ParsedMessage top_order = pq_top(pq);
    hash_entry *top_entry = hash_tbl_lookup(tbl, top_order.order_id);
#if ASSERT
    assert(top_entry != nullptr);
#endif
    if (top_entry->value > 0) {
      break;
    } else {
      // remove from hash table
      // std::cerr << "Removing " << top_entry->key << std::endl;
      top_entry->value = 0;
      top_entry->state = TOMBSTONE;
      pq_pop(pq);
    }
  }
}

/**
 * Removes shares from an entry in the orderbook based on `order_id`. Share
 * count will not drop below 0.
 */
void remove_shares(priority_queue &pq, hash_tbl &tbl, key_type order_id,
                   val_type shares) {
  hash_entry *curr_entry = hash_tbl_lookup(tbl, order_id);
  if (curr_entry != nullptr) {
    if (curr_entry->value > shares) {
      curr_entry->value -= shares;
    } else {
      curr_entry->value = 0;
    }
  }
}

/**
 * Removes all shares from a given order based on `order_id`.
 */
void remove_all_shares(priority_queue &pq, hash_tbl &tbl, key_type order_id) {
  hash_entry *curr_entry = hash_tbl_lookup(tbl, order_id);
  if (curr_entry != nullptr) {
    curr_entry->value = 0;
  }
}

void orderbook(hls::stream<ParsedMessage> &orders,
               hls::stream<bit32_t> &spot_prices) {
  static priority_queue bid_pq;
  static priority_queue ask_pq;
  static hash_tbl shares_per_order;

  // #if ASSERT
  //   assert(!orders.empty());
  // #endif

  ParsedMessage order = orders.read();
  priority_queue &curr_pq = (order.side == 'b') ? bid_pq : ask_pq;

  // Process order depending on type
  switch (order.type) {
  case ITCH::AddOrderMessageType: // Add Order Message
    keep_slim(curr_pq, shares_per_order);
    pq_push(curr_pq, order);
    hash_tbl_put(shares_per_order, order.order_id, order.shares);
    break;

  case ITCH::OrderExecutedMessageType:
  case ITCH::OrderExecutedWithPriceMessageType:
  case ITCH::OrderCancelMessageType:
    remove_shares(curr_pq, shares_per_order, order.order_id, order.shares);
    balance(bid_pq, shares_per_order);
    balance(ask_pq, shares_per_order);
    break;

  case ITCH::OrderDeleteMessageType:
    remove_all_shares(curr_pq, shares_per_order, order.order_id);
    balance(bid_pq, shares_per_order);
    balance(ask_pq, shares_per_order);
    break;

  case ITCH::OrderReplaceMessageType:
    remove_all_shares(curr_pq, shares_per_order, order.order_id);
    balance(bid_pq, shares_per_order);
    balance(ask_pq, shares_per_order);
    keep_slim(curr_pq, shares_per_order);
    pq_push(curr_pq, order);
    hash_tbl_put(shares_per_order, order.order_id, order.shares);
    break;

  default:
    // #if ASSERT
    //   // message type is probably not implemented
    //   assert(false);
    // #endif
    break;
  }

  // Output a spot price
  bit32_t spot_price;
  if (bid_pq.size == 0 && ask_pq.size == 0)
    spot_price = 6767; // try to fail
  else if (bid_pq.size == 0)
    spot_price = pq_top(ask_pq).price;
  else if (ask_pq.size == 0)
    spot_price = pq_top(bid_pq).price;
  else
    spot_price = (pq_top(ask_pq).price + pq_top(bid_pq).price) << 1;

  // std::cerr << "Top bid order_id is " << pq_top(ask_pq).order_id <<
  // std::endl;

  spot_prices.write(spot_price);
  std::cerr << "Order outputting spot_price of " << spot_price << std::endl;
}
