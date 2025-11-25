#==========================================================================
# Makefile
#==========================================================================
# @brief: Compiles and runs the filter program
#
# @desc: 1. "make" runs the filter program by default
#        2. "make filter MAX=XXX" creates a data file with the 1st XXX messages
#        2. "make filter_per_type LIM=XXX" creates a data file with the 1st XXX messages per type
#        2. "make filter_msg TYPE=XXX" creates a data file with the 1st XXX type message
#        3. "make clean" cleans up the directory

# Run locally! The input file is too large. 
DATE = 07302019
INPUT = $(DATE).NASDAQ_ITCH50.gz

.PHONY: all filter filter_per_type filter_msg clean

all: filter filter_per_type filter_msg

# Default value if not provided
MAX ?= 1'000'000
LIM ?= 2
TYPE ?= A

filter: filter.cpp
	g++ $^ -o $@ -lz

	@echo "Running $@ for $(MAX) messages..."
	./$@ $(INPUT) $(DATE)/filtered_$(MAX) $(MAX) | tee filter_result.txt

	@echo "Result recorded to filter_result.txt"

filter_per_type: filter_per_type.cpp
	g++ $^ -o $@ -lz

	@echo "Running $@ for $(LIM) messages per type..."
	./$@ $(INPUT) $(DATE)/filtered_$(LIM)_per_type $(LIM) | tee filter_result.txt

	@echo "Result recorded to filter_result.txt"

filter_msg: filter_msg.cpp
	g++ $^ -o $@ -lz

	@echo "Running $@ for 1 $(TYPE) message..."
	./$@ $(INPUT) $(DATE)/filtered_$(TYPE) $(TYPE) | tee filter_result.txt

	@echo "Result recorded to filter_result.txt"

clean:
	rm -f filter filter_per_type filter_msg filter_result.txt