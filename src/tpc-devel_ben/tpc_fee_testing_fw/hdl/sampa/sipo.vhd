library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity sampa_sipo is
    port(
        clk   : in std_logic;
        sdi_p : in std_logic;
        sdi_n : in std_logic;
        
        count_val : in std_logic_vector(4 downto 0);
        load_val  : in std_logic;
        
        before_idelay : out std_logic;
        after_idelay : out std_logic;
        
        dout : out std_logic_vector(9 downto 0)
     );
end sampa_sipo;

architecture behv of sampa_sipo is
    
    signal dout_i     : std_logic_vector(dout'range);
    signal sdi, sdi_i, sdi_reg : std_logic;
    
    --attribute IODELAY_GROUP : STRING;
    --attribute IODELAY_GROUP of IDELAYE2_0 : label is "idelay_group_0";

begin

    IBUFDS_i : IBUFDS
    generic map (
        DIFF_TERM  => true,
        IOSTANDARD => "PPDS_25"
    )
    port map (   
        O => sdi_i, 
        I => sdi_p,  
        IB => sdi_n 
    );
    
    IDELAYE2_0 : IDELAYE2
    generic map (
        CINVCTRL_SEL          => "FALSE",    -- Enable dynamic clock inversion (FALSE, TRUE)
        DELAY_SRC             => "IDATAIN",  -- Delay input (IDATAIN, DATAIN)
        HIGH_PERFORMANCE_MODE => "FALSE",    -- Reduced jitter ("TRUE"), Reduced power ("FALSE")
        IDELAY_TYPE           => "VAR_LOAD", -- FIXED, VARIABLE, VAR_LOAD, VAR_LOAD_PIPE
        IDELAY_VALUE          => 0,         -- Input delay tap setting (0-31)
        PIPE_SEL              => "FALSE",    -- Select pipelined mode, FALSE, TRUE
        REFCLK_FREQUENCY      => 200.0,      -- IDELAYCTRL clock input frequency in MHz (190.0-210.0, 290.0-310.0).
        SIGNAL_PATTERN        => "DATA"      -- DATA, CLOCK input signal
    )
    port map (
        CNTVALUEOUT => open,       -- 5-bit output: Counter value output
        DATAOUT     => sdi,        -- 1-bit output: Delayed data output
        C           => clk,        -- 1-bit input: Clock input
        CE          => '0',        -- 1-bit input: Active high enable increment/decrement input
        CINVCTRL    => '0',        -- 1-bit input: Dynamic clock inversion input
        CNTVALUEIN  => count_val,  -- 5-bit input: Counter value input
        DATAIN      => '0',        -- 1-bit input: Internal delay data input
        IDATAIN     => sdi_i,      -- 1-bit input: Data input from the I/O
        INC         => '0',        -- 1-bit input: Increment / Decrement tap delay input
        LD          => load_val,   -- 1-bit input: Load IDELAY_VALUE input
        LDPIPEEN    => '0',        -- 1-bit input: Enable PIPELINE register to load data input
        REGRST      => '0'         -- 1-bit input: Active-high reset tap-delay input
    );

    before_idelay <= sdi_i;
    after_idelay <= sdi;
    
    process(clk)
    begin
        if (rising_edge(clk)) then
            dout_i <= sdi & dout_i(dout_i'left downto 1);
            dout <= dout_i;
        end if;
    end process;
    
end behv;
