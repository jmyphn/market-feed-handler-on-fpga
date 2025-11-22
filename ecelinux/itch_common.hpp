#ifndef ORDER_BOOK_ITCH_COMMON
#define ORDER_BOOK_ITCH_COMMON

#include <cstddef>
#include <cstdint>

namespace ITCH {

constexpr size_t maxITCHMessageSize     = 50;

namespace Side {
    constexpr char BUY  = 'B';
    constexpr char SELL = 'S';
}

using MessageType_t = char;
using Timestamp_t   = uint64_t;

constexpr MessageType_t SystemEventMessageType               = 'S';
constexpr MessageType_t StockDirectoryMessageType            = 'R';
constexpr MessageType_t StockTradingActionMessageType        = 'H';
constexpr MessageType_t RegSHORestrictionMessageType         = 'Y';
constexpr MessageType_t MarketParticipantPositionMessageType = 'L';
constexpr MessageType_t MWCBDeclineLevelMessageType          = 'V';
constexpr MessageType_t MWCBStatusMessageType                = 'W';
constexpr MessageType_t IPOQuotingPeriodUpdateMessageType    = 'K';
constexpr MessageType_t LULDAuctionCollarMessageType         = 'J';
constexpr MessageType_t OperationalHaltMessageType           = 'h';
constexpr MessageType_t AddOrderMessageType                  = 'A';
constexpr MessageType_t AddOrderMPIDAttributionMessageType   = 'F';
constexpr MessageType_t OrderExecutedMessageType             = 'E';
constexpr MessageType_t OrderExecutedWithPriceMessageType    = 'C';
constexpr MessageType_t OrderCancelMessageType               = 'X';
constexpr MessageType_t OrderDeleteMessageType               = 'D';
constexpr MessageType_t OrderReplaceMessageType              = 'U';
constexpr MessageType_t TradeMessageType                     = 'P';
constexpr MessageType_t CrossTradeMessageType                = 'Q';
constexpr MessageType_t BrokenTradeMessageType               = 'B';
constexpr MessageType_t NOIIMessageType                      = 'I';
constexpr MessageType_t RetailInterestMessageType            = 'N';
constexpr MessageType_t DirectListingWithCapitalRaisePriceDiscoveryMessageType = 'O';

// TypeTag
template<char M> inline const char* TypeTag() { return "UNK"; }
template<> inline const char* TypeTag<SystemEventMessageType>()               { return "SYS"; }
template<> inline const char* TypeTag<StockDirectoryMessageType>()            { return "DIR"; }
template<> inline const char* TypeTag<StockTradingActionMessageType>()        { return "ACT"; }
template<> inline const char* TypeTag<RegSHORestrictionMessageType>()         { return "REG"; }
template<> inline const char* TypeTag<MarketParticipantPositionMessageType>() { return "POS"; }
template<> inline const char* TypeTag<MWCBDeclineLevelMessageType>()          { return "DCL"; }
template<> inline const char* TypeTag<MWCBStatusMessageType>()                { return "STS"; }
template<> inline const char* TypeTag<IPOQuotingPeriodUpdateMessageType>()    { return "IPO"; }
template<> inline const char* TypeTag<LULDAuctionCollarMessageType>()         { return "COL"; }
template<> inline const char* TypeTag<OperationalHaltMessageType>()           { return "HLT"; }
template<> inline const char* TypeTag<AddOrderMessageType>()                  { return "ADD"; }
template<> inline const char* TypeTag<AddOrderMPIDAttributionMessageType>()   { return "ADM"; }
template<> inline const char* TypeTag<OrderExecutedMessageType>()             { return "EXC"; }
template<> inline const char* TypeTag<OrderExecutedWithPriceMessageType>()    { return "EXP"; }
template<> inline const char* TypeTag<OrderCancelMessageType>()               { return "CNL"; }
template<> inline const char* TypeTag<OrderDeleteMessageType>()               { return "DEL"; }
template<> inline const char* TypeTag<OrderReplaceMessageType>()              { return "RPL"; }
template<> inline const char* TypeTag<TradeMessageType>()                     { return "TRD"; }
template<> inline const char* TypeTag<CrossTradeMessageType>()                { return "CRX"; }
template<> inline const char* TypeTag<BrokenTradeMessageType>()               { return "BRK"; }
template<> inline const char* TypeTag<NOIIMessageType>()                      { return "NOI"; }
template<> inline const char* TypeTag<RetailInterestMessageType>()            { return "RTL"; }
template<> inline const char* TypeTag<DirectListingWithCapitalRaisePriceDiscoveryMessageType>() { return "DSC"; }

// MessageLength
template<char M> inline uint16_t MessageLength() { return 0xFFFF; }
template<> inline uint16_t MessageLength<SystemEventMessageType>()                { return 12; }
template<> inline uint16_t MessageLength<StockDirectoryMessageType>()             { return 39; }
template<> inline uint16_t MessageLength<StockTradingActionMessageType>()         { return 25; }
template<> inline uint16_t MessageLength<RegSHORestrictionMessageType>()          { return 20; }
template<> inline uint16_t MessageLength<MarketParticipantPositionMessageType>()  { return 26; }
template<> inline uint16_t MessageLength<MWCBDeclineLevelMessageType>()           { return 35; }
template<> inline uint16_t MessageLength<MWCBStatusMessageType>()                 { return 12; }
template<> inline uint16_t MessageLength<IPOQuotingPeriodUpdateMessageType>()     { return 28; }
template<> inline uint16_t MessageLength<LULDAuctionCollarMessageType>()          { return 35; }
template<> inline uint16_t MessageLength<OperationalHaltMessageType>()            { return 21; }
template<> inline uint16_t MessageLength<AddOrderMessageType>()                   { return 36; }
template<> inline uint16_t MessageLength<AddOrderMPIDAttributionMessageType>()    { return 40; }
template<> inline uint16_t MessageLength<OrderExecutedMessageType>()              { return 31; }
template<> inline uint16_t MessageLength<OrderExecutedWithPriceMessageType>()     { return 36; }
template<> inline uint16_t MessageLength<OrderCancelMessageType>()                { return 23; }
template<> inline uint16_t MessageLength<OrderDeleteMessageType>()                { return 19; }
template<> inline uint16_t MessageLength<OrderReplaceMessageType>()               { return 35; }
template<> inline uint16_t MessageLength<TradeMessageType>()                      { return 44; }
template<> inline uint16_t MessageLength<CrossTradeMessageType>()                 { return 40; }
template<> inline uint16_t MessageLength<BrokenTradeMessageType>()                { return 19; }
template<> inline uint16_t MessageLength<NOIIMessageType>()                       { return 50; }
template<> inline uint16_t MessageLength<RetailInterestMessageType>()             { return 20; }
template<> inline uint16_t MessageLength<DirectListingWithCapitalRaisePriceDiscoveryMessageType>() { return 48; }

struct SystemEventMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     eventCode;
};

