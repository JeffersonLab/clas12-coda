----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 17.10.2016 20:26:10
-- Design Name: 
-- Module Name: phy_wrapper - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

library UNISIM;
use UNISIM.VComponents.all;

entity phy_wrapper is
    port ( 
        -- Reset Logic (50 MHz) --
        clk   : in STD_LOGIC;
        reset : in STD_LOGIC;
        phy_ready : out std_logic;
        
        -- MGT Reference Clock (400 MHz) --
        mgt_ref_clk_p : in STD_LOGIC;
        mgt_ref_clk_n : in STD_LOGIC;
        
        -- SFP TX --
        gth_tx_n : out std_logic;
        gth_tx_p : out std_logic;
        
        -- SFP RX --
        gth_rx_n : in std_logic;
        gth_rx_p : in std_logic;
        
        -- TX Output --
        tx_clk       : out STD_LOGIC;
        tx_data      : in STD_LOGIC_VECTOR(15 downto 0);
        tx_kcode     : in STD_LOGIC_VECTOR(1 downto 0);
        tx_disparity : in std_logic_vector(15 downto 0);
        tx_is_locked : out std_logic;
        
        -- RX Input --
        rx_clk             : out STD_LOGIC;
        rx_data            : out STD_LOGIC_VECTOR(15 downto 0);
        rx_kcode           : out STD_LOGIC_VECTOR(1 downto 0);
        rx_disparity_error : out std_logic_vector(1 downto 0);
        rx_comma_char      : out std_logic_vector(1 downto 0);
        rx_invalid_char    : out std_logic_vector(1 downto 0);
        rx_is_locked       : out std_logic;
        rx_is_byte_aligned : out std_logic
    );
end phy_wrapper;

architecture Behavioral of phy_wrapper is

    signal rxusrclk_out, rxusrclk_out_i : std_logic;
    signal txusrclk_out, txusrclk_out_i : std_logic;

    signal reset_out : std_logic_vector(5 downto 0);
    signal rxbufstatus_out :  std_logic_vector(2 downto 0);
    signal rxbyteisaligned_out :  std_logic;
    signal rxbyterealign_out :  std_logic;
    signal rxclkcorcnt_out :  std_logic_vector(1 downto 0);
    signal rxcommadet_out :  std_logic;
    signal rxctrl0_out :  std_logic_vector(15 downto 0);
    signal rxctrl1_out :  std_logic_vector(15 downto 0);
    signal rxctrl2_out :  std_logic_vector(7 downto 0);
    signal rxctrl3_out :  std_logic_vector(7 downto 0);
    signal txctrl0_in : std_logic_vector(15 downto 0);
    signal txctrl1_in : std_logic_vector(15 downto 0);
    signal txctrl2_in : std_logic_vector(7 downto 0);
    signal tx_prbs_sel : std_logic_vector(2 downto 0);

    signal mgtrefclk1_x0y3_int : std_logic;
    
    signal tx_ready, rx_ready, rx_is_byte_aligned_i : std_logic;
    
begin

--    txctrl0_in <= b"00000000" & tx_disparity(7 downto 0);
--    txctrl1_in <= b"00000000" & tx_disparity(15 downto 8);
--    txctrl2_in <= b"000000" & tx_kcode;

--    rx_kcode           <= rxctrl0_out(1 downto 0);
--    rx_disparity_error <= rxctrl1_out(1 downto 0);
--    rx_comma_char      <= rxctrl2_out(1 downto 0);
--    rx_invalid_char    <= rxctrl3_out(1 downto 0);

    tx_clk <= txusrclk_out;
    rx_clk <= rxusrclk_out;

    rx_is_locked <= rx_ready and rx_is_byte_aligned_i; --reset_out(3);
    tx_is_locked <= tx_ready; --reset_out(2);
    rx_is_byte_aligned <= rx_is_byte_aligned_i;
    
