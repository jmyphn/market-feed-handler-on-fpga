#include "priority_queue.hpp"
#include "typedefs.h"
#include <utility>

#if ASSERT
#include <cassert>
#endif

/*
 * Given a root index, returns its parent's index.
 * Examples: parent_idx(1) = 0, parent_idx(2) = 0
 * PRE: idx > 0
 */
int parent_idx(int idx) {
#pragma HLS INLINE
#if ASSERT
    assert(idx > 0);
#endif
    return (idx - 1) >> 1;
}

int level_of_idx(int idx) {
#pragma HLS INLINE
    const int MAX_LEVEL = 32;   // more than enough for CAPACITY<=2^32

    idx = idx + 1;
    int level = 0;

LEVEL_LOOP:
    for (int i = 0; i < MAX_LEVEL; i++) {
#pragma HLS UNROLL
        if ((idx >> i) == 0) break;
        // Check if high bit changed during this shift
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
        if (o1.side == 'b')     // bids prefer HIGHER price
            return o1.price > o2.price;
        else                    // asks prefer LOWER price
            return o1.price < o2.price;
    }
    return o1.order_id < o2.order_id;
}

ParsedMessage &pq_top(priority_queue &pq) {
#pragma HLS INLINE
    return pq.heap[0];
}

void pq_push(priority_queue &pq, ParsedMessage &order) {
    pq.heap[pq.size] = order;

    int insert_level = level_of_idx(pq.size);
    int curr = pq.size;

PUSH_BUBBLE_UP:
    for (int i = insert_level; i > 0; --i) {
#pragma HLS PIPELINE II=1
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

void pq_pop(priority_queue &pq) {
#if ASSERT
    assert(pq.size > 0);
#endif

    pq.size--;
    std::swap(pq.heap[0], pq.heap[pq.size]);

    int curr = 0;

POP_SIFT_DOWN:
    for (int i = 0; i < CAPACITY; i++) {
#pragma HLS PIPELINE II=1
        int l = 2 * curr + 1;
        int r = 2 * curr + 2;
        int best = curr;

        if (l < pq.size && cmp(pq.heap[l], pq.heap[best])) best = l;
        if (r < pq.size && cmp(pq.heap[r], pq.heap[best])) best = r;

        if (best == curr)
            break;

        std::swap(pq.heap[curr], pq.heap[best]);
        curr = best;
    }
}
