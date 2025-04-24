.PHONY: all ip project synth clean cleanall fresh

all: ${IP_DCP} ${TOP_MODULE}.bit

ip: ${IP_DCP}
%.dcp: %.xci
	${VIVADO_BUILD} ip --ip $^ --fpga ${FPGA}

${TOP_MODULE}.mcs : ${TOP_MODULE}.bit
	vivado -mode batch -nojournal -log ./log/vivado.log -notrace -source ./utils/make_mcs.tcl -tclargs $^ $@ 
	
${TOP_MODULE}.bit: ${HDL}
	#ghdl -s ${GHDLFLAGS} ${HDL}
	${VIVADO_BUILD} all --hdl $^ --postsynth ${DEBUG_PROBES} \
	--xdc ${XDC} --ip ${IP} --top ${TOP_MODULE} --fpga ${FPGA} -o $@

synth: ${HDL}
	${VIVADO_BUILD} synth --hdl $^ --xdc ${XDC} --postsynth ${DEBUG_PROBES} \
	--ip ${IP} --top ${TOP_MODULE} --fpga ${FPGA}

continue:
	${VIVADO_BUILD} continue --postsynth $(DEBUG_PROBES) --top ${TOP_MODULE} --fpga ${FPGA} -o ${TOP_MODULE}.bit

project: ${HDL}
	${VIVADO_BUILD} project --hdl $^ --xdc ${XDC} --ip ${IP} --top ${TOP_MODULE} --fpga ${FPGA}
	
bitstream:
	${VIVADO_BUILD} bitstream  --xdc ${XDC} --postsynth ${DEBUG_PROBES} --fpga ${FPGA}
