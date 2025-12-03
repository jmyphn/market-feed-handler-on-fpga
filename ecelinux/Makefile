#==========================================================================
# Makefile
#==========================================================================
# @brief: A makefile the compiles and synthesizes the feed handler program
#
# @desc: 1. "make" runs csim by default
#        2. "make csim" compiles & executes the software implementation
#        3. "make clean" cleans up the directory

XILINX_VIVADO?=/opt/xilinx/Vivado/2019.2
XIL_HLS=source $(XILINX_VIVADO)/settings64.sh; vivado_hls
VHLS_INC=$(XILINX_VIVADO)/include
# Specify compilation flags
CFLAGS=-g -I${VHLS_INC} -DHLS_NO_XIL_FPO_LIB -std=c++11 -O3

ifeq ($(USE_HLS_MATH),1)
    CFLAGS += -DUSE_HLS_MATH
    LDFLAGS = -L/opt/xilinx/Vivado/2019.2/lnx64/tools/fpo_v7_0 -lhls_fpo
endif

TCL_SCRIPT=run_hft.tcl

.PHONY: all itch_csim bs_csim orderbook_csim bitstream clean

all: itch_csim orderbook_csim bs_csim

itch: itch_test.cpp itch.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt

result/itch_csim.txt: itch
	@echo "Running itch sim..."
	mkdir -p result
	./$< | tee -a $@

itch_csim: result/itch_csim.txt
	@echo "Result recorded to $<"

ob: orderbook.cpp orderbook_tb.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt

result/ob_csim.txt: ob
	@echo "Running orderbook sim..."
	mkdir -p result
	./$< | tee $@

ob_csim: result/ob_csim.txt
	@echo "Result recorded to $<"

bs: blackscholes_test.cpp blackscholes.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt ${LDFLAGS}

result/bs_csim.txt: bs
	@echo "Running Black–Scholes sim..."
	mkdir -p result
	./$< | tee $@

bs_csim: result/bs_csim.txt
	@echo "Result recorded to $<"

hft: hft_test.cpp hft.cpp itch.cpp orderbook.cpp blackscholes.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt ${LDFLAGS}

result/hft_csim.txt: hft
	@echo "Running hft sim..."
	mkdir -p result
	./$< | tee $@

hft_csim: result/hft_csim.txt
	@echo "Result recorded to $<"

xillydemo.bit:
	@echo "================================================================="
	@echo "Synthesizing HFT and creating bitstream with $(TCL_SCRIPT)..."
	@echo "================================================================="
	$(XIL_HLS) -f $(TCL_SCRIPT)
	source $(XILINX_VIVADO)/settings64.sh; ./run_bitstream.sh

bitstream: xillydemo.bit
	@echo "Bitstream saved to $<"

clean:
	rm -rf itch bs orderbook hft *.dat *.prj *.log
	rm -rf zedboard_project* xillydemo.bit
