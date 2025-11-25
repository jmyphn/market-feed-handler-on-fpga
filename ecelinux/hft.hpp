//===========================================================================
// hft.hpp
//===========================================================================
// @brief: This header file defines the interface for the core functions.

#ifndef HFT_H
#define HFT_H
#include "itch_common.hpp"
#include "itch_defs.h"
#include <hls_stream.h>
#include <cstdint>

#include <cfloat>
#include <ap_int.h>
#include "typedefs.h"

#include <cmath>
#include <iostream>

#define MAX_MESSAGE_SIZE 36

// Top function for parser
ParsedMessage parser(char* buffer);

// Top function for orderbook
bit32_t orderbook(ParsedMessage &orders);

////////////////////////////////////////////////

// Parameter type for Black–Scholes
typedef float theta_type;

// Global Black–Scholes parameters (strike, rate, volatility, maturity)
extern theta_type K;
extern theta_type r;
extern theta_type v;
extern theta_type T;

// Result struct (call + put)
struct result_type {
  theta_type call;
  theta_type put;
};

// Closed-form Black–Scholes pricing for a given spot price S_in
void black_scholes_price(theta_type S_in, result_type &result);

// Top function for synthesis
void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

#endif
