library ieee, UNISIM, xpm;
use ieee.numeric_std.all;
use unisim.vcomponents.all;
use xpm.vcomponents.all;
use ieee.std_logic_unsigned.all; 
use ieee.std_logic_1164.all;

use work.pcie_package.all;
use work.phy_pkg.all;
use work.gtm_pkg.all;

entity tpc_dam_top is
  generic(
    NUMBER_OF_INTERRUPTS            : integer := 8;
    NUMBER_OF_DESCRIPTORS           : integer := 2;
    APP_CLK_FREQ                    : integer := 200;
    BUILD_DATETIME                  : std_logic_vector(39 downto 0) := x"DEADBEEF02";
    GBT_NUM                         : integer := 8; -- number of GBT channels
    GENERATE_GBT                    : boolean := false;
    AUTOMATIC_CLOCK_SWITCH          : boolean := false;
    USE_BACKUP_CLK                  : boolean := true; -- true to use 100/200 mhz board crystal, false to use TTC clock
    DEBUG_MODE                      : boolean := false; -- true debug ON
    CARD_TYPE                       : integer := 712;
    GBT_MAPPING                     : integer := 0; -- GBT mapping: 0 NORMAL CXP1 -> GBT1-12 | 1 ALTERNATE CXP1 -> GBT 1-4,9-12,17-20
    OPTO_TRX                        : integer := 2;
    crInternalLoopbackMode          : boolean := false;
    TTC_test_mode                   : boolean := false;
    generateTTCemu                  : boolean := false;
    GIT_HASH                        : std_logic_vector(159 downto 0) := x"0000000000000000000000000000000000000000";
    COMMIT_DATETIME                 : std_logic_vector(39 downto 0) := x"DEADBEEF03";
    STATIC_CENTRALROUTER            : boolean := false; -- removes update process from central router register map, only initial constant values are used
    GTHREFCLK_SEL                   : std_logic := '0'; -- GREFCLK: '1', MGTREFCLK: '0'
    useToFrontendGBTdataEmulator    : boolean := true;
    useToHostGBTdataEmulator        : boolean := true;
    CREnableFromHost                : boolean := true;
    FIRMWARE_MODE                   : integer := 0;
    PLL_SEL                         : std_logic := '0'; -- 0: CPLL, 1: QPLL
    generate_IC_EC_TTC_only         : boolean := false;
    includeDirectMode               : boolean := true;
    include_strips_ila              : boolean := false;
    strips_mapping                  : string  := "unknown";
    toHostTimeoutBitn               : integer := 10;
    GIT_TAG                         : std_logic_vector(127 downto 0) := x"00000000000000000000000000000000";
    GIT_COMMIT_NUMBER               : integer := 0;
    GENERATE_XOFF                   : boolean := false);
  port (
    BUSY_OUT                  : out    std_logic;
    CLK40_FPGA2LMK_N          : out    std_logic;
    CLK40_FPGA2LMK_P          : out    std_logic;
    CLK_TTC_N                 : in     std_logic;
    CLK_TTC_P                 : in     std_logic;
    DATA_TTC_N                : in     std_logic;
    DATA_TTC_P                : in     std_logic;
    I2C_SMB                   : out    std_logic;
    I2C_SMBUS_CFG_nEN         : out    std_logic;
    I2C_nRESET                : out    std_logic;
    I2C_nRESET_PCIe           : out    std_logic;
    LMK_CLK                   : out    std_logic;
    LMK_DATA                  : out    std_logic;
    LMK_GOE                   : out    std_logic;
    LMK_LD                    : in     std_logic;
    LMK_LE                    : out    std_logic;
    LMK_SYNCn                 : out    std_logic;
    LOL_ADN                   : in     std_logic;
    LOS_ADN                   : in     std_logic;
    MGMT_PORT_EN              : out    std_logic;
    NT_PORTSEL                : out    std_logic_vector(2 downto 0);
    PCIE_PERSTn1              : out    std_logic;
    PCIE_PERSTn2              : out    std_logic;
    PEX_PERSTn                : out    std_logic;
    PEX_SCL                   : out    std_logic;
    PEX_SDA                   : inout  std_logic;
    PORT_GOOD                 : in     std_logic_vector(7 downto 0);
    Perstn1_open              : in     std_logic; -- @suppress "Unused port: Perstn1_open is not used in work.felix_top_bnl711(structure)"
    Perstn2_open              : in     std_logic; -- @suppress "Unused port: Perstn2_open is not used in work.felix_top_bnl711(structure)"
    Q2_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q2_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    --Q4_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    --Q4_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    Q5_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q5_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    Q6_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q6_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    Q8_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q8_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    --SCL                       : inout  std_logic;
    --SDA                       : inout  std_logic;
    SHPC_INT                  : out    std_logic;
        gth_refclk_p : in std_logic;
        gth_refclk_n : in std_logic;
        gth_tx_n : out std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        gth_tx_p : out std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        gth_rx_n : in std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        gth_rx_p : in std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
    STN0_PORTCFG              : out    std_logic_vector(1 downto 0);
    STN1_PORTCFG              : out    std_logic_vector(1 downto 0);
    SmaOut_x3                 : out    std_logic;
    SmaOut_x4                 : out    std_logic;
    SmaOut_x5                 : out    std_logic;
    SmaOut_x6                 : out    std_logic;
    TACH                      : in     std_logic;
    TESTMODE                  : out    std_logic_vector(2 downto 0);
    UPSTREAM_PORTSEL          : out    std_logic_vector(2 downto 0);
    app_clk_in_n              : in     std_logic;
    app_clk_in_p              : in     std_logic;
   -- clk40_ttc_ref_out_n       : out    std_logic; -- Towards Si5345 CLKIN
   -- clk40_ttc_ref_out_p       : out    std_logic; -- Towards Si5345 CLKIN
    clk_ttcfx_ref1_in_n       : in     std_logic;
    clk_ttcfx_ref1_in_p       : in     std_logic;
    clk_ttcfx_ref2_in_n       : in     std_logic;
    clk_ttcfx_ref2_in_p       : in     std_logic;
    emcclk                    : in     std_logic;
    flash_SEL                 : out    std_logic;
    flash_a                   : out    std_logic_vector(24 downto 0);
    flash_a_msb               : inout  std_logic_vector(1 downto 0);
    flash_adv                 : out    std_logic;
    flash_cclk                : out    std_logic;
    flash_ce                  : out    std_logic;
    flash_d                   : inout  std_logic_vector(15 downto 0);
    flash_re                  : out    std_logic;
    flash_we                  : out    std_logic;
    opto_inhibit              : out    std_logic_vector(OPTO_TRX-1 downto 0);
    pcie_rxn                  : in     std_logic_vector(15 downto 0);
    pcie_rxp                  : in     std_logic_vector(15 downto 0);
    pcie_txn                  : out    std_logic_vector(15 downto 0);
    pcie_txp                  : out    std_logic_vector(15 downto 0);
    sys_clk0_n                : in     std_logic;
    sys_clk0_p                : in     std_logic;
    sys_clk1_n                : in     std_logic;
    sys_clk1_p                : in     std_logic;
    sys_reset_n               : in     std_logic;
    uC_reset_N                : out    std_logic;
        SI5345_INSEL : out std_logic_vector(1 downto 0);
        SI5345_nLOL  : in  std_logic;
        SI5345_SEL   : out std_logic;
        SI5345_OE    : out std_logic;
        SI5345_A     : out std_logic_vector(1 downto 0);
        SI5345_nRST  : out std_logic;
        I2C_SW_SCL : inout std_logic;
        I2C_SW_SDA : inout std_logic;
        
        si5345_ref_clk_p : out std_logic;
        si5345_ref_clk_n : out std_logic;
    
        gtm_ref_clk_p : in std_logic;
        gtm_ref_clk_n : in std_logic;
 
        gtm_tx_n : out std_logic;
        gtm_tx_p : out std_logic;
        gtm_rx_n : in std_logic;
        gtm_rx_p : in std_logic;
        SFP_TX_DISABLE : out std_logic;
    leds                      : out std_logic_vector(7 downto 0)
);
end entity tpc_dam_top;

