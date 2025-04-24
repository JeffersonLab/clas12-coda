library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.math_real.all;

entity sampa_reset_manager is
    generic (
        CLK_FREQ  : real := 40.0E+6; -- Hz
        WAIT_TIME : real := 0.5      -- Seconds
    );
    port(
        clk         : in std_logic;
        reset       : in std_logic;
        sampa_init  : in std_logic;
        
        sampa_por    : in std_logic;
        sampa_pwr_dig_en : out std_logic;
        sampa_pwr_ana_en : out std_logic;
        sampa_ext_reset : out std_logic;
        sampa_reset  : out std_logic;
        sampa_clk_en : out std_logic
     );
end sampa_reset_manager;

architecture behv of sampa_reset_manager is

    constant CLK_EN_WAIT_TIME : natural := natural(WAIT_TIME * CLK_FREQ);
    constant RST_EN_WAIT_TIME : natural := natural(0.5 * CLK_FREQ);
    signal cnt : std_logic_vector(natural(ceil(log2(WAIT_TIME * CLK_FREQ))) downto 0) := (others => '0');
    signal init : std_logic_vector(1 downto 0);
    signal por : std_logic_vector(1 downto 0);
    
    type state_t is (IDLE, WAIT_FOR_POR, TIME_DELAY_FOR_CLK, TIME_DELAY_FOR_RST);
    signal state : state_t;
    
begin

    -- SAMPA Reset Manager --
    process(clk, reset)
    begin
        if (reset = '1') then
            cnt <= (others => '0');
            init <= (others => '0');
            por <= (others => '0');
            sampa_reset <= '1';
            sampa_clk_en <= '0';
            sampa_pwr_dig_en <= '0';
            sampa_pwr_ana_en <= '0';
            sampa_ext_reset <= '0';
            state <= IDLE;
        elsif (rising_edge(clk)) then
            por(0) <= sampa_por;
            por(1) <= por(0);
            case state is
                when IDLE =>
                    init(0) <= sampa_init;
                    init(1) <= init(0);
                    if (init = b"01") then
                        cnt <= (others => '0');
                        sampa_pwr_dig_en <= '1';
                        sampa_pwr_ana_en <= '0';
                        sampa_clk_en <= '0';
                        sampa_ext_reset <= '1';
                        sampa_reset <= '1';
                        state <= WAIT_FOR_POR;
                    end if;
                    
                when WAIT_FOR_POR =>
                    sampa_ext_reset <= '1';
                        sampa_reset <= '1';
                    if (por = b"01") then
                        state <= TIME_DELAY_FOR_CLK;
                        cnt <= (others => '0');
                    end if;
                    
                when TIME_DELAY_FOR_CLK =>
                    sampa_ext_reset <= '1';
                        sampa_reset <= '1';
                    cnt <= cnt + x"1";
                    if (cnt = CLK_EN_WAIT_TIME) then
                        cnt <= (others => '0');
                        sampa_clk_en <= '1';
                        state <= TIME_DELAY_FOR_RST;
                    end if;
                    
                when TIME_DELAY_FOR_RST =>
                    sampa_ext_reset <= '1';
                        sampa_reset <= '1';
                    cnt <= cnt + x"1";
                    if (cnt = RST_EN_WAIT_TIME) then
                        cnt <= (others => '0');
                        sampa_ext_reset <= '1';
                        sampa_pwr_ana_en <= '1';
                            sampa_reset <= '0';
                        state <= IDLE;
                    end if;
            end case;
        end if;
    end process;
    
end behv;
