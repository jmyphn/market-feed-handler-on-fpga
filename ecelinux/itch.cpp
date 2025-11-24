//==========================================================================
// itch.cpp
//==========================================================================
// @brief: A parser for ITCH input files

#include "itch.hpp"
#include "itch_common.hpp"

//----------------------------------------------------------
// Top function
//----------------------------------------------------------

void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
    // #pragma HLS INTERFACE axis port=strm_in
    // #pragma HLS INTERFACE axis port=strm_out
    // #pragma HLS PIPELINE off

    // ------------------------------------------------------
    // Input processing
    // ------------------------------------------------------    
    bit32_t hdr = strm_in.read();                
    uint16_t msg_len = (uint16_t)hdr(15, 0);


    char in_buffer[MAX_MESSAGE_SIZE];   // 36 bytes
    int idx = 0;
    int words = (msg_len + 3) >> 2;     // # of 32-bit words = ceil(msg_len/4)
    
    // #pragma HLS ARRAY_PARTITION variable=in_buffer complete dim=1
    for (int w = 0; w < words; w++) {
        // #pragma HLS PIPELINE II=1
        bit32_t word = strm_in.read();
        if (idx < msg_len) in_buffer[idx++] = (char)word(7, 0);
        if (idx < msg_len) in_buffer[idx++] = (char)word(15, 8);
        if (idx < msg_len) in_buffer[idx++] = (char)word(23, 16);
        if (idx < msg_len) in_buffer[idx++] = (char)word(31, 24);
    }

    // Print reconstructed message (for debugging)
    // std::cout << "DUT's reconstructed msg:      ";
    // for (int i = 0; i < msg_len; i++) {
    //     // Cast to unsigned int to print the byte as a number
    //     std::cout << std::hex << std::setw(2) << std::setfill('0') 
    //             << static_cast<unsigned int>(static_cast<unsigned char>(in_buffer[i])) 
    //             << " ";
    // }
    // std::cout << std::dec << std::endl; // reset to decimal

    // ------------------------------------------------------
    // Call Parser 
    // ------------------------------------------------------
    ParsedMessage parsed = parser(in_buffer);

    // ------------------------------------------------------
    // Output processing
    // ------------------------------------------------------
    bit32_t w0 = 0;
    w0(7,0)   = parsed.type;
    w0(15,8)  = parsed.side;
    w0 = parsed.order_id;
    strm_out.write(w0);
    strm_out.write((bit32_t)parsed.order_id.range(31,0));
    strm_out.write((bit32_t)parsed.order_id.range(63,32));
    strm_out.write((bit32_t)parsed.new_order_id.range(31,0));
    strm_out.write((bit32_t)parsed.new_order_id.range(63,32));
    strm_out.write((bit32_t)parsed.shares);
    strm_out.write((bit32_t)parsed.price);
}

//----------------------------------------------------------
// Parser
//----------------------------------------------------------
// @param[in] : in_buffer - the ITCH message 
// @return : the parsed message struct

ParsedMessage parser(char* buffer) {
    
    // Initialize the ParsedMessage struct
    ParsedMessage output;
    output.type        = 0;
    output.side        = 0;
    output.order_id    = 0;
    output.new_order_id = 0;
    output.shares      = 0;
    output.price       = 0;

    char msgType = buffer[0];
    output.type = (ap_uint<8>)msgType;

    switch (msgType) {
        case ITCH::AddOrderMessageType: {
            const ITCH::AddOrderMessage* m = (const ITCH::AddOrderMessage*)buffer;
            output.order_id = m->orderReferenceNumber;
            output.side     = m->buySellIndicator;
            output.shares   = m->shares;
            output.price    = m->price;
            break;
        }
        case ITCH::OrderExecutedMessageType: {
            const ITCH::OrderExecutedMessage* m = (const ITCH::OrderExecutedMessage*)buffer;
            output.order_id = m->orderReferenceNumber;
            output.shares   = m->executedShares;
            break;
        }
        case ITCH::OrderExecutedWithPriceMessageType: {
            const ITCH::OrderExecutedWithPriceMessage* m = (const ITCH::OrderExecutedWithPriceMessage*)buffer;
            output.order_id = m->orderReferenceNumber;
            output.shares   = m->executedShares;
            break;
        }
        case ITCH::OrderCancelMessageType: {
            const ITCH::OrderCancelMessage* m = (const ITCH::OrderCancelMessage*)buffer;
            output.order_id = m->orderReferenceNumber;
            output.shares = m->cancelledShares;
            break;
        }
        case ITCH::OrderDeleteMessageType: {
            const ITCH::OrderDeleteMessage* m = (const ITCH::OrderDeleteMessage*)buffer;
            output.order_id = m->orderReferenceNumber;
            break;
        }
        case ITCH::OrderReplaceMessageType: {
            const ITCH::OrderReplaceMessage* m = (const ITCH::OrderReplaceMessage*)buffer;
            output.order_id     = m->originalOrderReferenceNumber;
            output.new_order_id = m->newOrderReferenceNumber;
            output.shares       = m->shares;
            output.price        = m->price;
            break;
        }
        // case ITCH::TradeMessageType: {
        //     const ITCH::TradeMessage* m = (const ITCH::TradeMessage*)buffer;
        //     output.order_id = m->orderReferenceNumber;
        //     output.shares   = m->shares;
        //     output.price    = m->price;
        //     break;
        // }
        default:
            break;
    }

    return output;
}