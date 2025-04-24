library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;
use work.gtm_pkg;

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
        rx_data_reset : out std_logic;

        -- Stream Frame Data --
        stream_clk   : in  std_logic;
        stream_rden  : in  std_logic;
        stream_data  : out std_logic_vector(15 downto 0);
        stream_cnt   : out std_logic_vector(9 downto 0);
        stream_avail : out std_logic;
        stream_enable : in std_logic;
        stream_reset  : in std_logic;

        -- RX Stats --
        counters : out phy_stats_rec
    );

end rx_data_link;

architecture behv of rx_data_link is

    signal rx_general_data :  std_logic_vector(15 downto 0);
    signal rx_write_en, rx_write_full : std_logic;
    
    signal tx_fifo_rden : std_logic;
    signal tx_fifo_data : std_logic_vector(16 downto 0);
    signal tx_fifo_avail : std_logic;
    
    signal rx_empty_i, tx_empty_i, stream_empty_i : std_logic;
    
    type RX_STATE_T is (SYNC, IDLE, RECV_STREAMING_DATA, RECV_REGISTER_DATA);
    signal rx_state : RX_STATE_T;
    
    signal rx_stream_write_en : std_logic;
    signal rx_stream_data : std_logic_vector(15 downto 0);
    signal rx_stream_full: std_logic;
    
    signal rx_sync_count : std_logic_vector(11 downto 0);
    
    signal trig : std_logic_vector(2 downto 0);
    signal trig_ack : std_logic;
    
    signal generic_sof, generic_eof : std_logic;
    signal write_en : std_logic;
    signal general_data : std_logic_vector(15 downto 0);
    
    signal stream_sof, stream_eof : std_logic;
    signal stream_write_en, crc_err : std_logic;
    signal stream_data_i : std_logic_vector(15 downto 0);
    signal stream_en : std_logic;
    
    signal rx_data_reset_i : std_logic := '1';
    signal stream_valid : std_logic;

    signal stats : phy_stats_rec;
    
    --attribute dont_touch : string;
    --attribute dont_touch of rx_data : signal is "true";
    --attribute dont_touch of rx_kcode : signal is "true";
    --attribute dont_touch of rx_disparity_error : signal is "true";
    --attribute dont_touch of rx_comma_char : signal is "true";
    --attribute dont_touch of rx_invalid_char : signal is "true";
    --attribute dont_touch of rx_is_byte_aligned : signal is "true";    
    --attribute dont_touch of rx_is_locked : signal is "true";    
    --attribute dont_touch of rx_sync_count : signal is "true";    
    --attribute dont_touch of rx_state : signal is "true";    
    --attribute dont_touch of write_en: signal is "true";    
    --attribute dont_touch of general_data: signal is "true";    
    --attribute dont_touch of rx_write_full: signal is "true";    
    --attribute dont_touch of rx_write_en: signal is "true";    
    --attribute dont_touch of rx_general_data: signal is "true";    
    --attribute dont_touch of crc_err : signal is "true";    
    --attribute dont_touch of rx_data_reset_i : signal is "true";    

