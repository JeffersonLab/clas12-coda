
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;

entity phy_io is
    port (
        -- Reset Logic --
        reset_clk : in std_logic;
        reset_in  : in std_logic;
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
        
        -- Register Interface --
        reg_clk        :  in std_logic;
        reg_rst        :  in std_logic;
        reg_addr       : out std_logic_vector(15 downto 0);
        reg_read       : out std_logic;
        reg_write      : out std_logic;
        reg_data_write : out std_logic_vector(31 downto 0);
        reg_ack        :  in std_logic;
        reg_fail       :  in std_logic;
        reg_data_read  :  in std_logic_vector(31 downto 0);
        
        -- Data Encoder Interface --
        data_clk   : in std_logic;
        data_wr_en : in std_logic;
        data_sob   : in std_logic;       -- Start of Block
        data_eob   : in std_logic;       -- End of Block
        data_in    : in std_logic_vector(15 downto 0);
        
        -- Trigger Out --
        trigger_clk  : in std_logic;
        trigger_ack  : in std_logic;
        trigger_en   : in std_logic;
        trigger      : out std_logic;
        trigger_freq : out std_logic_vector(31 downto 0);
              
        gtm_recv : out gtm_recv_rec;
        
        broadcast_recv : out broadcast_recv_rec;
                
        tx_clk_out   : out std_logic;
        rx_clk_out   : out std_logic;
        
        tx_activity : out std_logic;
        rx_activity : out std_logic;
        tg_activity : out std_logic;
        
        -- Status --
        status : out std_logic_vector(1 downto 0)

    );
end phy_io;

architecture behv of phy_io is

     -- RX Data --
    signal    rx_data_clk   : std_logic;
    signal    rx_data_avail : std_logic;
    signal    rx_data_rden  : std_logic;
    signal    rx_data_out   : std_logic_vector(15 downto 0);
    signal    rx_data_cnt   : std_logic_vector(9 downto 0);
    signal    rx_data_reset : std_logic;

    -- TX Data --
    signal    tx_data_clk  : std_logic;
    signal    tx_data_wren : std_logic;
    signal    tx_data_in   : std_logic_vector(16 downto 0);
    signal    tx_data_cnt  : std_logic_vector(9 downto 0);
    signal    tx_data_full : std_logic;

    -- Stream Data --
    signal    stream_clk  : std_logic;
    signal    stream_wren : std_logic;
    signal    stream_data : std_logic_vector(16 downto 0);
    signal    stream_cnt  : std_logic_vector(9 downto 0);
    signal    stream_full : std_logic;

    signal trigger_clk_i  : std_logic;
    signal   trigger_i    : std_logic;
    
    signal tx_clk_out_i : std_logic;
    signal rx_clk_out_i : std_logic;

begin

    tx_clk_out <= tx_clk_out_i;

    rx_clk_out <= rx_clk_out_i;

    phy_driver_i : entity work.phy_driver
    port map (
        reset_clk => reset_clk,
        reset_in  => reset_in,
        reset_out => reset_out,
        
        -- MGT Reference Clock --
        mgt_ref_clk_p => mgt_ref_clk_p,
        mgt_ref_clk_n => mgt_ref_clk_n,
        
        -- SFP TX --
        gth_tx_n => gth_tx_n,
        gth_tx_p => gth_tx_p,
        
        -- SFP RX --
        gth_rx_n => gth_rx_n,
        gth_rx_p => gth_rx_p,
        
        -- RX Data --
        rx_data_clk   => rx_data_clk,
        rx_data_avail => rx_data_avail,
        rx_data_rden  => rx_data_rden,
        rx_data_out   => rx_data_out,
        rx_data_cnt   => rx_data_cnt,
        rx_data_reset => rx_data_reset,
        
        -- TX Data --
        tx_data_clk  => tx_data_clk,
        tx_data_wren => tx_data_wren,
        tx_data_in   => tx_data_in,
        tx_data_cnt  => tx_data_cnt,
        tx_data_full => tx_data_full,
        
        -- TX Streaming Data --
        stream_clk  => stream_clk,
        stream_wren => stream_wren,
        stream_data => stream_data,
        stream_cnt  => stream_cnt,
        stream_full => stream_full,
        
        gtm_recv => gtm_recv,
        
        broadcast_recv => broadcast_recv,
        
        tx_clk_out => tx_clk_out_i,
        rx_clk_out => rx_clk_out_i,
        
        tx_activity => tx_activity,
        rx_activity => rx_activity,
        
        -- Status --
        status => status
    );
    
    stream_tx_i : entity work.stream_tx
    port map (
        reset      => reg_rst,
        -- Data Encoder Interface --
        data_clk   => data_clk,
        data_wr_en => data_wr_en,
        data_sob   => data_sob,
        data_eob   => data_eob,
        data_in    => data_in,
        
        -- TX Streaming Data --
        stream_clk  => stream_clk,
        stream_wren => stream_wren,
        stream_data => stream_data,
        stream_cnt  => stream_cnt,
        stream_full => stream_full
    );

    register_slave_i : entity work.register_slave
    port map (
        -- Register Interface --
        reg_clk    => reg_clk,
        reg_rst    => reg_rst or rx_data_reset,
        reg_addr   => reg_addr,
        reg_read   => reg_read, 
        reg_write  => reg_write, 
        reg_data_write => reg_data_write,
        reg_ack    => reg_ack,
        reg_fail   => reg_fail,
        reg_data_read => reg_data_read,

        -- RX Data --
        rx_data_clk   => rx_data_clk,
        rx_data_avail => rx_data_avail,
        rx_data_rden  => rx_data_rden,
        rx_data_out   => rx_data_out,
        rx_data_cnt   => rx_data_cnt,
        
        -- TX Data --
        tx_data_clk  => tx_data_clk,
        tx_data_wren => tx_data_wren,
        tx_data_in   => tx_data_in,
        tx_data_cnt  => tx_data_cnt,
        tx_data_full => tx_data_full
    );
    
    trigger_rx_i : entity work.trigger_rx
    port map ( 
            clk          => trigger_clk,
            reset        => reset_in,
            en           => trigger_en,
            trigger_ack  => trigger_ack,
            trigger      => trigger,
            pulse        => trigger_i,
            freq         => trigger_freq
    );

end behv;
