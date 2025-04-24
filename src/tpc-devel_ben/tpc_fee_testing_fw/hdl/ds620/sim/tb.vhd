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


entity tb is
end entity;

architecture sim of tb is
    signal clk_period : time;
    
    signal clk, rst : std_logic;
    signal en, busy : std_logic;
    signal sda, scl : std_logic;
    
begin

    clk_gen(clk, 50.000E6, clk_period);

    pll_init_i : entity work.ds620_ctrl
    port map (
        clk_50MHz  => clk,
        rst        => rst,

        en   => en,
        busy => busy,
        
        valid => open,
        temp_0 => open,
        temp_1 => open,
        temp_2 => open,
        temp_3 => open,

        i2c_sda => sda,
        i2c_scl => scl
    );

    i2c_slave_i : entity work.i2c_slave_fsm
    generic map (
    SLAVE_ADDR => "1001000"
    )
    port map (
    scl             => scl,
    sda             => sda,
        
    in_progress     => open,

    tx_done         => open,
    tx_byte         => x"c0",

    rx_byte         => open,
    rx_data_rdy     => open,

    clk         => clk 
    );
    
    i2c_slave_2 : entity work.i2c_slave_fsm
    generic map (
    SLAVE_ADDR => "1001001"
    )
    port map (
    scl             => scl,
    sda             => sda,
        
    in_progress     => open,

    tx_done         => open,
    tx_byte         => x"c1",

    rx_byte         => open,
    rx_data_rdy     => open,

    clk         => clk 
    );
    
    i2c_slave_3 : entity work.i2c_slave_fsm
    generic map (
    SLAVE_ADDR => "1001010"
    )
    port map (
    scl             => scl,
    sda             => sda,
        
    in_progress     => open,

    tx_done         => open,
    tx_byte         => x"c2",

    rx_byte         => open,
    rx_data_rdy     => open,

    clk         => clk 
    );
    
    i2c_slave_4 : entity work.i2c_slave_fsm
    generic map (
    SLAVE_ADDR => "1001011"
    )
    port map (
    scl             => scl,
    sda             => sda,
        
    in_progress     => open,

    tx_done         => open,
    tx_byte         => x"c3",

    rx_byte         => open,
    rx_data_rdy     => open,

    clk         => clk 
    );



    scl <= '1' when scl = 'Z' else
           'Z' when scl = 'X' else
           'Z' when scl = 'U'; 
    sda <= '1' when sda = 'Z' else
           'Z' when sda = 'X' else
           'Z' when sda = 'U'; 

    process
    begin
        rst <= '1';
        en <= '0';
        wait for 200 ns;
        rst <= '0';
        report "out of reset";
        
        wait for 3 ms;
        
        en <= '1';
        wait for clk_period*3;
        en <= '0';
        
        wait for 1 ms;
        
        en <= '1';
        wait for clk_period*3;
        en <= '0';
        wait;
    end process;

end architecture;

