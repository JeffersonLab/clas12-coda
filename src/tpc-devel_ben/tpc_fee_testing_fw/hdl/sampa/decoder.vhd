library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity sampa_decoder is
    port(
        clk  : in std_logic;
        rst  : in std_logic;
        en   : in std_logic;
        sdi_p : in std_logic;
        sdi_n : in std_logic;
        
        rd_clk        : in std_logic;
        rd_data       : out std_logic_vector(10 downto 0);
        rd_en         : in std_logic;
        rd_available  : out std_logic;
        rd_data_count : out std_logic_vector (12 downto 0)
     );
end sampa_decoder;

architecture behv of sampa_decoder is
    
    signal sampa_dout : std_logic_vector(9 downto 0);
    signal sampa_data : std_logic_vector(9 downto 0);
    signal sampa_dval : std_logic;
    signal sampa_locked : std_logic;
    
    signal tap_value : std_logic_vector(4 downto 0);
    signal tap_load  : std_logic;
    signal align_reset : std_logic;
    
    signal ber : std_logic;
    signal locked : std_logic;
    signal words_err : std_logic_vector(31 downto 0);
    signal words_ok  : std_logic_vector(31 downto 0);
    signal eye : std_logic_vector(31 downto 0);
    
    --attribute mark_debug : string;
   -- attribute mark_debug of locked : signal is "true";
   -- attribute mark_debug of sampa_data : signal is "true";
  --  attribute mark_debug of sampa_dval : signal is "true";
  --  attribute mark_debug of sampa_locked : signal is "true";
    
begin

    --vio_delay_i : entity work.vio_idelay
    --port map (
    --    clk        => clk, 
    --    probe_in0 => words_ok,
    --    probe_in1 => words_err, 
    --    probe_in2 => eye,
    --    probe_out0(0) => align_reset 
    --);

    sipo_i : entity work.sampa_sipo
    port map (
        clk   => clk,
        sdi_p => sdi_p,
        sdi_n => sdi_n,
        
        count_val => tap_value,
        load_val => tap_load,
        
        dout  => sampa_dout 
    );

    sampa_sync_i : entity work.sampa_sync 
    port map (
        clk  => clk,
        rst  => rst or tap_load,
        din  => sampa_dout,
        
        ber       => ber,
        words_ok  => words_ok,
        words_err => words_err,
        locked    => locked,
        
        dout => sampa_data,
        dval => sampa_dval,
        dlocked => sampa_locked
    );
    
    eyescan_i : entity work.sampa_eyescan 
    port map (
        clk  => clk,
        rst  => rst or align_reset,
        
        ber       => ber,
        is_locked => locked,
        words_ok  => words_ok,
        words_err => words_err,
        
        tap_load  => tap_load,
        tap_value => tap_value,

        eye => eye
     );
     
     elink_i : entity work.sampa_elink
     port map (
        clk => clk,
        rst => rst,
        en => en,
        
        din  => sampa_data,
        dval => sampa_dval,
        dlocked => sampa_locked,
        
        rd_clk => rd_clk,
        rd_en => rd_en,
        rd_data => rd_data,
        rd_available  => rd_available
     );
    
end behv;
