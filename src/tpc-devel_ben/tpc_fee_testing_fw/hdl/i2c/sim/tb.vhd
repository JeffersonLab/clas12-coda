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
    
    signal reg_write_data, reg_read_data : std_logic_vector(7 downto 0);
    signal reg_addr : std_logic_vector(5 downto 0);
    signal opcode : std_logic_vector(1 downto 0);
    
begin

    clk_gen(clk, 50.000E6, clk_period);

    i2c_master_i : entity work.sampa_i2c_master
    port map (
        clk   => clk,
        reset => rst,

        slave_addr     => "1001",
        reg_addr       => reg_addr,
        reg_write_data => reg_write_data,
        reg_read_data  => reg_read_data,
        
        opcode => opcode,
        en     => en,
    
        busy      => open,
        ack_error => open,

        sda => sda,
        scl => scl
    );

    i2c_slave_i : entity work.i2c_slave_fsm
    generic map (
    SLAVE_ADDR => "1111010"
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
        
        wait for 200 ns;
        
        en <= '1';
        opcode <= "11";
        reg_addr <= "011110";
        wait for clk_period * 100;
        en <= '0';
        wait for 110 us;

        en <= '1';
        opcode <= "10";
        reg_write_data <= x"be";
        wait for clk_period * 100;
        en <= '0';
        wait for 110 us;
        
        en <= '1';
        opcode <= "00";
        reg_write_data <= x"be";
        wait for clk_period * 100;
        en <= '0';
        wait for 110 us;
        
        en <= '1';
        opcode <= "01";
        reg_write_data <= x"be";
        wait for clk_period * 100;
        en <= '0';
        wait for 110 us;
        
        wait;
    end process;

end architecture;

