#include "orderbook.hpp"
#include "priority_queue.hpp"
#include "typedefs.h"

#define ASSERT true
#if ASSERT
  #include <cassert>
#endif


void orderbook(hls::stream<ParsedMessage> &orders, hls::stream<bit32_t> &spot_prices) {
  static priority_queue bid_pq;
  static priority_queue bid_remove_pq;
  static priority_queue ask_pq;
  static priority_queue ask_remove_pq;

  #if ASSERT
    assert(!orders.empty());
  #endif

  ParsedMessage order = orders.read();
  priority_queue &curr_pq = (order.side == 'b') ? bid_pq : ask_pq;
  priority_queue &curr_remove_pq = ((order.side) == 'b') ? bid_remove_pq : ask_remove_pq;

  // Process order depending on type
  switch (order.type) {
    case 'A':  // Add Order Message
      keep_slim(curr_pq);
      pq_push(curr_pq, order);
      break;

    case 'Q':  // Cross Trade Message
      // TODO.
      // THINK(tean): need to augment double-heap, since prices aren't tracked. we should track price per order?
      // i'm thinking we can use a circular buffer of some kind...
      // keep_slim(curr_remove_pq);
      // pq_push(curr_remove_pq, order);
      // balance(curr_pq, curr_remove_pq);
      break;

    case 'B':  // Broken Trade Message
      // TODO

    default:
      #if ASSERT
        // message type is probably not implemented
        assert(false);
      #endif
  }

  // Output a spot price
  if (bid_pq.size == 0 && ask_pq.size == 0)
    spot_prices.write(0xdeadbeef);  // try to fail
  else if (bid_pq.size == 0)
    spot_prices.write(pq_top(ask_pq).price);
  else if (ask_pq.size == 0)
    spot_prices.write(pq_top(bid_pq).price);
  else
    spot_prices.write((pq_top(ask_pq).price + pq_top(bid_pq).price) << 1);





}