architecture behv of tpc_dam_top is

  signal clk40                                : std_logic;
  signal clk80                                : std_logic;
  signal clk160                               : std_logic;
  signal clk240                               : std_logic;
  signal clk10_xtal                           : std_logic;
  signal clk40_xtal                           : std_logic;
  signal pcie0_appreg_clk                     : std_logic;
  signal pcie1_appreg_clk                     : std_logic;
  signal clk_ttc_40_s                         : std_logic;
  signal clk_ttcfx_mon1                       : std_logic;
  signal clk_ttcfx_mon2                       : std_logic;
  signal clk_adn_160                          : std_logic;
  signal RXUSRCLK                             : std_logic_vector(GBT_NUM-1 downto 0);
  
  signal rst_hw                               : std_logic;
  signal rst_soft_40_0                        : std_logic;
  signal rst_soft_40_1                        : std_logic;
  signal reset_soft_appregclk_pcie0           : std_logic;
  --signal reset_soft_appregclk_pcie1           : std_logic;
    
  signal register_map_gen_board_info          : register_map_gen_board_info_type;

  signal pcie0_register_map_control           : register_map_control_type;
  signal pcie0_register_map_40_control        : register_map_control_type;
  signal pcie1_register_map_control           : register_map_control_type;
  signal pcie1_register_map_40_control        : register_map_control_type;
  signal cr0_interrupt_call                   : std_logic_vector(NUMBER_OF_INTERRUPTS-1 downto 4);
  signal cr1_interrupt_call                   : std_logic_vector(NUMBER_OF_INTERRUPTS-1 downto 4);
  signal lnk_up1                              : std_logic;
  signal lnk_up0                              : std_logic;
  signal LMK_locked                        : std_logic_vector(0 downto 0);
  signal cdrlocked_out                        : std_logic;
  signal MMCM_Locked_out                      : std_logic;
  signal MMCM_OscSelect_out                   : std_logic;
   
  signal cr0_GBTlinkValid_array               : std_logic_vector((GBT_NUM /2-1) downto 0);
  signal cr1_GBTlinkValid_array               : std_logic_vector((GBT_NUM /2-1) downto 0);
  
  signal fo0_FRAME_LOCKED_O                   : std_logic_vector(0 to (GBT_NUM/2-1));
  signal fo1_FRAME_LOCKED_O                   : std_logic_vector((GBT_NUM /2-1) downto 0);
  
  signal emu0_GBTlinkValid                    : std_logic;
  signal emu0_emu_GBTdata                     : std_logic_vector(119 downto 0);
  signal emu1_GBTlinkValid                    : std_logic;
  signal emu1_emu_GBTdata                     : std_logic_vector(119 downto 0);
  
  signal TTC_out                              : std_logic_vector(15 downto 0);

  signal DMA_BUSY_in                          : std_logic;
  signal pcie0_tohost_busy_out                : std_logic;
  signal pcie1_tohost_busy_out                : std_logic;
  signal cr1_thFIFObusyOut                    : std_logic;
  signal cr0_thFIFObusyOut                    : std_logic;
  signal FIFO_BUSY_in                         : std_logic;
  signal BUSY_OUT_s                           : std_logic;
  
  
  signal fromHostFifo0_dout                   : std_logic_vector(255 downto 0);
  signal fromHostFifo0_rd_en                  : std_logic;
  signal fromHostFifo0_empty                  : std_logic;
  signal fromHostFifo0_rd_clk                 : std_logic;
  signal fromHostFifo0_rst                    : std_logic;
  
  signal fromHostFifo1_dout                   : std_logic_vector(255 downto 0);
  signal fromHostFifo1_rd_en                  : std_logic;
  signal fromHostFifo1_empty                  : std_logic;
  signal fromHostFifo1_rd_clk                 : std_logic;
  signal fromHostFifo1_rst                    : std_logic;
  
  signal toHostFifo0_din                      : std_logic_vector(255 downto 0);
  signal toHostFifo0_wr_en                    : std_logic;
  signal toHostFifo0_prog_full                : std_logic;
  signal toHostFifo0_wr_clk                   : std_logic;
  signal toHostFifo0_rst                      : std_logic;
  signal toHostFifo0_wr_data_count            : std_logic_vector(11 downto 0);
  
  signal toHostFifo1_din                      : std_logic_vector(255 downto 0);
  signal toHostFifo1_wr_en                    : std_logic;
  signal toHostFifo1_prog_full                : std_logic;
  signal toHostFifo1_wr_clk                   : std_logic;
  signal toHostFifo1_rst                      : std_logic;
  signal toHostFifo1_wr_data_count            : std_logic_vector(11 downto 0);
  
   --   attribute MARK_DEBUG    : string;
   -- attribute MARK_DEBUG of pcie0_register_map_control : signal is "TRUE";
    
    -- Frontend Card Interfaces --
    signal fee_stream_ctrl : stream_ctrl_t;  -- Streaming Data Control
    signal fee_stream      : stream_t;       -- Streaming Data
    signal fee_reg_req     : reg_request_t;  -- Register Requests
    signal fee_reg_rep     : reg_reply_t;    -- Register Replys
    signal fee_link_counters : phy_stats_t;
    signal fee_stream_en : std_logic_vector(NUMBER_OF_FEEs-1 downto 0) := (others => '0');
    
    -- Timing Interface --
    signal gtm_recv : gtm_recvr_out_rec;
    signal tx_broadcast : broadcast_recv_rec;

    signal register_map_fee_reply  : register_map_fee_reply_type;
    signal register_map_gth_status : register_map_gth_status_type;  
    signal register_map_fee_link_counters : register_map_fee_link_counters_type;
    signal register_map_evt_stat : register_map_evt_stat_type;

    signal tx_locked : std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
    signal rx_locked : std_logic_vector(NUMBER_OF_FEEs-1 downto 0);

    signal rst_lmk_reg, vio_reset_reg, rst_gth_reg : std_logic;
    signal rst_lmk, vio_reset, gtm_clk, gtm_locked : std_logic;
    signal bco_valid : std_logic;
    
    signal clk_250MHz : std_logic;
    signal gth_rx_clk : std_logic;
    signal mgt_ref_clk : std_logic;
    signal rst_gth    : std_logic;
    
