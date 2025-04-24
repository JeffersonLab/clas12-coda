# xc7a200tfbg484-2
FPGA_DEVICE  = xc7a200t
FPGA_PACKAGE = fbg484-2
FPGA         := ${FPGA_DEVICE}${FPGA_PACKAGE}

TOP_MODULE   = sphx_fee_top
HDL          = ./hdl/tpc_fee_top.vhd
HDL          += ./hdl/misc/test_pattern.vhd
HDL          += ./hdl/misc/led_heartbeat.vhd
HDL          += ./hdl/misc/sampa_reset_controller.vhd
HDL          += ./hdl/misc/reset_manager.vhd
HDL          += ./hdl/misc/xadc_drp.vhd
HDL          += ./hdl/misc/delay_counter.vhd
HDL          += ./hdl/misc/led_activity.vhd
HDL          += ./hdl/sampa/sipo.vhd
HDL          += ./hdl/sampa/data_align.vhd
HDL          += ./hdl/sampa/decoder.vhd
HDL          += ./hdl/i2c/i2c_large_address_master.vhd
include ./hdl/phy/Makefile
include ./hdl/ds620/Makefile
include ./hdl/ltc2991/Makefile
include ./hdl/i2c/Makefile
include ./hdl/opencore_gc/Makefile

IP           = ./ip/vio_0/vio_0.xci 
IP           += ./ip/sys_pll/sys_pll.xci
IP           += ./ip/gtwizard_0/gtwizard_0.xci
IP           += ./ip/fifo_16b_1024/fifo_16b_1024.xci
IP           += ./ip/fifo_17b_1024/fifo_17b_1024.xci
IP           += ./ip/fifo_11b_4096/fifo_11b_4096.xci
IP           += ./ip/xadc_0/xadc_0.xci

XDC          = ./hdl/pins.xdc 
DEBUG_PROBES = ./hdl/debug.xdc 
VIVADO_BUILD = python3 utils/vivado_build.py
#GHDLFLAGS= --std=08 --ieee=synopsys -fexplicit --workdir=./hdl -P/home/kuzew/ghdl/xilnx/xilinx-vivado

all: ${TOP_MODULE}.bit
.PHONY: all ip project synth clean cleanall fresh

${TOP_MODULE}.mcs : ${TOP_MODULE}.bit
	vivado -mode batch -nojournal -log ./log/vivado.log -notrace -source ./utils/make_mcs.tcl -tclargs $^ $@ 

${TOP_MODULE}.bit: ${HDL}
	#ghdl -s ${GHDLFLAGS} ${HDL}
	${VIVADO_BUILD} all --hdl $^ --postsynth ${DEBUG_PROBES} \
	--xdc ${XDC} --ip ${IP} --top ${TOP_MODULE} --fpga ${FPGA} -o $@

ip: ${IP}
	${VIVADO_BUILD} ip --ip $^ --fpga ${FPGA}

synth: ${HDL}
	${VIVADO_BUILD} synth --hdl $^ --xdc ${XDC} --postsynth ${DEBUG_PROBES} \
	--ip ${IP} --top ${TOP_MODULE} --fpga ${FPGA}

continue:
	${VIVADO_BUILD} continue --postsynth $(DEBUG_PROBES) --top ${TOP_MODULE} --fpga ${FPGA} -o ${TOP_MODULE}.bit

project: ${HDL}
	${VIVADO_BUILD} project --hdl $^ --xdc ${XDC} --ip ${IP} --top ${TOP_MODULE} --fpga ${FPGA}
	
bitstream:
	${VIVADO_BUILD} bitstream  --xdc ${XDC} --postsynth ${DEBUG_PROBES} --fpga ${FPGA}

clean:
	rm -f ./bin/*.bit ./bin/*.dcp ./log/* ./${TOP_MODULE}.bit ./*.ltx ./build.tcl

cleanall: clean
	rm -rf ./vivado/*
	find ./ip/ -type f -not \( -name "*.xci" -o -name ".gitignore" \) -print0 | xargs -0 rm --
	find ./ip/ -mindepth 2 -maxdepth 2 -type d -print0 |  xargs -r0 rm -R

fresh: cleanall ip all

