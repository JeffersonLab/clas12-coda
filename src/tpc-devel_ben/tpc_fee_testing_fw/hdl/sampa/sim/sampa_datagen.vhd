library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;
--use work.sampa_pkg.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity sampa_datagen is
    generic (
        CHANNEL_NUMBER : integer := 0;
        DATA_LENGTH    : integer := 1007 
    );
    port (
        clk  : in std_logic;
        rst  : in std_logic;
        
        en   : in std_logic;

        rd_clk        : in std_logic;
        rd_data       : out std_logic_vector(10 downto 0);
        rd_en         : in std_logic;
        rd_available  : out std_logic
    );
end sampa_datagen;

architecture behv of sampa_datagen is

    signal words_to_write : integer;
    signal total_words    : integer := DATA_LENGTH;

    signal start : std_logic_vector(2 downto 0);
    
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
            if (rst = '1') then
                start <= (others => '0');
                wr_en <= '0';
                fifo_din <= (others => '0');
                words_to_write <= total_words;
            else
                start(0) <= en;
                start(1) <= start(0);

                if (start = "01") then
                    start(2) <= '1';
                    wr_en <= '1';
                    fifo_din <= '1' & std_logic_vector(to_unsigned(words_to_write, 10));
                end if;

                if (start(2) = '1') then
                    if (words_to_write = 1) then
                        wr_en <= '0';
                        words_to_write <= total_words;
                        fifo_din <= (others => '0');
                        start(2) <= '0';
                    elsif (words_to_write > 1) then
                        wr_en <= '1';
                        fifo_din <= '0' & std_logic_vector(to_unsigned(words_to_write-1, 10));
                        words_to_write <= words_to_write - 1;
                    end if;
                end if;

            end if;
        end if;
    end process;

    
end behv;