begin
    SmaOut_x3 <= gth_rx_clk;
    SmaOut_x4 <= gth_rx_clk;--mgt_ref_clk;
    SmaOut_x5 <= tx_locked(0);
    SmaOut_x6 <= rx_locked(0);

    opto_inhibit <= (others => '1');
    SFP_TX_DISABLE <= '0';
    leds <= pcie0_register_map_control.STATUS_LEDS;

    fee_stream_en <= pcie0_register_map_control.fee_stream_enable(NUMBER_OF_FEEs-1 downto 0);
    
    --Various ports specific to the BNL-711
    --| TP1
    --| TP2
    NT_PORTSEL <= "111";
    TESTMODE <= "000";
    UPSTREAM_PORTSEL <= "000";
    STN0_PORTCFG <= "0Z";
    STN1_PORTCFG <= "01";
    I2C_nRESET_PCIe <= '1';
    uC_reset_N <= '1';
    
   FEE_REG_REQ_LOOP : for i in 0 to NUMBER_OF_FEEs-1 generate
    fee_reg_req(i).reg_clk        <= pcie0_appreg_clk;
    fee_reg_req(i).reg_rst        <= vio_reset_reg or vio_reset or rst_hw;
    fee_reg_req(i).reg_addr       <= pcie0_register_map_control.fee_request(i).reg_address;
    fee_reg_req(i).reg_read       <= pcie0_register_map_control.fee_request(i).reg_read(49);
    fee_reg_req(i).reg_write      <= pcie0_register_map_control.fee_request(i).reg_write(48);
    fee_reg_req(i).reg_data_write <= pcie0_register_map_control.fee_request(i).data_write;
   end generate FEE_REG_REQ_LOOP;

   FEE_REG_REP_LOOP : for i in 0 to NUMBER_OF_FEEs-1 generate
    register_map_fee_reply.fee_reply(i).rx_ready(50) <= fee_reg_rep(i).rx_ready;
    register_map_fee_reply.fee_reply(i).reg_ack(49)  <= fee_reg_rep(i).reg_ack;
    register_map_fee_reply.fee_reply(i).reg_fail(48) <= fee_reg_rep(i).reg_fail;
    register_map_fee_reply.fee_reply(i).reg_address  <= fee_reg_rep(i).reg_addr_read;
    register_map_fee_reply.fee_reply(i).data_read    <= fee_reg_rep(i).reg_data_read;
   end generate FEE_REG_REP_LOOP;
   
   FEE_PHY_LINK_STAT : for i in 0 to NUMBER_OF_FEEs-1 generate
    register_map_fee_link_counters.fee_link_block_cnt(i).rx_sob <= fee_link_counters(i).rx_sob;
    register_map_fee_link_counters.fee_link_block_cnt(i).rx_eob <= fee_link_counters(i).rx_eob;
    register_map_fee_link_counters.fee_link_crc_errors(i).fifo_full <= fee_link_counters(i).stream_fifo_full_count;
    register_map_fee_link_counters.fee_link_crc_errors(i).stream_data <= fee_link_counters(i).crc_stream_errors;
    register_map_fee_link_counters.fee_link_crc_errors(i).control_data <= fee_link_counters(i).crc_reg_errors;
   end generate FEE_PHY_LINK_STAT;   
   
   GTH_STATUS_LOOP : for i in 0 to NUMBER_OF_FEEs-1 generate
       register_map_gth_status.tx_locked(i) <= tx_locked(i);
       register_map_gth_status.rx_locked(i) <= rx_locked(i);
   end generate GTH_STATUS_LOOP;

    freq_meter_i : entity work.gc_frequency_meter 
    generic map (
        g_WITH_INTERNAL_TIMEBASE => TRUE,
        g_CLK_SYS_FREQ           => 25e6,
        g_SYNC_OUT               => TRUE,
        g_COUNTER_BITS           => 32)
    port map (
        clk_sys_i    => pcie0_appreg_clk,
        clk_in_i     => gtm_clk,
        rst_n_i      => '0',
        pps_p1_i     => '0',
        -- synced to clk_in_i or clk_sys_i, depending on g_SYNC_OUT value
        freq_o       => register_map_gth_status.gtm_recv_clock_freq, 
        -- synced to clk_sys_i, always
        freq_valid_o => open);

    bco_count_val_sync_i : entity work.gc_pulse_synchronizer2
    port map (
        clk_in_i => gtm_clk,
        rst_in_n_i => '1',
        clk_out_i => pcie0_appreg_clk,
        rst_out_n_i => '1',
        d_ready_o => open,
        d_ack_p_o => open,
        d_p_i => gtm_recv.bco_count_val,
        q_p_o => bco_valid);

    bco_count_sync_i : entity work.gc_sync_word_rd
    generic map (
        g_WIDTH => 40)
    port map (
        --  Output clock and reset (wishbone side)
        clk_out_i   => pcie0_appreg_clk,
        rst_out_n_i => '1',
        --  Input clock and reset (user side)
        clk_in_i    => gtm_clk,
        rst_in_n_i  => '1',
        --  Input data (user side)
        data_in_i   => gtm_recv.bco_count,  
        --  Trigger a read (wishbone side)
        rd_out_i    => bco_valid,
        --  Pulse when the read is available (wishbone side)
        ack_out_o   => open,
        --  Output data (wishbone side)
        data_out_o  => register_map_gth_status.gtm_bco,
        --  Pulse when a data is transfered (user side)
        rd_in_o     => open);

    vio_0: entity work.vio_0
    PORT MAP (
        clk => pcie0_appreg_clk,
        probe_in0(0) => LMK_LOCKED(0),
        probe_in1(0) => SI5345_nLOL,
        probe_in2(0) => gtm_locked,
        probe_in3(0) => '0',
        probe_in4(0) => '0',
        probe_out0(0) => vio_reset,
        probe_out1(0) => rst_lmk,
        probe_out2(0) => rst_gth,
        probe_out3(0) => open,
        probe_out4(0) => open 
    );

    si5345_clk_buffer : OBUFDS
    generic map (
        IOSTANDARD => "DEFAULT",
        SLEW => "FAST"
    )
    port map (
        O   => si5345_ref_clk_p,
        OB  => si5345_ref_clk_n,
        I   => clk40_xtal 
    );

    gtm_recvr_i: entity work.gtm_recvr
    port map (
        -- Reset Logic (50 MHz) --
        clk   => pcie0_appreg_clk,
        reset => rst_hw,
        pll_locked_n => SI5345_nLOL,
        
        -- MGT Reference Clock (400 MHz) --
        mgt_ref_clk_p => gtm_ref_clk_p,
        mgt_ref_clk_n => gtm_ref_clk_n,
        
        gtm_clk => open,--gtm_clk,
        gtm_locked => open,--gtm_locked,
        gtm_recv => open,--gtm_recv,
        
        -- SFP TX --
        gth_tx_n => gtm_tx_n,
        gth_tx_p => gtm_tx_p,
        
        -- SFP RX --
        gth_rx_n => gtm_rx_n,
        gth_rx_p => gtm_rx_p
    );

   vio_reset_reg <= pcie0_register_map_control.PHY_RESET(1);
   rst_lmk_reg   <= pcie0_register_map_control.PHY_RESET(2);
   rst_gth_reg   <= pcie0_register_map_control.PHY_RESET(3);

   xpm_cdc_pulse_inst: xpm_cdc_pulse
     generic map(
       DEST_SYNC_FF   => 4,
       INIT_SYNC_FF   => 0,
       REG_OUTPUT     => 1,
       RST_USED       => 0,
       SIM_ASSERT_CHK => 0
     )
     port map(
       dest_pulse  => gtm_recv.lvl1_accept(0),
       dest_clk    => clk40,
       dest_rst    => '0',
       src_clk     => pcie0_appreg_clk,
       src_pulse   => pcie0_register_map_control.DMA_TEST.TRIGGER(64),
       src_rst     => '0'
     );
 
    gtm_clk <= clk40;
    
    gtm_locked <= '1';
    gtm_recv.fem_user_bits <= "000";
    gtm_recv.modebits_enb <= "0";
    gtm_recv.fem_endat <= "00";