begin

    rx_data_reset <= rx_data_reset_i;
    
    counters <= stats;

    -- RX Register/Generic FIFO --
    rx_fifo_0 : entity work.blkram_fifo_16b
    port map (
        rst => rx_data_reset_i,
        
        wr_clk => rx_clk,
        wr_en  => write_en,
        din    => general_data,
        full   => rx_write_full,

        rd_clk => rx_data_clk,
        rd_en  => rx_data_rden,
        dout   => rx_data_out,
        empty  => rx_empty_i,
        rd_data_count => rx_data_cnt
    );
    rx_data_avail <= not rx_empty_i;

    -- CRC-16 RX Register Frame Checker --
    generic_crc_check_0 : entity work.crc_checker
    port map (
        rst => rx_data_reset_i,
        clk => rx_clk,
        sof => generic_sof,
        eof => generic_eof,
        
        wren_in => rx_write_en,
        data_in => rx_general_data,
        fifo_full => rx_write_full,

        wren_out => write_en,
        data_out => general_data,
        
        fifo_full_count => open,
        crc_error_count => stats.crc_reg_errors,
        crc_error => crc_err
    );
        
    -- RX Streaming Data FIFO --
    rx_stream_fifo_0 : entity work.blkram_fifo_16b
    port map (
        rst => rx_data_reset_i or stream_reset,
        
        wr_clk => rx_clk,
        wr_en  => stream_write_en,
        din    => stream_data_i,
        full   => rx_stream_full,

        rd_clk => stream_clk,
        rd_en  => stream_rden,
        dout   => stream_data,
        empty  => stream_empty_i,
        rd_data_count => stream_cnt
    );
    stream_avail <= not stream_empty_i;

    -- CRC-16 RX Streaming Data Frame Checker --
    stream_crc_check_0 : entity work.crc_checker
    port map (
            rst => rx_data_reset_i or stream_reset,
            clk => rx_clk,
            sof => stream_sof,
            eof => stream_eof,
            
            wren_in => rx_stream_write_en,
            data_in => rx_stream_data,
            fifo_full => rx_stream_full,
    
            wren_out => stream_write_en,
            data_out => stream_data_i,
            
            fifo_full_count => stats.stream_fifo_full_count,
            crc_error_count => stats.crc_stream_errors,
            crc_error => open
    );
    
    -- RX Frame Decoder -- 
    process(rx_clk)
    begin
        if (rising_edge(rx_clk)) then
        if (reset = '1' or rx_is_locked = '0') then
            rx_data_reset_i <= '1';
            generic_sof <= '0';
            generic_eof <= '0';
            stream_sof  <= '0';
            stream_eof  <= '0';
            stats.rx_sob   <= (others => '0');
            stats.rx_eob   <= (others => '0');
            stats.rx_stream_bytes <= (others => '0');
            rx_write_en <= '0';
            rx_stream_write_en <= '0'; 
            rx_stream_data <= (others => '0');
            rx_general_data <= (others => '0');
            rx_sync_count <= (others => '0');
            stream_en <= '0';
            stream_valid <= '0';
            rx_state <= SYNC;
        else
            generic_sof <= '0';
            generic_eof <= '0';
            stream_sof  <= '0';
            stream_eof  <= '0';

            rx_write_en <= '0';
            rx_stream_write_en <= '0';
            stream_en <= stream_enable;
            
            case rx_state is
                when SYNC =>
                    rx_data_reset_i <= '1';
                    if (rx_kcode = x"1" and rx_disparity_error = x"0" and rx_invalid_char = x"0" and rx_data = RX_SYNC_C) then
                        rx_sync_count <= rx_sync_count + x"1";
                    else
                        rx_sync_count <= (others => '0');
                    end if;
                    
                    if (rx_sync_count >= x"fff") then
                        rx_data_reset_i <= '0';
                        rx_sync_count <= (others => '0');
                        rx_state <= IDLE;
                    end if;
                    
                when IDLE =>
                    if (rx_kcode = x"1" and rx_disparity_error = x"0" and rx_invalid_char = x"0") then
                        if (rx_data = START_OF_REG_C) then
                            generic_sof <= '1';
                            rx_state <= RECV_REGISTER_DATA;
                        elsif (rx_data = RESUME_BLOCK_C and stream_en = '1') then
                            rx_state <= RECV_STREAMING_DATA;
                        elsif (rx_data = START_OF_BLOCK_C and stream_en = '1') then
                            stream_sof <= '1';
                            stats.rx_sob <= stats.rx_sob + 1;
                            rx_state <= RECV_STREAMING_DATA;
                        end if;
                    elsif (rx_disparity_error /= x"0" and rx_invalid_char /= x"0") then
                        rx_state <= SYNC;
                    end if;
                
                when RECV_STREAMING_DATA =>
                    if (rx_kcode = x"0" and rx_disparity_error = x"0" and rx_invalid_char = x"0") then
                        rx_stream_write_en <= '1'; 
                        rx_stream_data <= rx_data;
                        stats.rx_stream_bytes <= stats.rx_stream_bytes + 1;
                    elsif (rx_kcode = x"1" and rx_disparity_error = x"0" and rx_invalid_char = x"0") then
                        rx_stream_write_en <= '0';
                        if (rx_data = END_OF_BLOCK_C) then
                            stream_eof <= '1';
                            stats.rx_eob <= stats.rx_eob + 1;
                            rx_state <= IDLE;
                        elsif (rx_data = PAUSED_BLOCK_C) then
                            rx_state <= IDLE;
                        end if;
                    elsif (rx_disparity_error /= x"0" and rx_invalid_char /= x"0") then
                        rx_state <= SYNC;
                    end if;
                    
                when RECV_REGISTER_DATA =>
                    if (rx_kcode = x"0" and rx_comma_char = x"0" and rx_disparity_error = x"0" and rx_invalid_char = x"0") then
                        rx_write_en <= '1';
                        rx_general_data <= rx_data;
                    elsif ((rx_kcode = x"1" or rx_comma_char = x"1") and rx_disparity_error = x"0" and rx_invalid_char = x"0") then
                        rx_write_en <= '0';
                        if (rx_data = WAIT_FOR_REG_C) then
                            rx_state <= RECV_REGISTER_DATA;
                        elsif (rx_data = END_OF_REG_C) then
                            generic_eof <= '1';
                            rx_state <= IDLE;
                        end if;
                    elsif (rx_disparity_error /= x"0" and rx_invalid_char /= x"0") then
                        rx_state <= SYNC;
                    end if;
            end case;
        end if;
        end if;
    end process;
end behv;

