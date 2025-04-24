SI5345_VHD_PKG = $(DAM_GTM_DIR)/hdl/si5345/si5345_register_pkg.vhd
SI5345_H_FILE = $(DAM_GTM_DIR)/config/Si5345-RevD-sPHX_GTM-Registers.h

HDL += $(wildcard $(DAM_GTM_DIR)/hdl/*.vhd)
HDL += $(wildcard $(DAM_GTM_DIR)/hdl/*/*.vhd)
HDL += $(SI5345_VHD_PKG)

IP_DCP += $(patsubst %.xci,%.dcp,$(wildcard $(DAM_GTM_DIR)/ip/*/*.xci))
IP += $(wildcard $(DAM_GTM_DIR)/ip/*/*.xci)
XDC += $(DAM_GTM_DIR)/hdl/si5345_pins.xdc

# Build tool
REGMAP_H2VHD := python3 $(DAM_GTM_DIR)/utils/regmap_h2vhd.py

$(SI5345_VHD_PKG): $(SI5345_H_FILE)
	${REGMAP_H2VHD} $^ $@

CLEAN_FILES += SI5345_VHD_PKG

