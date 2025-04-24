library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity dma_datagen is
    generic (
        TOTAL_LENGTH : std_logic_vector(15 downto 0) := x"FFFF"
    );
    port (
        clk : in std_logic;
        rst_i : in std_logic;
        trig_i : in std_logic;

        data_o : out std_logic_vector(255 downto 0);
        wren_o : out std_logic
    );
end dma_datagen;

architecture behv of dma_datagen is

    signal trig : std_logic_vector(1 downto 0);
    signal enabled : std_logic := '0';
    signal count : std_logic_vector(15 downto 0);
    

    attribute MARK_DEBUG    : string;
    attribute MARK_DEBUG of trig : signal is "TRUE";
    attribute MARK_DEBUG of enabled : signal is "TRUE";
    attribute MARK_DEBUG of count : signal is "TRUE";
    attribute MARK_DEBUG of data_o : signal is "TRUE";
    attribute MARK_DEBUG of wren_o : signal is "TRUE";

begin
    
    G_LOOP:
    for i in 0 to 15 generate
        data_o((i+1)*16-1 downto i*16) <= count;
    end generate;
    wren_o <= enabled;

    process(clk)
    begin
        if (rising_edge(clk)) then
            if (rst_i = '1') then
                trig <= (others => '0');
                count <= (others => '0');
                enabled <= '0';
            else
                trig(0) <= trig_i;
                trig(1) <= trig(0);
                if (trig = "01") then
                    enabled <= '1';
                    count <= (others => '0');
                end if;

                if (enabled = '1') then
                    if (count = TOTAL_LENGTH) then
                        enabled <= '0';
                        count <= (others => '0');
                    else
                        count <= count + x"0001";
                        enabled <= '1';
                    end if;
                end if;

            end if;
        end if;
    end process;

end;

