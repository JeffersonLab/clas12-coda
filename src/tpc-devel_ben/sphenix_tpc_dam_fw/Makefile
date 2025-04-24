DAM_WUPPER_DIR = ./submodules/dam_wupper
DAM_GTM_DIR = ./submodules/dam_gtm

FPGA = xcku115-flvf1924-2-e
TOP_MODULE = tpc_dam_top

# Register map to use for design
REGISTER_MAP := ./config/register_map_v1.2.yaml

# Additional user source and IP paths
HDL += $(wildcard hdl/*/*.vhd)
HDL += hdl/tpc_dam712_top.vhd
IP_DCP += $(patsubst %.xci,%.dcp,$(wildcard ip/*/*.xci))
IP += $(wildcard ip/*/*.xci)
DEBUG_PROBES = hdl/debug.xdc
XDC += hdl/dam_custom_pins.xdc

VIVADO_BUILD = vivado_build
VIVADO_BUILD_DIR := $(shell vivado_build --get-directory -f dummy dummy)

include $(DAM_WUPPER_DIR)/dam_wupper.mk
include $(DAM_GTM_DIR)/dam_gtm.mk
include $(VIVADO_BUILD_DIR)/vivado_build.mk

clean:
	rm ${HDL_WUPPER}
	rm -f ./bin/*.bit ./bin/*.dcp ./log/* ./${TOP_MODULE}.bit ./*.ltx

cleanall: clean
	rm -rf ./vivado/*
	find ${DAM_WUPPER_DIR}/ip/ -type f -not \( -name "*.xci" -o -name ".gitignore" \) -print0 | xargs -0 rm --
	find ${DAM_WUPPER_DIR}/ip/ -mindepth 2 -maxdepth 2 -type d -print0 |  xargs -r0 rm -R
	find ./ip/ -type f -not \( -name "*.xci" -o -name ".gitignore" \) -print0 | xargs -0 rm --
	find ./ip/ -mindepth 2 -maxdepth 2 -type d -print0 |  xargs -r0 rm -R

fresh: cleanall ip all
