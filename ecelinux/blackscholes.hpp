//===========================================================================
// blackscholes.hpp
//===========================================================================
// @brief: This header file defines the interface for the blackscholes module.

#ifndef BLACKSCHOLES_HPP
#define BLACKSCHOLES_HPP

#include "typedefs.h"

#include <hls_stream.h>

#include <cmath>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

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

// Helpers
bit32_t float_to_bits(float x);
float bits_to_float(bit32_t w);

// Closed-form Black–Scholes pricing for a given spot price S_in
void black_scholes_price(theta_type S_in, result_type &result);

// Top function
result_type bs(bit32_t spot_price);

// Black-Scholes HLS DUT:
//   - strm_in:  1 x 32-bit word containing float-encoded spot price S
//   - strm_out: 2 x 32-bit words containing float-encoded call, then put
void bs_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

#endif // BLACKSCHOLES_HPP
