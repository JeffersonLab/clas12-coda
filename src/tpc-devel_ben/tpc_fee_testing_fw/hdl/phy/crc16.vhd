-------------------------------------------------------------------------------
-- CRC module for data(15:0)
--   lfsr(15:0)= 1 + x^2 + x^15 + x^16;
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
            elsif (crc_en = '1') then
                lfsr_q <= lfsr_c;
            end if;
        end if;
    end process;
end architecture behv; 

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity crc_generator is
    port ( 
        rst : in std_logic;
        clk : in std_logic;
        
        wren_in : in std_logic;
        data_in : in std_logic_vector(16 downto 0);

        wren_out : out std_logic;
        data_out : out std_logic_vector(16 downto 0)
        
);
end crc_generator;

architecture behv of crc_generator is

    signal crc_data, data_0, data_1, data_2 : std_logic_vector(16 downto 0);
    signal crc_result : std_logic_vector(15 downto 0);
    signal crc_rst, crc_wren, wren_0, wren_1, wren_2 : std_logic;
    
begin

    data_out <= data_2;
    wren_out <= wren_2;

    crc : entity work.crc
    port map (
        clk     => clk,
        rst     => crc_rst,
        crc_en  => crc_wren,
        data_in => crc_data(15 downto 0),
        crc_out => crc_result
    );

    process(clk)
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                crc_data <= (others => '0');
                crc_rst <= '1';
                crc_wren <= '0';
                wren_2 <= '0';
                data_2 <= (others => '0');
                wren_1 <= '0';
                data_1 <= (others => '0');
                wren_0 <= '0';
                data_0 <= (others => '0');
            else
                crc_rst <= '0';
                
                data_0 <= data_in;
                data_1 <= data_0;
                data_2 <= data_1;
                wren_0 <= wren_in;
                wren_1 <= wren_0;
                wren_2 <= wren_1;

                if (data_0 = '1' & x"FEED" and wren_0 = '1') then
                    crc_rst <= '1';
                    crc_wren <= '0';
                    crc_data <= (others => '1');
                elsif (data_0 = '1' & x"FACE" and wren_0 = '1') then
                    crc_wren <= '0';
                    data_2 <= '0' & crc_result;
                    wren_2 <= '1';
                else
                    crc_data <= data_0;
                    crc_wren <= wren_0;
                end if;
            end if;
        end if;
    end process;

end architecture behv; 
