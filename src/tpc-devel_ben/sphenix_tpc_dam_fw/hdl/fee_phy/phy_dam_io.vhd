library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;
use work.gtm_pkg.all;

entity phy_io is
    port (
        -- Reset Logic --
        reset_clk : in std_logic;
        reset_in  : in std_logic;
        reset_out : out std_logic;
        
        -- MGT Reference Clock --
        mgt_ref_clk : in STD_LOGIC;
       
        -- SFP TX --
        gth_tx_n : out std_logic;
        gth_tx_p : out std_logic;
        
        -- SFP RX --
        gth_rx_n : in std_logic;
        gth_rx_p : in std_logic;
        
        -- Register Interface --
        reg_clk        :  in std_logic;
        reg_rst        :  in std_logic;
        reg_addr       :  in std_logic_vector(15 downto 0);
        reg_read       :  in std_logic;
        reg_write      :  in std_logic;
        reg_data_write :  in std_logic_vector(31 downto 0);
        reg_ack        :  out std_logic;
        reg_fail       :  out std_logic;
        reg_data_read  :  out std_logic_vector(31 downto 0);
        reg_addr_read  :  out std_logic_vector(15 downto 0);
        rx_ready       :  out std_logic;
        
        -- Streaming Data Interface --
        stream_clk     : in std_logic;
        stream_rden    : in std_logic;
        stream_data    : out std_logic_vector(15 downto 0);
        stream_count   : out std_logic_vector(9 downto 0);
        stream_avail   : out std_logic;
        stream_enable  : in  std_logic;
        stream_reset   : in std_logic;
        
        -- Trigger Interface --
        timing        : in gtm_recvr_out_rec;
        
        -- Broadcast Interface --
        tx_broadcast : in broadcast_recv_rec;
        
        counters : out phy_stats_rec;
        
        -- Status --
        status : out std_logic_vector(1 downto 0);

        rx_clk_out : out std_logic

    );
end phy_io;

architecture behv of phy_io is

     -- RX Streaming Data --
    --signal    stream_clk   : std_logic;
    --signal    stream_avail : std_logic;
    --signal    stream_rden  : std_logic;
  --  signal    stream_data  : std_logic_vector(15 downto 0);
   -- signal    stream_cnt   : std_logic_vector(9 downto 0);
   
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
    
    signal rx_clk : std_logic;

begin

    rx_ready <= not rx_data_reset;
    rx_clk_out <= rx_clk;

    phy_driver_i : entity work.phy_driver
    port map (
        reset_clk => reset_clk,
        reset_in  => reset_in,
        reset_out => reset_out,
        
        -- MGT Reference Clock --
        mgt_ref_clk => mgt_ref_clk,
        
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
                
        -- RX Streaming Data --
        stream_clk   => stream_clk,
        stream_rden  => stream_rden,
        stream_data  => stream_data,
        stream_cnt   => stream_count,
        stream_avail => stream_avail,
        stream_enable => stream_enable,
        stream_reset  => stream_reset,

        -- TX Trigger --
        timing   => timing,

        -- Broadcast Interface --
        tx_broadcast => tx_broadcast,

        rx_clk_out => rx_clk,

        counters  => counters,
        
        -- Status --
        status => status
    );

    register_master_i : entity work.register_master
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
        reg_addr_read => reg_addr_read,

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

end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;
use work.gtm_pkg.all;

library UNISIM;
use UNISIM.VCOMPONENTS.ALL;

entity fee_phys is
    port (
        -- Reset Logic --
        reset_clk : in std_logic;
        reset_in  : in std_logic;
        reset_out : out std_logic;
        
        -- MGT Reference Clock --
        mgt_ref_clk_p : in STD_LOGIC;
        mgt_ref_clk_n : in STD_LOGIC;
        
        -- SFP TX --
        gth_tx_n : out std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        gth_tx_p : out std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        
        -- SFP RX --
        gth_rx_n : in std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        gth_rx_p : in std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        
        -- Register Interface --
        reg_request : in  reg_request_t;
        reg_reply   : out reg_reply_t;
        
        -- Streaming Data Interface --
        stream      : out stream_t;
        stream_ctrl : in stream_ctrl_t;
        stream_en   : in std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        
        -- Trigger Interface --
        timing        : in gtm_recvr_out_rec;
        
        -- Broadcast Interface --
        tx_broadcast : in broadcast_recv_rec;
        
        -- Stats Counters --
        counters : out phy_stats_t;

        rx_clk_out : out std_logic;
        mgt_ref_clk_out : out std_logic;

        -- Locked Status --
        tx_locked : out std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        rx_locked : out std_logic_vector(NUMBER_OF_FEEs-1 downto 0)
    );
end fee_phys;

architecture behv of fee_phys is

    signal mgt_ref_clk : std_logic;
    signal rx_clk : std_logic_vector(NUMBER_OF_FEEs-1 downto 0);

begin

    rx_clk_out <= rx_clk(0);
    mgt_ref_clk_out <= mgt_ref_clk;

    IBUFDS_GTE3_MGTREFCLK1_X0Y3_INST : IBUFDS_GTE3 
    port map (
        I     => mgt_ref_clk_p, 
        IB    => mgt_ref_clk_n,
        CEB   => '0',
        O     => mgt_ref_clk,
        ODIV2 => open
    );

    PHY_LOOP : for i in 0 to NUMBER_OF_FEEs-1 generate
        fee_phy : entity work.phy_io
        port map (        
            reset_clk => reset_clk,
            reset_in  => reset_in,
            reset_out => open,
            
            -- MGT Reference Clock --
            mgt_ref_clk => mgt_ref_clk,
            
            -- SFP TX --
            gth_tx_n => gth_tx_n(i),
            gth_tx_p => gth_tx_p(i),
            
            -- SFP RX --
            gth_rx_n => gth_rx_n(i),
            gth_rx_p => gth_rx_p(i),
            
            -- Register Interface --
            reg_clk        => reg_request(i).reg_clk,
            reg_rst        => reg_request(i).reg_rst,
            reg_addr       => reg_request(i).reg_addr,
            reg_read       => reg_request(i).reg_read,
            reg_write      => reg_request(i).reg_write,
            reg_data_write => reg_request(i).reg_data_write,
            reg_ack        => reg_reply(i).reg_ack,
            reg_fail       => reg_reply(i).reg_fail,
            reg_data_read  => reg_reply(i).reg_data_read,
            reg_addr_read  => reg_reply(i).reg_addr_read,
            rx_ready       => reg_reply(i).rx_ready,
            
            -- Streaming Data Interface --
            stream_clk     => stream_ctrl(i).clk, 
            stream_rden    => stream_ctrl(i).read_en,
            stream_data    => stream(i).data, 
            stream_avail   => stream(i).avail,
            stream_count   => stream(i).count,
            stream_enable  => stream_en(i),
            stream_reset   => stream_ctrl(i).rst,
            
            timing => timing,
            
            tx_broadcast => tx_broadcast,
                    
            counters => counters(i),

            rx_clk_out => rx_clk(i),
            
            -- Status --
            status(0) => tx_locked(i), 
            status(1) => rx_locked(i) 
        );
    end generate PHY_LOOP;

end behv;

