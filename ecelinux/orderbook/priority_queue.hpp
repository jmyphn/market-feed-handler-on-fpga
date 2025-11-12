#include <cfloat>
#include <hls_stream.h>
#include <ap_int.h>
#include "typedefs.cpp"

#define CAPACITY 4096

using namespace hls;

typedef ap_unit<64> Time;  // TODO: refactor this out into type file

/*
 * NOTES:
 *   - can add timestamp here if we decide to do something with that
 *   - might want to add a second ref_num for replace orders
 *
 */
struct order {
  ap_uint<3>  type;        /* order type:
                                000: order  ask  001: order  bid
                                010: cancel ask  011: cancel bid  <-- making this count for partial cancel and total delete for now THINK: do we want to separate them?
                                TODO: add replace?
                                TODO: */
  ap_uint<8>  shares;      // num of shares. can be meaningless for some order types, like deletions since they don't include share count.
  ap_uint<32> ref_num;    // unique reference number for each order
  price4    price;  // order price // TODO: also refactor this float type and name it?
}

/* Input a stream of orders, output a stream of spot prices. */
void process_order(stream<order> orders, stream<price4> spot_prices);
