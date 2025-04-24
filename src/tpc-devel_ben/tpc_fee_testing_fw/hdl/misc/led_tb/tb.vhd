library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_bit.all;
use ieee.std_logic_textio.all;
use ieee.math_real.all;

library std;
use std.textio.all;

library work;
use work.sim_pkg.all;
--use work.txt_util.all;

entity tb is
end entity;

architecture sim of tb is
    signal reset : std_logic;
    signal clk : std_logic;
    signal act : std_logic;

begin

    clk_gen(clk, 200.000E6); 

 led_i : entity work.led_heartbeat 
    port map ( 
        clk     => clk,
        reset   => reset,
        led_out => open
    );

    process
    begin
        reset <= '0';
        act <= '0';
        wait for 200 ns;
        reset <= '0';
        report "out of reset";

	wait for 50 ns;
	act <= '1';
	wait for 50 ns;
	act <= '0';

        wait;
    end process;

end architecture;

