//=========================================================================
// blackscholes_test.cpp  (Black–Scholes testbench)
//=========================================================================

#include <iostream>
#include <cstdlib>
#include <cmath>

#include "blackscholes.hpp"

int main(int argc, char **argv)
{
  // Default spot price
  theta_type S = 100.0f;

  if (argc > 1) {
    S = static_cast<theta_type>(std::atof(argv[1]));
  }

  // Prepare input stream
  hls::stream<bit32_t> in_stream;
  hls::stream<bit32_t> out_stream;

  // Pack S into 32-bit word
  union {
    float fval;
    int   ival;
  } u_in;

  u_in.fval = S;


  in_stream.write((bit32_t)u_in.ival);
  dut(in_stream, out_stream);
  // Read outputs (call then put)
  union { float fval; int ival; } u_call, u_put;

  u_call.ival = out_stream.read();
  u_put.ival  = out_stream.read();

  float call_price = u_call.fval;
  float put_price  = u_put.fval;

  std::cout << "=========================================\n";
  std::cout << "Spot Price (S):    " << S           << "\n";
  std::cout << "Strike Price (K):  " << K           << "\n";
  std::cout << "Risk-Free Rate r:  " << r           << "\n";
  std::cout << "Volatility v:      " << v           << "\n";
  std::cout << "Maturity T:        " << T           << "\n";
  std::cout << "-----------------------------------------\n";
  std::cout << "Call Price:        " << call_price  << "\n";
  std::cout << "Put Price:         " << put_price   << "\n";
  std::cout << "=========================================\n";

  return 0;
}
