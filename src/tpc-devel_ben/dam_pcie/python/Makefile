FIRMWARE_DIR = ../../sphenix_tpc_dam_fw
DAM_WUPPER_DIR = $(FIRMWARE_DIR)/submodules/dam_wupper

# Register map to use for design
REGISTER_MAP := $(FIRMWARE_DIR)/config/register_map_v1.2.yaml

# Python class
PY_FILE = dam.py

# Build tool
WUPPERCODEGEN := python3 $(DAM_WUPPER_DIR)/utils/wuppercodegen/wuppercodegen/cli.py

%.py: $(REGISTER_MAP) %.py.template
	${WUPPERCODEGEN} $^ $@

all: $(PY_FILE)

clean:
	rm -f $(PY_FILE)
