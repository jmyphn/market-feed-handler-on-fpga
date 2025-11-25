//==========================================================================
// hft.cpp
//==========================================================================
// @brief: A HFT model for market feed data handling & Black-Scholes option pricing

#include "hft.hpp"
#include "itch_common.hpp"
#include "hash_tbl.hpp"
#include "priority_queue.hpp"
#include "typedefs.h"

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
    // Call Orderbook
    // ------------------------------------------------------
    bit32_t spot_price = orderbook(parsed);

    // ------------------------------------------------------
    // Call Black-Scholes
    // ------------------------------------------------------
    union {
        float fval;
        int   ival;
    } u_in;

    u_in.ival = spot_price;
    theta_type S_in = u_in.fval;

    // Compute Black–Scholes price
    result_type result;
    black_scholes_price(S_in, result);

    // ------------------------------------------------------
    // Output processing
    // ------------------------------------------------------

    // Convert results back to 32-bit words
    union { float fval; int ival; } ucall;
    union { float fval; int ival; } uput;

    ucall.fval = result.call;
    uput.fval  = result.put;

    bit32_t icall = static_cast<bit32_t>(ucall.ival);
    bit32_t iput  = static_cast<bit32_t>(uput.ival);

    // Write output to stream (call, put)
    strm_out.write(icall);
    strm_out.write(iput);
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

//----------------------------------------------------------
// Orderbook
//----------------------------------------------------------
/*
 * If pq is at max size, remove an arbitrary element that's not the biggest.
 * Also deletes its corresponding element from the hashtablej
 * POST: pq is not at full capacity
 */
void keep_slim(priority_queue &pq, hash_tbl tbl) {
#pragma hls inline
  if (pq.size == CAPACITY) {
    --pq.size;
    std::cerr << "Keeping slim" << std::endl;
    ParsedMessage &order = pq.heap[pq.size];
    hash_tbl_remove(tbl, order.order_id);
  }
}

void balance(priority_queue &pq, hash_tbl tbl) {
  while (pq.size > 0) {
    ParsedMessage top_order = pq_top(pq);
    hash_entry *top_entry = hash_tbl_lookup(tbl, top_order.order_id);
#if ASSERT
    assert(top_entry != nullptr);
#endif
    if (top_entry->value >= 0) {
      break;
    } else {
      // remove from hash table
      top_entry->value = 0;
      top_entry->state = TOMBSTONE;
      pq_pop(pq);
    }
  }
}

void remove_shares(priority_queue &pq, hash_tbl tbl, key_type order_id,
                   val_type shares) {
  hash_entry *curr_entry = hash_tbl_lookup(tbl, order_id);
#if ASSERT
  assert(curr_entry != nullptr);
#endif
  if (curr_entry->value > shares) {
    curr_entry->value -= shares;
  } else {
    curr_entry->value = 0;
    balance(pq, tbl);
  }
}

bit32_t orderbook(ParsedMessage &order) {
  static priority_queue bid_pq;
  static hash_tbl bid_shares;
  static priority_queue ask_pq;
  static hash_tbl ask_shares;

// #if ASSERT
//   assert(!orders.empty());
// #endif

//   ParsedMessage order = orders.read();
  priority_queue &curr_pq = (order.side == 'b') ? bid_pq : ask_pq;
  hash_tbl &curr_shares = ((order.side) == 'b') ? bid_shares : ask_shares;

  // Process order depending on type
  switch (order.type) {
  case 'A': // Add Order Message
    keep_slim(curr_pq, curr_shares);
    pq_push(curr_pq, order);
    hash_tbl_put(curr_shares, order.order_id, order.shares);
    break;

  case 'E': // Order Executed Message
    remove_shares(curr_pq, curr_shares, order.order_id, order.shares);

    // TODO: rest of the cases

  default:
#if ASSERT
    // message type is probably not implemented
    assert(false);
#endif
  }

  // Output a spot price
  if (bid_pq.size == 0 && ask_pq.size == 0)
    return 0xdeadbeef;
  else if (bid_pq.size == 0)
    return pq_top(ask_pq).price; 
  else if (ask_pq.size == 0)
    return pq_top(bid_pq).price;
  else
    return (pq_top(ask_pq).price + pq_top(bid_pq).price) << 1;
}

//----------------------------------------------------------
// Black-Scholes
//----------------------------------------------------------
theta_type K = 100.0f; // Strike price
theta_type r = 0.05f;  // Risk-free rate 
theta_type v = 0.2f;   // Volatility of the underlying 
theta_type T = 1.0f;   // One year until expiry

template <typename T>
T custom_log(const T& x)
{
  if (x <= 0) {
    std::cerr << "Error: Input must be greater than 0" << std::endl;
    return -1.0; 
  }

  const int logTerms = 10;

  T result = 0.0;
  T term = (x - 1) / (x + 1);
  T term_squared = term * term;
  T numerator = term;
  T denominator = 1;

  for (int i = 1; i <= logTerms; i++) {
    result += numerator / denominator;
    numerator *= term_squared;
    denominator += 2;
  }

  return 2 * result;
}

template <typename T>
T custom_exp(const T& x)
{
  T result = 1.0;
  T term = 1.0;
  const int expTerms = 10;

  for (int i = 1; i <= expTerms; i++) {
    term *= x / (T)i;
    result += term;
  }

  return result;
}


static theta_type normal_cdf(theta_type x)
{
  const theta_type inv_sqrt2 = 0.7071067811865475f; // 1/sqrt(2)
  return 0.5f * (1.0f + std::erf(x * inv_sqrt2));
}

// ---------------------------------------------------------------------
// Black–Scholes pricing 
// ---------------------------------------------------------------------
void black_scholes_price(theta_type S_in, result_type &result){
  if (S_in <= 0 || K <= 0 || v <= 0 || T <= 0) {
    result.call = 0.0f;
    result.put  = 0.0f;
    return;
  }

  theta_type sigma   = v;
  theta_type sqrtT   = std::sqrt(T); 
  theta_type S_over_K = S_in / K;

  theta_type log_S_over_K = custom_log<theta_type>(S_over_K);
  theta_type sigma_sq     = sigma * sigma;

  theta_type numerator   = log_S_over_K + (r + 0.5f * sigma_sq) * T;
  theta_type denominator = sigma * sqrtT;

  theta_type d1 = numerator / denominator;
  theta_type d2 = d1 - sigma * sqrtT;

  theta_type Nd1       = normal_cdf(d1);
  theta_type Nd2       = normal_cdf(d2);
  theta_type Nminus_d1 = normal_cdf(-d1);
  theta_type Nminus_d2 = normal_cdf(-d2);

  theta_type discount = custom_exp<theta_type>(-r * T);

  result.call = S_in * Nd1 - K * discount * Nd2;
  result.put  = K * discount * Nminus_d2 - S_in * Nminus_d1;
}