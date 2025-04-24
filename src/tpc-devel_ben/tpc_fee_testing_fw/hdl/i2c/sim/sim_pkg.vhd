library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_bit.all;
use ieee.std_logic_textio.all;
use ieee.math_real.all;

library std;
use std.textio.all;

package sim_pkg is
    constant DATA_BITS : natural := 16;
    constant DEPTH : natural := 178;

    subtype word_t  is std_logic_vector(DATA_BITS - 1 downto 0);
    type    ram_t   is array(0 to DEPTH - 1) of word_t;
    
    procedure clk_gen(signal clk : out std_logic; constant FREQ : real);
    procedure clk_gen(signal clk : out std_logic; constant FREQ : real; signal period_o : out time);
    impure function read_mem_file(FileName : STRING) return ram_t;
end package sim_pkg;

package body sim_pkg is 
  -- Procedure for clock generation
  procedure clk_gen(signal clk : out std_logic; constant FREQ : real) is
    constant PERIOD    : time := 1 sec / FREQ;        -- Full period
    constant HIGH_TIME : time := PERIOD / 2;          -- High time
    constant LOW_TIME  : time := PERIOD - HIGH_TIME;  -- Low time; always >= HIGH_TIME
  begin
    -- Check the arguments
    assert (HIGH_TIME /= 0 fs) 
    report "clk_plain: High time is zero; time resolution to large for frequency" severity FAILURE;
    -- Generate a clock cycle
    INF_LOOP:
    loop
      clk <= '1';
      wait for HIGH_TIME;
      clk <= '0';
      wait for LOW_TIME;
    end loop INF_LOOP;
  end procedure clk_gen;

  procedure clk_gen(signal clk : out std_logic; constant FREQ : real; signal period_o : out time) is
    constant PERIOD    : time := 1 sec / FREQ;        -- Full period
  begin
    period_o <= PERIOD;
    clk_gen(clk, FREQ);
  end procedure clk_gen;
  

    impure function read_mem_file(FileName : STRING) return ram_t is
      file FileHandle       : TEXT open READ_MODE is FileName;
      variable CurrentLine  : LINE;
      variable TempWord     : STD_LOGIC_VECTOR(DATA_BITS-1 downto 0);
      variable Result       : ram_t    := (others => (others => '0'));
    begin
      for i in 0 to DEPTH - 1 loop
        exit when endfile(FileHandle);

        readline(FileHandle, CurrentLine);
        hread(CurrentLine, TempWord);
        Result(i)    := TempWord;
      end loop;

      return Result;
    end function;
  
end sim_pkg;
