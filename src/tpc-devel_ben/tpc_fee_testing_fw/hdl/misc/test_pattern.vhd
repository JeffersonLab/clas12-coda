library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity test_pattern is
    port (
        clk : in std_logic;
        rst : in std_logic;
        trig : in std_logic;
        length : in std_logic_vector(15 downto 0);
       
        data_clk   : out std_logic;
        data_wr_en : out std_logic;
        data_sob   : out std_logic;
        data_eob   : out std_logic;
        data_in    : out std_logic_vector(15 downto 0)
    );
end test_pattern;

architecture behv of test_pattern is
   signal cnt : std_logic_vector(15 downto 0);
   signal len : std_logic_vector(length'range);
   
   signal trig0, trig1, start : std_logic;
begin

    data_clk <= clk;
    
    process(clk, rst)
    begin
        if (rst = '1') then
            data_sob <= '0';
            data_eob <= '0';
            data_in <= (others => '0');
            trig0 <= '0';
            trig1 <= '0';
            start <= '0';
            data_wr_en <= '0';
            cnt <= (others => '0');
            len <= x"03ea";
        elsif (rising_edge(clk)) then
            data_sob <= '0';
            data_eob <= '0';
            trig0 <= trig;
            trig1 <= trig0;
            
            if (trig0 = '1' and trig1 = '0') then
                start <= '1';
                len <= length;
                cnt <= (others => '0');
                data_sob <= '1';
            end if;
            
            if (start = '1') then
                data_wr_en <= '1';
                cnt <= cnt + 1;
                data_in <= cnt;
                
                if (cnt = len) then
                    data_wr_en <= '0';
                    cnt <= (others => '0');
                    data_eob <= '1';
                    start <= '0';
                end if;
            end if;
            
        end if;
    end process;

end behv;
