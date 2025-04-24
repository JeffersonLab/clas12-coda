library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;
use work.gtm_pkg;

entity tx_data_link is
    port ( 
        -- Async Reset --
        reset        : in std_logic;

        -- TX PHY Encoder --
        tx_clk       : in STD_LOGIC;
        tx_data      : out STD_LOGIC_VECTOR(15 downto 0);
        tx_kcode     : out STD_LOGIC_VECTOR(1 downto 0);
        tx_disparity : out std_logic_vector(15 downto 0);
        tx_is_locked : in std_logic;

        -- User Side FIFO --
        tx_data_clk  : in std_logic;
        tx_data_wren : in std_logic;
        tx_data_in   : in std_logic_vector(16 downto 0);
        tx_data_cnt  : out std_logic_vector(9 downto 0);
        tx_data_full : out std_logic;

        -- GTM Interface --
        timing : in gtm_pkg.gtm_recvr_out_rec;
        
        -- Broadcast Interface --
        tx_broadcast : in broadcast_recv_rec
    );
end tx_data_link;

architecture behv of tx_data_link is
    type ACTIVITY_STATE_T is (IDLE, READ_REGISTER_FIFO, WAIT_FOR_REGISTER_DATA);
    signal act_state : ACTIVITY_STATE_T;

    signal count : std_logic_vector(7 downto 0);
    
    signal gtm_timing_0 : gtm_pkg.gtm_recvr_out_rec;
    signal gtm_timing   : gtm_pkg.gtm_recvr_out_rec;
    
    signal broadcast_0, broadcast_tx : broadcast_recv_rec;
    
    signal tx_empty_i : std_logic;
    signal tx_fifo_data : std_logic_vector(16 downto 0);
    signal tx_fifo_rden : std_logic;
    signal tx_fifo_avail : std_logic;
    
    --attribute mark_debug : string;
    --attribute mark_debug of act_state : signal is "true";
    --attribute mark_debug of tx_data : signal is "true";
    --attribute mark_debug of tx_kcode : signal is "true";
    --attribute mark_debug of tx_disparity : signal is "true";
    --attribute mark_debug of tx_is_locked : signal is "true";
    --attribute mark_debug of tx_fifo_avail : signal is "true";
    --attribute mark_debug of tx_fifo_rden : signal is "true";
    --attribute mark_debug of tx_fifo_data : signal is "true";

begin
    tx_fifo_0 : entity work.blkram_fifo_17b
    port map (
        srst => reset,
        
        wr_clk => tx_data_clk,
        wr_en  => tx_data_wren,
        din    => tx_data_in,
        full   => tx_data_full,

        rd_clk => tx_clk,
        rd_en  => tx_fifo_rden,
        dout   => tx_fifo_data,
        empty  => tx_empty_i,
        wr_rst_busy => open,
        rd_rst_busy => open
    );    
    
    tx_fifo_avail <= not tx_empty_i;
       
          
    process(tx_clk, tx_is_locked, reset)
        procedure tx_send_data(constant data_upper : std_logic_vector(7 downto 0);
                                constant data_lower : std_logic_vector(7 downto 0);
                                constant upper_isK : std_logic;
                                constant lower_isK : std_logic) is
        begin
                tx_data <= data_upper & data_lower;
                tx_kcode <= upper_isK & lower_isK;
        end procedure;

    begin
        if (reset = '1' or tx_is_locked = '0') then
            count <= (others => '0');
            tx_data <= (others => '0');
            tx_kcode <= (others => '0');
            act_state <= IDLE;
            tx_fifo_rden <= '0';
        elsif (rising_edge(tx_clk)) then           
            case act_state is
                when IDLE =>
                    tx_send_data(D_16_2_C, K_28_5_C, '0', '1');

                    if (gtm_timing.modebits_val = '1' or gtm_timing.lvl1_accept(0) = '1') then
                        tx_send_data(gtm_timing.lvl1_accept(0) &
                                     gtm_timing.fem_endat & 
                                     gtm_timing.fem_user_bits(1 downto 0) & 
                                     gtm_timing.modebits(2 downto 0), 
                                     K_28_2_C, '0', '1');
                    elsif (tx_fifo_avail = '1') then
                        tx_fifo_rden <= '1';
                        if (tx_fifo_data = '1' & x"FEED") then
                            tx_send_data(TX_START_OF_REG_C, K_28_5_C, '0', '1');
                        end if;
                        act_state <= WAIT_FOR_REGISTER_DATA;
                    end if;
                    
                when READ_REGISTER_FIFO =>
                    if (tx_fifo_avail = '0') then
                        tx_fifo_rden <= '0';
                        tx_send_data(TX_WAIT_FOR_REG_C, K_28_5_C, '0', '1');
                        act_state <= WAIT_FOR_REGISTER_DATA;
                    else
                        tx_fifo_rden <= '1';
                        if (tx_fifo_data = '1' & x"FACE") then
                            tx_fifo_rden <= '0';
                            tx_send_data(TX_END_OF_REG_C, K_28_5_C, '0', '1');
                            act_state <= IDLE;
                        else
                            tx_data <= tx_fifo_data(15 downto 0);
                            tx_kcode <= b"00";
                        end if;
                    end if;
                    
                when WAIT_FOR_REGISTER_DATA =>
                    tx_send_data(TX_WAIT_FOR_REG_C, K_28_5_C, '0', '1');
                    if (tx_fifo_avail = '1') then
                        tx_fifo_rden <= '1';
                        act_state <= READ_REGISTER_FIFO;
                    end if;
            end case;
        end if;
    end process;
    
    process(tx_clk)
    begin
        if (rising_edge(tx_clk)) then
            gtm_timing_0 <= timing;
            gtm_timing <= gtm_timing_0;
            
            broadcast_0 <= tx_broadcast;
            broadcast_tx <= broadcast_0;
        end if;
    end process;
          
end behv;
