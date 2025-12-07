#include "itch.hpp"

static inline bit64_t read_u64_be(const char* p) {
    bit64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v <<= 8;
        v |= (bit64_t)((unsigned char)p[i]);
    }
    return v;
}

static inline bit32_t read_u32_be(const char* p) {
    bit32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v <<= 8;
        v |= (bit32_t)((unsigned char)p[i]);
    }
    return v;
}

void itch_dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
    // ------------------------------------------------------
    // Input processing
    // ------------------------------------------------------
    bit32_t hdr = strm_in.read();
    bit16_t msg_len = (bit16_t)hdr(15, 0);
    assert(msg_len == (bit16_t)hdr(15, 0));

    char in_buffer[36];   // 36 bytes is enough for our ITCH msgs
    int  idx   = 0;

    // Handle variable msg length
    bit4_t words = (msg_len + 3) >> 2;    // # of 32-bit words = ceil(msg_len/4)
    assert((msg_len + 3) >> 2 == (bit16_t)words);
    for (int w = 0; w < words; ++w) {
        bit32_t word = strm_in.read();

        if (idx < msg_len){
            in_buffer[idx++] = (char)word(31,24);
            in_buffer[idx++] = (char)word(23,16);
            in_buffer[idx++] = (char)word(15, 8);
            in_buffer[idx++] = (char)word( 7, 0);
        } 
    }

    // ------------------------------------------------------
    // Call parser
    // ------------------------------------------------------
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
}

ParsedMessage parser(char* buffer) {
    ParsedMessage out;

    char msgType = buffer[0];
    out.type = (bit8_t)msgType;

    switch (msgType) {

    // ---------------- Add Order ('A') ----------------
    case ITCH::AddOrderMessageType: {
        out.order_id = read_u64_be(buffer + 11);
        out.side     = (bit8_t)buffer[19];
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

    // // ---- PRINTING HERE IS NOT SYNTHESIZABLE ----
    // double price_display = out.price / 10000.0;
    // std::cout << std::fixed << std::setprecision(4)
    //          << "Msg_Price=" << std::setw(10) << price_display << " | "; 

    return out;
}
