#pragma once

#ifndef BLACKSCHOLES_H
#define BLACKSCHOLES_H

#include <cmath>
#include <iostream>
#include <cmath>

#include "typedefs.h"
#include <hls_stream.h>

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

// Top-level HLS DUT:
//   - strm_in:  1 x 32-bit word containing float-encoded spot price S
//   - strm_out: 2 x 32-bit words containing float-encoded call, then put
void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

#endif // BLACKSCHOLES_H
