library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

entity counter is
    port (
        clk_i : in std_logic;
        rst_i : in std_logic;
        d_i   : in std_logic;
        count_o : out std_logic_vector(31 downto 0));
end counter;

architecture behv of counter is
    signal edge : std_logic_vector(1 downto 0) := (others => '0');
    signal cnt  : unsigned(31 downto 0) := (others => '0');
begin

    count_o <= std_logic_vector(cnt);

    process(clk_i)
    begin
        if (rising_edge(clk_i)) then
            edge(0) <= d_i;
            edge(1) <= edge(0);
        end if;
    end process;

    process(clk_i)
    begin
        if (rising_edge(clk_i)) then
            if (rst_i = '1' ) then
                cnt <= (others => '0');
            else
                if (edge = "01") then
                    cnt <= cnt + x"0000_0001";
                end if;
            end if;
        end if;
    end process;

end behv;
