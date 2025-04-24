----------------------------------------------------------------------------------
-- Company: Sydor Instruments
-- Engineer: 
-- 
-- Create Date:    14:18:57 12/21/2012 
-- Design Name: 
-- Module Name:    Random - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--  Dead Time Calibration.  Uses Linear feedback shift register to form psuedo random
--  values.  At each clock event, a number is formed and compared to a probability constant
--  If the value is below that probability, a output tick is generated.  This creates a 
--  Gaussian distribution.    The mean frequency is probabity * clock_frequency.
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
-- See http://en.wikipedia.org/wiki/Linear_feedback_shift_register
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity pseudorandom is
    Port ( clk : in  STD_LOGIC; -- master clock (80 MHz)
           rst : in  STD_LOGIC;
           prob  : in std_logic_vector(9 downto 0);
           pulse : out  STD_LOGIC;
		   debug0 : out STD_LOGIC);
end pseudorandom;

architecture Behavioral of pseudorandom is
signal pulse_int : std_logic;
signal pulse_count: integer range 0 to 3;

-- Make one register 18 bits long  - this repeats every 26 ms at 10 MHz, or every 262140 times
signal bit0 : std_logic;
signal lfsr0 : std_logic_vector(17 downto 0) := "101011001110000100" ; -- was x"ACE1";

-- Make another register 18 bits long 
signal bit1 : std_logic;
signal lfsr1 : std_logic_vector(17 downto 0) := "101011001110001000"; -- was x"ACE2";

signal bit2 : std_logic;
signal lfsr2 : std_logic_vector(15 downto 0) := x"ACE4";

signal bit3 : std_logic;
signal lfsr3 : std_logic_vector(15 downto 0) := x"BEEF";

signal bit4 : std_logic;
signal lfsr4 : std_logic_vector(15 downto 0) := x"BABE";

signal bit5 : std_logic;
signal lfsr5 : std_logic_vector(15 downto 0) := x"C80A";

signal bit6 : std_logic;
signal lfsr6 : std_logic_vector(15 downto 0) := x"C93C";

signal bit7 : std_logic;
signal lfsr7 : std_logic_vector(15 downto 0) := x"D57D";

signal bit8 : std_logic;
signal lfsr8 : std_logic_vector(15 downto 0) := x"E68A";

signal bit9 : std_logic;
signal lfsr9 : std_logic_vector(15 downto 0) := x"ABCD";

begin

random:process (clk,rst)
variable val : std_logic_vector(9 downto 0);
--variable prob : std_logic_vector(9 downto 0) := "0000000000";
--variable prob : std_logic_vector(9 downto 0) := "1000000000";

begin
    if (rst = '1') then
       bit0 <= '0';
       bit1 <= '0';
       bit2 <= '0';
       bit3 <= '0';
       bit4 <= '0';
       bit5 <= '0';
       bit6 <= '0';
       bit7 <= '0';
       bit8 <= '0';
       bit9 <= '0';
       
	elsif rising_edge(clk) then
-- Commented out code for 16 bit long register	
--	   bit0  <= ((lfsr0(0)) xor (lfsr0(2)) xor (lfsr0(3)) xor (lfsr0(5) ) );
--		lfsr0(15 downto 0) <= bit0 & lfsr0(15 downto 1); -- shift right

--    New code for 18 bit long register
	   bit0  <= ((lfsr0(0)) xor (lfsr0(1)) xor (lfsr0(2)) xor (lfsr0(5)) );
		lfsr0(17 downto 0) <= bit0 & lfsr0(17 downto 1); -- shift right
		
		-- add 'random' element by using the amount of time that unit has been on.
		-- everytime the other registers wrap around to the starting seed value,
		-- modify the first register with more psuedo random

		-- TODO - perhaps the processor (Arcturus) could send some bits based on something random
		-- like last user keypress or something.. 
		-- yah- instead of hardware randomness Use the Arcturus - makes more sense..
		
--	   bit1  <= ((lfsr1(0)) xor (lfsr1(2)) xor (lfsr1(3)) xor (lfsr1(5) ) );
--		lfsr1(15 downto 0) <= bit1 & lfsr1(15 downto 1); -- shift right

	   bit1  <= ((lfsr1(0)) xor (lfsr1(1)) xor (lfsr1(2)) xor (lfsr1(5) ) );
		lfsr1(17 downto 0) <= bit1 & lfsr1(17 downto 1); -- shift right

	   bit2  <= ((lfsr2(0)) xor (lfsr2(2)) xor (lfsr2(3)) xor (lfsr2(5) ) );
		lfsr2(15 downto 0) <= bit2 & lfsr2(15 downto 1); -- shift right

	   bit3  <= ((lfsr3(0)) xor (lfsr3(2)) xor (lfsr3(3)) xor (lfsr3(5) ) );
		lfsr3(15 downto 0) <= bit3 & lfsr3(15 downto 1); -- shift right

	   bit4  <= ((lfsr4(0)) xor (lfsr4(2)) xor (lfsr4(3)) xor (lfsr4(5) ) );
		lfsr4(15 downto 0) <= bit4 & lfsr4(15 downto 1); -- shift right

	   bit5  <= ((lfsr5(0)) xor (lfsr5(2)) xor (lfsr5(3)) xor (lfsr5(5) ) );
		lfsr5(15 downto 0) <= bit5 & lfsr5(15 downto 1); -- shift right

	   bit6  <= ((lfsr6(0)) xor (lfsr6(2)) xor (lfsr6(3)) xor (lfsr6(5) ) );
		lfsr6(15 downto 0) <= bit6 & lfsr6(15 downto 1); -- shift right

	   bit7  <= ((lfsr7(0)) xor (lfsr7(2)) xor (lfsr7(3)) xor (lfsr7(5) ) );
		lfsr7(15 downto 0) <= bit7 & lfsr7(15 downto 1); -- shift right

	   bit8  <= ((lfsr8(0)) xor (lfsr8(2)) xor (lfsr8(3)) xor (lfsr8(5) ) );
		lfsr8(15 downto 0) <= bit8 & lfsr8(15 downto 1); -- shift right

	   bit9  <= ((lfsr9(0)) xor (lfsr9(2)) xor (lfsr9(3)) xor (lfsr9(5) ) );
		lfsr9(15 downto 0) <= bit9 & lfsr9(15 downto 1); -- shift right

		val := bit9 & bit8 & bit7 & bit6 & bit5 & bit4 & bit3 & bit2 & bit1 & bit0;
		if (val <= prob) and (pulse_int = '0') then
			pulse_int <= '1';
		else
			pulse_int <= '0';
		end if;
	end if;
end process random;


p_stretch:process (pulse_int,clk)
begin
	if (pulse_int = '1') then 
		pulse_count <= 1;
	elsif rising_edge(clk) then
		if (pulse_count /= 0) then
			pulse_count <= pulse_count-1;
		end if;
	end if;
end process p_stretch;


-- concurrent assignment
--  pulse when pulse_int is active and for as long a counter is active to 
--  stretch out the pulse.
	pulse <= '1' when (pulse_int = '1' or pulse_count /= 0) else '0'; 
	

debug0 <= '1' when (
		lfsr0 = "101011001110000100" and 
		lfsr1 = "101011001110001000"
		) else '0';
end Behavioral;