struct StockDirectoryMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint8_t     marketCategory;
    uint8_t     financialStatusIndicator;
    uint32_t    roundLotSize;
    uint8_t     roundLotsOnly;
    uint8_t     issueClassification;
    uint8_t     issueSubType[2];
    uint8_t     authenticity;
    uint8_t     shortSaleThresholdIndicator;
    uint8_t     IPOFlag;
    uint8_t     LULDReferencePriceTier;
    uint8_t     ETPFlag;
    uint32_t    ETPLeverageFactor;
    uint8_t     inverseIndicator;
};

struct StockTradingActionMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint8_t     tradingState;
    uint8_t     reserved;
    uint8_t     reason[4];
};

struct RegSHORestrictionMessage {
    char        messageType;
    uint16_t    locateCode;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint8_t     RegSHOAction;
};

struct MarketParticipantPositionMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     MPID[4];
    uint8_t     stock[8];
    uint8_t     primaryMarketMaker;
    uint8_t     marketMakerMode;
    uint8_t     marketParticipantState;
};

struct MWCBDeclineLevelMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint64_t    level1;
    uint64_t    level2;
    uint64_t    level3;
};

struct MWCBStatusMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     breachedLevel;
};

struct IPOQuotingPeriodUpdateMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint32_t    IPOQuotationReleaseTime;
    uint8_t     IPOQuotationReleaseQualifier;
    uint32_t    IPOPrice;
};

