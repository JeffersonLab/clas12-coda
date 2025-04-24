set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]
# Rising Edge System Synchronous Inputs
#
# A Single Data Rate (SDR) System Synchronous interface is
# an interface where the external device and the FPGA use
# the same clock, and a new data is captured one clock cycle
# after being launched
#
# input      __________            __________
# clock   __|          |__________|          |__
#           |
#           |------> (tco_min+trce_dly_min)
#           |------------> (tco_max+trce_dly_max)
#         __________      ________________
# data    __________XXXXXX_____ Data _____XXXXXXX
#



# Input Delay Constraint
set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa7_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa7_sdo_p[*]}]

set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa6_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa6_sdo_p[*]}]

set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa5_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa5_sdo_p[*]}]

set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa4_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa4_sdo_p[*]}]

set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa3_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa3_sdo_p[*]}]

set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa2_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa2_sdo_p[*]}]

set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa1_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa1_sdo_p[*]}]

set_input_delay -clock clk_160_sys_pll -max 1.500 [get_ports {sampa0_sdo_p[*]}]
set_input_delay -clock clk_160_sys_pll -min -1.500 [get_ports {sampa0_sdo_p[*]}]

set_output_delay -clock clk_160_sys_pll -min -1.500 [get_ports {lemo_out[1]}]

#create_clock -name GTREFCLK0_CLK -period 8.0 [get_ports mgt0_ref_clk_p]
#create_clock -name SYSCLK0_CLK -period 20.000 [get_ports sys_clk_p]
set_false_path -from [get_clocks -of_objects [get_pins sys_clk_0/inst/mmcm_adv_inst/CLKOUT4]] -to [get_clocks phy_io_0/phy_driver_i/phy_wrapper_0/gtwizard_i/U0/gtwizard_0_init_i/gtwizard_0_i/gt0_gtwizard_0_i/gtpe2_i/TXOUTCLK]
set_false_path -from [get_clocks phy_io_0/phy_driver_i/phy_wrapper_0/gtwizard_i/U0/gtwizard_0_init_i/gtwizard_0_i/gt0_gtwizard_0_i/gtpe2_i/RXOUTCLK] -to [get_clocks -of_objects [get_pins sys_clk_0/inst/mmcm_adv_inst/CLKOUT4]]
set_false_path -from [get_clocks -of_objects [get_pins sys_clk_0/inst/mmcm_adv_inst/CLKOUT4]] -to [get_clocks phy_io_0/phy_driver_i/phy_wrapper_0/gtwizard_i/U0/gtwizard_0_init_i/gtwizard_0_i/gt0_gtwizard_0_i/gtpe2_i/RXOUTCLK]

set_clock_groups -asynchronous -group [get_clocks phy_io_0/phy_driver_i/phy_wrapper_0/gtwizard_i/U0/gtwizard_0_init_i/gtwizard_0_i/gt0_gtwizard_0_i/gtpe2_i/TXOUTCLK] -group [get_clocks phy_io_0/phy_driver_i/phy_wrapper_0/gtwizard_i/U0/gtwizard_0_init_i/gtwizard_0_i/gt0_gtwizard_0_i/gtpe2_i/RXOUTCLK] -group [get_clocks clkfbout_sys_pll] -group [get_clocks clk_160_sys_pll] -group [get_clocks clk_40_sys_pll] -group [get_clocks clk_20_sys_pll] -group [get_clocks clk_40i_sys_pll] -group [get_clocks clk_200_sys_pll]




