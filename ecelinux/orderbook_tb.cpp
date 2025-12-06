#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "orderbook.hpp"

typedef ap_uint<32> bit32_t;

static const char* INPUT_ORDERBOOK_FILE = "data/ob_20.dat";

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
    float Spot_expected[NMAX];

    int N = 0;

    // Read each line of the input file
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);

        std::string w0, w1, w2, w3, w4, w5, w6;
        float sexp;
        char comma;

        std::getline(ss, w0, ',');
        std::getline(ss, w1, ',');
        std::getline(ss, w2, ',');
        std::getline(ss, w3, ',');
        std::getline(ss, w4, ',');
        std::getline(ss, w5, ',');
        std::getline(ss, w6, ',');
        
        // trim whitespace
        auto trim = [](std::string &s) {
            s.erase(0, s.find_first_not_of(" \t"));
            s.erase(s.find_last_not_of(" \t") + 1);
        };

        trim(w0); trim(w1); trim(w2); trim(w3); trim(w4); trim(w5); trim(w6);

        // Read final float field
        std::string sexp_str;
        std::getline(ss, sexp_str);
        trim(sexp_str);
        sexp = std::stof(sexp_str);

        msgs[N][0] = (bit32_t)std::stoul(w0, nullptr, 16);
        msgs[N][1] = (bit32_t)std::stoul(w1, nullptr, 16);
        msgs[N][2] = (bit32_t)std::stoul(w2, nullptr, 16);
        msgs[N][3] = (bit32_t)std::stoul(w3, nullptr, 16);
        msgs[N][4] = (bit32_t)std::stoul(w4, nullptr, 16);
        msgs[N][5] = (bit32_t)std::stoul(w5, nullptr, 16);
        msgs[N][6] = (bit32_t)std::stoul(w6, nullptr, 16);
        Spot_expected[N] = sexp;

        N++;
        if (N >= NMAX) break;
    }

    infile.close();
    std::cout << "Loaded " << N << " orderbook messages.\n\n";

    int errors = 0;

    // Process all messages
    for (int i = 0; i < N; i++) {
        // Write message to stream
        for (int w = 0; w < 7; w++) in_stream.write(msgs[i][w]);

        // Run DUT
        orderbook_dut(in_stream, out_stream);

        // Output spot price
        bit32_t spot_bits = out_stream.read();
        float spot = ticks_to_float(spot_bits);

        float spot_exp = Spot_expected[i];
        bool pass = (spot == spot_exp);
        if (!pass) errors++;

        std::cout << std::fixed << std::setprecision(3)
                  << "Msg " << std::left << std::setw(2) << i
                  << " | SpotPrice=" << std::setw(7) << spot
                  << "  Exp=" << std::left << std::setw(7) << spot_exp
                  << " | Status=" << (pass ? "PASS" : "FAIL")
                  << "\n";
    }

    std::cout << "\n============================================\n";
    std::cout << " OrderBook FPGA Testbench Summary\n";
    std::cout << "============================================\n";
    std::cout << "Input file            : " << INPUT_ORDERBOOK_FILE << "\n";
    std::cout << "Total messages        : " << N << "\n\n";

    std::cout << "Error rate            : " << std::setprecision(4)
              << (100.0 * errors / N) << "%\n";
    std::cout << "============================================\n\n";

    return 0;
}
