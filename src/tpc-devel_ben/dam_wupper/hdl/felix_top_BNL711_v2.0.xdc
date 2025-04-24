###############################################################################
# User Configuration
# Link Width   - x16
# Link Speed   - gen3
# Family       - KintexUltrascale
# Part         -
# Package      -
# Speed grade  - -2
# PCIe Block   -
###############################################################################
#
###############################################################################
# User Constraints
###############################################################################

###############################################################################
# User Time Names / User Time Groups / Time Specs
###############################################################################

###############################################################################
# User Physical Constraints
###############################################################################

#! file TEST.XDC
#! net constraints for TEST design

set_property IOSTANDARD LVCMOS25 [get_ports emcclk]
set_property PACKAGE_PIN AK26 [get_ports emcclk]

#unused pin on bank 66
#set_property IOSTANDARD LVCMOS18 [get_ports emcclk_out]
#set_property PACKAGE_PIN BB22 [get_ports emcclk_out]


#set_property BITSTREAM.CONFIG.BPI_SYNC_MODE Type1 [current_design]
set_property BITSTREAM.CONFIG.BPI_SYNC_MODE disable [current_design]
#set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN div-1 [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]

set_property CONFIG_MODE BPI16 [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 6 [current_design]
#set_property BITSTREAM.CONFIG.CONFIGRATE 9 [current_design]
set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN DISABLE [current_design]
#set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN DIV-6 [current_design]
#set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN DIV-8 [current_design]


##System Reset, User Reset, User Link Up, User Clk Heartbeat
set_property PACKAGE_PIN H11 [get_ports {leds[0]}]
set_property PACKAGE_PIN H10 [get_ports {leds[1]}]
set_property PACKAGE_PIN T10 [get_ports {leds[2]}]
set_property PACKAGE_PIN U15 [get_ports {leds[3]}]
set_property PACKAGE_PIN U14 [get_ports {leds[4]}]
set_property PACKAGE_PIN V12 [get_ports {leds[5]}]
set_property PACKAGE_PIN R12 [get_ports {leds[6]}]
set_property PACKAGE_PIN R10 [get_ports {leds[7]}]
##
set_property IOSTANDARD LVCMOS33 [get_ports {leds[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {leds[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {leds[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {leds[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {leds[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {leds[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {leds[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {leds[7]}]

################################################################################
# End User Constraints
################################################################################
#
#
#
#################################################################################
# PCIE Core Constraints
#################################################################################

#
# SYS reset (input) signal.  The sys_reset_n signal should be
# obtained from the PCI Express interface if possible.  For
# slot based form factors, a system reset signal is usually
# present on the connector.  For cable based form factors, a
# system reset signal may not be available.  In this case, the
# system reset signal must be generated locally by some form of
# supervisory circuit.  You may change the IOSTANDARD and LOC
# to suit your requirements and VCCO voltage banking rules.
# Some 7 series devices do not have 3.3 V I/Os available.
# Therefore the appropriate level shift is required to operate
# with these devices that contain only 1.8 V banks.
#

set_property PACKAGE_PIN J10 [get_ports sys_reset_n]
set_property IOSTANDARD LVCMOS33 [get_ports sys_reset_n]
set_property PULLUP true [get_ports sys_reset_n]

#set_property PACKAGE_PIN AK8 [get_ports sys_clk0_p]



###############################################################################
# Timing Constraints, specific to BNL711 design. Other timing constraints are in timing_constraints.xdc
###############################################################################
create_clock -period 10.000 -name sys_clk0 [get_pins pcie0/ep0/g_NoSim.g_ultrascale.refclk_buff/O]
create_clock -period 10.000 -name sys_clk1 [get_pins pcie1/ep0/g_NoSim.g_ultrascale.refclk_buff/O]
create_clock -period 20.000 -name sys_clkdiv2_0 [get_pins pcie0/ep0/g_NoSim.g_ultrascale.refclk_buff/ODIV2]
create_clock -period 20.000 -name sys_clkdiv2_1 [get_pins pcie1/ep0/g_NoSim.g_ultrascale.refclk_buff/ODIV2]
create_clock -period 2500 -name clk400 [get_pins pex_init0/bufg_i2c/O]
create_clock -period 20.000 -name emcclk [get_ports emcclk]
create_generated_clock -name clk_250mhz_x0y1 [get_pins pcie1/ep0/g_NoSim.g_ultrascale.g_devid_7039.u1/U0/gt_top_i/phy_clk_i/bufg_gt_pclk/O]

###############################################################################
# Timing
###############################################################################


create_clock -period 6.25 -name clk_adn_160 [get_pins u2/ibuf_ttc_clk/DIFFINBUF_INST/O]
set_false_path -from [get_ports sys_reset_n]
#set_false_path -reset_path -from [get_pins pcie0/ep0/g_NoSim.g_ultrascale.u1/inst/gt_top_i/pipe_wrapper_i/pipe_reset_i/cpllreset_reg/C]
#set_false_path -reset_path -from [get_pins pcie1/ep0/g_NoSim.g_ultrascale.u1/inst/gt_top_i/pipe_wrapper_i/pipe_reset_i/cpllreset_reg/C]
#set_false_path -from [get_pins pcie0/dma0/u1/flush_fifo_reg/C]
#set_false_path -from [get_pins pcie1/dma0/u1/reset_global_soft_40_s_reg/C]

#taken /Projects/felix_top_ultrascale/felix_top_ultrascale.srcs/sources_1/ip/gtwizard_ultrascale_single_channel_cpll/synth/gtwizard_ultrascale_single_channel_cpll.xdc, which we disabled
set_false_path -to [get_cells -hierarchical -filter {NAME =~ *bit_synchronizer*inst/i_in_meta_reg}]
set_false_path -to [get_cells -hierarchical -filter {NAME =~ *reset_synchronizer*inst/rst_in_*_reg}]
###############################################################################
# End
###############################################################################

## MiniPOD enable input
set_property PACKAGE_PIN G14 [get_ports {opto_inhibit[0]}]
set_property PACKAGE_PIN H14 [get_ports {opto_inhibit[2]}]
set_property PACKAGE_PIN K10 [get_ports {opto_inhibit[1]}]
set_property PACKAGE_PIN K11 [get_ports {opto_inhibit[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {opto_inhibit[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {opto_inhibit[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {opto_inhibit[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {opto_inhibit[3]}]


#set_property LOC PCIE_3_1_X0Y3 [get_cells pcie1/ep0/g_NoSim.g_ultrascale.g_devid_7039.u1/U0/pcie3_uscale_top_inst/pcie3_uscale_wrapper_inst/PCIE_3_1_inst]
#set_property LOC PCIE_3_1_X0Y1 [get_cells pcie1/ep0/g_NoSim.g_ultrascale.g_devid_7038.u1/U0/pcie3_uscale_top_inst/pcie3_uscale_wrapper_inst/PCIE_3_1_inst]


#constraints for Bank 129/130
set_property PACKAGE_PIN V8 [get_ports sys_clk1_p]

#set_property LOC GTHE3_CHANNEL_X1Y20 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[29].*gen_gthe3_channel_inst[0].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y21 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[29].*gen_gthe3_channel_inst[1].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y22 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[29].*gen_gthe3_channel_inst[2].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y23 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[29].*gen_gthe3_channel_inst[3].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y24 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[30].*gen_gthe3_channel_inst[0].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y25 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[30].*gen_gthe3_channel_inst[1].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y26 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[30].*gen_gthe3_channel_inst[2].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y27 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[30].*gen_gthe3_channel_inst[3].GTHE3_CHANNEL_PRIM_INST}]

## PCIe system clock inputs
set_property PACKAGE_PIN AK8 [get_ports sys_clk0_p]
## These loc constraints also set the PCIe transceiver pins
#set_property LOC GTHE3_CHANNEL_X1Y8 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[26].*gen_gthe3_channel_inst[0].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y9 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[26].*gen_gthe3_channel_inst[1].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y10 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[26].*gen_gthe3_channel_inst[2].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y11 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[26].*gen_gthe3_channel_inst[3].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y12 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[27].*gen_gthe3_channel_inst[0].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y13 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[27].*gen_gthe3_channel_inst[1].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y14 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[27].*gen_gthe3_channel_inst[2].GTHE3_CHANNEL_PRIM_INST}]
#set_property LOC GTHE3_CHANNEL_X1Y15 [get_cells -hierarchical -filter {NAME =~ *gen_channel_container[27].*gen_gthe3_channel_inst[3].GTHE3_CHANNEL_PRIM_INST}]

## I2C interface for the jitter cleaner
set_property PACKAGE_PIN V13 [get_ports SCL]
set_property PACKAGE_PIN V14 [get_ports SDA]
set_property IOSTANDARD LVCMOS33 [get_ports SCL]
set_property IOSTANDARD LVCMOS33 [get_ports SDA]

# I2C switch reset
set_property PACKAGE_PIN W11 [get_ports I2C_nRESET]
set_property IOSTANDARD LVCMOS33 [get_ports I2C_nRESET]

set_property PACKAGE_PIN W10 [get_ports I2C_nRESET_PCIe]
set_property IOSTANDARD LVCMOS33 [get_ports I2C_nRESET_PCIe]

## 200 MHz crystal clock input
#set_property IOSTANDARD LVDS [get_ports SYSCLK_P]
#set_property IOSTANDARD LVDS [get_ports SYSCLK_N]
#set_property DIFF_TERM_ADV TERM_100 [get_ports SYSCLK_P]
#set_property DIFF_TERM_ADV TERM_100 [get_ports SYSCLK_N]
#set_property PACKAGE_PIN R31 [get_ports SYSCLK_P]

set_property IOSTANDARD LVDS [get_ports app_clk_in_p]
set_property IOSTANDARD LVDS [get_ports app_clk_in_n]
set_property DIFF_TERM_ADV TERM_100 [get_ports app_clk_in_p]
set_property DIFF_TERM_ADV TERM_100 [get_ports app_clk_in_n]
set_property PACKAGE_PIN AT18 [get_ports app_clk_in_p]

## ADN TTC inputs (Data and Clock)
set_property PACKAGE_PIN H26 [get_ports DATA_TTC_P]
set_property PACKAGE_PIN G25 [get_ports CLK_TTC_P]
set_property IOSTANDARD LVDS [get_ports DATA_TTC_P]
set_property IOSTANDARD LVDS [get_ports DATA_TTC_N]
set_property IOSTANDARD LVDS [get_ports CLK_TTC_P]
set_property IOSTANDARD LVDS [get_ports CLK_TTC_N]
set_property IOSTANDARD LVCMOS33 [get_ports LOL_ADN]
set_property PACKAGE_PIN L12 [get_ports LOL_ADN]
set_property IOSTANDARD LVCMOS33 [get_ports LOS_ADN]
set_property PACKAGE_PIN M11 [get_ports LOS_ADN]

## BUSY Out LEMO connector
set_property PACKAGE_PIN J11 [get_ports BUSY_OUT]
set_property IOSTANDARD LVCMOS33 [get_ports BUSY_OUT]

## These input buffers have to be declared but are unconnected in the design
set_property PACKAGE_PIN AN27 [get_ports Perstn1_open]
set_property IOSTANDARD LVCMOS25 [get_ports Perstn1_open]
set_property PACKAGE_PIN AV28 [get_ports Perstn2_open]
set_property IOSTANDARD LVCMOS25 [get_ports Perstn2_open]

## Ports to configure the PEX switch
set_property PACKAGE_PIN AL12 [get_ports I2C_SMB]
set_property IOSTANDARD LVCMOS18 [get_ports I2C_SMB]
set_property PACKAGE_PIN AN22 [get_ports I2C_SMBUS_CFG_nEN]
set_property IOSTANDARD LVCMOS18 [get_ports I2C_SMBUS_CFG_nEN]
set_property PACKAGE_PIN AJ10 [get_ports MGMT_PORT_EN]
set_property IOSTANDARD LVCMOS18 [get_ports MGMT_PORT_EN]
set_property PACKAGE_PIN AK10 [get_ports SHPC_INT]
set_property IOSTANDARD LVCMOS18 [get_ports SHPC_INT]
set_property PACKAGE_PIN AH12 [get_ports PEX_PERSTn]
set_property IOSTANDARD LVCMOS18 [get_ports PEX_PERSTn]
set_property PACKAGE_PIN AH11 [get_ports PCIE_PERSTn1]
set_property IOSTANDARD LVCMOS18 [get_ports PCIE_PERSTn1]
set_property PACKAGE_PIN AG11 [get_ports PCIE_PERSTn2]
set_property IOSTANDARD LVCMOS18 [get_ports PCIE_PERSTn2]
set_property PACKAGE_PIN AT12 [get_ports PEX_SDA]
set_property IOSTANDARD LVCMOS18 [get_ports PEX_SDA]
set_property PACKAGE_PIN AU12 [get_ports PEX_SCL]
set_property IOSTANDARD LVCMOS18 [get_ports PEX_SCL]

#Test points
set_property PACKAGE_PIN AL14 [get_ports TP1_P]
set_property IOSTANDARD LVDS [get_ports TP1_P]
set_property IOSTANDARD LVDS [get_ports TP1_N]

set_property PACKAGE_PIN K22     [get_ports TP2_P]
set_property IOSTANDARD LVDS [get_ports TP2_P]
set_property IOSTANDARD LVDS [get_ports TP2_N]

#The ports below are unused in the design, but can be added to chipscope for debugging purposes.
set_property PACKAGE_PIN AU10 [get_ports {PORT_GOOD[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[0]}]
set_property PACKAGE_PIN AL14 [get_ports {PORT_GOOD[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[1]}]
set_property PACKAGE_PIN AK13 [get_ports {PORT_GOOD[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[2]}]
set_property PACKAGE_PIN AM14 [get_ports {PORT_GOOD[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[3]}]
set_property PACKAGE_PIN AL13 [get_ports {PORT_GOOD[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[4]}]
set_property PACKAGE_PIN AP14 [get_ports {PORT_GOOD[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[5]}]
set_property PACKAGE_PIN AN14 [get_ports {PORT_GOOD[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[6]}]
set_property PACKAGE_PIN AL15 [get_ports {PORT_GOOD[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {PORT_GOOD[7]}]

## LMK03200
set_property PACKAGE_PIN AU22 [get_ports CLK40_FPGA2LMK_P]
set_property PACKAGE_PIN AV22 [get_ports CLK40_FPGA2LMK_N]
set_property IOSTANDARD LVDS  [get_ports CLK40_FPGA2LMK_P]
set_property IOSTANDARD LVDS  [get_ports CLK40_FPGA2LMK_N]
set_property PACKAGE_PIN K13 [get_ports LMK_DATA]
set_property IOSTANDARD LVCMOS33 [get_ports LMK_DATA]
set_property PACKAGE_PIN J13 [get_ports LMK_CLK]
set_property IOSTANDARD LVCMOS33 [get_ports LMK_CLK]
set_property PACKAGE_PIN K12 [get_ports LMK_LE]
set_property IOSTANDARD LVCMOS33 [get_ports LMK_LE]
set_property PACKAGE_PIN L14 [get_ports LMK_GOE]
set_property IOSTANDARD LVCMOS33 [get_ports LMK_GOE]
set_property PACKAGE_PIN J14 [get_ports LMK_SYNCn]
set_property IOSTANDARD LVCMOS33 [get_ports LMK_SYNCn]
set_property PACKAGE_PIN L13 [get_ports LMK_LD]
set_property IOSTANDARD LVCMOS33 [get_ports LMK_LD]
## Si5345 constraints
set_property PACKAGE_PIN N16 [get_ports {SI5345_INSEL[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {SI5345_INSEL[0]}]
set_property PACKAGE_PIN M16 [get_ports {SI5345_INSEL[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {SI5345_INSEL[1]}]
set_property PACKAGE_PIN P16 [get_ports SI5345_nLOL]
set_property IOSTANDARD LVCMOS18 [get_ports SI5345_nLOL]
set_property PACKAGE_PIN R18 [get_ports SI5345_SEL]
set_property IOSTANDARD LVCMOS18 [get_ports SI5345_SEL]
set_property PACKAGE_PIN R17 [get_ports SI5345_OE]
set_property IOSTANDARD LVCMOS18 [get_ports SI5345_OE]
set_property PACKAGE_PIN R15 [get_ports {SI5345_A[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {SI5345_A[0]}]
set_property PACKAGE_PIN P15 [get_ports {SI5345_A[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {SI5345_A[1]}]
# Si5345 input from the main MMCM
set_property IOSTANDARD LVDS   [get_ports clk40_ttc_ref_out_p]
set_property PACKAGE_PIN AT20  [get_ports clk40_ttc_ref_out_n]
set_property IOSTANDARD LVDS   [get_ports clk40_ttc_ref_out_n]
# Si5345 output to the FPGA fabric
## those are abused pins called DDR4 clocks
set_property IOSTANDARD LVDS   [get_ports clk_ttcfx_ref1_in_p]
set_property PACKAGE_PIN AT22  [get_ports clk_ttcfx_ref1_in_n]
set_property IOSTANDARD LVDS   [get_ports clk_ttcfx_ref1_in_n]
set_property IOSTANDARD LVDS   [get_ports clk_ttcfx_ref2_in_p]
set_property PACKAGE_PIN H22   [get_ports clk_ttcfx_ref2_in_n]
set_property IOSTANDARD LVDS   [get_ports clk_ttcfx_ref2_in_n]

# Test Points connected to the Debug Port
# TP1_P - J3
set_property PACKAGE_PIN AU17    [get_ports SmaOut_x3]
set_property IOSTANDARD LVCMOS18 [get_ports SmaOut_x3]
# TP1_N - J4
set_property PACKAGE_PIN AU16    [get_ports SmaOut_x4]
set_property IOSTANDARD LVCMOS18 [get_ports SmaOut_x4]
# TP2_P - J5
set_property PACKAGE_PIN P34     [get_ports SmaOut_x5]
set_property IOSTANDARD LVCMOS18 [get_ports SmaOut_x5]
# TP2_N - J9
set_property PACKAGE_PIN P35     [get_ports SmaOut_x6]
set_property IOSTANDARD LVCMOS18 [get_ports SmaOut_x6]

set_property PACKAGE_PIN B34     [get_ports uC_reset_N]
set_property IOSTANDARD LVCMOS18 [get_ports uC_reset_N]


## some more constraints
set_property PACKAGE_PIN AP21 [get_ports {NT_PORTSEL[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {NT_PORTSEL[0]}]
set_property PACKAGE_PIN AN23 [get_ports {NT_PORTSEL[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {NT_PORTSEL[1]}]
set_property PACKAGE_PIN AP23 [get_ports {NT_PORTSEL[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {NT_PORTSEL[2]}]

set_property PACKAGE_PIN AK22 [get_ports {TESTMODE[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {TESTMODE[0]}]
set_property PACKAGE_PIN AK21 [get_ports {TESTMODE[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {TESTMODE[1]}]
set_property PACKAGE_PIN AJ23 [get_ports {TESTMODE[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {TESTMODE[2]}]

set_property PACKAGE_PIN AK23 [get_ports {UPSTREAM_PORTSEL[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {UPSTREAM_PORTSEL[0]}]
set_property PACKAGE_PIN AM21 [get_ports {UPSTREAM_PORTSEL[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {UPSTREAM_PORTSEL[1]}]
set_property PACKAGE_PIN AM20 [get_ports {UPSTREAM_PORTSEL[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {UPSTREAM_PORTSEL[2]}]

set_property PACKAGE_PIN AL23 [get_ports {STN0_PORTCFG[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {STN0_PORTCFG[0]}]
set_property PACKAGE_PIN AL22 [get_ports {STN0_PORTCFG[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {STN0_PORTCFG[1]}]

set_property PACKAGE_PIN AJ20 [get_ports {STN1_PORTCFG[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {STN1_PORTCFG[0]}]
set_property PACKAGE_PIN AK20 [get_ports {STN1_PORTCFG[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {STN1_PORTCFG[1]}]

## KCU settings
#set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets mmcm0_i_1_n_2]
set_property BITSTREAM.CONFIG.OVERTEMPSHUTDOWN ENABLE [current_design]
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets -hierarchical -filter { NAME =~ "mmcm0*" } ]
#PBLOCKS, put channels 0..7 in the lower SRL + PCIe0, put channels 8..15 in the upper SRL + PCIe1

#BPI Flash pins
set_property PACKAGE_PIN AR27 [get_ports {flash_a[0]}]
set_property PACKAGE_PIN AT27 [get_ports {flash_a[1]}]
set_property PACKAGE_PIN AP25 [get_ports {flash_a[2]}]
set_property PACKAGE_PIN AR25 [get_ports {flash_a[3]}]
set_property PACKAGE_PIN AU26 [get_ports {flash_a[4]}]
set_property PACKAGE_PIN AU27 [get_ports {flash_a[5]}]
set_property PACKAGE_PIN AT25 [get_ports {flash_a[6]}]
set_property PACKAGE_PIN AU25 [get_ports {flash_a[7]}]
set_property PACKAGE_PIN AW25 [get_ports {flash_a[8]}]
set_property PACKAGE_PIN AW26 [get_ports {flash_a[9]}]
set_property PACKAGE_PIN AV26 [get_ports {flash_a[10]}]
set_property PACKAGE_PIN AV27 [get_ports {flash_a[11]}]
set_property PACKAGE_PIN AY26 [get_ports {flash_a[12]}]
set_property PACKAGE_PIN BA27 [get_ports {flash_a[13]}]
set_property PACKAGE_PIN AW28 [get_ports {flash_a[14]}]
set_property PACKAGE_PIN AY28 [get_ports {flash_a[15]}]
set_property PACKAGE_PIN AY25 [get_ports {flash_a[16]}]
set_property PACKAGE_PIN BA25 [get_ports {flash_a[17]}]
set_property PACKAGE_PIN AY27 [get_ports {flash_a[18]}]
set_property PACKAGE_PIN BA28 [get_ports {flash_a[19]}]
set_property PACKAGE_PIN BD25 [get_ports {flash_a[20]}]
set_property PACKAGE_PIN BD26 [get_ports {flash_a[21]}]
set_property PACKAGE_PIN BB26 [get_ports {flash_a[22]}]
set_property PACKAGE_PIN BB27 [get_ports {flash_a[23]}]
set_property PACKAGE_PIN BB24 [get_ports {flash_a[24]}]
#set_property PACKAGE_PIN BC26 [get_ports {flash_a[25]}]
#set_property PACKAGE_PIN BC27 [get_ports {flash_a[26]}]
set_property PACKAGE_PIN BC26 [get_ports {flash_a_msb[0]}]
set_property PACKAGE_PIN BC27 [get_ports {flash_a_msb[1]}]


set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[0]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[1]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[2]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[3]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[4]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[5]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[6]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[7]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[8]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[9]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[10]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[11]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[12]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[13]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[14]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[15]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[16]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[17]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[18]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[19]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[20]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[21]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[22]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[23]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[24]}]
#set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[25]}]
#set_property IOSTANDARD LVCMOS25 [get_ports {flash_a[26]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a_msb[0]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_a_msb[1]}]


set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[0]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[1]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[2]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[3]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[4]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[5]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[6]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[7]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[8]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[9]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[10]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[11]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[12]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[13]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[14]}]
set_property IOSTANDARD LVCMOS25 [get_ports {flash_d[15]}]

set_property PACKAGE_PIN AK25 [get_ports {flash_d[0]}]
set_property PACKAGE_PIN BB25 [get_ports {flash_d[1]}]
set_property PACKAGE_PIN BC28 [get_ports {flash_d[2]}]
set_property PACKAGE_PIN BD28 [get_ports {flash_d[3]}]
set_property PACKAGE_PIN AL27 [get_ports {flash_d[4]}]
set_property PACKAGE_PIN AM27 [get_ports {flash_d[5]}]
set_property PACKAGE_PIN AL24 [get_ports {flash_d[6]}]
set_property PACKAGE_PIN AM24 [get_ports {flash_d[7]}]
set_property PACKAGE_PIN AM26 [get_ports {flash_d[8]}]
set_property PACKAGE_PIN AN26 [get_ports {flash_d[9]}]
set_property PACKAGE_PIN AL25 [get_ports {flash_d[10]}]
set_property PACKAGE_PIN AM25 [get_ports {flash_d[11]}]
set_property PACKAGE_PIN AP26 [get_ports {flash_d[12]}]
set_property PACKAGE_PIN AR26 [get_ports {flash_d[13]}]
set_property PACKAGE_PIN AN24 [get_ports {flash_d[14]}]
set_property PACKAGE_PIN AP24 [get_ports {flash_d[15]}]

#set_property IOSTANDARD LVCMOS25 [get_ports clk]
#set_property PACKAGE_PIN AK26 [get_ports clk]

set_property IOSTANDARD LVCMOS25 [get_ports flash_re]
set_property PACKAGE_PIN BC24 [get_ports flash_re]


set_property PACKAGE_PIN BD24 [get_ports flash_we]
set_property IOSTANDARD LVCMOS25 [get_ports flash_we]

set_property PACKAGE_PIN AT24 [get_ports flash_adv]
set_property IOSTANDARD LVCMOS25 [get_ports flash_adv]

set_property IOSTANDARD LVCMOS25 [get_ports flash_ce]
set_property PACKAGE_PIN BA24 [get_ports flash_ce]

set_property IOSTANDARD LVCMOS18 [get_ports flash_SEL]
set_property PACKAGE_PIN AY18 [get_ports flash_SEL]

set_property IOSTANDARD LVCMOS18 [get_ports PEX_SEL1]
set_property PACKAGE_PIN AP20 [get_ports PEX_SEL1]

set_property IOSTANDARD LVCMOS18 [get_ports PEX_SEL0]
set_property PACKAGE_PIN AR20 [get_ports PEX_SEL0]


set_property IOSTANDARD LVCMOS25 [get_ports flash_cclk]
set_property PACKAGE_PIN AJ25 [get_ports flash_cclk]

#set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets emcclk_IBUF]

set_property IOSTANDARD LVCMOS18 [get_ports TACH]
set_property PACKAGE_PIN AU36 [get_ports TACH]


#bank 126, 127, 128 use clk from bank 127
set_property PACKAGE_PIN AH37 [get_ports Q2_CLK0_GTREFCLK_PAD_P_IN]
set_property PACKAGE_PIN AH38 [get_ports Q2_CLK0_GTREFCLK_PAD_N_IN]
#bank 131, 132, 133 use clk from bank 132
set_property PACKAGE_PIN T37 [get_ports Q8_CLK0_GTREFCLK_PAD_P_IN]
set_property PACKAGE_PIN T38 [get_ports Q8_CLK0_GTREFCLK_PAD_N_IN]

#bank 231, 232, 233,use clk from bank 232
set_property PACKAGE_PIN M8 [get_ports Q4_CLK0_GTREFCLK_PAD_P_IN]
set_property PACKAGE_PIN M7 [get_ports Q4_CLK0_GTREFCLK_PAD_N_IN]
#bank 228 use clk from bank 228
set_property PACKAGE_PIN AF8 [get_ports Q5_CLK0_GTREFCLK_PAD_P_IN]
set_property PACKAGE_PIN AF7 [get_ports Q5_CLK0_GTREFCLK_PAD_N_IN]

#bank 224, 225 use clk from bank 225
set_property PACKAGE_PIN AP8 [get_ports Q6_CLK0_GTREFCLK_PAD_P_IN]
set_property PACKAGE_PIN AP7 [get_ports Q6_CLK0_GTREFCLK_PAD_N_IN]

###############################################################################
# End
###############################################################################