set_property MARK_DEBUG true [get_nets sampa_trig_i]
set_property MARK_DEBUG true [get_nets sampa_trig_en]
set_property MARK_DEBUG true [get_nets sampa_reset]
set_property MARK_DEBUG true [get_nets {gtm_trig_sync_i/gtm_recv[lvl1_accept][0]}]
set_property MARK_DEBUG true [get_nets gtm_trig_sync_i/sampa_trig_i]
create_debug_core u_ila_0 ila
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_0]
set_property ALL_PROBE_SAME_MU_CNT 1 [get_debug_cores u_ila_0]
set_property C_ADV_TRIGGER false [get_debug_cores u_ila_0]
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_0]
set_property C_EN_STRG_QUAL false [get_debug_cores u_ila_0]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_0]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_0]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_0]
set_property port_width 1 [get_debug_ports u_ila_0/clk]
connect_debug_port u_ila_0/clk [get_nets [list phy_io_0/phy_driver_i/phy_wrapper_0/gtwizard_i/U0/gt_usrclk_source/GT0_TXUSRCLK2_OUT]]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe0]
set_property port_width 2 [get_debug_ports u_ila_0/probe0]
connect_debug_port u_ila_0/probe0 [get_nets [list {phy_io_0/phy_driver_i/tx_data_link_i/tx_kcode[0]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_kcode[1]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe1]
set_property port_width 17 [get_debug_ports u_ila_0/probe1]
connect_debug_port u_ila_0/probe1 [get_nets [list {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[0]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[1]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[2]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[3]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[4]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[5]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[6]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[7]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[8]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[9]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[10]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[11]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[12]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[13]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[14]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[15]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_data[16]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe2]
set_property port_width 16 [get_debug_ports u_ila_0/probe2]
connect_debug_port u_ila_0/probe2 [get_nets [list {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[0]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[1]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[2]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[3]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[4]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[5]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[6]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[7]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[8]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[9]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[10]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[11]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[12]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[13]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[14]} {phy_io_0/phy_driver_i/tx_data_link_i/tx_data[15]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe3]
set_property port_width 12 [get_debug_ports u_ila_0/probe3]
connect_debug_port u_ila_0/probe3 [get_nets [list {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[0]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[1]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[2]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[3]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[4]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[5]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[6]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[7]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[8]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[9]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[10]} {phy_io_0/phy_driver_i/tx_data_link_i/stream_data[16]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe4]
set_property port_width 3 [get_debug_ports u_ila_0/probe4]
connect_debug_port u_ila_0/probe4 [get_nets [list {phy_io_0/phy_driver_i/tx_data_link_i/act_state[0]} {phy_io_0/phy_driver_i/tx_data_link_i/act_state[1]} {phy_io_0/phy_driver_i/tx_data_link_i/act_state[2]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe5]
set_property port_width 8 [get_debug_ports u_ila_0/probe5]
connect_debug_port u_ila_0/probe5 [get_nets [list {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[0]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[1]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[2]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[3]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[4]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[5]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[6]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_sync_count[7]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe6]
set_property port_width 2 [get_debug_ports u_ila_0/probe6]
connect_debug_port u_ila_0/probe6 [get_nets [list {phy_io_0/phy_driver_i/rx_data_link_i/rx_kcode[0]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_kcode[1]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe7]
set_property port_width 2 [get_debug_ports u_ila_0/probe7]
connect_debug_port u_ila_0/probe7 [get_nets [list {phy_io_0/phy_driver_i/rx_data_link_i/rx_disparity_error[0]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_disparity_error[1]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe8]
set_property port_width 2 [get_debug_ports u_ila_0/probe8]
connect_debug_port u_ila_0/probe8 [get_nets [list {phy_io_0/phy_driver_i/rx_data_link_i/rx_invalid_char[0]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_invalid_char[1]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe9]
set_property port_width 16 [get_debug_ports u_ila_0/probe9]
connect_debug_port u_ila_0/probe9 [get_nets [list {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[0]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[1]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[2]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[3]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[4]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[5]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[6]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[7]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[8]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[9]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[10]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[11]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[12]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[13]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[14]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_data[15]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe10]
set_property port_width 2 [get_debug_ports u_ila_0/probe10]
connect_debug_port u_ila_0/probe10 [get_nets [list {phy_io_0/phy_driver_i/rx_data_link_i/rx_comma_char[0]} {phy_io_0/phy_driver_i/rx_data_link_i/rx_comma_char[1]}]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe11]
set_property port_width 1 [get_debug_ports u_ila_0/probe11]
connect_debug_port u_ila_0/probe11 [get_nets [list phy_io_0/phy_driver_i/rx_data_link_i/rx_is_byte_aligned]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe12]
set_property port_width 1 [get_debug_ports u_ila_0/probe12]
connect_debug_port u_ila_0/probe12 [get_nets [list phy_io_0/phy_driver_i/rx_data_link_i/rx_is_locked]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe13]
set_property port_width 1 [get_debug_ports u_ila_0/probe13]
connect_debug_port u_ila_0/probe13 [get_nets [list phy_io_0/phy_driver_i/rx_data_link_i/rx_state]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe14]
set_property port_width 1 [get_debug_ports u_ila_0/probe14]
connect_debug_port u_ila_0/probe14 [get_nets [list phy_io_0/phy_driver_i/tx_data_link_i/stream_full]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe15]
set_property port_width 1 [get_debug_ports u_ila_0/probe15]
connect_debug_port u_ila_0/probe15 [get_nets [list phy_io_0/phy_driver_i/tx_data_link_i/tx_empty_i]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe16]
set_property port_width 1 [get_debug_ports u_ila_0/probe16]
connect_debug_port u_ila_0/probe16 [get_nets [list phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_avail]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe17]
set_property port_width 1 [get_debug_ports u_ila_0/probe17]
connect_debug_port u_ila_0/probe17 [get_nets [list phy_io_0/phy_driver_i/tx_data_link_i/tx_fifo_rden]]
create_debug_port u_ila_0 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe18]
set_property port_width 1 [get_debug_ports u_ila_0/probe18]
connect_debug_port u_ila_0/probe18 [get_nets [list phy_io_0/phy_driver_i/tx_data_link_i/tx_is_locked]]
create_debug_core u_ila_1 ila
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_1]
set_property ALL_PROBE_SAME_MU_CNT 1 [get_debug_cores u_ila_1]
set_property C_ADV_TRIGGER false [get_debug_cores u_ila_1]
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_1]
set_property C_EN_STRG_QUAL false [get_debug_cores u_ila_1]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_1]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_1]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_1]
set_property port_width 1 [get_debug_ports u_ila_1/clk]
connect_debug_port u_ila_1/clk [get_nets [list phy_io_0/phy_driver_i/phy_wrapper_0/rx_clk_out]]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe0]
set_property port_width 1 [get_debug_ports u_ila_1/probe0]
connect_debug_port u_ila_1/probe0 [get_nets [list {gtm_trig_sync_i/gtm_recv[lvl1_accept][0]}]]
create_debug_core u_ila_2 ila
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_2]
set_property ALL_PROBE_SAME_MU_CNT 1 [get_debug_cores u_ila_2]
set_property C_ADV_TRIGGER false [get_debug_cores u_ila_2]
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_2]
set_property C_EN_STRG_QUAL false [get_debug_cores u_ila_2]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_2]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_2]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_2]
set_property port_width 1 [get_debug_ports u_ila_2/clk]
connect_debug_port u_ila_2/clk [get_nets [list sys_clk_0/inst/clk_40i]]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_2/probe0]
set_property port_width 1 [get_debug_ports u_ila_2/probe0]
connect_debug_port u_ila_2/probe0 [get_nets [list sampa_reset]]
create_debug_port u_ila_2 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_2/probe1]
set_property port_width 1 [get_debug_ports u_ila_2/probe1]
connect_debug_port u_ila_2/probe1 [get_nets [list sampa_trig_en]]
create_debug_core u_ila_3 ila
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_3]
set_property ALL_PROBE_SAME_MU_CNT 1 [get_debug_cores u_ila_3]
set_property C_ADV_TRIGGER false [get_debug_cores u_ila_3]
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_3]
set_property C_EN_STRG_QUAL false [get_debug_cores u_ila_3]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_3]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_3]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_3]
set_property port_width 1 [get_debug_ports u_ila_3/clk]
connect_debug_port u_ila_3/clk [get_nets [list sys_clk_0/inst/clk_40]]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_3/probe0]
set_property port_width 1 [get_debug_ports u_ila_3/probe0]
connect_debug_port u_ila_3/probe0 [get_nets [list sampa_trig_i]]
create_debug_port u_ila_3 probe
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_3/probe1]
set_property port_width 1 [get_debug_ports u_ila_3/probe1]
connect_debug_port u_ila_3/probe1 [get_nets [list gtm_trig_sync_i/sampa_trig_i]]
set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets clk_40MHz]
