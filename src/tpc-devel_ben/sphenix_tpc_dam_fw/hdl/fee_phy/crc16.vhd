-------------------------------------------------------------------------------
-- CRC module for data(15:0)
--   lfsr(15:0)=1+x^2+x^15+x^16;
-------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;

entity crc is
    port ( 
    crc_en , rst, clk : in std_logic;
    data_in : in std_logic_vector(15 downto 0);
    crc_out : out std_logic_vector(15 downto 0)
);
end crc;

architecture behv of crc is
    signal lfsr_q: std_logic_vector(15 downto 0);
    signal lfsr_c: std_logic_vector(15 downto 0);
begin
    crc_out <= lfsr_q;

    lfsr_c(0) <= lfsr_q(0) xor lfsr_q(1) xor lfsr_q(2) xor lfsr_q(3) xor lfsr_q(4) xor lfsr_q(5) xor lfsr_q(6) xor lfsr_q(7) xor lfsr_q(8) xor lfsr_q(9) xor lfsr_q(10) xor lfsr_q(11) xor lfsr_q(12) xor lfsr_q(13) xor lfsr_q(15) xor data_in(0) xor data_in(1) xor data_in(2) xor data_in(3) xor data_in(4) xor data_in(5) xor data_in(6) xor data_in(7) xor data_in(8) xor data_in(9) xor data_in(10) xor data_in(11) xor data_in(12) xor data_in(13) xor data_in(15);
    lfsr_c(1) <= lfsr_q(1) xor lfsr_q(2) xor lfsr_q(3) xor lfsr_q(4) xor lfsr_q(5) xor lfsr_q(6) xor lfsr_q(7) xor lfsr_q(8) xor lfsr_q(9) xor lfsr_q(10) xor lfsr_q(11) xor lfsr_q(12) xor lfsr_q(13) xor lfsr_q(14) xor data_in(1) xor data_in(2) xor data_in(3) xor data_in(4) xor data_in(5) xor data_in(6) xor data_in(7) xor data_in(8) xor data_in(9) xor data_in(10) xor data_in(11) xor data_in(12) xor data_in(13) xor data_in(14);

    lfsr_c(2) <= lfsr_q(0) xor lfsr_q(1) xor lfsr_q(14) xor data_in(0) xor data_in(1) xor data_in(14);
    lfsr_c(3) <= lfsr_q(1) xor lfsr_q(2) xor lfsr_q(15) xor data_in(1) xor data_in(2) xor data_in(15);
    lfsr_c(4) <= lfsr_q(2) xor lfsr_q(3) xor data_in(2) xor data_in(3);
    lfsr_c(5) <= lfsr_q(3) xor lfsr_q(4) xor data_in(3) xor data_in(4);
    lfsr_c(6) <= lfsr_q(4) xor lfsr_q(5) xor data_in(4) xor data_in(5);
    lfsr_c(7) <= lfsr_q(5) xor lfsr_q(6) xor data_in(5) xor data_in(6);
    lfsr_c(8) <= lfsr_q(6) xor lfsr_q(7) xor data_in(6) xor data_in(7);
    lfsr_c(9) <= lfsr_q(7) xor lfsr_q(8) xor data_in(7) xor data_in(8);
    lfsr_c(10) <= lfsr_q(8) xor lfsr_q(9) xor data_in(8) xor data_in(9);
    lfsr_c(11) <= lfsr_q(9) xor lfsr_q(10) xor data_in(9) xor data_in(10);
    lfsr_c(12) <= lfsr_q(10) xor lfsr_q(11) xor data_in(10) xor data_in(11);
    lfsr_c(13) <= lfsr_q(11) xor lfsr_q(12) xor data_in(11) xor data_in(12);
    lfsr_c(14) <= lfsr_q(12) xor lfsr_q(13) xor data_in(12) xor data_in(13);

    lfsr_c(15) <= lfsr_q(0) xor lfsr_q(1) xor lfsr_q(2) xor lfsr_q(3) xor lfsr_q(4) xor lfsr_q(5) xor lfsr_q(6) xor lfsr_q(7) xor lfsr_q(8) xor lfsr_q(9) xor lfsr_q(10) xor lfsr_q(11) xor lfsr_q(12) xor lfsr_q(14) xor lfsr_q(15) xor data_in(0) xor data_in(1) xor data_in(2) xor data_in(3) xor data_in(4) xor data_in(5) xor data_in(6) xor data_in(7) xor data_in(8) xor data_in(9) xor data_in(10) xor data_in(11) xor data_in(12) xor data_in(14) xor data_in(15);

    process (clk)
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                lfsr_q <= (others => '1');
            else
                if (crc_en = '1') then
                    lfsr_q <= lfsr_c;
                end if;
            end if;
        end if;
    end process;
end architecture behv; 

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity crc_checker is
    port ( 
        rst : in std_logic;
        clk : in std_logic;
        sof : in std_logic;
        eof : in std_logic;
        
        wren_in : in std_logic;
        data_in : in std_logic_vector(15 downto 0);
        fifo_full : in std_logic;

        wren_out : out std_logic;
        data_out : out std_logic_vector(15 downto 0);
        
        fifo_full_count : out std_logic_vector(15 downto 0);
        crc_error_count : out std_logic_vector(15 downto 0);
        crc_error : out std_logic
);
end crc_checker;

architecture behv of crc_checker is
    signal crc_data, crc_result, crc_error_cnt : std_logic_vector(15 downto 0);
    signal crc_rst, crc_wren, sof_i, eof_i : std_logic;
    
   -- attribute mark_debug : string;
   -- attribute mark_debug of sof : signal is "true";
   -- attribute mark_debug of eof : signal is "true";
   -- attribute mark_debug of crc_error_cnt : signal is "true";
   -- attribute mark_debug of crc_data : signal is "true";
   -- attribute mark_debug of crc_wren : signal is "true";
   -- attribute mark_debug of crc_result : signal is "true";
   -- attribute mark_debug of crc_error : signal is "true";
   -- attribute mark_debug of crc_rst : signal is "true";
   -- attribute mark_debug of fifo_full_cnt : signal is "true";
   -- attribute mark_debug of fifo_full : signal is "true";
    
begin

    crc_error_count <= crc_error_cnt;
    data_out <= crc_data;
    wren_out <= crc_wren when fifo_full = '0' else '0';

    crc : entity work.crc
    port map (
        clk     => clk,
        rst     => crc_rst,
        crc_en  => crc_wren,
        data_in => crc_data,
        crc_out => crc_result
    );
    
    full_counter_i : entity work.counter
    port map (
        clk_i => clk,
        rst_i => rst,
        d_i   => fifo_full,
        count_o(31 downto 16) => open,
        count_o(15 downto 0) => fifo_full_count);

    process(clk)
    begin
        if (rising_edge(clk)) then
        if (rst = '1') then
            crc_error_cnt <= (others => '0');
            crc_error <= '0';
            crc_rst <= '1';
            sof_i <= '0';
            eof_i <= '0';
        else
            crc_rst <= '0';
            crc_error <= '0';

            sof_i    <= sof;
            eof_i    <= eof;
            crc_data <= data_in;
            crc_wren <= wren_in;
            
            if (sof_i = '1') then
                crc_data <= (others => '0');
                crc_rst <= '1';
            elsif (eof_i = '1') then
                crc_wren <= '0';
                if (crc_result /= x"0000") then
                    crc_error <= '1';
                    crc_error_cnt <= crc_error_cnt + x"1";
                end if;
            end if;
        end if;
        end if;
    end process;

end architecture behv; 

