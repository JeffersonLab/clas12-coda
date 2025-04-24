
--!------------------------------------------------------------------------------
--!                                                             
--!           NIKHEF - National Institute for Subatomic Physics 
--!
--!                       Electronics Department                
--!                                                             
--!-----------------------------------------------------------------------------
--! @class felix_top
--! 
--!
--! @author      Andrea Borga    (andrea.borga@nikhef.nl)<br>
--!              Frans Schreuder (frans.schreuder@nikhef.nl)
--!
--!
--! @date        07/01/2015    created
--!
--! @version     1.0
--!
--! @brief 
--! Top level for the FELIX project, containing GBT, CentralRouter and PCIe DMA core
--! 
--! 
--! 
--! @detail
--!
--!-----------------------------------------------------------------------------
--! @TODO
--!  
--!
--! ------------------------------------------------------------------------------
--! Virtex7 PCIe Gen3 DMA Core
--! 
--! \copyright GNU LGPL License
--! Copyright (c) Nikhef, Amsterdam, All rights reserved. <br>
--! This library is free software; you can redistribute it and/or
--! modify it under the terms of the GNU Lesser General Public
--! License as published by the Free Software Foundation; either
--! version 3.0 of the License, or (at your option) any later version.
--! This library is distributed in the hope that it will be useful,
--! but WITHOUT ANY WARRANTY; without even the implied warranty of
--! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
--! Lesser General Public License for more details.<br>
--! You should have received a copy of the GNU Lesser General Public
--! License along with this library.
--! 
-- 
--! @brief ieee



library ieee, UNISIM;
use ieee.numeric_std.all;
use UNISIM.VCOMPONENTS.all;
use ieee.std_logic_unsigned.all; -- @suppress "Deprecated package"
use ieee.std_logic_1164.all;

use work.pcie_package.all;

entity felix_top_bnl711 is
  generic(
    NUMBER_OF_INTERRUPTS            : integer := 8;
    NUMBER_OF_DESCRIPTORS           : integer := 2;
    APP_CLK_FREQ                    : integer := 200;
    BUILD_DATETIME                  : std_logic_vector(39 downto 0) := x"DEADBEEF00";
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
    COMMIT_DATETIME                 : std_logic_vector(39 downto 0) := x"DEADBEEF01";
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
    Q4_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q4_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    Q5_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q5_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    Q6_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q6_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    Q8_CLK0_GTREFCLK_PAD_N_IN : in     std_logic;
    Q8_CLK0_GTREFCLK_PAD_P_IN : in     std_logic;
    SCL                       : inout  std_logic;
    SDA                       : inout  std_logic;
    SHPC_INT                  : out    std_logic;
    SI5345_A                  : out    std_logic_vector(1 downto 0);
    SI5345_INSEL              : out    std_logic_vector(1 downto 0);
    SI5345_OE                 : out    std_logic;
    SI5345_SEL                : out    std_logic;
    SI5345_nLOL               : in     std_logic;
--    RX_N                      : in     std_logic_vector(GBT_NUM-1 downto 0);
--    RX_P                      : in     std_logic_vector(GBT_NUM-1 downto 0);
--    TX_N                      : out    std_logic_vector(GBT_NUM-1 downto 0);
--    TX_P                      : out    std_logic_vector(GBT_NUM-1 downto 0);
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
    clk40_ttc_ref_out_n       : out    std_logic; -- Towards Si5345 CLKIN
    clk40_ttc_ref_out_p       : out    std_logic; -- Towards Si5345 CLKIN
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
    leds                      : out std_logic_vector(7 downto 0)
);
end entity felix_top_bnl711;

architecture structure of felix_top_bnl711 is

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
  
begin

    leds <= pcie0_register_map_control.STATUS_LEDS;

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
        clk250_out => open,
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
      clk_ttcfx_ref_out_n  => clk40_ttc_ref_out_n,
      clk_ttcfx_ref_out_p  => clk40_ttc_ref_out_p,
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
    port map(
      rst_lmk => '0',
      hw_rst => rst_hw,
      LMK_locked => LMK_locked(0),
      clk40m_in => clk40_xtal,
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

end architecture structure ; -- of felix_top_bnl711
