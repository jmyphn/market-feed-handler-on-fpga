#include "hash_tbl.hpp"
#include "typedefs.h"
#include <iostream>

ap_uint<16> hash_func(key_type key) {
  // NOTE: ripped this hash function from GPT, there might be a better constant
  const ap_uint<32> A = 40503u;    // good odd 16-bit constant
  return ((uint32_t)key * A) >> 4; // keep top 12 bits → 0..4095
}

hash_entry *hash_tbl_lookup(hash_entry tbl[CAPACITY], key_type key) {
  // TODO
  ap_uint<16> idx = hash_func(key);
  for (int i = 0; i < CAPACITY; i++) {
    // TODO: find better probing pattern
    if (!tbl[idx].state == VALID) {
      continue;
    }
    if (tbl[idx].key == key)
      return &tbl[idx];
    idx = (idx + 1) % CAPACITY;
  }
  std::cerr << "We are looking for a key that doesn't exist in the table!"
            << std::endl;
  return nullptr;
}

void hash_tbl_remove(hash_tbl tbl, key_type key) {
  hash_entry *entry = hash_tbl_lookup(tbl, key);
#if ASSERT
  assert(entry != nullptr);
#endif
  entry->state = TOMBSTONE;
}

void hash_tbl_put(hash_tbl tbl, key_type key, val_type val) {
  ap_uint<16> idx = hash_func(key);
  for (int i = 0; i < CAPACITY; i++) {
    if (~(tbl[key].state &
          0)) { // bit-arithmetic to check if its TOMBSTONE or EMPTY
      tbl[key].value = val;
      tbl[key].value = VALID;
      return;
    }
    idx = (idx + 1) % CAPACITY; // TODO: better probing here too
  }
}
