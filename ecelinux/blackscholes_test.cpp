#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cmath>

#include "blackscholes.hpp"
#include <hls_stream.h>

// Helper: float → bits
static inline bit32_t float_to_bits(float x) {
    union { float f; uint32_t u; } u;
    u.f = x;
    return (bit32_t)u.u;
}

// Helper: bits → float
static inline float bits_to_float(bit32_t w) {
    union { float f; uint32_t u; } u;
    u.u = (uint32_t)w;
    return u.f;
}

int main() {

    // --------------------------------------------------------------
    // Open the input test file
    // --------------------------------------------------------------
    std::ifstream infile("data/bs_5.dat");
    // std::ofstream outfile("result/out_bs.dat");

    if (!infile.is_open()) {
        std::cerr << "ERROR: Could not open data/testing_set.dat\n";
        return 1;
    }

    hls::stream<bit32_t> in_stream;
    hls::stream<bit32_t> out_stream;

    const int NMAX = 10000;

    float S_list[NMAX];
    float Call_expected[NMAX];
    float Put_expected[NMAX];

    int N = 0;

    // --------------------------------------------------------------
    // Read input file into arrays
    // --------------------------------------------------------------
    std::string line;
    while (std::getline(infile, line)) {
        if (line.size() == 0) continue;

        float S, cexp, pexp;
        char comma1, comma2;

        std::stringstream ss(line);
        ss >> S >> comma1 >> cexp >> comma2 >> pexp;

        S_list[N] = S;
        Call_expected[N] = cexp;
        Put_expected[N] = pexp;

        N++;
        if (N >= NMAX) break;
    }

    infile.close();
    std::cout << "Loaded " << N << " Black–Scholes test vectors.\n";

    int errors = 0;

    // Process all test spot prices
    for (int i = 0; i < N; i++) {
        float S = S_list[i];

        // Send spot price into DUT
        in_stream.write(float_to_bits(S));
        bs_dut(in_stream, out_stream);

        // Get output
        float call_hw = bits_to_float(out_stream.read());
        float put_hw  = bits_to_float(out_stream.read());

        float call_exp = Call_expected[i];
        float put_exp  = Put_expected[i];

        // Compute errors
        float call_err = std::fabs(call_hw - call_exp);
        float put_err  = std::fabs(put_hw  - put_exp);

        // Failing threshold
        const float EPS = 0.01f;

        bool pass = (call_err < EPS && put_err < EPS);
        if (!pass) errors++;

        // ---- PRINT EACH RESULT TO STDOUT ----
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "S=" << S
                  << " | Call_HW=" << call_hw << "  Exp=" << call_exp
                  << " | Put_HW="  << put_hw  << "  Exp=" << put_exp
                  << " | Status="  << (pass ? "PASS" : "FAIL")
                  << "\n";

        // // Log result
        // outfile << "S=" << S
        //         << "  HW(Call)=" << call_hw << "  Exp(Call)=" << call_exp
        //         << "  HW(Put)="  << put_hw  << "  Exp(Put)="  << put_exp
        //         << "  Status=" << (pass ? "PASS" : "FAIL") << "\n";
    }

    // Report overall error out of all testing instances
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << " Black–Scholes FPGA Testbench Summary\n";
    std::cout << "============================================\n";
    std::cout << "Total test instances: " << N << "\n";
    std::cout << "Total mismatches:     " << errors << "\n";
    std::cout << "Error rate:           "
              << std::setprecision(4)
              << (100.0 * errors / N) << "%\n";
    std::cout << "============================================\n";

    // outfile.close();

    return 0;
}
