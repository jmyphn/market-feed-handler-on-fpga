#==========================================================================
# Makefile
#==========================================================================
# @brief: Compiles and runs the filter program
#
# @desc: 1. "make" runs the filter program by default
#        2. "make data" compiles & executes the filter program
#        3. "make clean" cleans up the directory

# Run locally! The input file is too large. 
INPUT = 12302019.NASDAQ_ITCH50.gz
OUTPUT = filtered_data

.PHONY: all filter data clean

all: data

filter: filter.cpp
	g++ $^ -o $@ -lz

filter_result.txt: filter
	@echo "Running filter..."
	./$<  $(INPUT) $(OUTPUT) | tee $@

data: filter_result.txt
	@echo "Result recorded to $<"

clean:
	rm -f filter filter_result.txt