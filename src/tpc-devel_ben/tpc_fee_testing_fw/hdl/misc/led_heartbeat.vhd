library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity led_heartbeat is
    generic (
        CLK_FREQ : real := 40.0E+6; -- Hz
        TOGGLE   : real := 0.2      -- Seconds
    );
    port ( 
        clk     : in std_logic;
        reset   : in std_logic;
        led_out : out std_logic_vector(3 downto 0) 
    );
end led_heartbeat;

architecture behv of led_heartbeat is

    constant TOGGLE_TIME : natural := natural(TOGGLE * CLK_FREQ);
    signal cnt : std_logic_vector(natural(ceil(log2(TOGGLE * CLK_FREQ))) downto 0) := (others => '0');
    signal led : std_logic_vector(3 downto 0) := b"0001";
    
begin

    led_out <= led;

    process(clk, reset)
        variable count_up : std_logic := '1';
    begin
        if (reset = '1') then
            led <= b"0001";
            cnt <= (others => '0');
            count_up := '1';
        elsif (rising_edge(clk)) then
            cnt <= cnt + 1;
            if (cnt = TOGGLE_TIME) then
                cnt <= (others => '0');
                
                if (led = b"1000") then
                    count_up := '0';
                elsif (led = b"0001") then
                    count_up := '1';
                end if;
                
                if (count_up = '1') then
                    led <= std_logic_vector(shift_left(unsigned(led), 1));
                else
                    led <= std_logic_vector(shift_right(unsigned(led), 1));
                end if;
            end if;
        end if;
    end process;

end behv;
