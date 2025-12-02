#include <iostream>
#include "hls_stream.h"
#include "orderbook.hpp"

using namespace std;

// Helper constructors
OBInput make_add(char side, order_ref_t ref, shares_t shares, price_t price) {
    OBInput m;
    m.type = MSG_ADD;  // numeric 0..4 fits in ap_uint<3>
    m.add.orderReferenceNumber = ref;
    m.add.buySellIndicator = (side == 'b' || side == 'B') ? 'B' : 'S';
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

int main() {

    hls::stream<OBInput>  in_stream;
    hls::stream<OBOutput> out_stream;

    int N = 0;

    // Sequence:
    // add(b, id0, 1, 10)
    // add(b, id1, 2, 9)
    // add(b, id2, 3, 11)
    // delete(id2)
    // delete(id0)
    in_stream.write(make_add('b', 0, 1, 10));
    in_stream.write(make_add('b', 1, 2,  9));
    in_stream.write(make_add('b', 2, 3, 11));
    in_stream.write(make_delete(2));
    in_stream.write(make_delete(0));

    int expected[] = {10, 10, 11, 10, 9};
    const int expected_size = sizeof(expected) / sizeof(int);

    // Drive DUT until inputs consumed
    while (!in_stream.empty()) {
        N++;
        orderbook_dut(in_stream, out_stream);
    }

    bool good = true;
    int i = 0;
    int errors = 0;

    while (!out_stream.empty() && i < expected_size) {
        OBOutput o = out_stream.read();
        int actual = (int)o.bestBid;

        if (actual != expected[i]) {
            cout << "Mismatch at #" << i
                 << ": expected " << expected[i]
                 << ", got " << actual << endl;
            good = false;
            errors++;
        }
        i++;
    }

    if (i < expected_size) {
        cout << "Missing outputs (only " << i
             << " of " << expected_size << ")" << endl;
        good = false;
    }
    while (!out_stream.empty()) {
        OBOutput o = out_stream.read();
        cout << "Extra output: bestBid=" << (int)o.bestBid << endl;
        good = false;
    }

    cout << "\n";
    cout << "============================================\n";
    cout << " Orderbook FPGA Testbench Summary\n";
    cout << "============================================\n";
    cout << "Total test instances: " << N << "\n";
    cout << "Total mismatches:     " << errors << "\n";
    cout << "Error rate:           "
              << std::setprecision(4)
              << (100.0 * errors / N) << "%\n";
    cout << "============================================\n";
    return good ? 0 : 1;
}
