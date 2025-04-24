SRCS= ../i2c_master.vhd ../si5345_register_pkg.vhd ../si5345_pll_init.vhd ./i2c_slave.vhd ./sim_pkg.vhd ./tb.vhd
#GHDLFLAGS= --std=02 --ieee=synopsys
#GHDLFLAGS= --std=93c --ieee=synopsys -fexplicit
GHDLFLAGS= --std=08 --ieee=synopsys -fexplicit --workdir=. -P/home/kuzew/ghdl/src/xilinx/v93c/unisim
SIMFLAGS= --stop-time=500ms --wave=output.ghw

TOP_LEVEL=tb

GHDL=ghdl

all:
	${GHDL} -a ${GHDLFLAGS} ${SRCS}
	${GHDL} -e ${GHDLFLAGS} ${TOP_LEVEL}
	${GHDL} -r ${GHDLFLAGS} ${TOP_LEVEL} ${SIMFLAGS}

clean:
	rm *.o *.cf ${TOP_LEVEL}
	
