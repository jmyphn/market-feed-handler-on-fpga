//===========================================================================
// itch.h
//===========================================================================
// @brief: This header file defines the interface for the core functions.

#ifndef ITCH_H
#define ITCH_H
// #include "itch_reader.hpp"
#include "itch_common.hpp"
#include "typedefs.h"
#include <hls_stream.h>
#include <cstdint>

void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out);

#endif
