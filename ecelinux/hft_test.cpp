#include <iostream>
#include <iomanip>
#include "itch.hpp"
#include "itch_reader.hpp"
#include "itch_common.hpp"
#include "blackscholes.hpp"
#include "orderbook.hpp"

#include <hls_stream.h>

void hft(
    hls::stream<bit32_t> &itch_in,
    hls::stream<bit32_t> &call_out,
    hls::stream<bit32_t> &put_out
);

static const char* INPUT_ITCH_FILE = "./data/filtered_data_500";

static inline bit32_t pack_header(uint16_t len) {
    bit32_t x = 0;
    x(15,0) = len;
    return x;
}

int main() {
    try {
        ITCH::Reader reader(INPUT_ITCH_FILE, 16384);

        hls::stream<bit32_t> itch_in;
        hls::stream<bit32_t> call_out;
        hls::stream<bit32_t> put_out;

        const char* msg = nullptr;
        while ((msg = reader.nextMessage())) {

            uint16_t net_len = *(uint16_t*)msg;
            uint16_t len = be16toh(net_len);
            const unsigned char* payload = (unsigned char*)(msg + 2);

            // Send header
            itch_in.write(pack_header(len));

            // Pack message
            for (int i = 0; i < len; i += 4) {
                bit32_t w = 0;
                w(31,24) = payload[i];
                w(23,16) = (i+1 < len ? payload[i+1] : 0);
                w(15,8)  = (i+2 < len ? payload[i+2] : 0);
                w(7,0)   = (i+3 < len ? payload[i+3] : 0);
                itch_in.write(w);
            }

            // HFT
            hft(itch_in, call_out, put_out);

            if (!call_out.empty() && !put_out.empty()) {
                union { float f; uint32_t u; } C, P;
                C.u = (uint32_t)call_out.read();
                P.u = (uint32_t)put_out.read();

                std::cout << std::fixed << std::setprecision(6)
                          << "Call=" << C.f << "  Put=" << P.f << "\n";
            }
        }

    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
