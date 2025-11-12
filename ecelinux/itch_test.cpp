// itch_test.cpp — minimal ITCH parser using a hardcoded file path

#include "itch_reader.hpp"
#include "itch_common.hpp"

#include <cstdint>
#include <iostream>
#include <unordered_map>

static const char* INPUT_ITCH_FILE = "./data/tvagg";

// Short names for message types
static const char* type_name(ITCH::MessageType_t t) {
    switch (t) {
        case ITCH::SystemEventMessageType:                           return "SystemEvent";
        case ITCH::StockDirectoryMessageType:                        return "StockDirectory";
        case ITCH::StockTradingActionMessageType:                    return "StockTradingAction";
        case ITCH::RegSHORestrictionMessageType:                     return "RegSHORestriction";
        case ITCH::MarketParticipantPositionMessageType:             return "MarketParticipantPosition";
        case ITCH::MWCBDeclineLevelMessageType:                      return "MWCBDeclineLevel";
        case ITCH::MWCBStatusMessageType:                            return "MWCBStatus";
        case ITCH::IPOQuotingPeriodUpdateMessageType:                return "IPOQuotingPeriodUpdate";
        case ITCH::LULDAuctionCollarMessageType:                     return "LULDAuctionCollar";
        case ITCH::OperationalHaltMessageType:                       return "OperationalHalt";
        case ITCH::AddOrderMessageType:                              return "AddOrder";
        case ITCH::AddOrderMPIDAttributionMessageType:               return "AddOrderMPIDAttribution";
        case ITCH::OrderExecutedMessageType:                         return "OrderExecuted";
        case ITCH::OrderExecutedWithPriceMessageType:                return "OrderExecutedWithPrice";
        case ITCH::OrderCancelMessageType:                           return "OrderCancel";
        case ITCH::OrderDeleteMessageType:                           return "OrderDelete";
        case ITCH::OrderReplaceMessageType:                          return "OrderReplace";
        case ITCH::TradeMessageType:                                 return "Trade";
        case ITCH::CrossTradeMessageType:                            return "CrossTrade";
        case ITCH::BrokenTradeMessageType:                           return "BrokenTrade";
        case ITCH::NOIIMessageType:                                  return "NOII";
        case ITCH::RetailInterestMessageType:                        return "RetailInterest";
        case ITCH::DirectListingWithCapitalRaisePriceDiscoveryMessageType: return "DLCRPriceDiscovery";
        default:                                                     return "Unknown";
    }
}

static inline double to_dollars(uint32_t p) {
    return static_cast<double>(p) / 10000.0;  // adjust if your scale differs
}

int main() {
    try {
        ITCH::Reader reader(INPUT_ITCH_FILE, 16384);

        std::unordered_map<ITCH::MessageType_t, uint64_t> counts;
        uint64_t total = 0;

        const char* msg = nullptr;
        while ((msg = reader.nextMessage())) {
            auto t = ITCH::Parser::getDataMessageType(msg);
            counts[t]++;
            total++;

            // ===========================================
            // Uncomment to print decoded version
            // ===========================================
            // const auto ts = ITCH::Parser::getDataTimestamp(msg);  // uint64_t per your code

            // switch (t) {
            //     case ITCH::AddOrderMessageType: {
            //         // struct AddOrderMessage {
            //         //   char messageType; uint16_t stockLocate; uint64_t timestamp;
            //         //   uint64_t orderReferenceNumber; char buySellIndicator;
            //         //   uint32_t shares; uint32_t price;
            //         // }
            //         const ITCH::AddOrderMessage* m = reinterpret_cast<const ITCH::AddOrderMessage*>(msg);
            //         std::cout << ts << " [AddOrder]"
            //                 << " Ref="   << m->orderReferenceNumber
            //                 << " Side="  << m->buySellIndicator
            //                 << " Sz="    << m->shares
            //                 << " Px="    << m->price << " (" << to_dollars(m->price) << ")"
            //                 << "\n";
            //         break;
            //     }

            //     case ITCH::OrderCancelMessageType: {
            //         // struct OrderCancelMessage {
            //         //   char messageType; uint16_t stockLocate; uint64_t timestamp;
            //         //   uint64_t orderReferenceNumber; uint32_t cancelledShares;
            //         // }
            //         const ITCH::OrderCancelMessage* m = reinterpret_cast<const ITCH::OrderCancelMessage*>(msg);
            //         std::cout << ts << " [Cancel]"
            //                 << " Ref="     << m->orderReferenceNumber
            //                 << " Canceled="<< m->cancelledShares
            //                 << "\n";
            //         break;
            //     }

            //     case ITCH::TradeMessageType: {
            //         // struct TradeMessage {
            //         //   char messageType; uint16_t stockLocate; uint64_t timestamp;
            //         //   uint64_t orderReferenceNumber; char buySellIndicator;
            //         //   uint32_t shares; uint32_t price;
            //         // }
            //         const ITCH::TradeMessage* m = reinterpret_cast<const ITCH::TradeMessage*>(msg);
            //         std::cout << ts << " [Trade]"
            //                 << " Ref="   << m->orderReferenceNumber
            //                 << " Side="  << m->buySellIndicator
            //                 << " Sz="    << m->shares
            //                 << " Px="    << m->price << " (" << to_dollars(m->price) << ")"
            //                 << "\n";
            //         break;
            //     }

            //     // Add other message types here, matching names in itch_common.hpp

            //     default:
            //         break;
            // }




        }

        std::cout << "Parsed file: " << INPUT_ITCH_FILE << "\n";
        std::cout << "Total messages: " << total << "\n";
        std::cout << "Total bytes read: " << reader.getTotalBytesRead() << "\n\n";

        for (auto& kv : counts) {
            std::cout << type_name(kv.first) << ": " << kv.second << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
