library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.math_real.all;

entity led_activity is
    generic (
        CLK_FREQ : real := 125.0E+6; -- Hz
        TOGGLE   : real := 0.2       -- Seconds
    );
    port ( 
        clk     : in STD_LOGIC;
        reset   : in STD_LOGIC;
    	act_in  : in STD_LOGIC;
        led_out : out STD_LOGIC
    );
end led_activity;

architecture behv of led_activity is

    constant DELAY_TIME  : natural := natural(0.1 * CLK_FREQ); 
    constant TOGGLE_TIME : natural := natural(TOGGLE * CLK_FREQ); 
    signal cnt : std_logic_vector(natural(ceil(log2(TOGGLE * CLK_FREQ))) downto 0);
    signal led : std_logic;

    signal act : std_logic_vector(1 downto 0);
    type  state_t is (IDLE, FLASH, DELAY);
    signal state : state_t;
    
begin

    led_out <= led;

    process(clk, reset)
    begin
        if (reset = '1') then
            led <= '0';
            cnt <= (others => '0');
            act <= (others => '0');
	    state <= IDLE;
        elsif (rising_edge(clk)) then
	    act(0) <= act_in;
	    act(1) <= act(0);

	    case (state) is
		    when IDLE =>
			led <= '1';
		        if (act = "01") then -- Rising edge
			    state <= FLASH;
		         end if;
		    when FLASH =>
			led <= '0';
			cnt <= cnt + 1;
			if (cnt = TOGGLE_TIME) then
			    state <= DELAY;
			    cnt <= (others => '0');
			    led <= '1';
			 end if;
		    when DELAY =>
			led <= '1';
			cnt <= cnt + 1;
			if (cnt = DELAY_TIME) then
			    state <= IDLE;
			    cnt <= (others => '0');
			end if;
	    end case;
        end if;
    end process;

end behv;