--    gtm_recv.lvl1_accept <= "0";
    gtm_recv.modebit_n_bco <= "0";
    gtm_recv.modebits <= x"00";
    gtm_recv.modebits_val <= '0';
    gtm_recv.bco_count <= x"0000000000";
    gtm_recv.bco_count_val <= '0';
--    type gtm_recvr_out_rec is record
--        fem_user_bits : std_logic_vector(2 downto 0);
--        modebits_enb  : std_logic_vector(0 downto 0);
--        fem_endat     : std_logic_vector(1 downto 0);
--        lvl1_accept   : std_logic_vector(0 downto 0);
--        modebit_n_bco : std_logic_vector(0 downto 0);
--        modebits      : std_logic_vector(7 downto 0);
--        modebits_val  : std_logic;
--        bco_count     : std_logic_vector(39 downto 0);
--        bco_count_val : std_logic;
--    end record;

    si5345_init_0 : entity work.si5345_pll_init
    port map (
        clk_50MHz  => pcie0_appreg_clk,
        rst        => vio_reset_reg or vio_reset or rst_hw,
        
        en => pcie0_register_map_control.si5345_pll.program(2),
        busy => register_map_gen_board_info.si5345_pll.i2c_busy(0),
        
        status_strb => '0',
        status      => open,

        in_sel   => SI5345_INSEL,
        sel      => SI5345_SEL,
        output_en => SI5345_OE,
        i2c_addr  => SI5345_A,
        n_dev_reset => SI5345_nRST,

        i2c_sda => I2C_SW_SDA,
        i2c_scl => I2C_SW_SCL
    );
   
   -- PHY --
    fee_phys_0 : entity work.fee_phys
    port map (
        -- Reset Logic --
        reset_clk   => pcie0_appreg_clk,
        reset_in => rst_gth_reg or rst_gth,--not gtm_locked, 
        reset_out => open,
        
        -- MGT Reference Clock --
        mgt_ref_clk_p => gth_refclk_p,
        mgt_ref_clk_n => gth_refclk_n,
        
        -- SFP TX --
        gth_tx_n => gth_tx_n,
        gth_tx_p => gth_tx_p,
        
        -- SFP RX --
        gth_rx_n => gth_rx_n,
        gth_rx_p => gth_rx_p,
        
        -- Register Interface --
        reg_request => fee_reg_req,
        reg_reply   => fee_reg_rep,
        
        -- Streaming Data Interface --
        stream_ctrl  => fee_stream_ctrl,
        stream       => fee_stream,
        stream_en    => fee_stream_en,
        
        -- Trigger/Timing Interface --
        timing  => gtm_recv,
        
        -- Broadcast Interface --
        tx_broadcast => tx_broadcast,
        
        counters => fee_link_counters,

        rx_clk_out => gth_rx_clk,
        mgt_ref_clk_out => mgt_ref_clk,

        tx_locked => tx_locked,
        rx_locked => rx_locked
    );    
    
    dma_dategen_i : entity work.dma_datagen
    generic map (
        TOTAL_LENGTH => x"FFFF"
    )
    port map (
        clk    => pcie0_appreg_clk,
        rst_i  => reset_soft_appregclk_pcie0,
        trig_i => pcie0_register_map_control.DMA_TEST.TRIGGER(64),
        data_o => toHostFifo0_din,
        wren_o => toHostFifo0_wr_en
    );

    toHostFifo0_rst <= reset_soft_appregclk_pcie0;
    toHostFifo0_wr_clk <= pcie0_appreg_clk;

    fifo_gen: for I in 0 to 7 generate
      signal evt_fifo_rst   : std_logic;
      signal evt_fifo_rd    : std_logic;
    begin

      -- gtx_rx_clk
      fee_stream_ctrl(I).clk <= gth_rx_clk;
      fee_stream_ctrl(I).read_en <= fee_stream(I).avail;

      -- pcie0_appreg_clk
      register_map_evt_stat.EVT_FIFO_STATUS(I)(30 downto 12) <= (others=>'0');

      xpm_cdc_single_inst: xpm_cdc_single
        generic map(
          DEST_SYNC_FF   => 4,
          INIT_SYNC_FF   => 0,
          SIM_ASSERT_CHK => 0,
          SRC_INPUT_REG  => 1
        )
        port map(
          dest_out => evt_fifo_rst,
          dest_clk => gth_rx_clk,
          src_clk  => pcie0_appreg_clk,
          src_in   => pcie0_register_map_control.PHY_RESET(4)
        );

      xpm_fifo_async_inst: xpm_fifo_async
        generic map(
          CDC_SYNC_STAGES => 2,
          DOUT_RESET_VALUE => "0",
          ECC_MODE => "no_ecc",   
          FIFO_MEMORY_TYPE => "block",
          FIFO_READ_LATENCY => 0,
          FIFO_WRITE_DEPTH => 2048,
          FULL_RESET_VALUE => 0,
          PROG_EMPTY_THRESH => 10,
          PROG_FULL_THRESH => 10,
          RD_DATA_COUNT_WIDTH => 12,
          READ_DATA_WIDTH => 32,
          READ_MODE => "fwft",
          RELATED_CLOCKS => 0,
          SIM_ASSERT_CHK => 0,
          USE_ADV_FEATURES => "0707",
          WAKEUP_TIME => 0,
          WRITE_DATA_WIDTH => 32,
          WR_DATA_COUNT_WIDTH => 12
        )
        port map(
          almost_empty      => open,
          almost_full       => open,
          data_valid        => open,
          dbiterr           => open,
          dout              => register_map_evt_stat.EVT_FIFO_DATA(I),
          empty             => register_map_evt_stat.EVT_FIFO_STATUS(I)(31),
          full              => open,
          overflow          => open,
          prog_empty        => open,
          prog_full         => open,
          rd_data_count     => register_map_evt_stat.EVT_FIFO_STATUS(I)(11 downto 0),
          rd_rst_busy       => open,
          sbiterr           => open,
          underflow         => open,
          wr_ack            => open,
          wr_data_count     => open,
          wr_rst_busy       => open,
          din(15 downto 0)  => fee_stream(I).data,
          din(31 downto 16) => x"0000",
          injectdbiterr     => '0',
          injectsbiterr     => '0',
          rd_clk            => pcie0_appreg_clk,
          rd_en             => pcie0_register_map_control.EVT_FIFO_RD(I)(64),
          rst               => evt_fifo_rst,
          sleep             => '0',
          wr_clk            => gth_rx_clk,
          wr_en             => fee_stream_ctrl(I).read_en
        );
      end generate;
    
