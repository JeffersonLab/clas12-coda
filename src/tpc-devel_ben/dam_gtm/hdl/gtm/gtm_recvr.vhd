library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
library UNISIM;
use UNISIM.VComponents.all;

use work.gtm_pkg.all;

entity gtm_recvr is
    port (
        -- Reset Logic (50 MHz) --
        clk   : in STD_LOGIC;
        reset : in STD_LOGIC;
        pll_locked_n : in std_logic;
        
        -- MGT Reference Clock (400 MHz) --
        mgt_ref_clk_p : in STD_LOGIC;
        mgt_ref_clk_n : in STD_LOGIC;
        
        -- GTM Decoded Outputs --
        gtm_clk    : out std_logic;
        gtm_locked : out std_logic;
        gtm_recv   : out gtm_recvr_out_rec;
        
        -- SFP TX --
        gth_tx_n : out std_logic;
        gth_tx_p : out std_logic;
        
        -- SFP RX --
        gth_rx_n : in std_logic;
        gth_rx_p : in std_logic
    );
end gtm_recvr;

architecture behv of gtm_recvr is
    signal mgt_ref_clk, phy_reset : std_logic;
    
    signal tx_clk : std_logic;
    signal tx_data : std_logic_vector(15 downto 0);
    signal tx_kcode : std_logic_vector(1 downto 0);
    signal tx_is_locked : std_logic;
    signal rx_is_locked : std_logic;
    
    signal rx_clk :  std_logic;
    signal rx_data, prev_rx_data :  std_logic_vector(15 downto 0);
    signal rx_kcode :  std_logic_vector(1 downto 0);
    signal rx_disparty_error :  std_logic_vector(1 downto 0);
    signal rx_comma_char : std_logic_vector(1 downto 0);
    signal rx_invalid_char : std_logic_vector(1 downto 0);
    signal rx_is_byte_aligned :  std_logic;
      
    type ACTIVITY_STATE_T is (IDLE, COUNTING);
    signal act_state : ACTIVITY_STATE_T;
    
    type RX_STATE_T is (SYNCING, LOCKED);
    signal rx_state : RX_STATE_T;
    signal rx_sync_count : natural range 0 to 64 := 0;
    
    signal count : std_logic_vector(7 downto 0);
    
    attribute mark_debug : string;
    attribute mark_debug of rx_data : signal is "true";
    attribute mark_debug of rx_kcode : signal is "true";
    attribute mark_debug of rx_disparty_error : signal is "true";
    attribute mark_debug of rx_comma_char : signal is "true";
    attribute mark_debug of rx_invalid_char : signal is "true";
    attribute mark_debug of rx_is_byte_aligned : signal is "true";    
    attribute mark_debug of rx_is_locked : signal is "true";
    attribute mark_debug of gtm_recv : signal is "true";
    attribute mark_debug of rx_state : signal is "true"; 


begin
    gtm_clk                 <= rx_clk;

    phy_reset <= reset or (not pll_locked_n);
    
    gtm_locked <= '1' when (rx_state = LOCKED) else '0';

    IBUFDS_GTE3_MGTREFCLK1_X0Y3_INST: IBUFDS_GTE3 
    port map (
        I     => mgt_ref_clk_p, 
        IB    => mgt_ref_clk_n,
        CEB   => '0',
        O     => mgt_ref_clk,
        ODIV2 => open
    );

    phy_gtm_wrapper_i : entity work.phy_gtm_wrapper
    port map ( 
        -- Reset Logic --
        clk   => clk,
        reset => phy_reset,

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
        tx_disparity => (others => '0'),
        tx_is_locked => tx_is_locked,
        
        -- RX Input --
        rx_clk             => rx_clk, 
        rx_data            => rx_data,
        rx_kcode           => rx_kcode, 
        rx_disparity_error => rx_disparty_error,
        rx_comma_char      => rx_comma_char,
        rx_invalid_char    => rx_invalid_char,
        rx_is_locked       => rx_is_locked,
        rx_is_byte_aligned => rx_is_byte_aligned
    );
    
    
    process(rx_clk, rx_is_locked, reset)
        variable rx_upper_bits_v : std_logic_vector(2 downto 0);
        variable modebits_enb_v  : std_logic;
        variable fem_endat_v     : std_logic_vector(1 downto 0);
        variable lvl1_accept_v   : std_logic;
        variable modebit_n_bco_v : std_logic;
        variable rx_lower_bits_v : std_logic_vector(7 downto 0);
    begin
        if (reset = '1' or rx_is_locked = '0') then
            rx_state <= SYNCING;
            rx_sync_count <= 0;
        elsif (rising_edge(rx_clk)) then
            case rx_state is
                when SYNCING =>
                    if (rx_disparty_error = x"0" and rx_invalid_char = x"0" and rx_kcode = x"1" and rx_data = GTM_IDLE_CODE) then
                        rx_sync_count <= rx_sync_count + 1;
                    else
                        rx_sync_count <= 0;
                    end if;
                    
                    if (rx_sync_count = 64) then
                        rx_state <= LOCKED;
                        rx_sync_count <= 0;
                    end if; 
                when LOCKED =>
                    if (rx_disparty_error = x"0" and rx_invalid_char = x"0" and rx_kcode = x"0") then
                        (rx_upper_bits_v, modebits_enb_v, fem_endat_v, lvl1_accept_v, modebit_n_bco_v, rx_lower_bits_v) := rx_data;

                        gtm_recv.modebits_enb(0)  <= modebits_enb_v;                        
                        gtm_recv.modebit_n_bco(0) <= modebit_n_bco_v;
                        gtm_recv.fem_endat        <= fem_endat_v;
                        gtm_recv.lvl1_accept(0)   <= lvl1_accept_v;
                        
                        if (modebit_n_bco_v = '1') then
                            gtm_recv.modebits <= rx_lower_bits_v;
                            gtm_recv.fem_user_bits <= rx_upper_bits_v;
                            gtm_recv.modebits_val  <= '1';
                        else
                            case rx_upper_bits_v is
                                when "000" => gtm_recv.bco_count(7 downto 0)   <= rx_lower_bits_v;
                                                        gtm_recv.bco_count_val <= '0';
                                when "001" => gtm_recv.bco_count(15 downto 8)  <= rx_lower_bits_v;
                                when "010" => gtm_recv.bco_count(23 downto 16) <= rx_lower_bits_v;
                                when "011" => gtm_recv.bco_count(31 downto 24) <= rx_lower_bits_v;
                                when "100" => gtm_recv.bco_count(39 downto 32) <= rx_lower_bits_v;
                                                        gtm_recv.modebits_val  <= '0';
                                                        gtm_recv.bco_count_val <= '1';
                                when others => null;
                            end case;
                        end if;
                    elsif  (rx_disparty_error = x"0" and rx_invalid_char = x"0" and rx_kcode = x"1" and rx_data = GTM_IDLE_CODE) then
                        rx_state <= LOCKED;
                    else
                        rx_state <= SYNCING;
                    end if;
            end case;
        end if;
    end process;

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
        elsif (rising_edge(tx_clk)) then
            case act_state is
                when IDLE =>
                    tx_send_data(D_16_2_C, K_28_5_C, '0', '1');
                    act_state <= COUNTING;
                    
                when COUNTING =>
                    count <= count + 1;
                    tx_send_data(x"aa", count, '0', '0');
                    
                    if (count = x"ff") then
                        act_state <= IDLE;
                    end if;  
            end case;
        end if;
    end process; 
end behv;
