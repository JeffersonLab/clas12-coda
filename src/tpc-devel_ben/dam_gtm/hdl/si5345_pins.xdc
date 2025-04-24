# SI5345 I/O
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
set_property PACKAGE_PIN D35 [get_ports SI5345_nRST]
set_property IOSTANDARD LVCMOS18 [get_ports SI5345_nRST]

# Reference Clock 
set_property IOSTANDARD LVDS [get_ports si5345_ref_clk_n]
set_property PACKAGE_PIN AR21 [get_ports si5345_ref_clk_p]
set_property PACKAGE_PIN AT20 [get_ports si5345_ref_clk_n]
set_property IOSTANDARD LVDS [get_ports si5345_ref_clk_p]


## I2C interface for the jitter cleaner
set_property PACKAGE_PIN V13 [get_ports I2C_SW_SCL]
set_property PACKAGE_PIN V14 [get_ports I2C_SW_SDA]
set_property IOSTANDARD LVCMOS33 [get_ports I2C_SW_SCL]
set_property IOSTANDARD LVCMOS33 [get_ports I2C_SW_SDA]


set_property PACKAGE_PIN M7 [get_ports gtm_ref_clk_n]
set_property PACKAGE_PIN M8 [get_ports gtm_ref_clk_p]

