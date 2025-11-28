#==========================================================================
# Makefile for Zedboard HFT Project
#==========================================================================
# @brief: A makefile that compiles for software simulation and cross-compiles
#         the host program for the Zedboard ARM processor.
#
# @desc: 1. "make" or "make sw" runs the software simulation on this machine.
#        2. "make fpga" cross-compiles the host program for the Zedboard.
#        3. "make run-fpga" is a placeholder to run the executable on the Zedboard.
#        4. "make clean" cleans up the directory.

# --- Compilers ---
# Define the ARM cross-compiler for the Zedboard
CXX_ARM=arm-linux-gnueabihf-g++
# Define the standard g++ compiler for local software simulation
CXX_SW=g++

# --- Flags and Paths ---
# Add Vivado HLS include path (needed for cross-compilation on dev machine)
XILINX_VIVADO?=/opt/xilinx/Vivado/2019.2
VHLS_INC=$(XILINX_VIVADO)/include
# Note: -I. is added to find the symbolically linked headers in this directory
CFLAGS=-g -I. -I${VHLS_INC} -DHLS_NO_XIL_FPO_LIB -std=c++11 -O3

# --- Source Files ---
# Source files for the full software simulation
SW_SRCS = hft_test.cpp hft.cpp itch.cpp orderbook.cpp priority_queue.cpp hash_tbl.cpp blackscholes.cpp
# Source files for the FPGA host program
FPGA_SRCS = host.cpp itch.cpp orderbook.cpp priority_queue.cpp hash_tbl.cpp blackscholes.cpp

# --- Executable Names ---
EXE_SW=hft-sw
EXE_ARM=hft-fpga

.PHONY: all sw fpga run-fpga clean

# Default target
all: sw

# --- Software Simulation Target ---
$(EXE_SW): $(SW_SRCS)
	@echo "Compiling HFT software for local simulation..."
	$(CXX_SW) $(CFLAGS) $^ -o $@ -lrt

result/sw_sim.txt: $(EXE_SW)
	@echo "Running HFT software simulation..."
	mkdir -p result
	./$< | tee $@

sw: result/sw_sim.txt
	@echo "Software simulation complete. Result recorded to $@"

# --- FPGA Host Compilation & Execution Targets ---
$(EXE_ARM): $(FPGA_SRCS)
	@echo "Cross-compiling host program for Zedboard (ARM)..."
	$(CXX_ARM) $(CFLAGS) $^ -o $@

fpga: $(EXE_ARM)
	@echo "Host program '$(EXE_ARM)' compiled for Zedboard."
	@echo "Copy it to your Zedboard and run it there."
	@echo "Make sure the bitstream is loaded first!"

run-fpga:
	@echo "This target should be run on the Zedboard itself."
	@echo "Running HFT accelerator..."
	./$(EXE_ARM)

# --- Cleanup Target ---
clean:
	@echo "Cleaning up output files..."
	rm -rf $(EXE_SW) $(EXE_ARM) result vivado_hls.log *.log
	# Note: This does not remove the symbolic links