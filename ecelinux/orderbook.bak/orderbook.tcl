open_project orderbook_hls -reset
set_top orderbook
add_files orderbook_src/priority_queue.cpp
add_files orderbook_src/priority_queue.hpp
add_files orderbook_src/orderbook.cpp
add_files orderbook_src/orderbook.hpp
add_files -tb orderbook_tb.cpp
open_solution "solution1"
set_part {xcku115-flva1517-2-e} -tool vivado
create_clock -period 5 -name default
csynth_design
export_design -format ip_catalog
exit
