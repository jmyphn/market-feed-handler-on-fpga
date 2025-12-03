#include "blackscholes.hpp"

static const char* INPUT_ITCH_FILE = "data/bs_15.dat";

int main() {

    // --------------------------------------------------------------
    // Open the input test file
    // --------------------------------------------------------------
    std::ifstream infile(INPUT_ITCH_FILE);

    if (!infile.is_open()) {
        std::cerr << "ERROR: Could not open " << INPUT_ITCH_FILE << "\n";
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
    std::cout << "Loaded " << N << " Black–Scholes test vectors.\n\n";

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

        std::cout << std::fixed << std::setprecision(2);
        std::cout 
            << "S=" << std::left << std::setw(6) << S
            << " | Call_HW=" << std::left << std::setw(7) << call_hw
            << " Exp=" << std::left << std::setw(7) << call_exp
            << " | Put_HW=" << std::left << std::setw(7) << put_hw
            << " Exp=" << std::left << std::setw(7) << put_exp
            << " | Status=" << (pass ? "PASS" : "FAIL")
            << "\n";
    }

    // Summary
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << " Black–Scholes FPGA Testbench Summary\n";
    std::cout << "============================================\n";
    std::cout << "Input file            : " << INPUT_ITCH_FILE << "\n";
    std::cout << "Total test instances  : " << N << "\n\n";

    std::cout << "Error rate            : " << std::setprecision(4)
            << (100.0 * errors / N) << "%\n";
    std::cout << "============================================\n";

    return 0;
}
