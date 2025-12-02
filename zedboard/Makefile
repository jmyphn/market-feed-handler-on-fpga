#==========================================================================
# Makefile
#==========================================================================
# @brief: A makefile the compiles and runs the digitrec program
#
# @desc: 1. "make" or "make sw" runs software execution by default
#        2. "make fpga" invokes the HW accelerator
#        3. "make clean" cleans up the directory

INC_PATH=/usr/include/vivado_hls
CFLAGS = -I${INC_PATH} -DHLS_NO_XIL_FPO_LIB -O3 -std=c++11

.PHONY: all sw fpga

all: sw

hft-arm: hft.cpp itch.cpp orderbook.cpp blackscholes.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt

result/hft_arm_result.txt: hft-arm
	@echo "Compiling & executing hft software program on ARM ..."
	mkdir -p result
	./$< | tee $@

sw: result/hft_arm_result.txt
	@echo "Result saved to $@"

#=========================================================================
# FPGA
#=========================================================================
hft-fpga: host.cpp
	@echo "Compiling host program"
	g++ ${CFLAGS} $^ -o $@
	@echo "Make sure bitstream is loaded!"

fpga: hft-fpga
	@echo "Running hft accelerator..."
	./$<

clean:
	@echo "Clean up output files..."
	rm -rf hft hft-fpga vivado_hls.log *.prj out.dat *~
	rm -rf result