#pragma once

#include "itch_common.hpp"
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <endian.h>

#define ASSERT  false
#if ASSERT
  #include <cassert>
#endif

namespace ITCH {

static constexpr size_t MESSAGE_HEADER_LENGTH = 2;
static constexpr size_t DEFAULT_BUFFER_SIZE   = 2048;

class Reader {
public:
  Reader() = delete;

  Reader(const char* _filename)
  : Reader(_filename, DEFAULT_BUFFER_SIZE) {}

  Reader(const char* _filename, size_t _bufferSize)
  : fdItch(::open(_filename, O_RDONLY)),
    bufferSize(_bufferSize),
    buffer(new char[_bufferSize]),
    _buffer(buffer),
    validBytes(0),
    totalBytesRead(0) {
#if ASSERT
    assert(bufferSize > MESSAGE_HEADER_LENGTH + maxITCHMessageSize);
#endif
    if (fdItch == -1) {
      delete[] buffer;
      throw std::invalid_argument(std::string("Failed to open file: ") + _filename);
    }
    ssize_t readBytes = ::read(fdItch, buffer, bufferSize);
    if (readBytes <= 0) {
      delete[] buffer;
      ::close(fdItch);
      throw std::invalid_argument(std::string("Failed to read from file: ") + _filename);
    }
    validBytes = static_cast<size_t>(readBytes);
  }

  Reader(const Reader&)            = delete;
  Reader& operator=(const Reader&) = delete;

  ~Reader() {
    ::close(fdItch);
    delete[] buffer;
  }

  const char* nextMessage() {
    // Refill if buffer fully consumed
    if (_buffer == (buffer + validBytes)) {
      ssize_t readBytes = ::read(fdItch, buffer, bufferSize);
      if (readBytes <= 0) return nullptr;
      validBytes = static_cast<size_t>(readBytes);
      _buffer = buffer;
    }

    // Ensure 2-byte header contiguous
    if ((_buffer + MESSAGE_HEADER_LENGTH) > (buffer + bufferSize)) {
      buffer[0] = *_buffer;
      constexpr size_t remainingLength = 1;
      ssize_t readBytes = ::read(fdItch, buffer + remainingLength, bufferSize - remainingLength);
      if (readBytes <= 0) return nullptr;
      validBytes = static_cast<size_t>(readBytes) + remainingLength;
      _buffer = buffer;
    }

    // Parse BE16 message length
    uint16_t messageLength = be16toh(*reinterpret_cast<const uint16_t*>(_buffer));
    if (messageLength == 0) return nullptr;

    // If message straddles end, compact and refill
    if ((_buffer + MESSAGE_HEADER_LENGTH + messageLength) > (buffer + validBytes)) {
      size_t offset = (buffer + validBytes) - _buffer; // remaining bytes of this message already in buffer
      std::memcpy(buffer, _buffer, offset);
      ssize_t readBytes = ::read(fdItch, buffer + offset, bufferSize - offset);
      if (readBytes <= 0) {
        if (readBytes == 0) return nullptr;
        throw std::runtime_error("Failed to read from file");
      }
      validBytes = static_cast<size_t>(readBytes) + offset;
      _buffer = buffer;

      if ((MESSAGE_HEADER_LENGTH + messageLength) > validBytes) return nullptr;
    }

#if ASSERT
    switch (_buffer[messageTypeIndex]) {
      case SystemEventMessageType:                             assert(messageLength == MessageLength<SystemEventMessageType>); break;
      case StockDirectoryMessageType:                          assert(messageLength == MessageLength<StockDirectoryMessageType>); break;
      case StockTradingActionMessageType:                      assert(messageLength == MessageLength<StockTradingActionMessageType>); break;
      case RegSHORestrictionMessageType:                       assert(messageLength == MessageLength<RegSHORestrictionMessageType>); break;
      case MarketParticipantPositionMessageType:               assert(messageLength == MessageLength<MarketParticipantPositionMessageType>); break;
      case MWCBDeclineLevelMessageType:                        assert(messageLength == MessageLength<MWCBDeclineLevelMessageType>); break;
      case MWCBStatusMessageType:                              assert(messageLength == MessageLength<MWCBStatusMessageType>); break;
      case IPOQuotingPeriodUpdateMessageType:                  assert(messageLength == MessageLength<IPOQuotingPeriodUpdateMessageType>); break;
      case LULDAuctionCollarMessageType:                       assert(messageLength == MessageLength<LULDAuctionCollarMessageType>); break;
      case OperationalHaltMessageType:                         assert(messageLength == MessageLength<OperationalHaltMessageType>); break;
      case AddOrderMessageType:                                assert(messageLength == MessageLength<AddOrderMessageType>); break;
      case AddOrderMPIDAttributionMessageType:                 assert(messageLength == MessageLength<AddOrderMPIDAttributionMessageType>); break;
      case OrderExecutedMessageType:                           assert(messageLength == MessageLength<OrderExecutedMessageType>); break;
      case OrderExecutedWithPriceMessageType:                  assert(messageLength == MessageLength<OrderExecutedWithPriceMessageType>); break;
      case OrderCancelMessageType:                             assert(messageLength == MessageLength<OrderCancelMessageType>); break;
      case OrderDeleteMessageType:                             assert(messageLength == MessageLength<OrderDeleteMessageType>); break;
      case OrderReplaceMessageType:                            assert(messageLength == MessageLength<OrderReplaceMessageType>); break;
      case TradeMessageType:                                   assert(messageLength == MessageLength<TradeMessageType>); break;
      case CrossTradeMessageType:                              assert(messageLength == MessageLength<CrossTradeMessageType>); break;
      case BrokenTradeMessageType:                             assert(messageLength == MessageLength<BrokenTradeMessageType>); break;
      case NOIIMessageType:                                    assert(messageLength == MessageLength<NOIIMessageType>); break;
      case RetailInterestMessageType:                          assert(messageLength == MessageLength<RetailInterestMessageType>); break;
      case DirectListingWithCapitalRaisePriceDiscoveryMessageType:
                                                               assert(messageLength == MessageLength<DirectListingWithCapitalRaisePriceDiscoveryMessageType>); break;
      default:                                                 assert(false); break;
    }
#endif

    const char* out = _buffer;
    _buffer += (MESSAGE_HEADER_LENGTH + messageLength);
    totalBytesRead += (MESSAGE_HEADER_LENGTH + messageLength);

#if ASSERT
    assert(_buffer <= (buffer + bufferSize));
#endif
    return out;
  }

