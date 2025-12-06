#==========================================================================
# Makefile
#==========================================================================
# @brief: A makefile the compiles and runs the hft program
#
# @desc: 1. "make" or "make sw" runs software execution by default
#        2. "make fpga" invokes the HW accelerator
#        3. "make clean" cleans up the directory

INC_PATH=/usr/include/vivado_hls
CFLAGS = -I${INC_PATH} -DHLS_NO_XIL_FPO_LIB -O3 -std=c++11

ifeq ($(USE_HLS_MATH),1)
    CFLAGS += -DUSE_HLS_MATH
    LDFLAGS = -L/opt/xilinx/Vivado/2019.2/lnx64/tools/fpo_v7_0 -lhls_fpo
endif

.PHONY: all sw fpga

all: sw

hft-arm: hft_test.cpp hft.cpp itch.cpp orderbook.cpp blackscholes.cpp
	g++ ${CFLAGS} $^ -o $@ -lrt ${LDFLAGS}

result/hft_arm_sim.txt: hft-arm
	@echo "Compiling & executing hft software program on ARM ..."
	mkdir -p result
	./$< | tee $@

sw: result/hft_arm_sim.txt
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
	rm -rf hft-arm vivado_hls.log *.prj result out.dat *~
	rm -rf hft-fpga
	