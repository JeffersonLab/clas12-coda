set_property IOSTANDARD LVDS [get_ports SYSCLK_P]
set_property IOSTANDARD LVDS [get_ports SYSCLK_N]
set_property DIFF_TERM_ADV TERM_100 [get_ports SYSCLK_P]
set_property DIFF_TERM_ADV TERM_100 [get_ports SYSCLK_N]
set_property PACKAGE_PIN R31 [get_ports SYSCLK_P]

set_property -dict {PACKAGE_PIN N14 IOSTANDARD LVCMOS33} [get_ports SFP_TX_DISABLE]
