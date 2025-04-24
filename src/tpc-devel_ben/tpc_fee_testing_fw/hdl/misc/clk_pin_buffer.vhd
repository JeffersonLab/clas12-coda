library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

library UNISIM;
use UNISIM.VCOMPONENTS.ALL;

entity clk_pin_buffer is
    port ( 
        I    : in STD_LOGIC;
		  O : out std_logic;
		  OB : out std_logic
    );
end clk_pin_buffer;

architecture Behavioral of clk_pin_buffer is

	signal clk_n : std_logic;
	signal diff_clk_0: std_logic;
	
--	component OBUFDS
--	port(
--		I : in std_logic;
--		O : out std_logic;
--		OB : out std_logic);
--	end component;
	
--	component ODDR2
--	port(
--		Q : out std_logic;
--		CE : in std_logic;
--		D0 : in std_logic;
--		D1 : in std_logic;
--		C0 : in std_logic;
--		C1 : in std_logic
--		);
--	end component;

begin

	clk_n <= not I;

	CLK_ODDR2_0 : ODDR2 
	port map (
		Q => diff_clk_0,
		C0 => clk_n,
		C1 => I,
		CE => '1',
		D0 => '1',
		D1 => '0'
	);
	
	clk_0 : OBUFDS
--	generic map (
--		IOSTANDARD => "LVDS_25"
--	)
	port map (
		O	=> O,
		OB	=> OB,
		I	=> diff_clk_0
	);



end Behavioral;