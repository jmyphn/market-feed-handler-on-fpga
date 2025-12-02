#ifndef P_HEAP_HPP
#define P_HEAP_HPP

#include "itch.hpp"

// These should match the constants in your original design
#define CAPACITY 4096
#define LEVELS 12 // log2(CAPACITY)

// A hardware-optimized pipelined heap (P-heap)
struct PipelinedHeap {
  ParsedMessage heap[LEVELS][CAPACITY / 2];
  unsigned counter;
  int hole_counter;
  int hole_idx[CAPACITY];
  int hole_lvl[CAPACITY];
  bool is_max_heap; // True for bids (max-heap), false for asks (min-heap)
};

// --- Function Declarations ---

// Initializes the heap with default values
void p_heap_init(PipelinedHeap &p_heap, bool is_max_heap);

// Adds a new order to the heap, maintaining heap property
void p_heap_add(PipelinedHeap &p_heap, ParsedMessage &new_order);

// Removes the top element from the heap and re-heapifies
void p_heap_remove_top(PipelinedHeap &p_heap);

// Returns the top element of the heap
ParsedMessage p_heap_get_top(const PipelinedHeap &p_heap);

#endif // P_HEAP_HPP
