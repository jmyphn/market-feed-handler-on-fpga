//===========================================================================
// typedefs.h
//===========================================================================
// @brief: This header defines the shorthand of several ap_uint data types.

#ifndef TYPEDEFS
#define TYPEDEFS

#include <ap_int.h>

typedef bool bit;
typedef ap_uint<32> bit32_t;

struct ParsedMessage {
    ap_uint<8>  type;
    ap_uint<8>  side;
    ap_uint<64> order_id;
    ap_uint<64> new_order_id;
    ap_uint<32> shares;
    ap_uint<32> price;
};

#endif
