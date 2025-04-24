library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;

entity rx_data_link is
    port (
        -- Async Reset --
        reset : in std_logic;

        -- PHY Rx Interface --
        rx_clk             : in std_logic;
        rx_data            : in std_logic_vector(15 downto 0);
        rx_kcode           : in std_logic_vector(1 downto 0);
        rx_disparity_error : in std_logic_vector(1 downto 0);
        rx_comma_char      : in std_logic_vector(1 downto 0);
        rx_invalid_char    : in std_logic_vector(1 downto 0);
        rx_is_byte_aligned : in std_logic;
        rx_is_locked       : in std_logic;

        -- Register Frame Data --
        rx_data_clk   : in std_logic;
        rx_data_avail : out std_logic;
        rx_data_rden  : in std_logic;
        rx_data_out   : out std_logic_vector(15 downto 0);
        rx_data_cnt   : out std_logic_vector(9 downto 0);
        rx_reset      : out std_logic;
        
        -- GTM Interface --
        gtm_recv : out gtm_recv_rec;
        
        -- Broadcast Interface --
        broadcast_recv : out broadcast_recv_rec;
        
        rx_activity : out std_logic
    );
end rx_data_link;

architecture behv of rx_data_link is

    type RX_STATE_T is (SYNC, IDLE);
    signal rx_state : RX_STATE_T;    

    signal rx_empty_i : std_logic;
    signal rx_data_in : std_logic_vector(15 downto 0);
    signal rx_write_en, rx_write_full : std_logic;

    signal rx_sync_count : std_logic_vector(7 downto 0);
    
    signal rx_reset_i, rst : std_logic;

    attribute mark_debug : string;
    attribute mark_debug of rx_data : signal is "true";
    attribute mark_debug of rx_kcode : signal is "true";
    attribute mark_debug of rx_disparity_error : signal is "true";
    attribute mark_debug of rx_comma_char : signal is "true";
    attribute mark_debug of rx_invalid_char : signal is "true";
    attribute mark_debug of rx_is_byte_aligned : signal is "true";    
    attribute mark_debug of rx_is_locked : signal is "true";   
    attribute mark_debug of rx_sync_count : signal is "true";   
    attribute mark_debug of rx_state : signal is "true";   
 
begin

    rx_reset <= rx_reset_i;
    
    rst <= reset or not rx_is_locked or rx_reset_i;

    rx_led_act : entity work.led_activity
    port map ( 
        clk     => rx_clk,
        reset   => rst,
    	act_in  => rx_write_en,
        led_out => rx_activity
    );

        rx_fifo_0 : entity work.fifo_16b_1024
        port map (
            rst => rst,
            
            wr_clk => rx_clk,
            wr_en  => rx_write_en,
            din    => rx_data_in,
            full   => rx_write_full,
    
            rd_clk => rx_data_clk,
            rd_en  => rx_data_rden,
            dout   => rx_data_out,
            empty  => rx_empty_i
        );
    
        rx_data_avail <= not rx_empty_i;
        
        process(rx_clk, reset, rx_is_locked)
        begin
            if (reset = '1' or rx_is_locked = '0') then
                rx_write_en <= '0';
                rx_reset_i <= '1';
                rx_data_in <= (others => '0');
                gtm_recv.lvl1_accept(0) <= '0';
                gtm_recv.modebits <= (others => '0');
                gtm_recv.modebits_val  <= '0';
                gtm_recv.fem_endat     <= (others => '0');
                gtm_recv.fem_user_bits <= (others => '0');
                broadcast_recv.heartbeat_trigger <= '0';
                broadcast_recv.bx_count_sync     <= '0';
                broadcast_recv.hard_reset        <= '0';
                rx_sync_count <= (others => '0');
                rx_state <= SYNC;
            elsif (rising_edge(rx_clk)) then
                rx_write_en <= '0';
                gtm_recv.modebits_val  <= '0';
                    
                case rx_state is
                    when SYNC =>
                        rx_reset_i <= '1';
                        
                        if (rx_kcode = x"1" and rx_disparity_error = x"0" and rx_invalid_char = x"0" and rx_data = RX_SYNC_C) then
                            rx_sync_count <= rx_sync_count + x"1";
                        else
                            rx_sync_count <= (others => '0');
                        end if;
                        
                        if (rx_sync_count >= x"5") then
                            rx_reset_i <= '0';
                            rx_state <= IDLE;
                        end if;
                    
                    when IDLE =>
                        if (rx_kcode = x"0" and rx_disparity_error = x"0" and rx_invalid_char = x"0" and rx_write_full = '0') then
                            rx_write_en <= '1';
                            rx_data_in <= rx_data;
                        elsif (rx_kcode = x"1" and rx_disparity_error = x"0" and rx_invalid_char = x"0") then
                            case rx_data(7 downto 0) is
                                when RX_GTM_CMD_C =>
                                    gtm_recv.modebits_val   <= '1';
                                    gtm_recv.lvl1_accept(0) <= rx_data(15);
                                    gtm_recv.fem_endat      <= rx_data(14 downto 13);
                                    gtm_recv.fem_user_bits(1 downto 0)  <= rx_data(12 downto 11);
                                    gtm_recv.modebits(2 downto 0) <= rx_data(10 downto 8);
                                when RX_BDC_CMD_C =>
                                    broadcast_recv.heartbeat_trigger <= rx_data(8);
                                    broadcast_recv.bx_count_sync     <= rx_data(9);
                                    broadcast_recv.hard_reset        <= rx_data(10);                                    
                                when others =>
                                    null;
                            end case;
                        elsif (rx_disparity_error /= x"0" and rx_invalid_char /= x"0") then
                            rx_sync_count <= (others => '0');
                            rx_state <= SYNC;
                        end if;
                end case;
            end if;
        end process;

end behv;
