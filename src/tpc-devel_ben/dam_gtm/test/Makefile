DAM_GTM_DIR = ..

FPGA = xcku115-flvf1924-2-e
TOP_MODULE = dam_gtm_flx_test

# Source paths
IP_DCP += $(patsubst %.xci,%.dcp,$(wildcard ./ip/*/*.xci))
IP += $(wildcard ./ip/*/*.xci)
HDL += ./hdl/opencore_gc/gc_frequency_meter.vhd
HDL += ./hdl/opencore_gc/gc_pulse_synchronizer.vhd
HDL += ./hdl/opencore_gc/gc_pulse_synchronizer2.vhd
HDL += ./hdl/opencore_gc/gc_sync_word_wr.vhd
HDL += ./hdl/opencore_gc/gc_edge_detect.vhd
HDL += ./hdl/opencore_gc/gc_sync.vhd
HDL += ./hdl/opencore_gc/gencores_pkg.vhd

DEBUG_PROBES = debug.xdc
HDL += dam_gtm_flx712_top.vhd
XDC += dam_gtm_test_pins.xdc

VIVADO_BUILD = python3 $(DAM_GTM_DIR)/utils/vivado_build.py

include ../dam_gtm.mk
include ../vivado_build.mk
include ../docker.mk

clean:
	rm -f ./bin/*.bit ./bin/*.dcp ./log/* ./${TOP_MODULE}.bit ./*.ltx

cleanall: clean
	rm -rf ./vivado/*
	find ./ip/ -type f -not \( -name "*.xci" -o -name ".gitignore" \) -print0 | xargs -0 rm --
	find ./ip/ -mindepth 2 -maxdepth 2 -type d -print0 |  xargs -r0 rm -R
	find ${DAM_GTM_DIR}/ip/ -type f -not \( -name "*.xci" -o -name ".gitignore" \) -print0 | xargs -0 rm --
	find ${DAM_GTM_DIR}/ip/ -mindepth 2 -maxdepth 2 -type d -print0 |  xargs -r0 rm -R
	
fresh: cleanall ip all
