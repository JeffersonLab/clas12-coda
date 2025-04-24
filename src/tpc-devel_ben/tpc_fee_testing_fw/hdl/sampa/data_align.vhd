library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package sampa_pkg is
        constant NUMBER_OF_ELINKS : integer := 8*4;
        --type std_array is array(natural range <>) of std_logic_vector;
        type sampa_array is array(NUMBER_OF_ELINKS-1 downto 0) of std_logic_vector(10 downto 0);
        type sampa_rd_array is array(NUMBER_OF_ELINKS-1 downto 0) of std_logic_vector(12 downto 0);
end package;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity sampa_sync is
    port(
        clk  : in std_logic;
        rst  : in std_logic;
        din  : in std_logic_vector(9 downto 0);
        
        ber       : in std_logic; 
        words_ok  : out std_logic_vector(31 downto 0);
        words_err : out std_logic_vector(31 downto 0);
        locked    : out std_logic;
        
        dout    : out std_logic_vector(9 downto 0);
        dval    : out std_logic;
        dlocked : out std_logic
     );
end sampa_sync;

architecture behv of sampa_sync is
    
    type slv_5x10bit is array (0 to 4) of std_logic_vector(9 downto 0);
    constant SAMPA_SYNC_WORD_C : slv_5x10bit := (
     0 => b"0100010011",  -- 0x113
     1 => b"0000000000",  -- 0x000
     2 => b"0000001111",  -- 0x00F
     3 => b"0101010101",  -- 0x155
     4 => b"0101010101"); -- 0x155
     
    signal word_count   : integer range 0 to 9   := 0;
    signal word_timeout : integer range 0 to 149 := 0;
    signal sync_idx     : integer range 0 to 4   := 0;
    
    type state_t is (FIRST_WORD_ALIGN, SYNC, ALIGNED_LOCKED);
    signal state : state_t;
     
    signal data : std_logic_vector(din'range);
    signal dlocked_i : std_logic;
    
begin

    dlocked <= dlocked_i;
    
    process(clk)
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                locked <= '0';
                words_err <= (others => '0');
                words_ok <= (others => '0');
                dout <= (others => '0');
                dval <= '0';
                word_timeout <= 0;
                word_count <= 0;
                sync_idx <= 0;
                dlocked_i <= '0';
                state <= FIRST_WORD_ALIGN;
            else
                data <= din;
                word_count <= word_count + 1;
                
                case state is
                    when FIRST_WORD_ALIGN =>
                        locked <= '0';
                        word_timeout <= word_timeout + 1;
                        if (word_timeout = 149) then
                            words_err <= words_err + x"0000_0001";
                        end if;
                        
                        if (data = SAMPA_SYNC_WORD_C(0)) then
                            word_timeout <= 0;
                            word_count <= 0;
                            sync_idx <= 1;
                            state <= SYNC;
                        end if;
                        
                    when SYNC =>
                        if (word_count = 9) then
                            word_count <= 0;
                            if (data = SAMPA_SYNC_WORD_C(sync_idx)) then
                                words_ok <= words_ok + x"0000_0001";
                                if (sync_idx = 4) then
                                    sync_idx <= 0;
                                    if (words_err = x"0000_0000") then
                                        locked <= '1';
                                        if (ber = '0') then
                                            state <= ALIGNED_LOCKED;
                                        else
                                            state <= SYNC;
                                        end if;
                                    end if;
                                else
                                    sync_idx <= sync_idx + 1;
                                end if;
                            else
                                sync_idx <= 0;
                                locked <= '0';
                                words_err <= words_err + x"0000_0001";
                            end if;
                        end if;
                    
                    when ALIGNED_LOCKED =>
                        if (word_count = 9) then
                            if (dlocked_i = '0' and data = SAMPA_SYNC_WORD_C(0)) then
                                dlocked_i <= '1';
                            end if;
                            word_count <= 0;
                            dout <= data;
                            dval <= '1';
                        else
                            dval <= '0';
                        end if;
                    
                end case;
            end if;
        end if;
    end process;
    
end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity sampa_eyescan is
    port(
        clk  : in std_logic;
        rst  : in std_logic;
        
        ber       : out std_logic;
        is_locked : in std_logic;
        words_ok  : in std_logic_vector(31 downto 0);
        words_err : in std_logic_vector(31 downto 0);
        
        tap_load  : out std_logic;
        tap_value : out std_logic_vector(4 downto 0);

        eye : out std_logic_vector(31 downto 0)
     );
end sampa_eyescan;

architecture behv of sampa_eyescan is
    
    type state_t is (OUT_OF_RESET, DWELL, LOAD_TAP, LOAD_DELAY, TRANS_DETECT, DONE);
    signal state : state_t;
    
    signal tap         : integer range 0 to 31 := 0;
    signal left_point, right_point : integer := 0;
    signal eye_diagram : std_logic_vector(31 downto 0);
    signal lock_trans : std_logic_vector(1 downto 0);
    
    --attribute mark_debug : string;
    --attribute mark_debug of eye_diagram : signal is "true";
    --attribute mark_debug of tap : signal is "true";
    --attribute mark_debug of state : signal is "true";
    
begin
    
    tap_value <= std_logic_vector(to_unsigned(tap, tap_value'length));
    
    process(clk)
        function reverse_vector(a: in std_logic_vector)
        return std_logic_vector is
            variable result: std_logic_vector(a'RANGE);
            alias aa: std_logic_vector(a'REVERSE_RANGE) is a;
        begin
            for i in aa'RANGE loop
                result(i) := aa(i);
            end loop;
            return result;
        end;
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                eye_diagram <= (others => '0');
                        eye <= (others => '0');
                lock_trans <= (others => '0');
                left_point <= 0;
                right_point <= 0;
                ber <= '1';
                state <= OUT_OF_RESET;
            else
                tap_load <= '0';
                ber <= '1';
                
                case state is
                    when OUT_OF_RESET =>
                        tap <= 0;
                        tap_load <= '1';
                        state <= LOAD_DELAY;
                        
                    when DWELL =>
                        if (is_locked = '1' and words_ok >= x"0000_ffff") then
                            eye_diagram(tap) <= '1';
                            lock_trans(0) <= '1';
                            state <= TRANS_DETECT;
                        elsif (is_locked = '0' and (words_err >= x"0000_ffff" or words_ok >= x"0000_ffff")) then
                            eye_diagram(tap) <= '0';
                            lock_trans(0) <= '0';
                            state <= TRANS_DETECT;
                        end if;
                        
                    when TRANS_DETECT =>
                        state <= LOAD_TAP;
                        lock_trans(1) <= lock_trans(0);
                        if (lock_trans = "10") then
                            right_point <= tap;
                        elsif (lock_trans = "01") then
                            left_point <= tap;
                        end if;
                        
                    when LOAD_TAP =>
                        if (tap = 31 and eye_diagram /= x"0000_0000") then
                            state <= DONE;
                            tap_load <= '1';
                            if (reverse_vector(eye_diagram(31 downto 16)) = eye_diagram(15 downto 0)) then
                                if (eye_diagram(15 downto 0) = x"ffff") then -- all ones, we go into the center
                                    tap <= 15;
                                else -- symmetric zeros and ones 
                                    tap <= 0;
                                end if;
                            elsif (reverse_vector(eye_diagram(31 downto 16)) < eye_diagram(15 downto 0)) then -- look right
                                tap <= right_point / 2;
                            elsif (reverse_vector(eye_diagram(31 downto 16)) > eye_diagram(15 downto 0)) then -- look left 
                                tap <= (left_point + 31) / 2;
                            end if;
                        elsif (tap = 31) then
                            tap <= 0;
                            tap_load <= '1';
                            state <= LOAD_DELAY;
                        else
                            tap <= tap + 1;
                            tap_load <= '1';
                            state <= LOAD_DELAY;
                        end if;
                    
                    when LOAD_DELAY =>
                        state <= DWELL;

                    when DONE =>
                        ber <= '0';
                        eye <= eye_diagram;
                        state <= DONE;
                end case;
            end if;
        end if;
    end process;
    
end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity elink_read_avail is
    port (
        clk  : in std_logic;
        
        rd_data       : in std_logic_vector(10 downto 0);
        rd_data_count : in std_logic_vector(12 downto 0);
        rd_empty      : in std_logic;

        rd_available : out std_logic
    );
end elink_read_avail;

architecture behv of elink_read_avail is

    signal rd_available_i : std_logic;

begin

    rd_available_i <= '1' when ((rd_empty = '0') and (rd_data(10) = '1') and (unsigned(rd_data_count) >= unsigned(rd_data(9 downto 0))))
                          else '0';

    process(clk)
    begin
        if (rising_edge(clk)) then
            rd_available <= rd_available_i;
        end if;
    end process;

end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity sampa_elink is
    port(
        clk  : in std_logic;
        rst  : in std_logic;
        
        en   : in std_logic;
        din     : in std_logic_vector(9 downto 0);
        dval    : in std_logic;
        dlocked : in std_logic;
        
        rd_clk        : in std_logic;
        rd_data       : out std_logic_vector(10 downto 0);
        rd_en         : in std_logic;
        rd_available  : out std_logic
    );
end sampa_elink;

architecture behv of sampa_elink is
    
    type state_t is (HEADER, DATA_HEADER, CHECK_PARITY_BIT, WRITE_OUT_HEADER, DATA_PAYLOAD, HEARTBEAT, SYNC, INVALID_DATA);
    signal state : state_t;
    
    signal data_cnt : std_logic_vector(9 downto 0);
    signal header_cnt : integer range 0 to 31 := 0;
    
    type slv_5x10bit is array (0 to 4) of std_logic_vector(9 downto 0);
    constant SAMPA_SYNC_WORD_C : slv_5x10bit := (
     0 => b"0100010011",  -- 0x113
     1 => b"0000000000",  -- 0x000
     2 => b"0000001111",  -- 0x00F
     3 => b"0101010101",  -- 0x155
     4 => b"0101010101"); -- 0x155
    
    signal hamming_code   : std_logic_vector(5 downto 0);   -- 6 bits  : Hamming code
    signal header_parity  : std_logic;                      -- 1 bit   : Odd parity of header including hamming
    signal packet_type    : std_logic_vector(2 downto 0);   -- 3 bits  : Packet type
    signal data_words     : std_logic_vector(9 downto 0);   -- 10 bits : Number of 10 bit words in data payload
    signal hw_address     : std_logic_vector(3 downto 0);   -- 4 bits  : Hardware chip address
    signal chan_address   : std_logic_vector(4 downto 0);   -- 5 bits  : Channel address
    signal bx_counter     : std_logic_vector(19 downto 0);  -- 20 bits : Bunchcrossing counter
    signal data_parity    : std_logic;                      -- 1 bit   : Odd parity of data payload
    
    signal lost_lock : std_logic;
    
    signal parity_check : std_logic;
    
    signal fifo_din      : std_logic_vector (10 downto 0);
    signal wr_en         : std_logic;
    signal dout          : std_logic_vector (10 downto 0);
    signal full          : std_logic;
    signal empty         : std_logic;
    signal wr_data_count : std_logic_vector (12 downto 0);
    signal wr_rst_busy   : std_logic;
    signal rd_rst_busy   : std_logic;
    signal rd_data_i     : std_logic_vector(10 downto 0);
    signal rd_data_count : std_logic_vector (12 downto 0);
    
    signal write_out : boolean;

    --attribute mark_debug : string;
    --attribute mark_debug of state : signal is "true";
    --attribute mark_debug of data_cnt : signal is "true";
    --attribute mark_debug of header_cnt : signal is "true";
    --attribute mark_debug of hamming_code : signal is "true";
    --attribute mark_debug of header_parity : signal is "true";
    --attribute mark_debug of packet_type : signal is "true";
    --attribute mark_debug of data_words : signal is "true";
    --attribute mark_debug of hw_address : signal is "true";
    --attribute mark_debug of chan_address : signal is "true";
    --attribute mark_debug of bx_counter : signal is "true";
    --attribute mark_debug of data_parity : signal is "true";
    --attribute mark_debug of parity_check : signal is "true";
    --attribute mark_debug of wr_data_count : signal is "true";
    --attribute mark_debug of wr_en : signal is "true";
    --attribute mark_debug of fifo_din : signal is "true";
    --attribute mark_debug of write_out : signal is "true";
    
begin

    rd_data <= rd_data_i;

    elink_read_avail_i : entity work.elink_read_avail
    port map (
        clk => rd_clk,

        rd_data       => rd_data_i,
        rd_data_count => rd_data_count,
        rd_empty      => empty,

        rd_available  => rd_available
    );

    fifo_11b_4096_i : entity work.fifo_11b_4096
    port map (
        rst           => rst,
        rd_clk        => rd_clk,
        rd_en         => rd_en,
        dout          => rd_data_i,
        rd_data_count => rd_data_count,
        rd_rst_busy   => open,
        empty         => empty,
        
        wr_clk        => clk,
        wr_en         => wr_en,
        din           => fifo_din,
        wr_data_count => wr_data_count,
        wr_rst_busy   => open,
        full          => full
    );

    process(clk)
    begin
        if (rising_edge(clk)) then
            if (dlocked = '0') then
                parity_check <= '1';
                lost_lock <= '0';
                data_cnt <= (others => '0');
                header_cnt <= 0;
                state <= HEADER;
                write_out <= false;
                wr_en <= '0';
            else
                case (state) is
                    when HEADER =>
                        wr_en <= '0';
                        if (dval = '1') then
                            parity_check <= (xor din) xor '1';
                            
                            data_cnt <= (others => '0');
                            hamming_code  <= din(5 downto 0);
                            header_parity <= din(6);
                            packet_type   <= din(9 downto 7);
                            case (din(9 downto 7)) is
                                when "000" => 
                                    state <= HEARTBEAT; 
                                    header_cnt <= 0;
                                when "010" =>
                                    state <= SYNC;
                                    header_cnt <= 1;
                                when "001" | "011" | "100" | 
                                     "101" | "110" | "111" => 
                                    state <= DATA_HEADER; 
                                    header_cnt <= 0;
                                when others => 
                                    state <= INVALID_DATA;
                            end case;
                        end if;
                        
                    when DATA_HEADER =>
                        if (dval = '1') then
                            parity_check <= (xor din) xor parity_check;
                            
                            header_cnt <= header_cnt + 1;
                            case (header_cnt) is
                                when 0 => data_words <= din(9 downto 0);
                                when 1 => hw_address    <= din(3 downto 0);
                                          chan_address  <= din(8 downto 4);
                                          bx_counter(0) <= din(9);
                                when 2 => bx_counter(10 downto 1)  <= din(9 downto 0);
                                when 3 => bx_counter(19 downto 11) <= din(8 downto 0);
                                          data_parity              <= din(9);
                                          header_cnt <= 0;
                                          state <= CHECK_PARITY_BIT;
                                when others => state <= INVALID_DATA;
                            end case;
                        end if;
                    
                    when CHECK_PARITY_BIT =>
                        if (parity_check = '1') then
                            if (data_words > x"000") then
                                -- Check if we are going to overflow the FIFO
                                write_out <= ((wr_data_count + data_words + x"005") > wr_data_count) and (en = '1');
                                state <= WRITE_OUT_HEADER;
                            else
                                state <= HEADER;
                            end if;
                        else
                            -- Here we should apply the hamming code to recover any data...
                            state <= INVALID_DATA;
                        end if;
                        
                    when WRITE_OUT_HEADER =>
                        if (write_out) then
                            wr_en <= '1';
                            header_cnt <= header_cnt + 1;
                            case (header_cnt) is
                                when 0 => fifo_din <= '1' & (data_words + x"05");
                                when 1 => fifo_din <= '0' & bx_counter(0) & hw_address & chan_address;
                                when 2 => fifo_din <= '0' & bx_counter(10 downto 1);
                                when 3 => fifo_din <= '0' & data_parity & bx_counter(19 downto 11);
                                when 4 => fifo_din <= "000" & x"fe";
                                          header_cnt <= 0;
                                          state <= DATA_PAYLOAD;
                                when others => state <= INVALID_DATA;
                            end case;
                        else
                            state <= DATA_PAYLOAD;
                        end if;
                        
                    when DATA_PAYLOAD =>
                        wr_en <= '0';
                        
                        if (dval = '1') then
                            if (write_out) then
                                fifo_din <= "0" & din;
                                wr_en <= '1';
                            end if;
                            
                            data_cnt <= data_cnt + x"01";
                            if (data_cnt = data_words-1) then
                                state <= HEADER;
                            else
                                state <= DATA_PAYLOAD;
                            end if;
                        end if;
                    
                    when HEARTBEAT =>
                        if (dval = '1') then
                            header_cnt <= header_cnt + 1;
                            if (header_cnt = 3) then
                                state <= HEADER;
                            end if;
                        end if;
                    
                    when SYNC =>
                        if (dval = '1') then
                            if (din = SAMPA_SYNC_WORD_C(header_cnt)) then
                                state <= SYNC;
                            else
                                state <= INVALID_DATA;
                            end if;
                            if (header_cnt < 4) then
                                header_cnt <= header_cnt + 1;
                            elsif (header_cnt = 4) then
                                header_cnt <= 0;
                                state <= HEADER;
                            end if;
                        end if;
                        
                    when INVALID_DATA =>
                        lost_lock <= '1';
                end case;
            end if;
        end if;
    end process;

end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;
use work.sampa_pkg.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity sampa_stream is
    generic (
        NUMBER_OF_ELINKS : natural := 4*8
    );
    port (
        clk  : in std_logic;
        rst  : in std_logic;
        
        sampa_data          : in sampa_array;
        sampa_rd_en         : out std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);
        sampa_rd_available  : in std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);
        
        data_clk   : out std_logic;
        data_wr_en : out std_logic;
        data_sob   : out std_logic;
        data_eob   : out std_logic;
        data_out   : out std_logic_vector(15 downto 0)
    );
end sampa_stream;

architecture behv of sampa_stream is
    
    type state_t is (CHECK_READABLE_FIFOS, GET_FIFO_IDX, START_OF_BLOCK, STREAM_FIFO, END_OF_BLOCK);
    signal state : state_t;
    
    signal fifos_to_read : std_logic_vector(sampa_rd_available'range) := (others => '0');
    signal fifos_done_reading : std_logic_vector(sampa_rd_available'range) := (others => '0');
    
    signal words_to_read : integer range 0 to 2047 := 0;
    signal fifo_idx, fifo_idx_i : integer range 0 to NUMBER_OF_ELINKS-1 := 0;
    signal sampa_rd_avail : std_logic_vector(NUMBER_OF_ELINKS-1 downto 0) := (others => '0');

    signal completed_read_i, completed_read : boolean := true;

begin

    data_clk <= clk;

    process(fifos_to_read, fifos_done_reading)
    begin
        SELECT_FIFO_LOOP:
        for idx in 0 to NUMBER_OF_ELINKS-1 loop
            if (fifos_to_read(idx) /= fifos_done_reading(idx)) then
                fifo_idx_i <= idx;
                exit;
            end if;
        end loop;
    end process;

    process(fifos_to_read, fifos_done_reading)
    begin
        completed_read_i <= fifos_to_read = fifos_done_reading;
    end process;


    process(clk)
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                state <= CHECK_READABLE_FIFOS;
                data_wr_en <= '0';
                data_sob   <= '0';
                data_eob   <= '0';
                data_out   <= (others => '0');
                sampa_rd_en <= (others => '0');
                sampa_rd_avail <= (others => '0');
            else
                data_sob <= '0';
                data_eob <= '0';
                sampa_rd_avail <= sampa_rd_available;
                completed_read <= completed_read_i;
                
                case (state) is
                    when CHECK_READABLE_FIFOS =>
                        if (completed_read) then
                            fifos_to_read <= sampa_rd_avail;
                            fifos_done_reading <= (others => '0');
                            state <= CHECK_READABLE_FIFOS;
                        else
                            state <= GET_FIFO_IDX;
                        end if; 
                        
                    when GET_FIFO_IDX =>
                        state <= START_OF_BLOCK;
                        fifo_idx <= fifo_idx_i;

                    when START_OF_BLOCK =>
                        sampa_rd_en <= (others => '0');
                        words_to_read <= to_integer(unsigned(sampa_data(fifo_idx)(9 downto 0)));
                        sampa_rd_en(fifo_idx) <= '1';
                        data_sob <= '1';
                        state <= STREAM_FIFO;
                       
                    when STREAM_FIFO =>
                        if (words_to_read = 1) then
                            fifos_done_reading(fifo_idx) <= '1';
                            sampa_rd_en(fifo_idx) <= '0';
                            data_wr_en <= '1';
                            data_out(9 downto 0) <= sampa_data(fifo_idx)(9 downto 0);
                            data_out(15 downto 10) <= (others => '0');
                            state <= END_OF_BLOCK;
                        elsif (words_to_read > 1) then
                            words_to_read <= words_to_read - 1;
                            sampa_rd_en(fifo_idx) <= '1';
                            data_wr_en <= '1';
                            data_out(9 downto 0) <= sampa_data(fifo_idx)(9 downto 0);
                            data_out(15 downto 10) <= (others => '0');
                            state <= STREAM_FIFO;
                        end if;
                        
                    when END_OF_BLOCK =>
                        data_eob <= '1';
                        data_wr_en <= '0';
                        state <= CHECK_READABLE_FIFOS;
                        
                end case;
            end if;
        end if;
    end process;
    
end behv;
