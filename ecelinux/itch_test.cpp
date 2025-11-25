#include "itch.hpp"
#include "itch_reader.hpp"
#include "itch_common.hpp"

#include <hls_stream.h>
#include <ap_int.h>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <endian.h>

static const char* INPUT_ITCH_FILE = "./data/itch_test_in";

static const char* type_name(ITCH::MessageType_t t) {
    switch (t) {
        case ITCH::SystemEventMessageType: return "SystemEvent";
        case ITCH::StockDirectoryMessageType: return "StockDirectory";
        case ITCH::StockTradingActionMessageType: return "StockTradingAction";
        case ITCH::RegSHORestrictionMessageType: return "RegSHORestriction";
        case ITCH::MarketParticipantPositionMessageType: return "MarketParticipantPosition";
        case ITCH::MWCBDeclineLevelMessageType: return "MWCBDeclineLevel";
        case ITCH::MWCBStatusMessageType: return "MWCBStatus";
        case ITCH::IPOQuotingPeriodUpdateMessageType: return "IPOQuotingPeriodUpdate";
        case ITCH::LULDAuctionCollarMessageType: return "LULDAuctionCollar";
        case ITCH::OperationalHaltMessageType: return "OperationalHalt";
        case ITCH::AddOrderMessageType: return "AddOrder";
        case ITCH::AddOrderMPIDAttributionMessageType: return "AddOrderMPIDAttribution";
        case ITCH::OrderExecutedMessageType: return "OrderExecuted";
        case ITCH::OrderExecutedWithPriceMessageType: return "OrderExecutedWithPrice";
        case ITCH::OrderCancelMessageType: return "OrderCancel";
        case ITCH::OrderDeleteMessageType: return "OrderDelete";
        case ITCH::OrderReplaceMessageType: return "OrderReplace";
        case ITCH::TradeMessageType: return "Trade";
        case ITCH::CrossTradeMessageType: return "CrossTrade";
        case ITCH::BrokenTradeMessageType: return "BrokenTrade";
        case ITCH::NOIIMessageType: return "NOII";
        case ITCH::RetailInterestMessageType: return "RetailInterest";
        case ITCH::DirectListingWithCapitalRaisePriceDiscoveryMessageType: return "DLCRPriceDiscovery";
        default: return "Unknown";
    }
}

int main() {
    try {
        ITCH::Reader reader(INPUT_ITCH_FILE, 16384);

        hls::stream<bit32_t> in_stream;
        hls::stream<bit32_t> out_stream;

        std::unordered_map<ITCH::MessageType_t, uint64_t> counts;
        uint64_t total = 0;
        bool pass = 0;
        int errors = 0;

        const char* msg = nullptr;
        while ((msg = reader.nextMessage())) {
            auto t = ITCH::Parser::getDataMessageType(msg);
            counts[t]++; total++;

            uint16_t net_len = *(const uint16_t*)(msg);
            uint16_t msg_len = be16toh(net_len);
            const unsigned char* payload = reinterpret_cast<const unsigned char*>(msg + 2);

            // Expected fields
            bit32_t type = payload[0];
            bit32_t side = 0;
            bit32_t order_id_hi = (payload[11] << 24) | (payload[12] << 16) |
                                (payload[13] << 8)  | payload[14];
            bit32_t order_id_lo = (payload[15] << 24) | (payload[16] << 16) |
                                (payload[17] << 8)  | payload[18];
            bit32_t new_order_hi = 0;
            bit32_t new_order_lo = 0;
            bit32_t shares = 0;
            bit32_t price = 0;

            switch ((char)type) {
            case 'A': {
                side = payload[19];
                shares = (payload[20] << 24) | (payload[21] << 16) | (payload[22] << 8)  | payload[23];
                price = (payload[32] << 24) | (payload[33] << 16) | (payload[34] << 8)  | payload[35];
                break;
            }
            case 'E': 
            case 'C': 
            case 'X': {
                shares = (payload[19] << 24) | (payload[20] << 16) | (payload[21] << 8)  | payload[22];
                break;
            }
            case 'U': {
                new_order_hi = (payload[19] << 24) | (payload[20] << 16) |
                                    (payload[21] << 8)  | payload[22];
                new_order_lo = (payload[23] << 24) | (payload[24] << 16) |
                                    (payload[25] << 8)  | payload[26];
                shares = (payload[27] << 24) | (payload[28] << 16) | (payload[29] << 8)  | payload[30];
                price = (payload[31] << 24) | (payload[32] << 16) | (payload[33] << 8)  | payload[34];
                break;
            }
            default: 
                break;
            }

            // // Print message details (for debugging)
            // std::cout << "Type: " << t << ", Length: " << msg_len << ", Payload: ";
            // for (int i = 0; i < msg_len; i++) {
            //     std::cout << std::hex << std::setw(2) << std::setfill('0')
            //             << static_cast<unsigned int>(static_cast<unsigned char>(payload[i]))
            //             << " ";
            // }
            // std::cout << std::dec << std::endl; // reset back to decimal

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
            itch_dut(in_stream, out_stream);

            // Read all 7 words to drain the stream
            std::cout << "Type " << t << " | ";
            for (int i = 0; i < 7; i++) {
                bit32_t out_word = out_stream.read();
                switch (i) {
                    case 0: pass = (type == out_word(7,0) && side == out_word(15,8)); break;
                    case 1: pass = (order_id_hi == out_word); break; 
                    case 2: pass = (order_id_lo == out_word); break; 
                    case 3: pass = (new_order_hi == out_word); break; 
                    case 4: pass = (new_order_lo == out_word); break; 
                    case 5: pass = (shares == out_word);break; 
                    case 6: {
                        pass = (price == out_word);
                        // std::cout << "| Spot Price=" << std::fixed << std::setprecision(4) << out_word;
                        break; 
                    }
                }
                if (!pass) errors++;
                std::cout << std::hex << std::setw(8) << std::setfill('0')
                        << static_cast<unsigned int>(out_word) << " ";
            }
            std::cout << "| Status=" << (pass ? "PASS" : "FAIL");
            std::cout << std::dec << "\n"; // reset back to decimal
        }

        // Summary only
    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << " Parser FPGA Testbench Summary\n";
    std::cout << "============================================\n";
        std::cout << "Parsed file:      " << INPUT_ITCH_FILE << "\n";
        std::cout << "Total messages:   " << total << "\n";
        std::cout << "Total bytes read: " << reader.getTotalBytesRead() << "\n\n";
        for (auto &kv : counts) std::cout << type_name(kv.first) << ": " << kv.second << "\n";
        std::cout << "\nError rate:       " 
              << std::setprecision(4)
              << (100.0 * errors / total) << "%\n";
    std::cout << "============================================\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