--    dma_data_builder_i : entity work.data_builder
--    port map (
--        clk          => gth_rx_clk,
--        rst          => pcie0_register_map_control.DMA_TEST.TRIGGER(64),
--        block_size   => pcie0_register_map_control.dma_packet_chunk_size,
    
--        stream_ctrl  => fee_stream_ctrl,
--        stream       => fee_stream,
        
--        dma_write_clk  => toHostFifo0_wr_clk,
--        dma_write_en   => toHostFifo0_wr_en,
--        dma_write_data => toHostFifo0_din,
--        dma_write_full => toHostFifo0_prog_full);
--        --dma_write_full => '0');
    
    register_map_gen_board_info.LMK_LOCKED <= LMK_locked;
    register_map_gen_board_info.si5345_pll.nLOL(1) <= SI5345_nLOL;
    
    counter_dma_full_i : entity work.counter
    port map (
        clk_i => toHostFifo0_wr_clk,
        rst_i => '0',
        d_i   => toHostFifo0_prog_full,
        count_o => register_map_gen_board_info.DMA_FIFO_FULL_COUNT);
        
    pcie0: entity work.wupper 
    generic map(
        NUMBER_OF_INTERRUPTS => NUMBER_OF_INTERRUPTS,
        NUMBER_OF_DESCRIPTORS => NUMBER_OF_DESCRIPTORS,
        BUILD_DATETIME => BUILD_DATETIME,
        CARD_TYPE => CARD_TYPE,
        GIT_HASH => GIT_HASH,
        COMMIT_DATETIME => COMMIT_DATETIME,
        GIT_TAG => GIT_TAG,
        GIT_COMMIT_NUMBER => GIT_COMMIT_NUMBER,
        GBT_GENERATE_ALL_REGS => true,
        EMU_GENERATE_REGS => false,
        MROD_GENERATE_REGS => false,
        GBT_NUM => GBT_NUM/2,
        FIRMWARE_MODE => FIRMWARE_MODE,
        PCIE_ENDPOINT => 0,
        PCIE_LANES => 8,
        DATA_WIDTH => 256,
        SIMULATION => false,
        BLOCKSIZE => 1024
    )
    port map(
        appreg_clk => pcie0_appreg_clk,
        sync_clk => clk40,
        flush_fifo => open,
        interrupt_call => cr0_interrupt_call,
        lnk_up => lnk_up0,
        pcie_rxn => pcie_rxn(7 downto 0),
        pcie_rxp => pcie_rxp(7 downto 0),
        pcie_txn => pcie_txn(7 downto 0),
        pcie_txp => pcie_txp(7 downto 0),
        pll_locked => open,
        register_map_control_sync => pcie0_register_map_40_control,
        register_map_control_appreg_clk => pcie0_register_map_control,
        register_map_gen_board_info => register_map_gen_board_info,
        register_map_fee_reply => register_map_fee_reply,
        register_map_fee_link_counters => register_map_fee_link_counters,    
        register_map_gth_status => register_map_gth_status,
        register_map_evt_stat => register_map_evt_stat,
        reset_hard => open,
        reset_soft => rst_soft_40_0,
        reset_soft_appreg_clk => reset_soft_appregclk_pcie0,
        reset_hw_in => rst_hw,
        sys_clk_n => sys_clk0_n,
        sys_clk_p => sys_clk0_p,
        sys_reset_n => sys_reset_n,
        tohost_busy_out => pcie0_tohost_busy_out,
        fromHostFifo_dout => fromHostFifo0_dout,
        fromHostFifo_empty => fromHostFifo0_empty,
        fromHostFifo_rd_clk => fromHostFifo0_rd_clk,
        fromHostFifo_rd_en => fromHostFifo0_rd_en,
        fromHostFifo_rst => fromHostFifo0_rst,
        toHostFifo_din(0) => toHostFifo0_din,
        toHostFifo_prog_full(0) => toHostFifo0_prog_full,
        toHostFifo_rst => toHostFifo0_rst,
        toHostFifo_wr_clk => toHostFifo0_wr_clk,
        wr_data_count(0) => toHostFifo0_wr_data_count,
        toHostFifo_wr_en(0) => toHostFifo0_wr_en,
        clk250_out => clk_250MHz,
        master_busy_in => BUSY_OUT_s
    );

  clk0: entity work.clock_and_reset
    generic map(
      APP_CLK_FREQ           => APP_CLK_FREQ,
      USE_BACKUP_CLK         => USE_BACKUP_CLK,
      AUTOMATIC_CLOCK_SWITCH => AUTOMATIC_CLOCK_SWITCH)
    port map(
      MMCM_Locked_out      => MMCM_Locked_out,
      MMCM_OscSelect_out   => MMCM_OscSelect_out,
      app_clk_in_n         => app_clk_in_n,
      app_clk_in_p         => app_clk_in_p,
      cdrlocked_in         => cdrlocked_out,
      clk10_xtal           => clk10_xtal,
      clk160               => clk160,
      clk240               => clk240,
      clk250               => open,
      clk320               => open,
      clk40                => clk40,
      clk40_xtal           => clk40_xtal,
      clk80                => clk80,
      clk_adn_160          => '0',
      clk_adn_160_out_n    => open,
      clk_adn_160_out_p    => open,
      clk_ttc_40           => clk_ttc_40_s,
      clk_ttcfx_mon1       => clk_ttcfx_mon1,
      clk_ttcfx_mon2       => clk_ttcfx_mon2,
      clk_ttcfx_ref1_in_n  => clk_ttcfx_ref1_in_n,
      clk_ttcfx_ref1_in_p  => clk_ttcfx_ref1_in_p,
      clk_ttcfx_ref2_in_n  => clk_ttcfx_ref2_in_n,
      clk_ttcfx_ref2_in_p  => clk_ttcfx_ref2_in_p,
      clk_ttcfx_ref2_out_n => open,
      clk_ttcfx_ref2_out_p => open,
      clk_ttcfx_ref_out_n  => open,
      clk_ttcfx_ref_out_p  => open,
      register_map_control => pcie0_register_map_control,
      reset_out            => rst_hw,
      sys_reset_n          => sys_reset_n);
  
 pcie1: entity work.wupper
   generic map(
     NUMBER_OF_INTERRUPTS => NUMBER_OF_INTERRUPTS,
     NUMBER_OF_DESCRIPTORS => NUMBER_OF_DESCRIPTORS,
     BUILD_DATETIME => BUILD_DATETIME,
     CARD_TYPE => CARD_TYPE,
     GIT_HASH => GIT_HASH,
     COMMIT_DATETIME => COMMIT_DATETIME,
     GIT_TAG => GIT_TAG,
     GIT_COMMIT_NUMBER => GIT_COMMIT_NUMBER,
     GBT_GENERATE_ALL_REGS => true,
     EMU_GENERATE_REGS => false,
     MROD_GENERATE_REGS => false,
     GBT_NUM => GBT_NUM/2,
     FIRMWARE_MODE => FIRMWARE_MODE,
     PCIE_ENDPOINT => 1,
     PCIE_LANES => 8,
     DATA_WIDTH => 256,
     SIMULATION => false,
     BLOCKSIZE => 1024)
   port map(
     appreg_clk => pcie1_appreg_clk,
     sync_clk => clk40,
     flush_fifo => open,
     interrupt_call => cr1_interrupt_call,
     lnk_up => lnk_up1,
     pcie_rxn => pcie_rxn(15 downto 8),
     pcie_rxp => pcie_rxp(15 downto 8),
     pcie_txn => pcie_txn(15 downto 8),
     pcie_txp => pcie_txp(15 downto 8),
     pll_locked => open,
     register_map_control_sync => pcie1_register_map_40_control,
     register_map_control_appreg_clk => pcie1_register_map_control,
     register_map_gen_board_info => register_map_gen_board_info,
     register_map_fee_reply => register_map_fee_reply_c,
     register_map_gth_status => register_map_gth_status_c,
     register_map_fee_link_counters => register_map_fee_link_counters_c,
     register_map_evt_stat => register_map_evt_stat_c,
     reset_hard => open,
     reset_soft => rst_soft_40_1,
     reset_soft_appreg_clk => open,
     reset_hw_in => rst_hw,
     sys_clk_n => sys_clk1_n,
     sys_clk_p => sys_clk1_p,
     sys_reset_n => sys_reset_n,
     tohost_busy_out => pcie1_tohost_busy_out,
     fromHostFifo_dout => fromHostFifo1_dout,
     fromHostFifo_empty => fromHostFifo1_empty,
     fromHostFifo_rd_clk => fromHostFifo1_rd_clk,
     fromHostFifo_rd_en => fromHostFifo1_rd_en,
     fromHostFifo_rst => fromHostFifo1_rst,
     toHostFifo_din(0) => toHostFifo1_din,
     toHostFifo_prog_full(0) => toHostFifo1_prog_full,
     toHostFifo_rst => toHostFifo1_rst,
     toHostFifo_wr_clk => toHostFifo1_wr_clk,
     wr_data_count(0) => toHostFifo1_wr_data_count,
     toHostFifo_wr_en(0) => toHostFifo1_wr_en,
     clk250_out => open,
     master_busy_in => BUSY_OUT_s);

  lmk_init0: entity work.LMK03200_wrapper
    generic map(
        R15 => x"1400320") -- 40 MHz input, 250 Mhz out
        --R13 => x"028F000",   -- For 60 MHz input, 250 Mhz out (R13, R14, R15)
        --R14 => x"0830120",
        --R15 => x"14004b0") 
    port map(
      rst_lmk => rst_lmk_reg or rst_lmk,
      hw_rst => not gtm_locked,
      LMK_locked => LMK_locked(0),
      clk40m_in => gtm_clk,
      clk10m_in => clk10_xtal,
      CLK40_FPGA2LMK_P => CLK40_FPGA2LMK_P,
      CLK40_FPGA2LMK_N => CLK40_FPGA2LMK_N,
      LMK_DATA => LMK_DATA,
      LMK_CLK => LMK_CLK,
      LMK_LE => LMK_LE,
      LMK_GOE => LMK_GOE,
      LMK_LD => LMK_LD,
      LMK_SYNCn => LMK_SYNCn);

  pex_init0: entity work.pex_init
    generic map(
      CARD_TYPE => CARD_TYPE)
    port map(
      I2C_SMB           => I2C_SMB,
      I2C_SMBUS_CFG_nEN => I2C_SMBUS_CFG_nEN,
      MGMT_PORT_EN      => MGMT_PORT_EN,
      PCIE_PERSTn1      => PCIE_PERSTn1,
      PCIE_PERSTn2      => PCIE_PERSTn2,
      PEX_PERSTn        => PEX_PERSTn,
      PEX_SCL           => PEX_SCL,
      PEX_SDA           => PEX_SDA,
      PORT_GOOD         => PORT_GOOD,
      SHPC_INT          => SHPC_INT,
      clk40             => clk40_xtal,
      lnk_up0           => lnk_up0,
      lnk_up1           => lnk_up1,
      reset_pcie        => open,
      sys_reset_n       => sys_reset_n);

end architecture;
