#include "priority_queue.hpp"
#include <cmath>


#define LEVELS = 12  // TODO: consteval this based off of capacity?
/*
 * Simple implementation of priority queue using an array.
 *
 * [0, 1, 5, 7, 3] with size 4 refers to the heap:
 *      0
 *     / \
 *    1   5
 *   /
 *  7
 */
struct priority_queue {
  order heap[CAPACITY];
  int   size;  // TODO: change this to a smaller ap_uint
};

/*
 * Given a root index, returns its parent's index.
 * Examples: parent_idx(1) = 0, parent_idx(2) = 0
 * PRE: idx > 0
 */
int parent_idx(int idx) {
#pragma hls inline
  return (idx - 1) >> 1;
}

/*
 * Return the depth of the heap that a given index corresponds to.
 */
int level_of_idx(int idx) {
#pragma hls inline
  // TODO
}

/*
 * If pq is at max size, remove an arbitrary element that's not the biggest.
 * POST: pq is not at full capacity
 */
void keep_slim(priority_queue &pq) {
#pragma hls inline
  if (pq.size == CAPACITY - 1) --pq.size;
}

void pq_top(priority_queue &pq) {
#pragma hls inline
  return pq.heap[0];
}

/*
 * Pushes new_order into pq.
 * PRE: pq is not at full capacity
 */
void pq_push(priority_queue &pq, order &new_order) {
  pq.heap[pq.size] = new_order;
  int insert_level = level_of_idx(pq.size);
  for (int i = insert_level; i > 0; --i) {
    // TODO: figure out how to implement this comparator. should it be an input to the function? how many comparator functions might we have?
    int parent_idx = parent_idx(pq.size);
    if (cmp(new_order, pq.heap[parent_idx]) > 0)
      swap(pq.heap[pq.size], pq.heap[parent_idx]);
    else break;
  }
  pq.size++;
}

void balance(priority_queue &pq, priority_queue &remove_pq) {
  // TODO
}

// TODO: probably better to refactor order book out of priority queue
void process_order(stream<order> orders, stream<price4> spot_prices) {
  // OPT: one optimization is to restructure these heaps into "levels"
  static priority_queue bid_pq;
  static priority_queue bid_remove_pq;
  static priority_queue ask_pq;
  static priority_queue ask_remove_pq;

  if (!orders.empty() & !spot_prices.full()) {
    order new_order = orders.read();
    switch (orders.type) {
      // TODO: make enums for the different types, case on them
      // case: add bid
      keep_slim(bid_pq);
      pq_push(bid_pq, new_order);

      // case: remove bid
      keep_slim(bid_remove_pq);
      pq_push(bid_remove_pq, new_order);
      balance(bid_pq, bid_remove_pq);

      // case: add ask
      keep_slim(ask_pq);
      pq_push(ask_pq, new_order);

      // case: remove ask
      keep_slim(ask_remove_pq);
      pq_push(ask_remove_pq, new_order);
      balance(ask_pq, ask_remove_pq);
    }


  }


}
