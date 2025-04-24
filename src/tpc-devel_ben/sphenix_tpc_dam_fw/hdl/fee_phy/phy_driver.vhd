----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 18.10.2016 00:12:39
-- Design Name: 
-- Module Name: phy_driver - Behavioral
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
use work.phy_pkg.all;
use work.gtm_pkg;

entity phy_driver is
    port (
        -- Reset Logic --
        reset_clk   : in STD_LOGIC;
        reset_in : in STD_LOGIC;
        reset_out : out std_logic;
        
        -- MGT Reference Clock --
        mgt_ref_clk : in STD_LOGIC;
        
        -- SFP TX --
        gth_tx_n : out std_logic;
        gth_tx_p : out std_logic;
        
        -- SFP RX --
        gth_rx_n : in std_logic;
        gth_rx_p : in std_logic;
        
        -- RX Data --
        rx_data_clk   : in std_logic;
        rx_data_avail : out std_logic;
        rx_data_rden  : in std_logic;
        rx_data_out   : out std_logic_vector(15 downto 0);
        rx_data_cnt   : out std_logic_vector(9 downto 0);
        rx_data_reset : out std_logic;

        -- RX Stream Data --
        stream_clk   : in std_logic;
        stream_rden  : in  std_logic;
        stream_data  : out std_logic_vector(15 downto 0);
        stream_cnt   : out std_logic_vector(9 downto 0);
        stream_avail : out std_logic;
        stream_enable : in std_logic;
        stream_reset  : in std_logic;
        
        -- TX Data --
        tx_data_clk  : in std_logic;
        tx_data_wren : in std_logic;
        tx_data_in   : in std_logic_vector(16 downto 0);
        tx_data_cnt  : out std_logic_vector(9 downto 0);
        tx_data_full : out std_logic;
       
        -- Tx Trigger --
        timing : in gtm_pkg.gtm_recvr_out_rec;
        
        -- Broadcast Interface --
        tx_broadcast : in broadcast_recv_rec;
       
        -- Stats --
        counters : out phy_stats_rec;
    
        rx_clk_out : out std_logic;

        -- Status --
        status : out std_logic_vector(1 downto 0)
    );
end phy_driver;

architecture Behavioral of phy_driver is

    signal rx_clk             : std_logic;
    signal rx_data            : std_logic_vector(15 downto 0);
    signal rx_kcode           : std_logic_vector(1 downto 0);
    signal rx_disparity_error : std_logic_vector(1 downto 0);
    signal rx_comma_char      : std_logic_vector(1 downto 0);
    signal rx_invalid_char    : std_logic_vector(1 downto 0);
    signal rx_is_byte_aligned : std_logic;
    signal rx_is_locked       : std_logic;

    signal tx_clk       : STD_LOGIC;
    signal tx_data      : STD_LOGIC_VECTOR(15 downto 0);
    signal tx_kcode     : STD_LOGIC_VECTOR(1 downto 0);
    signal tx_disparity : std_logic_vector(15 downto 0);
    signal tx_is_locked : std_logic;
    
    signal stats : phy_stats_rec;
    
    attribute mark_debug : string;
    attribute mark_debug of rx_data : signal is "true";
    attribute mark_debug of rx_kcode : signal is "true";
    attribute mark_debug of rx_disparity_error : signal is "true";
    attribute mark_debug of rx_comma_char : signal is "true";
    attribute mark_debug of rx_invalid_char : signal is "true";
    attribute mark_debug of rx_is_byte_aligned : signal is "true";    
    attribute mark_debug of rx_is_locked : signal is "true";    
    attribute mark_debug of tx_data : signal is "true";  
    attribute mark_debug of tx_kcode : signal is "true";  
    attribute mark_debug of tx_is_locked : signal is "true";
    attribute mark_debug of stream_avail : signal is "true";
    attribute mark_debug of stream_rden : signal is "true";
    attribute mark_debug of stream_data : signal is "true";

begin

    status(0) <= tx_is_locked;
    status(1) <= rx_is_locked;
    
    counters <= stats;

    rx_clk_out <= rx_clk;
    
    phy_wrapper_0 : entity work.phy_wrapper
    port map ( 
        -- Reset Logic --
        clk   => reset_clk,
        reset => reset_in,

        -- MGT Reference Clock --
        mgt_ref_clk => mgt_ref_clk,
        
        -- SFP TX --
        gth_tx_n => gth_tx_n,
        gth_tx_p => gth_tx_p,
        
        -- SFP RX --
        gth_rx_n => gth_rx_n,
        gth_rx_p => gth_rx_p,
        
        -- TX Output --
        tx_clk       => tx_clk,
        tx_data      => tx_data,
        tx_kcode     => tx_kcode,
        tx_disparity => tx_disparity,
        tx_is_locked => tx_is_locked,
        
        -- RX Input --
        rx_clk             => rx_clk, 
        rx_data            => rx_data,
        rx_kcode           => rx_kcode, 
        rx_disparity_error => rx_disparity_error,
        rx_comma_char      => rx_comma_char,
        rx_invalid_char    => rx_invalid_char,
        rx_is_locked       => rx_is_locked,
        rx_is_byte_aligned => rx_is_byte_aligned
    );

    tx_data_link_i : entity work.tx_data_link
    port map (
        reset => reset_in,

        -- PHY Encoder --
        tx_clk       => tx_clk,
        tx_data      => tx_data,
        tx_kcode     => tx_kcode,
        tx_disparity => tx_disparity,
        tx_is_locked => tx_is_locked,

        -- Tx FIFO --
        tx_data_clk  => tx_data_clk,  
        tx_data_wren => tx_data_wren, 
        tx_data_in   => tx_data_in, 
        tx_data_cnt  => tx_data_cnt,
        tx_data_full => tx_data_full, 
       
        -- Tx Trigger --
        timing => timing,
        
        tx_broadcast => tx_broadcast
    );

    rx_data_link_i : entity work.rx_data_link
    port map (
        reset => reset_in,

        -- PHY Decoder --
        rx_clk             => rx_clk, 
        rx_data            => rx_data,
        rx_kcode           => rx_kcode, 
        rx_disparity_error => rx_disparity_error,
        rx_comma_char      => rx_comma_char,
        rx_invalid_char    => rx_invalid_char,
        rx_is_locked       => rx_is_locked,
        rx_is_byte_aligned => rx_is_byte_aligned,

        -- RX Data --
        rx_data_clk   => rx_data_clk,
        rx_data_avail => rx_data_avail,
        rx_data_rden  => rx_data_rden,
        rx_data_out   => rx_data_out,
        rx_data_cnt   => rx_data_cnt,
        rx_data_reset => rx_data_reset,

        -- RX Stream Data --
        stream_clk   => stream_clk,
        stream_rden  => stream_rden,
        stream_data  => stream_data,
        stream_cnt   => stream_cnt,
        stream_avail => stream_avail,
        stream_enable => stream_enable,
        stream_reset  => stream_reset,
        
        counters => stats
    );

end Behavioral;
