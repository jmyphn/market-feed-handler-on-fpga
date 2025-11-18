#include <cfloat>
#include <hls_stream.h>
#include <ap_int.h>
#include "typedefs.h"

/* Input a stream of orders, output a stream of spot prices. */
void orderbook(hls::stream<ParsedMessage> &orders, hls::stream<bit32_t> &spot_prices);
