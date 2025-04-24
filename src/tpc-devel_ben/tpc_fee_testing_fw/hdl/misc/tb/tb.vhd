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
use work.txt_util.all;

entity tb is
end entity;

architecture sim of tb is
    signal fast_clk_period, clk_period : time;
    signal reset : std_logic;
    signal clk, fast_clk : std_logic;
    signal trig, ack, trig_out, en : std_logic;

begin

    clk_gen(fast_clk, 100.000E6, fast_clk_period); 
    clk_gen(clk, 10.000E6, clk_period); 
    --clk_gen(trig, 1.0E6);

    delay_counter_i : entity work.delay_counter
    port map (
        clk => clk,
        rst => reset,
        cnt => x"0000",
        en  => en,
        ack => ack,
        trig_in => trig_out,
        trig_out => open
    );

    trigger_rx_i : entity work.trigger_rx
    port map (
        clk          => fast_clk,
        reset        => reset,
        trigger_ack  => ack,
        en           => en,
        trigger      => trig_out,
        pulse        => trig,
        freq         => open
    );

    psrand_i : entity work.pseudorandom 
    port map (
        clk => fast_clk,
        rst => reset,
        prob  => b"0001000000",
        pulse => trig,
        debug0 => open
       );


    process
    begin
        reset <= '1';
        en <= '0';
        wait for 200 ns;

        en <= '1';
        reset <= '0';
        report "out of reset";

        wait;
    end process;

end architecture;

