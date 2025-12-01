set hls_prj orderbook.proj
open_project ${hls_prj} -reset
set_top orderbook

add_files priority_queue.cpp -cflags "-std=c++11"
add_files hash_tbl.cpp -cflags "-std=c++11"
add_files orderbook.cpp -cflags "-std=c++11"
add_files -tb orderbook_tb.cpp -cflags "-std=c++11"

open_solution "solution1"
set_part {xcku115-flva1517-2-e} -tool vivado
create_clock -period 10
csynth_design
export_design -format ip_catalog
exit
