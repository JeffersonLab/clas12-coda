
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity trigger_rx is
    port ( 
        clk          : in std_logic;
        reset        : in std_logic;
        trigger_ack  : in std_logic;
        trigger      : out std_logic;
        en           : in std_logic;
        pulse        : in std_logic;
        freq         : out std_logic_vector(31 downto 0)
);
end trigger_rx;

architecture behv of trigger_rx is

    signal curr_cnt, prev_cnt, freq_i : unsigned(freq'range);
    signal pulse_in, trig_ack : unsigned(1 downto 0);
    signal trigger_i, en_i : std_logic;

begin
    
    freq <= std_logic_vector(freq_i);
    trigger <= trigger_i;
    
    process(clk, reset)
    begin
        if (reset = '1') then
            curr_cnt <= (others => '0');
            prev_cnt <= (others => '0');
            freq_i <= (others => '0');
            trig_ack <= (others => '0');
            trigger_i <= '0';
            en_i <= '0';
        elsif (rising_edge(clk)) then            
            en_i <= en;
            curr_cnt <= curr_cnt + 1; 
            
            pulse_in(0) <= pulse;
            pulse_in(1) <= pulse_in(0);
            
            trig_ack(0) <= trigger_ack;
            trig_ack(1) <= trig_ack(0);
            
            if (pulse_in = b"01" and trigger_i = '0') then
                trigger_i <= '1';
                
                if (prev_cnt < curr_cnt) then
                    freq_i <= curr_cnt - prev_cnt;
                elsif (prev_cnt > curr_cnt) then
                    freq_i <= x"80000000" - prev_cnt + curr_cnt;
                end if;
                
                prev_cnt <= curr_cnt;           
            elsif (trig_ack(1) = '1' and trigger_i = '1') then
                trigger_i <= '0';
            end if;
            
        end if;
    end process;

end behv;
