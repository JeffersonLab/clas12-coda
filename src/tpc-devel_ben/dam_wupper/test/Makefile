DAM_WUPPER_DIR = ..

FPGA = xcku115-flvf1924-2-e
TOP_MODULE = felix_top_bnl711

# Register map to use for design
REGISTER_MAP := $(DAM_WUPPER_DIR)/config/register_map_v1.2.yaml

# Source paths
DEBUG_PROBES = debug.xdc
HDL += dam_flx712_top.vhd

VIVADO_BUILD = python3 $(DAM_WUPPER_DIR)/utils/vivado_build.py

include ../dam_wupper.mk
include ../vivado_build.mk

clean:
	rm ${HDL_WUPPER}
	rm -f ./bin/*.bit ./bin/*.dcp ./log/* ./${TOP_MODULE}.bit ./*.ltx

cleanall: clean
	rm -rf ./vivado/*
	find ${DAM_WUPPER_DIR}/ip/ -type f -not \( -name "*.xci" -o -name ".gitignore" \) -print0 | xargs -0 rm --
	find ${DAM_WUPPER_DIR}/ip/ -mindepth 2 -maxdepth 2 -type d -print0 |  xargs -r0 rm -R

fresh: cleanall ip all
