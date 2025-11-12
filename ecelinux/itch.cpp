#include "itch.hpp"
#include "itch_common.hpp"

#define MAX_MESSAGE_SIZE 512

void dut(hls::stream<bit32_t> &strm_in, hls::stream<bit32_t> &strm_out) {
#pragma HLS INTERFACE axis port=strm_in
#pragma HLS INTERFACE axis port=strm_out
#pragma HLS PIPELINE off

    
    bit32_t hdr = strm_in.read();                
    uint16_t msg_len = (uint16_t)hdr(15, 0);
    if (msg_len > MAX_MESSAGE_SIZE) msg_len = MAX_MESSAGE_SIZE;  

    char buffer[MAX_MESSAGE_SIZE];
#pragma HLS ARRAY_PARTITION variable=buffer complete dim=1
    for (int i = 0; i < msg_len; i++) {
#pragma HLS PIPELINE II=1
        bit32_t bw = strm_in.read();                
        buffer[i] = (char)bw(7, 0);
    }

    bit32_t out_word = 0;
    char msgType = buffer[0];

    switch (msgType) {
        case ITCH::AddOrderMessageType: {
            const ITCH::AddOrderMessage* m = (const ITCH::AddOrderMessage*)buffer;
            out_word = (bit32_t)m->shares;
            break;
        }
        case ITCH::TradeMessageType: {
            const ITCH::TradeMessage* m = (const ITCH::TradeMessage*)buffer;
            out_word = (bit32_t)m->price;
            break;
        }
        case ITCH::OrderCancelMessageType: {
            const ITCH::OrderCancelMessage* m = (const ITCH::OrderCancelMessage*)buffer;
            out_word = (bit32_t)m->cancelledShares;
            break;
        }
        default:
            out_word = 0;
            break;
    }
    strm_out.write(out_word);
}
