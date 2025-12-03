//===========================================================================
// hft.h
//===========================================================================
// @brief: This header file defines the interface for the core functions.
#ifndef HFT_HPP
#define HFT_HPP

#include <hls_stream.h>
#include <ap_int.h>
#include <iomanip>
// #include <cstdint>

#include "itch.hpp"
#include "orderbook.hpp"
#include "blackscholes.hpp"
#include "typedefs.h"


void dut(
    hls::stream<bit32_t> &strm_in,   
    hls::stream<bit32_t> &strm_out  
);

#endif // HFT_HPP