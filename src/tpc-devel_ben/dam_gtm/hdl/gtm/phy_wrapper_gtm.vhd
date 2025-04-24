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

entity phy_gtm_wrapper is
    port ( 
        -- Reset Logic (50 MHz) --
        clk   : in STD_LOGIC;
        reset : in STD_LOGIC;
        
        -- MGT Reference Clock (400 MHz) --
        mgt_ref_clk : in STD_LOGIC;
        
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
end phy_gtm_wrapper;

architecture behv of phy_gtm_wrapper is

    component gtwizard_ultrascale_gtm_0 is 
    port (
        signal gtwiz_userclk_tx_active_in : in std_logic_vector(0 downto 0);
        signal gtwiz_userclk_rx_active_in : in std_logic_vector(0 downto 0);
        
        signal gtwiz_reset_clk_freerun_in : in std_logic_vector(0 downto 0);
        signal gtwiz_reset_all_in : in std_logic_vector(0 downto 0);
        signal gtwiz_reset_tx_pll_and_datapath_in : in std_logic_vector(0 downto 0);
        signal gtwiz_reset_tx_datapath_in : in std_logic_vector(0 downto 0);
        signal gtwiz_reset_rx_pll_and_datapath_in : in std_logic_vector(0 downto 0);
        signal gtwiz_reset_rx_datapath_in : in std_logic_vector(0 downto 0);
        signal gtwiz_reset_rx_cdr_stable_out : out std_logic_vector(0 downto 0);
        signal gtwiz_reset_tx_done_out : out std_logic_vector(0 downto 0);
        signal gtwiz_reset_rx_done_out : out std_logic_vector(0 downto 0);
        
        signal gtwiz_userdata_tx_in : in std_logic_vector(15 downto 0);
        signal gtwiz_userdata_rx_out : out std_logic_vector(15 downto 0);
        
        signal gtrefclk0_in : std_logic_vector(0 downto 0);
        signal cpllreset_in : std_logic_vector(0 downto 0);
        --signal gtrefclk01_in : in std_logic_vector(0 downto 0);
        --signal qpll1outclk_out : out std_logic_vector(0 downto 0);
        --signal qpll1outrefclk_out : out std_logic_vector(0 downto 0);
        
        signal gthrxn_in : in std_logic_vector(0 downto 0);
        signal gthrxp_in : in std_logic_vector(0 downto 0);
        signal rx8b10ben_in : in std_logic_vector(0 downto 0);
      --  signal rxbufreset_in : in std_logic_vector(0 downto 0);
        signal rxcommadeten_in : in std_logic_vector(0 downto 0);
        signal rxmcommaalignen_in : in std_logic_vector(0 downto 0);
        signal rxpcommaalignen_in : in std_logic_vector(0 downto 0);
        signal rxusrclk_in : in std_logic_vector(0 downto 0);
        signal rxusrclk2_in : in std_logic_vector(0 downto 0);
        signal tx8b10ben_in : in std_logic_vector(0 downto 0);
        signal txctrl0_in : in std_logic_vector(15 downto 0);
        signal txctrl1_in : in std_logic_vector(15 downto 0);
        signal txctrl2_in : in std_logic_vector(7 downto 0);
        signal txusrclk_in : in std_logic_vector(0 downto 0);
        signal txusrclk2_in : in std_logic_vector(0 downto 0);
        signal gthtxn_out : out std_logic_vector(0 downto 0);
        signal gthtxp_out : out std_logic_vector(0 downto 0);
        --signal rxbufstatus_out : out std_logic_vector(2 downto 0);
        signal rxbyteisaligned_out : out std_logic_vector(0 downto 0);
        signal rxbyterealign_out : out std_logic_vector(0 downto 0);
     --   signal rxclkcorcnt_out : out std_logic_vector(1 downto 0);
        signal rxcommadet_out : out std_logic_vector(0 downto 0);
        signal rxctrl0_out : out std_logic_vector(15 downto 0);
        signal rxctrl1_out : out std_logic_vector(15 downto 0);
        signal rxctrl2_out : out std_logic_vector(7 downto 0);
        signal rxctrl3_out : out std_logic_vector(7 downto 0);
        signal rxoutclk_out : out std_logic_vector(0 downto 0);
        signal rxpmaresetdone_out : out std_logic_vector(0 downto 0);
        signal txoutclk_out : out std_logic_vector(0 downto 0);
       -- signal qpll1lock_out : out std_logic_vector(0 downto 0);
       -- signal qpll1refclklost_out  : out std_logic_vector(0 downto 0);
       
        signal rxprbserr_out : out std_logic_vector(0 downto 0);
        signal rxprbslocked_out : out std_logic_vector(0 downto 0);
        signal rxprbscntreset_in : in std_logic_vector(0 downto 0);
        signal rxprbssel_in : in std_logic_vector(3 downto 0);

        signal txprbssel_in : in std_logic_vector(3 downto 0);
        signal txprbsforceerr_in : in std_logic_vector(0 downto 0);

        signal RXELECIDLEMODE_in : std_logic_vector(1 downto 0);

       signal cplllock_out, cpllrefclklost_out, rxcdrlock_out : out std_logic_vector(0 downto 0);       
        signal txpmaresetdone_out : out std_logic_vector(0 downto 0);
        signal drpclk_in : in std_logic_vector(0 downto 0)
    );
    end component;

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

    signal mgtrefclk1_x0y3_int : std_logic;
    signal cplllock_out, cpllrefclklost_out, rxcdrlock_out : std_logic;
    
    signal rxprbserr_out : std_logic_vector(0 downto 0);
    signal rxprbslocked_out : std_logic_vector(0 downto 0);
    signal rxprbscntreset_in : std_logic_vector(0 downto 0);
    signal rxprbssel_in : std_logic_vector(3 downto 0);

    signal reset_all_in, reset_tx_pll_and_datapath, reset_tx_datapath, reset_rx_pll_and_datapath, reset_rx_datapath : std_logic;
    
    --attribute mark_debug : string;
    --attribute mark_debug of cplllock_out : signal is "true";
    --attribute mark_debug of cpllrefclklost_out : signal is "true";
    --attribute mark_debug of rxcdrlock_out : signal is "true";

begin

    txctrl0_in <= b"00000000" & tx_disparity(7 downto 0);
    txctrl1_in <= b"00000000" & tx_disparity(15 downto 8);
    txctrl2_in <= b"000000" & tx_kcode;

    rx_kcode           <= rxctrl0_out(1 downto 0);
    rx_disparity_error <= rxctrl1_out(1 downto 0);
    rx_comma_char      <= rxctrl2_out(1 downto 0);
    rx_invalid_char    <= rxctrl3_out(1 downto 0);

    tx_clk <= txusrclk_out;
    rx_clk <= rxusrclk_out;

    rx_is_locked <= reset_out(3) and cplllock_out;
    --rx_is_locked <= reset_out(3) and rxbyteisaligned_out and cplllock_out and rxcdrlock_out;
    tx_is_locked <= reset_out(2);
    rx_is_byte_aligned <= rxbyteisaligned_out;
    
--    IBUFDS_GTE3_MGTREFCLK1_X0Y3_INST: IBUFDS_GTE3 
--    port map (
--        I     => mgt_ref_clk_p, 
--        IB    => mgt_ref_clk_n,
--        CEB   => '0',
--        O     => mgtrefclk1_x0y3_int,
--        ODIV2 => open
--    );

--    vio_i: entity work.vio_0
--    PORT MAP (
--        clk => clk,
--        probe_in0(0) => rxbyteisaligned_out,
--        probe_in1(0) => reset_out(1),
--        probe_in2(0) => reset_out(2),
--        probe_in3(0) => reset_out(3),
--        probe_in4(0) => cplllock_out,
--        probe_out0(0) => reset_all_in,
--        probe_out1(0) => reset_tx_pll_and_datapath,
--        probe_out2(0) => reset_tx_datapath,
--        probe_out3(0) => reset_rx_pll_and_datapath,
--        probe_out4(0) => reset_rx_datapath
--    );
    
        
    gtwizard_ultrascale_gtm_i : gtwizard_ultrascale_gtm_0
    port map (
       
        gtwiz_userclk_tx_active_in(0) => txusrclk_out,
        gtwiz_userclk_rx_active_in(0) => rxusrclk_out,
        gtwiz_reset_clk_freerun_in(0) => clk,
        gtwiz_reset_all_in(0)  => reset or reset_all_in,
        gtwiz_reset_tx_pll_and_datapath_in(0) => reset or reset_tx_pll_and_datapath, 
        gtwiz_reset_tx_datapath_in(0) => reset or reset_tx_datapath,
        gtwiz_reset_rx_pll_and_datapath_in(0) => reset or reset_rx_pll_and_datapath,
        gtwiz_reset_rx_datapath_in(0) => reset or reset_rx_datapath,
        gtwiz_reset_rx_cdr_stable_out(0) => reset_out(1),
        gtwiz_reset_tx_done_out(0) => reset_out(2),
        gtwiz_reset_rx_done_out(0) => reset_out(3),
        
        gtwiz_userdata_tx_in  => tx_data,
        gtwiz_userdata_rx_out => rx_data,
        
        gtrefclk0_in(0) => mgt_ref_clk,
        cpllreset_in(0) => reset,
        cplllock_out(0) => cplllock_out,
        cpllrefclklost_out(0) => cpllrefclklost_out,
        rxcdrlock_out(0) => rxcdrlock_out,
        
        gthrxn_in(0) => gth_rx_n,
        gthrxp_in(0) => gth_rx_p,
                 
        rx8b10ben_in(0) => '1',
        rxcommadeten_in(0)  => '1',
        rxmcommaalignen_in(0) => '1',
        rxpcommaalignen_in(0)  => '1',
        rxusrclk_in(0)  => rxusrclk_out,
        rxusrclk2_in(0)  => rxusrclk_out,
        tx8b10ben_in(0)  => '1',
        txctrl0_in  => txctrl0_in,
        txctrl1_in  => txctrl1_in,
        txctrl2_in  => txctrl2_in,
        txusrclk_in(0) => txusrclk_out,
        txusrclk2_in(0)  => txusrclk_out,
        
        gthtxn_out(0) => gth_tx_n,
        gthtxp_out(0) => gth_tx_p,
                                 
        rxbyteisaligned_out(0)  => rxbyteisaligned_out,
        rxbyterealign_out(0)  => rxbyterealign_out,
        rxcommadet_out(0)     => rxcommadet_out,
        rxctrl0_out        => rxctrl0_out,
        rxctrl1_out        => rxctrl1_out,
        rxctrl2_out        => rxctrl2_out,
        rxctrl3_out        => rxctrl3_out,
        rxoutclk_out(0)       => rxusrclk_out_i,
        rxpmaresetdone_out(0) => reset_out(4),
        txoutclk_out(0)       => txusrclk_out_i,   
        
        rxprbserr_out     => rxprbserr_out,
        rxprbslocked_out  => rxprbslocked_out,
        rxprbscntreset_in => rxprbscntreset_in,
        rxprbssel_in      => rxprbssel_in,
        
        txprbssel_in      => rxprbssel_in,
        txprbsforceerr_in(0) => '0',
        txpmaresetdone_out(0) => reset_out(5),

        -- Fix byte-align issue?
        RXELECIDLEMODE_in => "11",

        drpclk_in(0) => clk
    );
    rxprbscntreset_in <= (others => '0');
    
--    vio_1 : entity work.vio_1
--    port map (
--    clk       => clk,
--    probe_in0 => rxprbserr_out,
--    probe_in1 => rxprbslocked_out,
--    probe_out0 => rxprbssel_in,
--    probe_out1 => rxprbscntreset_in
--    );

    --rxprbssel_in <= "0101";
    rxprbssel_in <= "0000";
    
     txclk_bufg : BUFG_GT 
     port map( 
        I => txusrclk_out_i, 
        CE => '1', 
        CEMASK => '1',
        clrmask => '1',
        CLR => '0',
        div => "000",
        O => txusrclk_out
     );
     
     rxclk_bufg : BUFG_GT 
     port map( 
        I => rxusrclk_out_i, 
        CE => '1', 
        CEMASK => '1',
        clrmask => '1',
        CLR => '0',
        div => "000",
        O => rxusrclk_out
     );

end behv;
