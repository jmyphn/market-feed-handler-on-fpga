#include <iostream>
#include "orderbook.hpp"
#include "typedefs.h"

using namespace std;

int main() {
  cout << "Starting tests" << endl;
  // TODO

  // Inputs
  hls::stream<ParsedMessage> test_stream;
  ParsedMessage test_inputs[] = {
    ParsedMessage{'A', 'b', 0, 0, 1, 10},
    ParsedMessage{'A', 'b', 0, 0, 2, 9},
    ParsedMessage{'A', 'b', 0, 0, 3, 11},
    // TODO: changed to AddOrderMessageType
  };
  for (int i = 0; i < sizeof(test_inputs) / sizeof(ParsedMessage); ++i) {
    test_stream.write(test_inputs[i]);
  }

  // Outputs
  hls::stream<bit32_t> spot_price_stream;
  bit32_t expected[] = { 10, 10, 11 };



  while (!test_stream.empty()) {
    orderbook(test_stream, spot_price_stream);
  }
  bool good = true;
  int expected_size = sizeof(expected) / sizeof(bit32_t);
  int i = 0;
  for ( ; !spot_price_stream.empty() && i < expected_size; ++i) {
    bit32_t actual = spot_price_stream.read();
    if (actual != expected[i]) {
      good = false;
      cout << "Mismatch, expected " << expected[i] << ", but got " << actual << endl;
    }
  }
  if (i < expected_size) {
    good = false;
    cout << "Less output messages than expected" << endl;
  } else while (!spot_price_stream.empty()) {
    good = false;
    cout << "Also got: " << spot_price_stream.read() << "\n";
  }
  cout << "Finished tests,";
  if (good) cout << " tests all pass" << endl;
  else cout << " tests dont pass" << endl;


}
