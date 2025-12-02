#=============================================================================
# run_orderbook.tcl 
#=============================================================================
# @brief: A Tcl script for synthesizing the orderbook design.

# Project name
set hls_prj orderbook.prj

# Open/reset the project
open_project ${orderbook_prj} -reset

# Top function of the design is "orderbook"
set_top orderbook_dut

# Add design and testbench files
add_files orderbook.cpp -cflags "-std=c++11"
add_files -tb orderbook_tb.cpp -cflags "-std=c++11"

open_solution "solution1"
# Use Zynq device
set_part {xc7z020clg484-1}

# Target clock period is 10ns
create_clock -period 10

### You can insert your own directives here ###

############################################

# Simulate the C++ design
csim_design -O
# Synthesize the design
csynth_design
# Co-simulate the design
#cosim_design
exit