library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity reset_manager is
    port(
        slow_clk   : in std_logic;
        user_reset : in std_logic;
        pll_locked : in std_logic;
        reset_out  : out std_logic;
        
        si5338_en   : out std_logic;
        si5338_busy : in std_logic;
        sampa_reset : out std_logic;
        phy_reset : out std_logic
     );
end reset_manager;

architecture behv of reset_manager is

    signal cnt : std_logic_vector(7 downto 0) := x"00";
    
    signal phy_cnt : std_logic_vector(7 downto 0) := x"00";

    signal si5338_busy_i : std_logic_vector(1 downto 0);
    signal phy_ready : std_logic := '0';
    
    attribute mark_debug : string;    
    attribute mark_debug of si5338_busy : signal is "true";  
    attribute mark_debug of si5338_busy_i : signal is "true";  
    attribute mark_debug of phy_ready : signal is "true";  
    attribute mark_debug of phy_cnt : signal is "true";  
    attribute mark_debug of cnt : signal is "true";  
    attribute mark_debug of si5338_en : signal is "true";  
    attribute mark_debug of reset_out : signal is "true";  
    attribute mark_debug of phy_reset : signal is "true";  
    
begin

    -- Main Reset Manager
    process(slow_clk, pll_locked)
    begin
        if (pll_locked = '0') then
            cnt <= (others => '0');
            reset_out <= '1';
            sampa_reset <= '1';
            si5338_en <= '0';
        elsif (rising_edge(slow_clk)) then
            if (user_reset = '1') then
                cnt <= (others => '0');
                reset_out <= '1';
                sampa_reset <= '1';
                si5338_en <= '0';
            else
                cnt <= cnt + x"1";
                case cnt is
                    when x"00" => reset_out <= '1'; sampa_reset <= '1';
                    when x"a0" => reset_out <= '0'; 
                    when x"a5" => if (phy_ready = '0') then si5338_en <= '1'; end if;
                    when x"af" => if (phy_ready = '0') then si5338_en <= '0'; end if;
                    when x"b0" => sampa_reset <= '0';
                    when x"ff" => cnt <= x"ff";
                    when others => null;
                end case;
            end if;
        end if;
    end process;
    
    -- PHY Reset
    process(slow_clk, pll_locked)
    begin
        if (pll_locked = '0') then
            phy_ready <= '0';
            phy_cnt <= (others => '0');
            phy_reset <= '1';
        elsif (rising_edge(slow_clk)) then
            si5338_busy_i(0) <= si5338_busy;
            si5338_busy_i(1) <= si5338_busy_i(0);
            
            if (si5338_busy_i = "10") then --falling edge
                phy_ready <= '1';
            end if;
            
            if (phy_ready = '1') then
                phy_cnt <= phy_cnt + x"1";
                phy_reset <= '1';
                if (phy_cnt = x"ff") then
                    phy_cnt <= x"ff";
                    phy_reset <= '0';
                end if;
            else
                phy_reset <= '1';
            end if;    
        end if;
    end process;    

end behv;
