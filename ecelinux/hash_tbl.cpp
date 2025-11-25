#include "hash_tbl.hpp"
#include "typedefs.h"
#include <iostream>

ap_uint<16> hash_func(key_type key) {
  // NOTE: ripped this hash function from GPT, there might be a better constant
  const ap_uint<32> A = 40503u;          // good odd 16-bit constant
  return ((uint32_t)key * A) % CAPACITY; // keep top 12 bits → 0..4095
}

hash_entry *hash_tbl_lookup(hash_tbl tbl, key_type key) {
  ap_uint<16> idx = hash_func(key);
  for (int i = 0; i < CAPACITY; i++) {
    // TODO: find better probing pattern
    if (tbl[idx].state == VALID && tbl[idx].key == key) {
      return &tbl[idx];
    } else if (tbl[idx].state == EMPTY)
      break;
    idx = (idx + 1) % CAPACITY;
  }
  return nullptr;
}

void hash_tbl_put(hash_tbl tbl, key_type key, val_type val) {
  ap_uint<16> idx = hash_func(key);
  for (int i = 0; i < CAPACITY; i++) {
    // bit-arithmetic to check if its TOMBSTONE or EMPTY
    if (~(tbl[key].state & 0)) {
      tbl[idx].key = key;
      tbl[idx].value = val;
      tbl[idx].state = VALID;
      return;
    }
    idx = (idx + 1) % CAPACITY; // TODO: better probing here too
  }
}
