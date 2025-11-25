/**
 * Orderbook testing.
 *
 * Testbench assumes every input as a corresponding output.
 *
 * Write tests by adding values to test_inputs and expected.
 */
#include "itch_common.hpp"
#include "orderbook.hpp"
#include "typedefs.h"
#include <iostream>

#define X 0 // for marking placeholders

using namespace std;

int main() {
  cout << "Starting tests" << endl;

  // Inputs
  hls::stream<ParsedMessage> test_stream;
  ParsedMessage test_inputs[] = {
      ParsedMessage{ITCH::AddOrderMessageType, 'b', 0, X, 1, 10},
      ParsedMessage{ITCH::AddOrderMessageType, 'b', 1, X, 2, 9},
      ParsedMessage{ITCH::AddOrderMessageType, 'b', 2, X, 3, 11},
      ParsedMessage{ITCH::OrderDeleteMessageType, 'b', 2, X, X, X},
  };
  for (int i = 0; i < sizeof(test_inputs) / sizeof(ParsedMessage); ++i) {
    test_stream.write(test_inputs[i]);
  }

  // Outputs
  hls::stream<bit32_t> spot_price_stream;
  bit32_t expected[] = {10, 10, 11, 10};

  // Simulate the tests
  while (!test_stream.empty()) {
    orderbook(test_stream, spot_price_stream);
  }

  // Check test outputs
  bool good = true;
  int expected_size = sizeof(expected) / sizeof(bit32_t);
  int i = 0;
  for (; !spot_price_stream.empty() && i < expected_size; ++i) {
    bit32_t actual = spot_price_stream.read();
    if (actual != expected[i]) {
      good = false;
      cout << "Mismatch, expected " << expected[i] << ", but got " << actual
           << endl;
    }
  }
  if (i < expected_size) {
    good = false;
    cout << "Less output messages than expected" << endl;
  } else
    while (!spot_price_stream.empty()) {
      good = false;
      cout << "Also got: " << spot_price_stream.read() << "\n";
    }
  cout << "Finished tests,";
  if (good)
    cout << " tests all pass" << endl;
  else
    cout << " tests dont pass" << endl;
}
