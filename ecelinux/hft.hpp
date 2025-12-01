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
    hls::stream<bit32_t> &itch_in,
    hls::stream<bit32_t> &call_out,
    hls::stream<bit32_t> &put_out
);

#endif // HFT_HPP