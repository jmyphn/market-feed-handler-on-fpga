#include "priority_queue.hpp"
#include "typedefs.h"
#include <utility>

#if ASSERT
#include <cassert>
#endif

// Compute log2(CAPACITY) at compile time
static const int MAX_LEVEL =
    (CAPACITY <= 1)      ? 0 :
    (CAPACITY <= 2)      ? 1 :
    (CAPACITY <= 4)      ? 2 :
    (CAPACITY <= 8)      ? 3 :
    (CAPACITY <= 16)     ? 4 :
    (CAPACITY <= 32)     ? 5 :
    (CAPACITY <= 64)     ? 6 :
    (CAPACITY <= 128)    ? 7 :
    (CAPACITY <= 256)    ? 8 :
    (CAPACITY <= 512)    ? 9 :
    (CAPACITY <= 1024)   ? 10 :
                           11;

/*
 * parent index: PRE: idx > 0
 */
int parent_idx(int idx) {
#pragma HLS INLINE
#if ASSERT
    assert(idx > 0);
#endif
    return (idx - 1) >> 1;
}

/*
 * Returns heap level of idx without variable loops.
 *
 * Equivalent to: while(idx >>= 1) level++;
 * But now fixed-bound for-loop.
 */
int level_of_idx(int idx) {
#pragma HLS INLINE
    int level = 0;
    idx = idx + 1;

LEVEL_LOOP:
    for (int i = 0; i < MAX_LEVEL; i++) {
#pragma HLS UNROLL
        // If shifting by (i+1) does not produce nonzero, stop.
        if ((idx >> (i + 1)) != 0)
            level++;
        else
            break;
    }
    return level;
}

bool cmp(ParsedMessage &o1, ParsedMessage &o2) {
#pragma HLS INLINE
    if (o1.price != o2.price) {
        if (o1.side == 'b')
            return o1.price > o2.price;
        else
            return o1.price < o2.price;
    }
    return o1.order_id < o2.order_id;
}

ParsedMessage &pq_top(priority_queue &pq) {
#pragma HLS INLINE
    return pq.heap[0];
}

/*
 * Pq push rewritten with fixed-bound for loop.
 */
void pq_push(priority_queue &pq, ParsedMessage &order) {
#pragma HLS INLINE off
    pq.heap[pq.size] = order;

    int curr = pq.size;

PUSH_LOOP:
    for (int i = 0; i < MAX_LEVEL; i++) {
#pragma HLS PIPELINE II=1
        if (curr == 0) break;

        int parent = parent_idx(curr);
        if (cmp(pq.heap[curr], pq.heap[parent])) {
            std::swap(pq.heap[curr], pq.heap[parent]);
            curr = parent;
        } else {
            break;
        }
    }

    pq.size++;
}

/*
 * Pq pop rewritten with fixed-bound for loop.
 */
void pq_pop(priority_queue &pq) {
#pragma HLS INLINE off
#if ASSERT
    assert(pq.size > 0);
#endif

    pq.size--;
    std::swap(pq.heap[0], pq.heap[pq.size]);

    int curr = 0;

SIFT_LOOP:
    for (int i = 0; i < MAX_LEVEL; i++) {
#pragma HLS PIPELINE II=1

        int l = 2 * curr + 1;
        int r = 2 * curr + 2;

        if (l >= pq.size) break;

        int best = curr;
        if (cmp(pq.heap[l], pq.heap[best])) best = l;
        if (r < pq.size && cmp(pq.heap[r], pq.heap[best])) best = r;

        if (best == curr) break;

        std::swap(pq.heap[curr], pq.heap[best]);
        curr = best;
    }
}
