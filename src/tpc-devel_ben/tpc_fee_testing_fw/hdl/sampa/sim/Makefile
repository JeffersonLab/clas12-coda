SRCS= ./fifo_11b_4096_sim_netlist.vhdl ../data_align.vhd ./sampa_datagen.vhd ./sim_pkg.vhd ./tb.vhd
GHDLFLAGS= --std=08 --ieee=synopsys -fexplicit -frelaxed --workdir=. -P/home/kuzew/ghdl/xilnx/xilinx-vivado/unisim/v08
SIMFLAGS= --stop-time=25us --wave=output.ghw

TOP_LEVEL=tb

GHDL=ghdl

all:
	${GHDL} -a ${GHDLFLAGS} ${SRCS}
	${GHDL} -e ${GHDLFLAGS} ${TOP_LEVEL}
	${GHDL} -r ${GHDLFLAGS} ${TOP_LEVEL} ${SIMFLAGS}

clean:
	rm *.o *.cf ${TOP_LEVEL}
	
