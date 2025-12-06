#include "blackscholes.hpp"

theta_type K = 200.0f; // Strike price
theta_type r = 0.05f;  // Risk-free rate 
theta_type v = 0.2f;   // Volatility of the underlying 
theta_type T = 1.0f;   // One year until expiry

// Helper: float → bits
bit32_t float_to_bits(float x) {
    union { float f; uint32_t u; } u;
    u.f = x;
    return (bit32_t)u.u;
}

// Helper: bits → float
float bits_to_float(bit32_t w) {
    union { float f; uint32_t u; } u;
    u.u = (uint32_t)w;
    return u.f;
}

static theta_type normal_cdf(theta_type x) {
#pragma HLS INLINE
    const theta_type a1 = 0.31938153f;
    const theta_type a2 = -0.356563782f;
    const theta_type a3 = 1.781477937f;
    const theta_type a4 = -1.821255978f;
    const theta_type a5 = 1.330274429f;

    theta_type L = (x >= 0.0f) ? x : -x;
    theta_type k = 1.0f / (1.0f + 0.2316419f * L);

    theta_type w = ((((a5 * k + a4) * k + a3) * k + a2) * k + a1) * k;

    theta_type exponent = -0.5f * L * L;

    theta_type pdf = theta_type(0.3989422804014327f) * std::exp(exponent);

    w = w * pdf;

    return (x >= 0.0f) ? (1.0f - w) : w;
}

// ---------------------------------------------------------------------
// Black–Scholes pricing 
// ---------------------------------------------------------------------
static const theta_type invK     = 1.0f / K;
static const theta_type sqrtT    = std::sqrt(T);
static const theta_type inv_sqrtT = 1.0f / sqrtT;

static const theta_type sigma      = v;
static const theta_type sigma_sq   = v * v;
static const theta_type denom      = sigma * sqrtT;
static const theta_type inv_denom  = 1.0f / denom;

void black_scholes_price(theta_type S_in, result_type &result) {
  #pragma HLS INLINE
  if (S_in <= 0 || K <= 0 || v <= 0 || T <= 0) {
    result.call = 0.0f;
    result.put  = 0.0f;
    return;
  }mak

  theta_type S_over_K = S_in * invK;
  theta_type log_S_over_K = std::log(S_over_K);
  theta_type num_tmp = (r + 0.5f * sigma_sq);
  theta_type numerator   = log_S_over_K + num_tmp * T;

  theta_type d1 = numerator * inv_denom;
  theta_type d2_tmp = sigma * sqrtT;
  theta_type d2 = d1 - d2_tmp;

  theta_type Nd1       = normal_cdf(d1);
  theta_type Nd2       = normal_cdf(d2);
  theta_type Nminus_d1 = normal_cdf(-d1);
  theta_type Nminus_d2 = normal_cdf(-d2);

  theta_type discount = std::exp(-r * T);

  theta_type call_tmp1 = S_in * Nd1;
  theta_type call_tmp2 = K * discount;
  theta_type call_tmp3 = call_tmp2 * Nd2;
  result.call = call_tmp1 - call_tmp3;

  theta_type put_tmp1 = K * discount;
  theta_type put_tmp2 = put_tmp1 * Nminus_d2;
  theta_type put_tmp3 = S_in * Nminus_d1;
  result.put  = put_tmp2 - put_tmp3;
}

void bs_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out){
  #pragma HLS INLINE off
  #pragma HLS PIPELINE II=1
  
  // ------------------------------------------------------
  // Input processing
  // ------------------------------------------------------
  // Read spot price from input stream
  bit32_t in_bits = strm_in.read();

  union {
    float fval;
    int   ival;
  } u_in;

  u_in.ival = static_cast<int>(in_bits);
  theta_type S_in = u_in.fval;

  // ------------------------------------------------------
  // Call Black–Scholes price
  // ------------------------------------------------------
  result_type result;
  black_scholes_price(S_in, result);

  // ------------------------------------------------------
  // Output processing
  // ------------------------------------------------------
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

result_type bs(bit32_t spot_price) {
    union {
      float fval;
      int   ival;
    } u_in;

    u_in.ival = static_cast<int>(spot_price);
    theta_type S_in = u_in.fval;

    // Compute Black–Scholes price
    result_type result;
    black_scholes_price(S_in, result);

    return result;
}