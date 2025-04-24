library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
--use ieee.numeric_bit.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all;
use ieee.math_real.all;

library std;
use std.textio.all;

library work;
use work.sim_pkg.all;
use work.sampa_pkg.all;

entity tb is
end entity;

architecture sim of tb is

    constant DATA_LENGTH : integer := 1007;

    signal data_clk_period : time;
    signal sampa_clk_period : time;

    signal data_clk, sampa_clk : std_logic;

    signal rst : std_logic;

    signal sampa_data          : sampa_array;
    signal sampa_rd_en         : std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);
    signal sampa_rd_available  : std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);
    signal sampa_rd_data_count : sampa_rd_array;
    signal sampa_elink_enable  : std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);

    signal data_wr_en : std_logic;
    signal data_sob   : std_logic;
    signal data_eob   : std_logic;
    signal data_out   : std_logic_vector(15 downto 0);

    signal running : boolean;

    signal word_cnt : integer := 0;

begin

    clk_gen(data_clk, 312.500E6, data_clk_period);
    clk_gen(sampa_clk, 160.00E6, sampa_clk_period);

     -- SAMPA Data Stream Multiplexer
    sampa_stream_i : entity work.sampa_stream 
    generic map (
        NUMBER_OF_ELINKS => NUMBER_OF_ELINKS
    )
    port map (
        clk  => data_clk,
        rst  => rst,
        
        sampa_data          => sampa_data,
        sampa_rd_en         => sampa_rd_en,
        sampa_rd_available  => sampa_rd_available,
        
        data_clk   => open,
        data_wr_en => data_wr_en,
        data_sob   => data_sob,
        data_eob   => data_eob,
        data_out   => data_out 
    );

    -- Data Generator
    SAMPA_DATA_GENERATOR_G : for idx in 0 to NUMBER_OF_ELINKS-1  generate
        sampa_datagen_i : entity work.sampa_datagen
        generic map (
            CHANNEL_NUMBER => idx,
            DATA_LENGTH    => DATA_LENGTH 
        )
        port map (
            clk           => sampa_clk,
            rst           => rst,
            en            => sampa_elink_enable(idx),

            rd_clk        => data_clk,
            rd_data       => sampa_data(idx),
            rd_en         => sampa_rd_en(idx),
            rd_available  => sampa_rd_available(idx)
        );
    end generate;

    process
    begin
        wait for 3.2 ns;
        rst <= '1';
        sampa_elink_enable <= (others => '0');
        wait for 3.2 ns * 20;
        rst <= '0';
        report "out of reset";
        
        wait for 3.2 ns * 30;
        sampa_elink_enable <= (others => '1');
        wait for 3.2 ns * 10;
        sampa_elink_enable <= (others => '0');
        wait;
    end process;

    process(data_clk)
    begin
        if (rising_edge(data_clk)) then
            if (data_sob = '1') then
                assert data_wr_en = '0' report "data_wr_en=1 when sob=1" severity error;
                assert not running report "sob before eob" severity error;
                running <= true;
                word_cnt <= DATA_LENGTH;
            elsif (data_eob = '1') then
                assert data_wr_en = '0' report "data_wr_en=1 when eob=1" severity error;
                assert running report "eob flag before sob flag" severity error;
                running <= false;
            end if;

            if (data_wr_en = '1') then
                assert word_cnt = to_integer(unsigned(data_out)) report "data_out doesn't match expected value" severity error;
                assert running report "data_wr_en=1 before rasing sob flag" severity error;
                word_cnt <= word_cnt - 1;
            end if;
        end if;
    end process;

end architecture;

