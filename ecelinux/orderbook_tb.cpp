#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "orderbook.hpp"

typedef ap_uint<32> bit32_t;

static const char* INPUT_ORDERBOOK_FILE = "data/orderbook_10.dat";

// Convert uint32 -> float spot price
static inline float ticks_to_float(bit32_t x) {
    return (float)x.to_uint() / 10000.0f;
}

int main() {
    std::ifstream infile(INPUT_ORDERBOOK_FILE);

    if (!infile.is_open()) {
        std::cerr << "ERROR: could not open " << INPUT_ORDERBOOK_FILE << "\n";
        return 1;
    }

    hls::stream<bit32_t> in_stream;
    hls::stream<bit32_t> out_stream;

    const int NMAX = 20000;
    bit32_t msgs[NMAX][7];

    int N = 0;
    std::string line;

    // Read each line of the input file
    while (std::getline(infile, line)) {
        if (line.size() == 0) continue;

        std::stringstream ss(line);

        std::string w0, w1, w2, w3, w4, w5, w6;
        char comma;

        ss >> w0 >> comma
           >> w1 >> comma
           >> w2 >> comma
           >> w3 >> comma
           >> w4 >> comma
           >> w5 >> comma
           >> w6;

        msgs[N][0] = (bit32_t)std::stoul(w0, nullptr, 16);
        msgs[N][1] = (bit32_t)std::stoul(w1, nullptr, 16);
        msgs[N][2] = (bit32_t)std::stoul(w2, nullptr, 16);
        msgs[N][3] = (bit32_t)std::stoul(w3, nullptr, 16);
        msgs[N][4] = (bit32_t)std::stoul(w4, nullptr, 16);
        msgs[N][5] = (bit32_t)std::stoul(w5, nullptr, 16);
        msgs[N][6] = (bit32_t)std::stoul(w6, nullptr, 16);

        N++;
        if (N >= NMAX) break;
    }

    infile.close();
    std::cout << "Loaded " << N << " orderbook messages.\n\n";

    // Process all messages
    for (int i = 0; i < N; i++) {
        // Write message to stream
        for (int w = 0; w < 7; w++) in_stream.write(msgs[i][w]);

        // Run DUT
        orderbook_dut(in_stream, out_stream);

        // Output spot price
        bit32_t spot_bits = out_stream.read();
        float spot = ticks_to_float(spot_bits);

        std::cout << std::fixed << std::setprecision(4);
        std::cout
            << "Msg " << std::setw(5) << i
            << " | SpotPrice=" << std::setw(10) << spot
            << "\n";
    }

    std::cout << "\n============================================\n";
    std::cout << " OrderBook FPGA Testbench Summary\n";
    std::cout << "============================================\n";
    std::cout << "Input file            : " << INPUT_ORDERBOOK_FILE << "\n";
    std::cout << "Total messages tested : " << N << "\n";
    std::cout << "============================================\n\n";

    return 0;
}
