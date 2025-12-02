#include "p_heap.hpp"
#include <algorithm> // For std::min/max, used in simulation

// --- Internal Helper Functions ---

// Pre-computed log2 for HLS efficiency. A full ROM is large, so for simulation
// a programmatic approach is fine. For synthesis, a ROM is preferred.
static int log_base_2(unsigned index) {
#pragma HLS INLINE
  if (index == 0) return 0;
  int result = 0;
  unsigned temp = index;
  while (temp > 1) {
#pragma HLS UNROLL
    temp >>= 1;
    result++;
  }
  return result;
}

static int pow2(int level) {
#pragma HLS INLINE
  return 1 << level;
}

// Finds the insertion path for a new element, using holes if available.
static int find_path(unsigned &counter, int &hole_counter, int hole_idx[CAPACITY], int level) {
#pragma HLS INLINE
  if (hole_counter > 0) {
    int temp = hole_idx[hole_counter];
    hole_counter--;
    return temp;
  } else {
    int leaf_node = pow2(level);
    return counter - leaf_node;
  }
}

// Calculates the index in the next level of the heap array.
static unsigned calculate_index(int insert_path, int level, int idx) {
#pragma HLS INLINE
  unsigned temp = (insert_path >> level) & 1;
  return (2 * idx) + temp;
}

// Generic comparison function for price-time priority.
// Returns true if 'a' has higher priority than 'b'.
static bool compare(const ParsedMessage &a, const ParsedMessage &b, bool is_max_heap) {
#pragma HLS INLINE
  if (is_max_heap) { // For Bids (Max-Heap)
    if (a.price > b.price) return true;
    if (a.price == b.price && a.timestamp < b.timestamp) return true;
  } else { // For Asks (Min-Heap)
    if (a.price < b.price) return true;
    if (a.price == b.price && a.timestamp < b.timestamp) return true;
  }
  return false;
}

// --- Public API Implementation ---

/**
 * @brief Initializes a pipelined heap.
 */
void p_heap_init(PipelinedHeap &p_heap, bool is_max_heap) {
  p_heap.is_max_heap = is_max_heap;
  p_heap.counter = 0;
  p_heap.hole_counter = 0;

  // Initialize heap with dummy values
  ParsedMessage dummy_order;
  dummy_order.order_id = 0;
  dummy_order.shares = 0;
  dummy_order.timestamp = 0;
  // For a min-heap (asks), use a very high price. For a max-heap (bids), use 0.
  dummy_order.price = is_max_heap ? 0 : 0xFFFFFFFF;

  for (int i = 0; i < LEVELS; ++i) {
    for (int j = 0; j < (CAPACITY / 2); ++j) {
#pragma HLS PIPELINE
      p_heap.heap[i][j] = dummy_order;
    }
  }
}

/**
 * @brief Adds a new order to the heap, bubbling it up to maintain heap property.
 */
void p_heap_add(PipelinedHeap &p_heap, ParsedMessage &new_order) {
#pragma HLS INLINE
  p_heap.counter++;
  int insert_level;
  if (p_heap.hole_counter > 0) {
    insert_level = p_heap.hole_lvl[p_heap.hole_counter];
  } else {
    insert_level = log_base_2(p_heap.counter);
  }

  int insert_path = find_path(p_heap.counter, p_heap.hole_counter, p_heap.hole_idx, insert_level);
  unsigned level = 0;
  unsigned new_idx = 0;

ADD_LOOP:
  for (int i = insert_level; i > 0; i--) {
#pragma HLS DEPENDENCE variable=p_heap.heap inter false
#pragma HLS LOOP_TRIPCOUNT max=11
#pragma HLS PIPELINE II=1
    if (compare(new_order, p_heap.heap[level][new_idx], p_heap.is_max_heap)) {
      ParsedMessage temp = p_heap.heap[level][new_idx];
      p_heap.heap[level][new_idx] = new_order;
      new_order = temp;
    }
    new_idx = calculate_index(insert_path, i - 1, new_idx);
    level++;
  }
  p_heap.heap[level][new_idx] = new_order;
}

/**
 * @brief Removes the top element and re-heapifies by sinking down.
 */
void p_heap_remove_top(PipelinedHeap &p_heap) {
#pragma HLS INLINE
  if (p_heap.counter == 0) return;

  p_heap.counter--;
  p_heap.hole_counter++;

  unsigned level = 0;
  unsigned new_idx = 0;

REMOVE_LOOP:
  while (level < LEVELS - 1) {
#pragma HLS DEPENDENCE variable=p_heap.heap inter false
#pragma HLS LOOP_TRIPCOUNT max=11
#pragma HLS PIPELINE II=1
    ParsedMessage left = p_heap.heap[level + 1][new_idx * 2];
    ParsedMessage right = p_heap.heap[level + 1][(new_idx * 2) + 1];

    // Determine which child has higher priority
    bool left_has_priority = compare(left, right, p_heap.is_max_heap);
    
    // Handle cases where one child might be a dummy node
    if (left.order_id != 0 && right.order_id == 0) {
        left_has_priority = true;
    } else if (left.order_id == 0 && right.order_id != 0) {
        left_has_priority = false;
    }

    if (left_has_priority) {
      p_heap.heap[level][new_idx] = left;
      new_idx = (new_idx * 2);
    } else {
      p_heap.heap[level][new_idx] = right;
      new_idx = (new_idx * 2) + 1;
    }
    level++;
  }

  // Record the location of the new hole
  p_heap.hole_lvl[p_heap.hole_counter] = level;
  p_heap.hole_idx[p_heap.hole_counter] = new_idx;

  // Place a dummy order in the hole
  ParsedMessage dummy_order;
  dummy_order.order_id = 0;
  dummy_order.price = p_heap.is_max_heap ? 0 : 0xFFFFFFFF;
  p_heap.heap[level][new_idx] = dummy_order;
}

/**
 * @brief Returns the top element of the heap without removing it.
 */
ParsedMessage p_heap_get_top(const PipelinedHeap &p_heap) {
#pragma HLS INLINE
  return p_heap.heap[0][0];
}
