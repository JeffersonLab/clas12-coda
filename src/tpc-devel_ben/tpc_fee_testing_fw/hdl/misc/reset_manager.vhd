library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity reset_manager is
    port(
        clk        : in std_logic;
        user_reset : in std_logic;
        pll_locked : in std_logic;
        
        reset_out  : out std_logic;
        sampa_init : out std_logic
     );
end reset_manager;

architecture behv of reset_manager is

    signal cnt : std_logic_vector(7 downto 0) := x"00";
    
begin

    -- Main Reset Manager
    process(clk, pll_locked)
    begin
        if (pll_locked = '0') then
            cnt <= (others => '0');
            reset_out <= '1';
            sampa_init <= '0';
        elsif (rising_edge(clk)) then
            if (user_reset = '1') then
                cnt <= (others => '0');
                reset_out <= '1';
                sampa_init <= '0';
            else
                cnt <= cnt + x"1";
                case cnt is
                    when x"00" => reset_out <= '1'; sampa_init <= '0';
                    when x"0f" => reset_out <= '0'; sampa_init <= '0';
                    when x"10" => reset_out <= '0'; sampa_init <= '1'; 
                    when x"11" => reset_out <= '0'; sampa_init <= '0';
                    when x"ff" => cnt <= x"ff";
                    when others => null;
                end case;
            end if;
        end if;
    end process;
    
end behv;
