// #include <iostream>
// #include "hls_stream.h"
// #include "orderbook.hpp"

// using namespace std;

// // Helper constructors
// OBInput make_add(char side, order_ref_t ref, shares_t shares, price_t price) {
//     OBInput m;
//     m.type = MSG_ADD;  // numeric 0..4 fits in ap_uint<3>
//     m.add.orderReferenceNumber = ref;
//     m.add.buySellIndicator = (side == 'b' || side == 'B') ? 'B' : 'S';
//     m.add.shares = shares;
//     m.add.price  = price;
//     return m;
// }

// OBInput make_delete(order_ref_t ref) {
//     OBInput m;
//     m.type = MSG_DELETE;
//     m.del.orderReferenceNumber = ref;
//     return m;
// }

// int main() {
//     cout << "Starting HLS OrderBook tests" << endl;

//     hls::stream<OBInput>  in_stream;
//     hls::stream<OBOutput> out_stream;

//     in_stream.write(make_add('b', 0, 1, 10));   // add(b, id0, 1, 10)
//     in_stream.write(make_add('b', 1, 2,  9));   // add(b, id1, 2, 9)
//     in_stream.write(make_add('b', 2, 3, 11));   // add(b, id2, 3, 11)
//     in_stream.write(make_delete(2));            // delete(id2)
//     in_stream.write(make_delete(0));            // delete(id0)

//     int expected[] = {10, 10, 11, 10, 9};
//     const int expected_size = sizeof(expected) / sizeof(int);

//     // Drive DUT until inputs consumed
//     while (!in_stream.empty()) {
//         orderbook_dut(in_stream, out_stream);
//     }

//     bool good = true;
//     int i = 0;

//     while (!out_stream.empty() && i < expected_size) {
//         OBOutput o = out_stream.read();
//         int actual = (int)o.bestBid;

//         if (actual != expected[i]) {
//             cout << "Mismatch at #" << i
//                  << ": expected " << expected[i]
//                  << ", got " << actual << endl;
//             good = false;
//         }
//         i++;
//     }

//     if (i < expected_size) {
//         cout << "Missing outputs (only " << i
//              << " of " << expected_size << ")" << endl;
//         good = false;
//     }
//     while (!out_stream.empty()) {
//         OBOutput o = out_stream.read();
//         cout << "Extra output: bestBid=" << (int)o.bestBid << endl;
//         good = false;
//     }

//     cout << "Finished tests: " << (good ? "PASS" : "FAIL") << endl;
//     return good ? 0 : 1;
// }
#include <hls_stream.h>
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
