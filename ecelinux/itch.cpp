#include "itch.hpp"
#include "itch_common.hpp"

#include <iostream>
#include <iomanip>

static inline ap_uint<64> read_u64_be(const char* p) {
// #pragma HLS INLINE
    ap_uint<64> v = 0;
    for (int i = 0; i < 8; ++i) {
    #pragma HLS PIPELINE II=1
        v <<= 8;
        v |= (ap_uint<64>)((unsigned char)p[i]);
    }
    return v;
}

static inline ap_uint<32> read_u32_be(const char* p) {
// #pragma HLS INLINE
    ap_uint<32> v = 0;
    for (int i = 0; i < 4; ++i) {
    #pragma HLS PIPELINE II=1
        v <<= 8;
        v |= (ap_uint<32>)((unsigned char)p[i]);
    }
    return v;
}

void itch_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
    // Input processing
    bit32_t hdr    = strm_in.read();
    uint16_t msg_len = (uint16_t)hdr(15, 0);

    char in_buffer[MAX_MESSAGE_SIZE];   // 36 bytes is enough for our ITCH msgs
    int  idx   = 0;
    int  words = (msg_len + 3) >> 2;    // # of 32-bit words = ceil(msg_len/4)

    for (int w = 0; w < words; ++w) {
    #pragma HLS PIPELINE II=1
        bit32_t word = strm_in.read();

        if (idx < msg_len){
            in_buffer[idx++] = (char)word(31,24);
            in_buffer[idx++] = (char)word(23,16);
            in_buffer[idx++] = (char)word(15, 8);
            in_buffer[idx++] = (char)word( 7, 0);
        } 
    }

    // Optional debug: reconstructed message
#ifdef ITCH_DEBUG_PRINT
    std::cout << "DUT's reconstructed msg:      ";
    for (int i = 0; i < msg_len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (unsigned int)(unsigned char)in_buffer[i] << " ";
    }
    std::cout << std::dec << "\n";
#endif

    ParsedMessage parsed = parser(in_buffer);

    // ------------------------------------------------------
    // Output processing
    // ------------------------------------------------------
    bit32_t w0 = 0;
    w0(7,0)   = parsed.type;
    w0(15,8)  = parsed.side;
    strm_out.write(w0);
    strm_out.write((bit32_t)parsed.order_id.range(63,32));
    strm_out.write((bit32_t)parsed.order_id.range(31, 0));
    strm_out.write((bit32_t)parsed.new_order_id.range(63,32));
    strm_out.write((bit32_t)parsed.new_order_id.range(31, 0));
    strm_out.write((bit32_t)parsed.shares);
    strm_out.write((bit32_t)parsed.price);
    strm_out.write((bit32_t)parsed.timestamp);
}

ParsedMessage parser(char* buffer) {
    ParsedMessage out;
    out.type         = 0;
    out.side         = 0;
    out.order_id     = 0;
    out.new_order_id = 0;
    out.shares       = 0;
    out.price        = 0;
    out.timestamp    = 0;

    char msgType = buffer[0];
    out.type = (ap_uint<8>)msgType;

    // All relevant messages have timestamp at the same location
    if (msgType == ITCH::AddOrderMessageType ||
        msgType == ITCH::OrderExecutedMessageType ||
        msgType == ITCH::OrderExecutedWithPriceMessageType ||
        msgType == ITCH::OrderCancelMessageType ||
        msgType == ITCH::OrderDeleteMessageType ||
        msgType == ITCH::OrderReplaceMessageType) {
        
        // Timestamp is 48 bits (6 bytes) starting at byte 5
        ap_uint<64> ts = 0;
        for (int i = 0; i < 6; ++i) {
        #pragma HLS PIPELINE II=1
            ts <<= 8;
            ts |= (ap_uint<64>)((unsigned char)buffer[5 + i]);
        }
        out.timestamp = ts;
    }

    switch (msgType) {

    // ---------------- Add Order ('A') ----------------
    case ITCH::AddOrderMessageType: {
        out.order_id = read_u64_be(buffer + 11);
        out.side     = (ap_uint<8>)buffer[19];
        out.shares   = read_u32_be(buffer + 20);
        out.price    = read_u32_be(buffer + 32);
        break;
    }

    // ------------- Order Executed ('E') --------------
    case ITCH::OrderExecutedMessageType: {
        out.order_id = read_u64_be(buffer + 11);
        out.shares   = read_u32_be(buffer + 19);
        break;
    }

    // ------ Order Executed With Price ('C') ----------
    case ITCH::OrderExecutedWithPriceMessageType: {
        out.order_id = read_u64_be(buffer + 11);
        out.shares   = read_u32_be(buffer + 19);
        // out.price    = read_u32_be(buffer + 32);
        break;
    }

    // ---------------- Order Cancel ('X') --------------
    case ITCH::OrderCancelMessageType: {
        out.order_id = read_u64_be(buffer + 11);
        out.shares   = read_u32_be(buffer + 19);
        break;
    }

    // ---------------- Order Delete ('D') --------------
    case ITCH::OrderDeleteMessageType: {
        out.order_id = read_u64_be(buffer + 11);
        break;
    }

    // ---------------- Order Replace ('U') -------------
    case ITCH::OrderReplaceMessageType: {
        out.order_id     = read_u64_be(buffer + 11);
        out.new_order_id = read_u64_be(buffer + 19);
        out.shares       = read_u32_be(buffer + 27);
        out.price        = read_u32_be(buffer + 31);
        break;
    }

    default:
        break;
    }

    return out;
}
