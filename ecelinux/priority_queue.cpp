#include "priority_queue.hpp"
#include "typedefs.h"

#if ASSERT
#include <cassert>
#endif

/*
 * Given a root index, returns its parent's index.
 * Examples: parent_idx(1) = 0, parent_idx(2) = 0
 * PRE: idx > 0
 */
int parent_idx(int idx) {
#pragma hls inline
#if ASSERT
  assert(idx > 0);
#endif
  return (idx - 1) >> 1;
}

/*
 * Return the depth of the heap that a given index corresponds to.
 *
 * mapping looks roughly like this:
 * level      heap
 * 0             0
 * 1            1 2
 * 2           3 4 5
 * 3          4 5 6 7
 */
int level_of_idx(int idx) {
#pragma hls inline
  // TODO
  ++idx;
  int level = 0;
  while (idx >>= 1) { // keep shifting until zero
    ++level;
  }
  return level;
}

bool cmp(ParsedMessage &o1, ParsedMessage &o2) {
#pragma hls inline
  if (o1.price != o2.price) {
    if (o1.side == 'b') // if it's bid, higher price is better
      return o1.price > o2.price;
    else // for asks, lower price is better
      return o1.price < o2.price;
  }
  return o1.order_id < o2.order_id;
}

ParsedMessage &pq_top(priority_queue &pq) {
#pragma hls inline
  return pq.heap[0];
}

void pq_push(priority_queue &pq, ParsedMessage &order) {
  pq.heap[pq.size] = order;
  int insert_level = level_of_idx(pq.size);
  for (int i = insert_level; i > 0; --i) {
    int parent = parent_idx(pq.size);
    if (cmp(order, pq.heap[parent])) {
      std::swap(pq.heap[pq.size], pq.heap[parent]);
    } else
      break;
  }
  pq.size++;
}

void pq_pop(priority_queue &pq) {
#if ASSERT
  assert(pq.size > 0);
#endif

  --pq.size;
  std::swap(pq.heap[0], pq.heap[pq.size]);
  int curr = 0;
  while (curr < pq.size) {
    int l = 2 * curr + 1;
    int r = 2 * curr + 2;
    int best = curr;
    if (l < pq.size && cmp(pq.heap[l], pq.heap[best]))
      best = l;
    if (r < pq.size && cmp(pq.heap[r], pq.heap[best]))
      best = r;

    if (best == curr)
      break;

    std::swap(pq.heap[curr], pq.heap[best]);
    curr = best;
  }
  ;
}
