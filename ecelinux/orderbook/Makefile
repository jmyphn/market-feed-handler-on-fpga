#==========================================================================
# Makefile
#==========================================================================
# @brief: A makefile the compiles and synthesizes the feed handler program
#
# @desc: 1. "make" runs csim by default
#        2. "make csim" compiles & executes the market data feed handler
#        3. "make clean" cleans up the directory

XILINX_VIVADO?=/opt/xilinx/Vivado/2019.2
XIL_HLS=source $(XILINX_VIVADO)/settings64.sh; vivado_hls
VHLS_INC=$(XILINX_VIVADO)/include
# Specify compilation flags
CFLAGS=-g -I${VHLS_INC}

.PHONY: all csim clean

all: csim

# TODO: need to update
orderbook: priority_queue.cpp tb.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt

result/orderbook_csim.txt: orderbook
	@echo "Running orderbook sim..."
	mkdir -p result
	./$< | tee $@


csim: result/orderbook_csim.txt
	@echo "Result reorded to $<"

clean:
	rm -rf orderbook *.dat *.prj *.log
