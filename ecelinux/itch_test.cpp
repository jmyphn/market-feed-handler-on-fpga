#include "itch.hpp"
#include "itch_reader.hpp"    
#include "itch_common.hpp"

#include <hls_stream.h>
#include <ap_int.h>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <endian.h>   

static const char* INPUT_ITCH_FILE = "./data/filtered_data_500";

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

        const char* msg = nullptr;
        while ((msg = reader.nextMessage())) {
            auto t = ITCH::Parser::getDataMessageType(msg);
            counts[t]++; total++;

            uint16_t net_len = *(const uint16_t*)(msg);
            uint16_t msg_len = be16toh(net_len);
            const unsigned char* payload = reinterpret_cast<const unsigned char*>(msg + 2);
            
            // Print message details (for debugging)
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
                w(7, 0)     = payload[i];
                w(15, 8)    = (i+1 < msg_len) ? payload[i+1] : 0;
                w(23, 16)   = (i+2 < msg_len) ? payload[i+2] : 0;
                w(31, 24)   = (i+3 < msg_len) ? payload[i+3] : 0;
                in_stream.write(w);
            }
            dut(in_stream, out_stream);
            
            // Read all 7 words to drain the stream
            // std::cout << "--> ";
            for (int i = 0; i < 7; i++) {
                bit32_t out_word = out_stream.read();
            //     std::cout << std::hex << std::setw(8) << std::setfill('0')
            //             << static_cast<unsigned int>(out_word) << " ";
            }
            // std::cout << std::dec << "\n\n"; // reset back to decimal
        }

        // Summary only
        std::cout << "Parsed file: " << INPUT_ITCH_FILE << "\n";
        std::cout << "Total messages: " << total << "\n";
        std::cout << "Total bytes read: " << reader.getTotalBytesRead() << "\n\n";
        for (auto &kv : counts) std::cout << type_name(kv.first) << ": " << kv.second << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
