#include <iostream>
#include <iomanip>
#include "itch.hpp"
#include "itch_reader.hpp"
#include "itch_common.hpp"
#include "blackscholes.hpp"
#include "orderbook.hpp"

#include <hls_stream.h>

typedef ap_uint<64> bs_out_t;

// Updated DUT signature
void dut(
    hls::stream<bit32_t> &strm_in,
    hls::stream<bs_out_t> &strm_out
);

static const char* INPUT_ITCH_FILE = "./data/12302019/filtered_10_per_type";

static inline bit32_t pack_header(uint16_t len) {
    bit32_t x = 0;
    x(15,0) = len;
    return x;
}

int main() {
    try {
        ITCH::Reader reader(INPUT_ITCH_FILE, 16384);

        hls::stream<bit32_t> strm_in;
        hls::stream<bs_out_t> strm_out;

        const char* msg = nullptr;
        uint64_t total = 0;

        while ((msg = reader.nextMessage())) {
            total++;

            uint16_t net_len = *(uint16_t*)msg;
            uint16_t len = be16toh(net_len);
            const unsigned char* payload = (unsigned char*)(msg + 2);

            // Send header
            strm_in.write(pack_header(len));

            // Pack ITCH message 32 bits per cycle
            for (int i = 0; i < len; i += 4) {
                bit32_t w = 0;
                w(31,24) = payload[i];
                w(23,16) = (i+1 < len ? payload[i+1] : 0);
                w(15,8)  = (i+2 < len ? payload[i+2] : 0);
                w(7,0)   = (i+3 < len ? payload[i+3] : 0);
                strm_in.write(w);
            }

            // Run full HFT + Black-Scholes pipeline
            dut(strm_in, strm_out);

            // If we got a BS result this cycle, print it
            if (!strm_out.empty()) {
                bs_out_t packed = strm_out.read();

                bit32_t call_bits = packed.range(31, 0);
                bit32_t put_bits  = packed.range(63, 32);

                union { float f; uint32_t u; } C, P;
                C.u = (uint32_t)call_bits;
                P.u = (uint32_t)put_bits;

                std::cout << std::fixed << std::setprecision(6)
                          << "Call=" << std::setw(9) << C.f
                          << "  Put=" << std::setw(9) << P.f << "\n";
            }
        }

        std::cout << "\n";
        std::cout << "============================================\n";
        std::cout << " Interconnect FPGA Testbench Summary\n";
        std::cout << "============================================\n";
        std::cout << "Parsed file                 : " << INPUT_ITCH_FILE << "\n";
        std::cout << "Total messages              : " << total << "\n";
        std::cout << "============================================\n";

    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
