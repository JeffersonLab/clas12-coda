SRCS=../led_heartbeat.vhd ./sim_pkg.vhd ./tb.vhd
#GHDLFLAGS= --std=02 --ieee=synopsys
#GHDLFLAGS= --std=93c --ieee=synopsys -fexplicit
GHDLFLAGS= --std=02 --ieee=synopsys -fexplicit --workdir=. -P/home/kuzew/ghdl/src/xilinx/v93c/unisim
SIMFLAGS= --stop-time=1000ms --wave=output.ghw

TOP_LEVEL=tb

GHDL=ghdl

all:
	${GHDL} -a ${GHDLFLAGS} ${SRCS}
	${GHDL} -e ${GHDLFLAGS} ${TOP_LEVEL}
	${GHDL} -r ${GHDLFLAGS} ${TOP_LEVEL} ${SIMFLAGS}

clean:
	rm *.o *.cf ${TOP_LEVEL}
	
