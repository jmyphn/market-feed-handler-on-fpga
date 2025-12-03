#include "lft.hpp"

#include <iostream>
#include <iomanip>
#include <hls_stream.h>
#include <fstream>
#include <unordered_map>
#include <cstdint>


static const char* INPUT_ITCH_FILE = "./data/12302019/filtered_A";

int main() {
    try {
        ITCH::Reader reader(INPUT_ITCH_FILE, 16384);
        std::ofstream outfile("result/itch_csim.txt");

        hls::stream<bit32_t> in_stream;
        hls::stream<bit32_t> out_stream;

        std::unordered_map<ITCH::MessageType_t, uint64_t> counts;
        uint64_t total = 0;

        const char* msg = nullptr;
        while ((msg = reader.nextMessage())) {
            auto t = ITCH::Parser::getDataMessageType(msg);
            counts[t]++; total++;

            uint16_t net_len = *(const uint16_t*)(msg);
            uint16_t msg_len = be16toh(net_len);
            const unsigned char* payload = reinterpret_cast<const unsigned char*>(msg + 2);

            bit32_t hdr = 0; hdr(15, 0) = msg_len;
            in_stream.write(hdr);

            // Pack 4 bytes per stream word
            for (int i = 0; i < msg_len; i+=4) {
                bit32_t w = 0;
                w(31,24) = payload[i];             // MSB
                w(23,16) = (i+1 < msg_len) ? payload[i+1] : 0;
                w(15,8)  = (i+2 < msg_len) ? payload[i+2] : 0;
                w(7,0)   = (i+3 < msg_len) ? payload[i+3] : 0;
                in_stream.write(w);
            }

            dut(in_stream, out_stream);

            // Get output
            float call_hw = bits_to_float(out_stream.read());
            float put_hw  = bits_to_float(out_stream.read());

            std::cout << std::fixed << std::setprecision(6);
            std::cout << "Call_HW=" << call_hw << " | Put_HW="  << put_hw << "\n";
        }

        // Summary only
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << " LFT FPGA Testbench Summary\n";
    std::cout << "============================================\n";
    std::cout << "Parsed file                 : " << INPUT_ITCH_FILE << "\n";
    std::cout << "Total messages              : " << total << "\n";
    std::cout << "Total bytes read            : " << reader.getTotalBytesRead() << "\n\n";

    std::cout << "AddOrder (A)                : " << counts['A'] << "\n";
    std::cout << "OrderExecuted (E)           : " << counts['E'] << "\n";
    std::cout << "OrderExecutedWithPrice (C)  : " << counts['C'] << "\n";
    std::cout << "OrderCancel (X)             : " << counts['X'] << "\n";
    std::cout << "OrderDelete (D)             : " << counts['D'] << "\n";
    std::cout << "OrderReplace (U)            : " << counts['U'] << "\n";
    std::cout << "============================================\n";

    outfile.close();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
