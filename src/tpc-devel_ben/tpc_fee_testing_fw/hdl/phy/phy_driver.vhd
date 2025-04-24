library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;

entity phy_driver is
    Port (
        -- Reset Logic --
        reset_clk : in STD_LOGIC;
        reset_in  : in STD_LOGIC;
        reset_out : out std_logic;
        
        -- MGT Reference Clock --
        mgt_ref_clk_p : in STD_LOGIC;
        mgt_ref_clk_n : in STD_LOGIC;
        
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
        
        -- TX Data --
        tx_data_clk  : in std_logic;
        tx_data_wren : in std_logic;
        tx_data_in   : in std_logic_vector(16 downto 0);
        tx_data_cnt  : out std_logic_vector(9 downto 0);
        tx_data_full : out std_logic;
        
        -- TX Stream Data --
        stream_clk   : in std_logic;
        stream_wren  : in std_logic;
        stream_data  : in std_logic_vector(16 downto 0);
        stream_cnt   : out std_logic_vector(9 downto 0);
        stream_full  : out std_logic;
        
        -- Trigger Out --
        gtm_recv      : out gtm_recv_rec;
        
        -- Broadcast Interface --
        broadcast_recv : out broadcast_recv_rec;
        
        tx_clk_out : out std_logic;
        rx_clk_out : out std_logic;
        
        tx_activity : out std_logic;
        rx_activity : out std_logic;
        
        -- Status --
        status : out std_logic_vector(1 downto 0)
    );
end phy_driver;

architecture behv of phy_driver is

    signal tx_clk : std_logic;
    signal tx_data : std_logic_vector(15 downto 0);
    signal tx_disparity : std_logic_vector(15 downto 0);
    signal tx_kcode : std_logic_vector(1 downto 0);
    signal tx_is_locked : std_logic;
    signal rx_is_locked : std_logic;
    
    signal phy_ready, reset : std_logic;
    
    signal rx_clk :  std_logic;
    signal rx_data :  std_logic_vector(15 downto 0);
    signal rx_kcode :  std_logic_vector(1 downto 0);
    signal rx_disparity_error :  std_logic_vector(1 downto 0);
    signal rx_comma_char : std_logic_vector(1 downto 0);
    signal rx_invalid_char : std_logic_vector(1 downto 0);
    signal rx_is_byte_aligned :  std_logic;
          
begin

    status(0) <= tx_is_locked;
    status(1) <= rx_is_locked;
    
    reset_out <= reset;
    
    tx_clk_out <= tx_clk;
    rx_clk_out <= rx_clk;
                    
    phy_wrapper_0 : entity work.phy_wrapper
    port map ( 
        -- Reset Logic --
        clk   => reset_clk,
        reset => reset_in,
        phy_ready => phy_ready,

        -- MGT Reference Clock --
        mgt_ref_clk_p => mgt_ref_clk_p,
        mgt_ref_clk_n => mgt_ref_clk_n,
        
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

        tx_activity => tx_activity,

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
        tx_data_full => tx_data_full,
        
        -- TX Stream Data --
        stream_clk   => stream_clk,
        stream_wren  => stream_wren,
        stream_data  => stream_data,
        stream_full  => stream_full
    );

    rx_data_link_i : entity work.rx_data_link
    port map (
        reset => reset_in,

        rx_activity => rx_activity,

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
        rx_reset      => rx_data_reset,
        
        -- GTM Interface --
        gtm_recv => gtm_recv,
        
        -- Broadcast Interface --
        broadcast_recv => broadcast_recv
    );
    
end behv;
