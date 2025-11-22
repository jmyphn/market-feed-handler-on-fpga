#==========================================================================
# Makefile
#==========================================================================
# @brief: A makefile the compiles and synthesizes the bnn program
#
# @desc: 1. "make" runs csim by default
#        2. "make csim" compiles & executes the software implementation
#        3. "make clean" cleans up the directory


# Extract Vivado HLS include path
XILINX_VIVADO?=/opt/xilinx/Vivado/2019.2
XIL_HLS=source $(XILINX_VIVADO)/settings64.sh; vivado_hls
VHLS_INC=$(XILINX_VIVADO)/include
# Specify compilation flags
CFLAGS=-g -I${VHLS_INC} -DHLS_NO_XIL_FPO_LIB -std=c++11 -O3

TCL_SCRIPT=run.tcl

.PHONY: all csim bitstream clean

all: csim

itch: itch_test.cpp itch.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt

result/itch_csim.txt: itch
	@echo "Running itch sim..."
	mkdir -p result
	./$< | tee $@

csim: result/itch_csim.txt
	@echo "Result recorded to $<"

bs.exe: blackscholes_test.cpp blackscholes.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt

.PHONY: bs
bs: bs.exe
	@echo "Running Black–Scholes sim..."
	mkdir -p result
	./bs.exe | tee result/bs_csim.txt


csim_bs: result/bs_csim.txt
	@echo "Result recorded to $<"

xillydemo.bit:
	@echo "================================================================="
	@echo "Synthesizing itch and creating bitstream with $(TCL_SCRIPT)..."
	@echo "================================================================="
	$(XIL_HLS) -f $(TCL_SCRIPT)
	source $(XILINX_VIVADO)/settings64.sh; ./run_bitstream.sh

bitstream: xillydemo.bit
	@echo "Bitstream saved to $<"

clean:
	rm -rf itch bs *.dat *.prj *.log
	rm -rf zedboard_project* xillydemo.bit
