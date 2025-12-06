#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>

#include <iostream>
#include <fstream>

#include "blackscholes.hpp"
#include "itch.hpp"
#include "orderbook.hpp"
#include "timer.h"

#include "itch_reader.hpp"

static const char* INPUT_ITCH_FILE = "./data/12302019/filtered_500";

//--------------------------------------
// main function
//--------------------------------------
int main(int argc, char **argv) {
  // Open channels to the FPGA board.
  // These channels appear as files to the Linux OS
  int fdr = open("/dev/xillybus_read_32", O_RDONLY);
  int fdw = open("/dev/xillybus_write_32", O_WRONLY);

  // Check that the channels are correctly opened
  if ((fdr < 0) || (fdw < 0)) {
    fprintf(stderr, "Failed to open Xillybus device channels\n");
    exit(-1);
  }

  // Timer
  Timer timer("FPGA Communication");

  // Use ITCH Reader to open and parse a data file
  ITCH::Reader reader(INPUT_ITCH_FILE);
  if (!reader.isOpen()) {
      std::cerr << "Failed to open data file: " << INPUT_ITCH_FILE << std::endl;
      return -1;
  }

  std::cout << "Sending ITCH messages to FPGA..." << std::endl;

  const char* buffer;
  int nbytes;
  int messages_sent = 0;

  timer.start();

  // Loop through all messages in the file
  while ((buffer = reader.nextMessage())) {
      uint16_t message_length = ITCH::Parser::getMessageLength(buffer);
      
      // 1. Send the message length (as a 32-bit integer)
      uint32_t length_to_send = message_length;
      nbytes = write(fdw, (void*)&length_to_send, sizeof(length_to_send));
      assert(nbytes == sizeof(length_to_send));

      // 2. Send the message body, word by word (32-bit)
      // The message from the reader does not include the length field
      for (int i = 0; i < ceil(message_length / 4.0); ++i) {

        uint32_t word = 0;
        int remaining = message_length - i * 4;
        int to_copy   = std::min(4, remaining);

        // Pack bytes so that buffer[2 + i*4] -> bits [31:24], etc.
        for (int b = 0; b < to_copy; ++b) {
            unsigned char byte = static_cast<unsigned char>(buffer[2 + i*4 + b]);
            word |= (uint32_t)byte << (8 * (3 - b));
        }

        // std::cout << "Writing word: 0x" << std::hex << word << std::dec << std::endl;
        nbytes = write(fdw, (void*)&word, sizeof(word));
        assert(nbytes == sizeof(word));
      }
      messages_sent++;
  }

  // std::cout << "All messages sent (" << messages_sent << " total). Waiting for results from FPGA..." << std::endl;

  // Read results from the FPGA
  // Expect one 64-bit result (call + put prices) per message sent
  uint64_t result_data;
  int results_received = 0;
  
  for (int i = 0; i < messages_sent; i++) {
    // std::cout << "Receiving result " << (i+1) << " of " << messages_sent << "..." << std::endl;
      nbytes = read(fdr, (void*)&result_data, sizeof(result_data));
      if (nbytes <= 0) {
          std::cerr << "Error: Expected " << messages_sent << " results but only received " << results_received << std::endl;
          break;
      }
      assert(nbytes == sizeof(result_data));
      
      // Extract call and put prices from the 64-bit result
      uint32_t call_bits = result_data & 0xFFFFFFFF;
      uint32_t put_bits = (result_data >> 32) & 0xFFFFFFFF;
      
      float call_price, put_price;
      memcpy(&call_price, &call_bits, sizeof(float));
      memcpy(&put_price, &put_bits, sizeof(float));
      
      // std::cout << "Message " << (i+1) << ": Call=" << call_price << ", Put=" << put_price << std::endl;
      results_received++;
  }

  timer.stop();

  // Report 
  std::cout << "Finished." << std::endl;
  std::cout << "Sent " << messages_sent << " messages and received " << results_received << " results." << std::endl;

  // Close the channels
  close(fdr);
  close(fdw);

  return 0;
}