#ifndef HFT_HPP
#define HFT_HPP

#include <hls_stream.h>
#include <ap_int.h>
#include <iomanip>

#include "itch.hpp"
#include "itch_common.hpp"
#include "orderbook.hpp"
#include "blackscholes.hpp"


void dut(
    hls::stream<bit32_t> &strm_in,   
    hls::stream<bs_out_t> &strm_out  
);

#endif // HFT_HPP