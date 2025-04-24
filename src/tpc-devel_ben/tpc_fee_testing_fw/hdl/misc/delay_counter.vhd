----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 09/21/2017 05:10:48 PM
-- Design Name: 
-- Module Name: delay_counter - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_unsigned.all;


-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity delay_counter is
    Port ( clk : in STD_LOGIC;
           rst : in STD_LOGIC;
           cnt : in STD_LOGIC_VECTOR (15 downto 0);
           en  : in std_logic;
           ack : out std_logic;
           trig_in : in STD_LOGIC;
           trig_out : out STD_LOGIC);
end delay_counter;

architecture Behavioral of delay_counter is

    type  state_t is (IDLE, COUNT, DELAY_1, DELAY_2);
    signal state : state_t;

    signal en_i : std_logic;
    signal cnt_i : std_logic_vector(cnt'range);
    signal trig_i : std_logic_vector(1 downto 0);
 
        
begin

    process(clk, rst)
    begin
        if (rst = '1') then
            trig_out <= '0';
            cnt_i <= (others => '0');
            trig_i <= (others => '0');
            state <= IDLE;
            ack <= '0';
        elsif (rising_edge(clk)) then
            en_i <= en;
            ack <= '0';
            
            trig_i(0) <= trig_in;
            trig_i(1) <= trig_i(0);

            case (state) is
                when IDLE =>
                    if (trig_i = "01" and en_i = '1') then
                        trig_out <= '1';
                        ack <= '1';
                        cnt_i <= cnt;
                        state <= COUNT;
                    end if;
                                       
                when COUNT =>
                    if (cnt_i = x"0000") then
                        trig_out <= '0';
                        cnt_i <= (others => '0');
                        state <= DELAY_1;
                    else
                        cnt_i <= cnt_i - 1;
                    end if;

                when DELAY_1 => state <= DELAY_2;
                when DELAY_2 => state <= IDLE;

            end case;
        end if;
    end process;

end Behavioral;
