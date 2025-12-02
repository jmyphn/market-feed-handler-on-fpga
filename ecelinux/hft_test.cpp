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
    hls::stream<bs_out_t> &strm_out,
    int num_msgs
);

static const char* INPUT_ITCH_FILE = "./data/12302019/filtered_500";

static inline bit32_t pack_header(uint16_t len) {
    bit32_t x = 0;
    x(15,0) = len;
    return x;
}

int main() {
    try {
        ITCH::Reader reader(INPUT_ITCH_FILE, 16384);

        hls::stream<bit32_t> strm_in("strm_in");
        hls::stream<bs_out_t> strm_out("strm_out");

        const char* msg = nullptr;
        int messages_sent = 0;
        int valid_messages = 0;

        // =================================================================
        // 1. SEND PHASE: Send all messages to the DUT
        // =================================================================
        std::cout << "INFO: Sending messages to DUT..." << std::endl;
        while ((msg = reader.nextMessage())) {
            messages_sent++;

            uint16_t net_len = *(uint16_t*)msg;
            uint16_t len = be16toh(net_len);
            const unsigned char* payload = (unsigned char*)(msg + 2);
            char msg_type = payload[0];

            // Count valid messages to know how many results to expect
            if (msg_type == 'A' || msg_type == 'C' || msg_type == 'D' || msg_type == 'E' || msg_type == 'U' || msg_type == 'X') {
                valid_messages++;
            }

            // Send header (length)
            strm_in.write(pack_header(len));

#ifndef __SYNTHESIS__
            std::cout << "TB sending msg (" << len << " bytes):   ";
            for (int i=0; i<len; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)payload[i] << " ";
            }
            std::cout << std::dec << "\n";
#endif

            // Send message body, word by word
            for (int i = 0; i < len; i += 4) {
                bit32_t w = 0;
                w(31,24) = payload[i];
                w(23,16) = (i+1 < len ? payload[i+1] : 0);
                w(15,8)  = (i+2 < len ? payload[i+2] : 0);
                w(7,0)   = (i+3 < len ? payload[i+3] : 0);
                strm_in.write(w);
            }
        }
        std::cout << "INFO: Done sending " << messages_sent << " messages (" << valid_messages << " valid)." << std::endl;


        // =================================================================
        // 2. EXECUTE PHASE: Call the DUT once to process all messages
        // =================================================================
        std::cout << "INFO: Executing DUT..." << std::endl;
        dut(strm_in, strm_out, valid_messages);
        std::cout << "INFO: DUT execution finished." << std::endl;


        // =================================================================
        // 3. RECEIVE PHASE: Read all results from the DUT
        // =================================================================
        std::cout << "INFO: Receiving results..." << std::endl;
        int results_received = 0;
        while(!strm_out.empty()) {
            bs_out_t packed = strm_out.read();
            results_received++;

            bit32_t call_bits = packed.range(31, 0);
            bit32_t put_bits  = packed.range(63, 32);

            union { float f; uint32_t u; } C, P;
            C.u = (uint32_t)call_bits;
            P.u = (uint32_t)put_bits;

            std::cout << std::fixed << std::setprecision(6)
                      << "Result " << std::setw(3) << results_received
                      << ": Call=" << std::setw(9) << C.f
                      << "  Put=" << std::setw(9) << P.f << "\n";
        }


        std::cout << "\n";
        std::cout << "============================================\n";
        std::cout << " HFT Testbench Summary\n";
        std::cout << "============================================\n";
        std::cout << "Parsed file                 : " << INPUT_ITCH_FILE << "\n";
        std::cout << "Total messages sent         : " << messages_sent << "\n";
        std::cout << "Valid messages (A,C,D,E,U,X): " << valid_messages << "\n";
        std::cout << "Total results received      : " << results_received << "\n";
        std::cout << "============================================\n";

        if (results_received != valid_messages) {
            std::cerr << "ERROR: Test Failed! Expected " << valid_messages << " results, but received " << results_received << "." << std::endl;
            return 1; // Return error
        }

    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    std::cout << "INFO: Test Passed!" << std::endl;
    return 0;
}
