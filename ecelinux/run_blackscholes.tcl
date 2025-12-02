#=============================================================================
# run_blackscholes.tcl 
#=============================================================================
# @brief: A Tcl script for synthesizing the black-scholes design.

# Project name
set hls_prj blackscholes.prj

# Open/reset the project
open_project ${hls_prj} -reset

# Top function of the design is "dut"
set_top bs_dut

# Add design and testbench files
add_files blackscholes.cpp -cflags "-std=c++11"
add_files -tb blackscholes_test.cpp -cflags "-std=c++11"
add_files -tb data

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
# cosim_design -rtl verilog -trace_level all -tool xsim -O
exit
