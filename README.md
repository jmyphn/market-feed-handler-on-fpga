# Market Feed Handler on FPGA
This project is an implementation of a market feed handler for a particular ticker, implemented in C++ and synthesized using Vivado HLS.

# Authors
Tean Lai, Amy Le, Grace Lo, Jimmy Phan

## To run this design on the ARM simulator
1. Navigate to the ecelinux folder within this repository
2. Run `vivado_hls -f run_hft.tcl`

## To run this design on the zedboard
1. Run `vivado_hls -f run_hft.tcl` to generate Verilog for this module 
2. Run `source run_bitstream.sh` to generate the bitstream that will be used to reconfigure the FPGA 
3. Scp the generated `xillydemo.bit` to the zedboard using `scp xillydemo.bit <user>@zhang-zedboard-xx.ece.cornell.edu:~`
4. Login to the zedboard - `ssh <user>@zhang-zedboard-xx.ece.cornell.edu`
5. Run `mount /mnt/sd` to mount the SD card
6. Run `cp xillydemo.bit /mnt/sd`
7. Run `sudo reboot` so these changes take effect
8. Scp the whole market-feed-handler-on-fpga folder to the zedboard that was just reprogrammed using `scp /path-to-zip/market-feed-handler-on-fpga.zip <user>@zhang-zedboard-xx.ece.cornell.edu:`
9. After unzipping the folder, navigate to the zedboard folder and run `make fpga` 