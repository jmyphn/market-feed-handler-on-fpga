#include "orderbook.hpp"

typedef ap_uint<32> bit32_t;

int main() {

    hls::stream<bit32_t> in, out;

    // -------------------------------------------------------
    // Test 1: ADD order
    // message format (7 words):
    // [0]=type, [1]=orderRef, [2]=price, [3]=shares, [4]=side, [5]=unused, [6]=unused
    // -------------------------------------------------------

    bit32_t msg_add[7];
    msg_add[0] = 'A';
    msg_add[1] = 1001;   // reference
    msg_add[2] = 5000;   // price
    msg_add[3] = 10;     // shares
    msg_add[4] = 'B';    // side
    msg_add[5] = 0;
    msg_add[6] = 0;

    for (int i = 0; i < 7; i++)
        in.write(msg_add[i]);

    orderbook_dut(in, out);

    // -------------------------------------------------------
    // Read output
    // -------------------------------------------------------
    bit32_t bestBid  = out.read();
    bit32_t bestAsk  = out.read();
    bit32_t count    = out.read();

    printf("After 1 add:\n");
    printf(" bestBid  = %u\n", bestBid.to_uint());
    printf(" bestAsk  = %u\n", bestAsk.to_uint());
    printf(" count    = %u\n", count.to_uint());

    return 0;
}