struct LULDAuctionCollarMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint32_t    auctionCollarReferencePrice;
    uint32_t    upperAuctionCollarPrice;
    uint32_t    lowerAuctionCollarPrice;
    uint32_t    auctionCollarExtension;
};

struct OperationalHaltMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint8_t     marketCode;
    uint8_t     operationalHaltAction;
};

struct AddOrderMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
    char        buySellIndicator;
    uint32_t    shares;
    uint32_t    price;
};

struct AddOrderMPIDAttributionMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
    char        buySellIndicator;
    uint32_t    shares;
    uint32_t    price;
};

struct OrderExecutedMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
    uint32_t    executedShares;
};

struct OrderExecutedWithPriceMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
    uint32_t    executedShares;
    uint32_t    executionPrice;
};

struct OrderCancelMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
    uint32_t    cancelledShares;
};

struct OrderDeleteMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
};

struct OrderReplaceMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    originalOrderReferenceNumber;
    uint64_t    newOrderReferenceNumber;
    uint32_t    shares;
    uint32_t    price;
};

struct TradeMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
    char        buySellIndicator;
    uint32_t    shares;
    uint32_t    price;
};

struct CrossTradeMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
    uint64_t    shares;
    uint32_t    crossPrice;
};

struct BrokenTradeMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint64_t    timestamp;
    uint64_t    orderReferenceNumber;
};

struct NOIIMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint64_t    pairedShares;
    uint64_t    imbalanceShares;
    uint8_t     imbalanceDirection;
    uint8_t     stock[8];
    uint32_t    farPrice;
    uint32_t    nearPrice;
    uint32_t    currentReferencePrice;
    uint8_t     crossType;
    uint8_t     priceVariationIndicator;
};

struct RetailInterestMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint8_t     InterestFlag;
};

struct DirectListingWithCapitalRaisePriceDiscoveryMessage {
    char        messageType;
    uint16_t    stockLocate;
    uint16_t    trackingNumber;
    uint64_t    timestamp;
    uint8_t     stock[8];
    uint8_t     openEligibilityStatus;
    uint32_t    minimumAllowablePrice;
    uint32_t    maximumAllowablePrice;
    uint32_t    nearExecutionPrice;
    uint64_t    nearExecutionTime;
    uint32_t    lowerPriceRangeCollar;
    uint32_t    upperPriceRangeCollar;
};

} // namespace ITCH

template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::AddOrderMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::AddOrderMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber <<
        " side " << m.buySellIndicator <<
        " shares " << m.shares <<
        " price " << m.price;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::AddOrderMPIDAttributionMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::AddOrderMPIDAttributionMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber <<
        " side " << m.buySellIndicator <<
        " shares " << m.shares <<
        " price " << m.price;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::OrderExecutedMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::OrderExecutedMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber <<
        " exec shares " << m.executedShares;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::OrderExecutedWithPriceMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::OrderExecutedWithPriceMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber <<
        " exec shares " << m.executedShares <<
        " exec price " << m.executionPrice;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::OrderCancelMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::OrderCancelMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber <<
        " cancelled shares " << m.cancelledShares;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::OrderDeleteMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::OrderDeleteMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::OrderReplaceMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::OrderReplaceMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " orig order " << m.originalOrderReferenceNumber <<
        " new order " << m.newOrderReferenceNumber <<
        " shares " << m.shares <<
        " price " << m.price;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::TradeMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::TradeMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber <<
        " side " << m.buySellIndicator <<
        " shares " << m.shares <<
        " price " << m.price;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::CrossTradeMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::CrossTradeMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp <<
        " order reference number " << m.orderReferenceNumber <<
        " shares " << m.shares <<
        " cross price " << m.crossPrice;
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, ITCH::BrokenTradeMessage const & m) {
    os <<
        ITCH::TypeTag<ITCH::BrokenTradeMessageType> <<
        " type " << m.messageType <<
        " stock locate " << m.stockLocate <<
        " timestamp " << m.timestamp;
    return os;
}

#endif // ORDER_BOOK_ITCH_COMMON