--    IBUFDS_GTE3_MGTREFCLK1_X0Y3_INST: IBUFDS_GTE3 
--    port map (
--        I     => mgt_ref_clk_p, 
--        IB    => mgt_ref_clk_n,
--        CEB   => '0',
--        O     => mgtrefclk1_x0y3_int,
--        ODIV2 => open
--    );

    phy_ready <= tx_ready and rx_ready;
    
    gtwizard_i : entity work.gtwizard_0 
    port map (
        SOFT_RESET_TX_IN               => reset,       --  : in   std_logic;
        SOFT_RESET_RX_IN               => reset,       --  : in   std_logic;
        DONT_RESET_ON_DATA_ERROR_IN    => '0',       --  : in   std_logic;
        Q0_CLK0_GTREFCLK_PAD_N_IN      => mgt_ref_clk_n,         --  : in   std_logic;
        Q0_CLK0_GTREFCLK_PAD_P_IN      => mgt_ref_clk_p,       --  : in   std_logic;
    
        GT0_TX_FSM_RESET_DONE_OUT     => tx_ready, --         : out  std_logic;
        GT0_RX_FSM_RESET_DONE_OUT     => rx_ready,    --      : out  std_logic;
        GT0_DATA_VALID_IN             => '1',     --     : in   std_logic;
       -- GT0_TX_MMCM_LOCK_OUT          => open,      --    : out  std_logic;
     
        GT0_TXUSRCLK_OUT              => open,          --: out  std_logic;
        GT0_TXUSRCLK2_OUT             => txusrclk_out_i,          --: out  std_logic;
        GT0_RXUSRCLK_OUT              => open,          --: out  std_logic;
        GT0_RXUSRCLK2_OUT             => rxusrclk_out_i,          --: out  std_logic;
    
        --_________________________________________________________________________
        --GT0  (X0Y3)
        --____________________________CHANNEL PORTS________________________________
        ---------------------------- Channel - DRP Ports  --------------------------
        gt0_drpaddr_in            => (others => '0'),     --         : in   std_logic_vector(8 downto 0);
        gt0_drpdi_in              => (others => '0'),     --         : in   std_logic_vector(15 downto 0);
        gt0_drpdo_out             => open,          --    : out  std_logic_vector(15 downto 0);
        gt0_drpen_in              => '0',           --   : in   std_logic;
        gt0_drprdy_out            => open,          --    : out  std_logic;
        gt0_drpwe_in              => '0',           --   : in   std_logic;
        ------------------------------- Loopback Ports -----------------------------
        --gt0_loopback_in           => "000", --              : in   std_logic_vector(2 downto 0);
        ------------------------------ Power-Down Ports ----------------------------
        --gt0_rxpd_in               => "00",         --     : in   std_logic_vector(1 downto 0);
        --gt0_txpd_in               => "00",         --     : in   std_logic_vector(1 downto 0);
        --------------------- RX Initialization and Reset Ports --------------------
        gt0_eyescanreset_in          => '0',      --     : in   std_logic;
        gt0_rxuserrdy_in             => '1', --reset,    --       : in   std_logic;
        -------------------------- RX Margin Analysis Ports ------------------------
        gt0_eyescandataerror_out       => open, --         : out  std_logic;
        gt0_eyescantrigger_in          => '0', --         : in   std_logic;
        ------------------ Receive Ports - FPGA RX Interface Ports -----------------
        gt0_rxdata_out                 => rx_data, --         : out  std_logic_vector(15 downto 0);
--        gt0_rxpolarity_in              => '1',
        ------------------- Receive Ports - Pattern Checker Ports ------------------
        --gt0_rxprbserr_out                => open, --       : out  std_logic;
        --gt0_rxprbssel_in                 => (others => 'X'),  --     : in   std_logic_vector(2 downto 0);
        ------------------- Receive Ports - Pattern Checker ports ------------------
        --gt0_rxprbscntreset_in            => '0', --       : in   std_logic;
        ------------------ Receive Ports - RX 8B/10B Decoder Ports -----------------
        gt0_rxchariscomma_out           => rx_comma_char, --        : out  std_logic_vector(1 downto 0);
        gt0_rxcharisk_out               => rx_kcode, --        : out  std_logic_vector(1 downto 0);
        gt0_rxdisperr_out               => rx_disparity_error, --        : out  std_logic_vector(1 downto 0);
        gt0_rxnotintable_out            => rx_invalid_char, --        : out  std_logic_vector(1 downto 0);
        ------------------------ Receive Ports - RX AFE Ports ----------------------
        gt0_gtprxn_in                   => gth_rx_n, --        : in   std_logic;
        gt0_gtprxp_in                   => gth_rx_p, --        : in   std_logic;
        -------------- Receive Ports - RX Byte and Word Alignment Ports ------------
        gt0_rxbyteisaligned_out         => rx_is_byte_aligned_i, --       : out  std_logic;
        gt0_rxbyterealign_out           => open, --       : out  std_logic;
        gt0_rxcommadet_out              => open, --        : out  std_logic;
        gt0_rxmcommaalignen_in          => '1', --        : in   std_logic;
        gt0_rxpcommaalignen_in          => '1', --        : in   std_logic;
        ------------ Receive Ports - RX Decision Feedback Equalizer(DFE) -----------
        gt0_dmonitorout_out            => open, --         : out  std_logic_vector(14 downto 0);
        -------------------- Receive Ports - RX Equailizer Ports -------------------
        gt0_rxlpmhfhold_in             => '0', --     : in   std_logic;
        gt0_rxlpmhfovrden_in           => '0', --         : in   std_logic;
        gt0_rxlpmlfhold_in             => '0', --         : in   std_logic;
        --------------- Receive Ports - RX Fabric Output Control Ports -------------
        gt0_rxoutclkfabric_out         => open, --         : out  std_logic;
        ------------- Receive Ports - RX Initialization and Reset Ports ------------
        gt0_gtrxreset_in               => reset, --         : in   std_logic;
        gt0_rxlpmreset_in              => reset, --         : in   std_logic;
        -------------- Receive Ports -RX Initialization and Reset Ports ------------
        gt0_rxresetdone_out            => open, --         : out  std_logic;
        ------------------------ TX Configurable Driver Ports ----------------------
      --  gt0_txpostcursor_in            => (others => 'X'), --         : in   std_logic_vector(4 downto 0);
      --  gt0_txprecursor_in             => (others => 'X'), --         : in   std_logic_vector(4 downto 0);
        --------------------- TX Initialization and Reset Ports --------------------
        gt0_gttxreset_in               => reset, --         : in   std_logic;
        gt0_txuserrdy_in               => '1', --         : in   std_logic;
        ------------------ Transmit Ports - FPGA TX Interface Ports ----------------
        gt0_txdata_in                  => tx_data, --         : in   std_logic_vector(15 downto 0);
        --------------------- Transmit Ports - PCI Express Ports -------------------
        gt0_txelecidle_in             => 'X', --          : in   std_logic;
        ------------------ Transmit Ports - Pattern Generator Ports ----------------
        --gt0_txprbsforceerr_in           => '0', --        : in   std_logic;
        ------------------ Transmit Ports - TX 8B/10B Encoder Ports ----------------
        gt0_txchardispmode_in         => "00",         --: in   std_logic_vector(1 downto 0);
        gt0_txchardispval_in          => tx_disparity(1 downto 0),          --: in   std_logic_vector(1 downto 0);
        gt0_txcharisk_in              => tx_kcode, --          --: in   std_logic_vector(1 downto 0);
        --------------- Transmit Ports - TX Configurable Driver Ports --------------
        gt0_gtptxn_out               => gth_tx_n, --           : out  std_logic;
        gt0_gtptxp_out               => gth_tx_p, --           : out  std_logic;
        ----------- Transmit Ports - TX Fabric Clock Output Control Ports ----------
        gt0_txoutclkfabric_out       => open, --           : out  std_logic;
        gt0_txoutclkpcs_out          => open,  --         : out  std_logic;
        ------------- Transmit Ports - TX Initialization and Reset Ports -----------
        gt0_txresetdone_out         => open, --            : out  std_logic;
        ------------------ Transmit Ports - pattern Generator Ports ----------------
        --gt0_txprbssel_in           => "00", --             : in   std_logic_vector(2 downto 0);
    
        --____________________________COMMON PORTS________________________________
        GT0_PLL0RESET_OUT  => open,
        GT0_PLL0OUTCLK_OUT  => open,
        GT0_PLL0OUTREFCLK_OUT  => open,
        GT0_PLL0LOCK_OUT  => open,
        GT0_PLL0REFCLKLOST_OUT  => open,    
        GT0_PLL1OUTCLK_OUT  => open,
        GT0_PLL1OUTREFCLK_OUT  => open,
        
        --gt0_txprbssel_in => tx_prbs_sel,
        
        sysclk_in => clk   
    );
   
--   vio_i : entity work.vio_0
--    port map (
--    clk => clk,
--    probe_in0(0) => '0',
--    probe_out0 => tx_prbs_sel
--    );
     
     txclk_bufg : BUFG 
     port map( 
        I => txusrclk_out_i, 
        O => txusrclk_out
     );
     
     rxclk_bufg : BUFG
     port map( 
        I => rxusrclk_out_i, 
        O => rxusrclk_out
     );

end Behavioral;
