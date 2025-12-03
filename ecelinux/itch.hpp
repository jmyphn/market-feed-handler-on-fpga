//===========================================================================
// itch.hpp
//===========================================================================
// @brief: This header file defines the interface for the itch module.

#ifndef ITCH_HPP
#define ITCH_HPP

#include "itch_reader.hpp"
#include "itch_common.hpp"
#include "typedefs.h"

#include <hls_stream.h>
#include <ap_int.h>

#include <cstdint>
#include <cassert>

#include <iostream>
#include <fstream>
#include <unordered_map>

#include <endian.h>

// Top function
ParsedMessage parser(char* buffer);

// ITCH Parser HLS DUT:
//   - strm_in:  5 to 9 x 32-bit words containing ITCH messages
//   - strm_out: 7 x 32-bit words containing extracted info
void itch_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

#endif // ITCH_HPP
