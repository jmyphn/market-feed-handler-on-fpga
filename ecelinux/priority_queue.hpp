#include "typedefs.h"

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
  ParsedMessage
      heap[CAPACITY]; // OPT: one way to optimize is to store in levels
  int size;           // TODO: change this to a smaller ap_uint
};

ParsedMessage &pq_top(priority_queue &pq);

/*
 * Pushes new_order into pq.
 * PRE: pq is not at full capacity
 */
void pq_push(priority_queue &pq, ParsedMessage &order);

/**
 * Removes the top element in `pq`.
 */
void pq_pop(priority_queue &pq);
