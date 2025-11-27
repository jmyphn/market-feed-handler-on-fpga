/**
 * Not really a general hash table.
 * Designed to be tightly-coupled with a priority queue. Because of this, a hash
 * table doesn't need have a size that's a part of it, because it should be the
 * same as the priority queue it's coupled with. Maybe it's better design to
 * have a hash table to have a size field we don't use?
 */

#include "typedefs.h"

typedef ap_uint<64> key_type;
typedef ap_uint<32> val_type;

enum hash_state { EMPTY = 0, VALID, TOMBSTONE };

struct hash_entry {
  key_type key;   // order_index
  val_type value; // shares
  hash_state state;
};

typedef hash_entry hash_tbl[2 * CAPACITY];

/**
 * Tries to find it in the table. Returns its corresponding index. Returns -1 if
 * nothing is found.
 */
int hash_tbl_lookup(hash_tbl tbl, key_type key);

/**
 * Adds a new key, value pair. Assumes key is not in the table.
 */
void hash_tbl_put(hash_tbl tbl, key_type key, val_type val);
