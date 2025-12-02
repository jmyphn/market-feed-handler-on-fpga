#include <iostream>
#include <cmath>
#include "blackscholes.hpp"

theta_type K = 100.0f; // Strike price
theta_type r = 0.05f;  // Risk-free rate 
theta_type v = 0.2f;   // Volatility of the underlying 
theta_type T = 1.0f;   // One year until expiry

template <typename T>
T custom_log(const T& x) {
  if (x <= 0) {
    std::cerr << "Error: Input must be greater than 0" << std::endl;
    return -1.0; 
  }

  const int logTerms = 10;

  T result = 0.0;
  T term = (x - 1) / (x + 1);
  T term_squared = term * term;
  T numerator = term;
  T denominator = 1;

  for (int i = 1; i <= logTerms; i++) {
    result += numerator / denominator;
    numerator *= term_squared;
    denominator += 2;
  }

  return 2 * result;
}

template <typename T>
T custom_exp(const T& x) {
  T result = 1.0;
  T term = 1.0;
  const int expTerms = 10;

  for (int i = 1; i <= expTerms; i++) {
    term *= x / (T)i;
    result += term;
  }

  return result;
}


static theta_type normal_cdf(theta_type x)
{
  const theta_type inv_sqrt2 = 0.7071067811865475f; // 1/sqrt(2)
  return 0.5f * (1.0f + std::erf(x * inv_sqrt2));
}

// ---------------------------------------------------------------------
// Black–Scholes pricing 
// ---------------------------------------------------------------------
void black_scholes_price(theta_type S_in, result_type &result) {
  if (S_in <= 0 || K <= 0 || v <= 0 || T <= 0) {
    result.call = 0.0f;
    result.put  = 0.0f;
    return;
  }

  theta_type sigma   = v;
  theta_type sqrtT   = std::sqrt(T); 
  theta_type S_over_K = S_in / K;

  theta_type log_S_over_K = custom_log<theta_type>(S_over_K);
  theta_type sigma_sq     = sigma * sigma;

  theta_type numerator   = log_S_over_K + (r + 0.5f * sigma_sq) * T;
  theta_type denominator = sigma * sqrtT;

  theta_type d1 = numerator / denominator;
  theta_type d2 = d1 - sigma * sqrtT;

  theta_type Nd1       = normal_cdf(d1);
  theta_type Nd2       = normal_cdf(d2);
  theta_type Nminus_d1 = normal_cdf(-d1);
  theta_type Nminus_d2 = normal_cdf(-d2);

  theta_type discount = custom_exp<theta_type>(-r * T);

  result.call = S_in * Nd1 - K * discount * Nd2;
  result.put  = K * discount * Nminus_d2 - S_in * Nminus_d1;
}

void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out){
  // Read spot price from input stream
  bit32_t in_bits = strm_in.read();

  union {
    float fval;
    int   ival;
  } u_in;

  u_in.ival = static_cast<int>(in_bits);
  theta_type S_in = u_in.fval;

  // Compute Black–Scholes price
  result_type result;
  black_scholes_price(S_in, result);

  // Convert results back to 32-bit words
  union { float fval; int ival; } ucall;
  union { float fval; int ival; } uput;

  ucall.fval = result.call;
  uput.fval  = result.put;

  bit32_t icall = static_cast<bit32_t>(ucall.ival);
  bit32_t iput  = static_cast<bit32_t>(uput.ival);

  // Write output to stream (call, put)
  strm_out.write(icall);
  strm_out.write(iput);
}