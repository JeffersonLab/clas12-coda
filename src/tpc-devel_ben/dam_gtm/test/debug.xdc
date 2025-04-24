set_property LOC GTHE3_CHANNEL_X1Y28 [get_cells -hierarchical -filter {NAME =~ gtm_recvr_i/phy_gtm_wrapper_i/*/*/*gen_channel_container[9].*gen_gthe3_channel_inst[0].GTHE3_CHANNEL_PRIM_INST}]

create_debug_core u_ila_0 ila
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_0]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_0]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_0]
set_property C_ADV_TRIGGER false [get_debug_cores u_ila_0]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_0]
set_property C_EN_STRG_QUAL false [get_debug_cores u_ila_0]
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_0]
set_property ALL_PROBE_SAME_MU_CNT 1 [get_debug_cores u_ila_0]
create_debug_core u_ila_1 ila
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_1]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_1]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_1]
set_property C_ADV_TRIGGER false [get_debug_cores u_ila_1]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_1]
set_property C_EN_STRG_QUAL false [get_debug_cores u_ila_1]
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_1]
set_property ALL_PROBE_SAME_MU_CNT 1 [get_debug_cores u_ila_1]
connect_debug_port u_ila_0/clk [get_nets [list clk0/inst/clk40 ]]
connect_debug_port u_ila_1/clk [get_nets [list gtm_recvr_i/phy_gtm_wrapper_i/rx_clk ]]
set_property port_width 32 [get_debug_ports u_ila_0/probe0]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe0]
connect_debug_port u_ila_0/probe0 [get_nets [list {gtm_freq[0]} {gtm_freq[1]} {gtm_freq[2]} {gtm_freq[3]} {gtm_freq[4]} {gtm_freq[5]} {gtm_freq[6]} {gtm_freq[7]} {gtm_freq[8]} {gtm_freq[9]} {gtm_freq[10]} {gtm_freq[11]} {gtm_freq[12]} {gtm_freq[13]} {gtm_freq[14]} {gtm_freq[15]} {gtm_freq[16]} {gtm_freq[17]} {gtm_freq[18]} {gtm_freq[19]} {gtm_freq[20]} {gtm_freq[21]} {gtm_freq[22]} {gtm_freq[23]} {gtm_freq[24]} {gtm_freq[25]} {gtm_freq[26]} {gtm_freq[27]} {gtm_freq[28]} {gtm_freq[29]} {gtm_freq[30]} {gtm_freq[31]} ]]
create_debug_port u_ila_0 probe
set_property port_width 1 [get_debug_ports u_ila_0/probe1]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_0/probe1]
connect_debug_port u_ila_0/probe1 [get_nets [list gtm_freq_valid ]]
set_property port_width 2 [get_debug_ports u_ila_1/probe0]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe0]
connect_debug_port u_ila_1/probe0 [get_nets [list {gtm_recvr_i/rx_kcode[0]} {gtm_recvr_i/rx_kcode[1]} ]]
create_debug_port u_ila_1 probe
set_property port_width 2 [get_debug_ports u_ila_1/probe1]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe1]
connect_debug_port u_ila_1/probe1 [get_nets [list {gtm_recvr_i/rx_invalid_char[0]} {gtm_recvr_i/rx_invalid_char[1]} ]]
create_debug_port u_ila_1 probe
set_property port_width 2 [get_debug_ports u_ila_1/probe2]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe2]
connect_debug_port u_ila_1/probe2 [get_nets [list {gtm_recvr_i/rx_disparty_error[0]} {gtm_recvr_i/rx_disparty_error[1]} ]]
create_debug_port u_ila_1 probe
set_property port_width 16 [get_debug_ports u_ila_1/probe3]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe3]
connect_debug_port u_ila_1/probe3 [get_nets [list {gtm_recvr_i/rx_data[0]} {gtm_recvr_i/rx_data[1]} {gtm_recvr_i/rx_data[2]} {gtm_recvr_i/rx_data[3]} {gtm_recvr_i/rx_data[4]} {gtm_recvr_i/rx_data[5]} {gtm_recvr_i/rx_data[6]} {gtm_recvr_i/rx_data[7]} {gtm_recvr_i/rx_data[8]} {gtm_recvr_i/rx_data[9]} {gtm_recvr_i/rx_data[10]} {gtm_recvr_i/rx_data[11]} {gtm_recvr_i/rx_data[12]} {gtm_recvr_i/rx_data[13]} {gtm_recvr_i/rx_data[14]} {gtm_recvr_i/rx_data[15]} ]]
create_debug_port u_ila_1 probe
set_property port_width 8 [get_debug_ports u_ila_1/probe4]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe4]
connect_debug_port u_ila_1/probe4 [get_nets [list {gtm_recvr_i/gtm_recv[modebits][0]} {gtm_recvr_i/gtm_recv[modebits][1]} {gtm_recvr_i/gtm_recv[modebits][2]} {gtm_recvr_i/gtm_recv[modebits][3]} {gtm_recvr_i/gtm_recv[modebits][4]} {gtm_recvr_i/gtm_recv[modebits][5]} {gtm_recvr_i/gtm_recv[modebits][6]} {gtm_recvr_i/gtm_recv[modebits][7]} ]]
create_debug_port u_ila_1 probe
set_property port_width 1 [get_debug_ports u_ila_1/probe5]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe5]
connect_debug_port u_ila_1/probe5 [get_nets [list {gtm_recvr_i/gtm_recv[modebit_n_bco][0]} ]]
create_debug_port u_ila_1 probe
set_property port_width 1 [get_debug_ports u_ila_1/probe6]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe6]
connect_debug_port u_ila_1/probe6 [get_nets [list {gtm_recvr_i/gtm_recv[modebits_enb][0]} ]]
create_debug_port u_ila_1 probe
set_property port_width 1 [get_debug_ports u_ila_1/probe7]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe7]
connect_debug_port u_ila_1/probe7 [get_nets [list {gtm_recvr_i/gtm_recv[lvl1_accept][0]} ]]
create_debug_port u_ila_1 probe
set_property port_width 3 [get_debug_ports u_ila_1/probe8]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe8]
connect_debug_port u_ila_1/probe8 [get_nets [list {gtm_recvr_i/gtm_recv[fem_user_bits][0]} {gtm_recvr_i/gtm_recv[fem_user_bits][1]} {gtm_recvr_i/gtm_recv[fem_user_bits][2]} ]]
create_debug_port u_ila_1 probe
set_property port_width 2 [get_debug_ports u_ila_1/probe9]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe9]
connect_debug_port u_ila_1/probe9 [get_nets [list {gtm_recvr_i/gtm_recv[fem_endat][0]} {gtm_recvr_i/gtm_recv[fem_endat][1]} ]]
create_debug_port u_ila_1 probe
set_property port_width 40 [get_debug_ports u_ila_1/probe10]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe10]
connect_debug_port u_ila_1/probe10 [get_nets [list {gtm_recvr_i/gtm_recv[bco_count][0]} {gtm_recvr_i/gtm_recv[bco_count][1]} {gtm_recvr_i/gtm_recv[bco_count][2]} {gtm_recvr_i/gtm_recv[bco_count][3]} {gtm_recvr_i/gtm_recv[bco_count][4]} {gtm_recvr_i/gtm_recv[bco_count][5]} {gtm_recvr_i/gtm_recv[bco_count][6]} {gtm_recvr_i/gtm_recv[bco_count][7]} {gtm_recvr_i/gtm_recv[bco_count][8]} {gtm_recvr_i/gtm_recv[bco_count][9]} {gtm_recvr_i/gtm_recv[bco_count][10]} {gtm_recvr_i/gtm_recv[bco_count][11]} {gtm_recvr_i/gtm_recv[bco_count][12]} {gtm_recvr_i/gtm_recv[bco_count][13]} {gtm_recvr_i/gtm_recv[bco_count][14]} {gtm_recvr_i/gtm_recv[bco_count][15]} {gtm_recvr_i/gtm_recv[bco_count][16]} {gtm_recvr_i/gtm_recv[bco_count][17]} {gtm_recvr_i/gtm_recv[bco_count][18]} {gtm_recvr_i/gtm_recv[bco_count][19]} {gtm_recvr_i/gtm_recv[bco_count][20]} {gtm_recvr_i/gtm_recv[bco_count][21]} {gtm_recvr_i/gtm_recv[bco_count][22]} {gtm_recvr_i/gtm_recv[bco_count][23]} {gtm_recvr_i/gtm_recv[bco_count][24]} {gtm_recvr_i/gtm_recv[bco_count][25]} {gtm_recvr_i/gtm_recv[bco_count][26]} {gtm_recvr_i/gtm_recv[bco_count][27]} {gtm_recvr_i/gtm_recv[bco_count][28]} {gtm_recvr_i/gtm_recv[bco_count][29]} {gtm_recvr_i/gtm_recv[bco_count][30]} {gtm_recvr_i/gtm_recv[bco_count][31]} {gtm_recvr_i/gtm_recv[bco_count][32]} {gtm_recvr_i/gtm_recv[bco_count][33]} {gtm_recvr_i/gtm_recv[bco_count][34]} {gtm_recvr_i/gtm_recv[bco_count][35]} {gtm_recvr_i/gtm_recv[bco_count][36]} {gtm_recvr_i/gtm_recv[bco_count][37]} {gtm_recvr_i/gtm_recv[bco_count][38]} {gtm_recvr_i/gtm_recv[bco_count][39]} ]]
create_debug_port u_ila_1 probe
set_property port_width 1 [get_debug_ports u_ila_1/probe11]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe11]
connect_debug_port u_ila_1/probe11 [get_nets [list {gtm_recvr_i/gtm_recv[bco_count_val]} ]]
create_debug_port u_ila_1 probe
set_property port_width 1 [get_debug_ports u_ila_1/probe12]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe12]
connect_debug_port u_ila_1/probe12 [get_nets [list {gtm_recvr_i/gtm_recv[modebits_val]} ]]
create_debug_port u_ila_1 probe
set_property port_width 1 [get_debug_ports u_ila_1/probe13]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe13]
connect_debug_port u_ila_1/probe13 [get_nets [list gtm_recvr_i/rx_is_locked ]]
create_debug_port u_ila_1 probe
set_property port_width 1 [get_debug_ports u_ila_1/probe14]
set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_1/probe14]
connect_debug_port u_ila_1/probe14 [get_nets [list gtm_recvr_i/rx_state ]]



