#include <iostream>
#include "hls_stream.h"
#include "orderbook.hpp"     // MUST include new HLS header
using namespace std;

// ------------------------------
// Helper to create messages
// ------------------------------
OBInput make_add(char side, order_ref_t ref, shares_t shares, price_t price) {
    OBInput m;
    m.type = MSG_ADD;
    m.add.orderReferenceNumber = ref;
    m.add.stockLocate = 0;
    m.add.timestamp   = 0;
    m.add.buySellIndicator = (side == 'b' ? 'B' : 'S');
    m.add.shares = shares;
    m.add.price  = price;
    return m;
}

OBInput make_delete(order_ref_t ref) {
    OBInput m;
    m.type = MSG_DELETE;
    m.del.orderReferenceNumber = ref;
    return m;
}

// ------------------------------
// Main testbench
// ------------------------------
int main() {
    cout << "Starting HLS OrderBook tests" << endl;

    hls::stream<OBInput> in_stream;
    hls::stream<OBOutput> out_stream;

    // Test sequence (same as your old one)
    in_stream.write(make_add('b', 0, 1, 10));
    in_stream.write(make_add('b', 1, 2,  9));
    in_stream.write(make_add('b', 2, 3, 11));
    in_stream.write(make_delete(2));
    in_stream.write(make_delete(0));

    int expected[] = {10, 10, 11, 10, 9};
    int expected_size = sizeof(expected) / sizeof(int);

    // run simulation
    while (!in_stream.empty()) {
        orderbook_dut(in_stream, out_stream);
    }

    // validate output
    bool good = true;
    int i = 0;

    while (!out_stream.empty() && i < expected_size) {
        OBOutput o = out_stream.read();
        int actual = o.bestBid;

        if (actual != expected[i]) {
            cout << "Mismatch at " << i << ": expected "
                 << expected[i] << " got " << actual << endl;
            good = false;
        }
        i++;
    }

    cout << (good ? "PASS" : "FAIL") << endl;
    return 0;
}