  long long getTotalBytesRead() const { return totalBytesRead; }

private:
  int         fdItch;
  size_t      bufferSize;
  char*       buffer;
  char*       _buffer;
  size_t      validBytes;
  long long   totalBytesRead;
};


namespace Parser {

inline SystemEventMessage createSystemEventMessage(const char* data) {
  data += MESSAGE_HEADER_LENGTH;
  char      messageType = *data;
  uint16_t  stockLocate = be16toh(*(uint16_t*)(data + 1));
  uint64_t  timestamp   = be64toh(*(uint64_t*)(data + 5)) >> 16;
//   return {messageType, stockLocate, timestamp};
return { messageType, stockLocate, 0, timestamp, 0 };
}

inline StockDirectoryMessage createStockDirectoryMessage(const char* data) {
  data += MESSAGE_HEADER_LENGTH;
  StockDirectoryMessage m{};
  return m;
}

inline AddOrderMessage createAddOrderMessage(const char* data) {
  data += MESSAGE_HEADER_LENGTH;
  char      messageType = *data;
  uint16_t  stockLocate = be16toh(*(uint16_t*)(data + 1));
  uint64_t  timestamp   = be64toh(*(uint64_t*)(data + 5)) >> 16;
  uint64_t  orderRef    = be64toh(*(uint64_t*)(data + 11));
  char      side        = *(data + 19);
  uint32_t  shares      = be32toh(*(uint32_t*)(data + 20));
  uint32_t  price       = be32toh(*(uint32_t*)(data + 32));
  return {messageType, stockLocate, timestamp, orderRef, side, shares, price};
}

inline CrossTradeMessage createCrossTradeMessage(const char* data) {
  data += MESSAGE_HEADER_LENGTH;
  char      messageType = *data;
  uint16_t  stockLocate = be16toh(*(uint16_t*)(data + 1));
  uint64_t  timestamp   = be64toh(*(uint64_t*)(data + 5)) >> 16;
  uint64_t  orderRef    = be64toh(*(uint64_t*)(data + 11));
  uint64_t  shares      = be64toh(*(uint64_t*)(data + 19));
  uint32_t  crossPrice  = be32toh(*(uint32_t*)(data + 27));
  return {messageType, stockLocate, timestamp, orderRef, shares, crossPrice};
}

inline BrokenTradeMessage createBrokenTradeMessage(const char* data) {
  data += MESSAGE_HEADER_LENGTH;
  char      messageType = *data;
  uint16_t  stockLocate = be16toh(*(uint16_t*)(data + 1));
  uint64_t  timestamp   = be64toh(*(uint64_t*)(data + 5)) >> 16;
  uint64_t  matchNum    = be64toh(*(uint64_t*)(data + 11));
  return {messageType, stockLocate, timestamp, matchNum};
}

inline MessageType_t getDataMessageType(const char* data) {
  return *(data + MESSAGE_HEADER_LENGTH);
}

inline Timestamp_t getDataTimestamp(const char* data) {
  return be64toh(*(const uint64_t*)(data + MESSAGE_HEADER_LENGTH + 5)) >> 16;
}

inline Timestamp_t strToTimestamp(const char* s) {
  return strtoull(s, nullptr, 10);
}

} // namespace Parser
} // namespace ITCH
