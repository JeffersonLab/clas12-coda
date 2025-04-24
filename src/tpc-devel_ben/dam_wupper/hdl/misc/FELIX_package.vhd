library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package FELIX_package is  

--Some types combined from GBT / lpgbt package, to avoid conflicts removed them from both, only in phase2/master
    -- type txrx20b_type 				        is array (natural range <>) of std_logic_vector(19 downto 0);
    -- type txrx40b_type 				        is array (natural range <>) of std_logic_vector(39 downto 0);
    -- type txrx116b_type 				        is array (natural range <>) of std_logic_vector(115 downto 0);
    -- type txrx2b_type 				        is array (natural range <>) of std_logic_vector(1 downto 0);
    -- type txrx32b_type                         is array (natural range <>) of std_logic_vector(31 downto 0);
    -- type txrx230b_type                         is array (natural range <>) of std_logic_vector(229 downto 0);
    -- type txrx224b_type                         is array (natural range <>) of std_logic_vector(223 downto 0);
    -- type txrx10b_type                         is array (natural range <>) of std_logic_vector(9 downto 0);
    -- type txrx120b_type 					is array (natural range <>) of std_logic_vector(119 downto 0);
    -- type txrx116b_24ch_type 				is array (23 downto 0) of std_logic_vector(115 downto 0);
    -- type txrx116b_12ch_type 				is array (11 downto 0) of std_logic_vector(115 downto 0);

  -- GTH PLL selection
  -- When using GREFCLK, QPLL should be used
  -- use CPLL for VC-709 and BNL-711
  -- use QPLL for HTG-710 (cannot use dedicated clock pin) <- this has WORSE jitter performance
    --constant CPLL                                         : std_logic := '0';
    --constant QPLL                                         : std_logic := '1';
  
  constant FIRMWARE_MODE_GBT   : integer := 0;-- 0: GBT mode        
  constant FIRMWARE_MODE_FULL  : integer := 1;-- 1: FULL mode         
  constant FIRMWARE_MODE_LTDB  : integer := 2;-- 2: LTDB mode (GBT mode with only IC/EC/Aux)
  constant FIRMWARE_MODE_FEI4  : integer := 3;-- 3: FEI4 / RD53A         
  constant FIRMWARE_MODE_PIXEL : integer := 4;-- 4: ITK Pixel (RD53B)        
  constant FIRMWARE_MODE_STRIP : integer := 5;-- 5: ITK Strip         
  constant FIRMWARE_MODE_FELIG : integer := 6;-- 6: FELIG             
  constant FIRMWARE_MODE_FMEMU : integer := 7;-- 7: FULL mode emulator
  constant FIRMWARE_MODE_MROD  : integer := 8; --8: FELIX mrod (2Gb/s S-links) mode.
  
  --phase2/master only
  --type IntArray is array (natural range<>) of integer;
  --constant STREAMS_TOHOST_MODE : IntArray(0 to 8) :=
  --(
  --42, --GBT mode: 40 EPaths + IC + EC + TTCToHost + BusyXoff
  --1,  --FULL mode: + TTCToHost + BusyXoff 
  --5,  --LTDB mode: 
  --42, --FEI4 (tbd)
  --8,  --ITK Pixel --6x 1.28Gb + IC/EC
  --26,  --ITK Strip
  --42,  --FELIG
  --42,  --FMEmu
  --42   --FELIX mrod
  --);

  --To go from nenory depth to number of address bits
  function f_log2 (constant x : positive) return natural; 
  --! Removed this function because it was not synthesis friendly...
  --function log2ceil(arg : positive) return natural;
  function div_ceil(a : natural; b : positive) return natural;
end FELIX_package;

package body FELIX_package is
    function f_log2 (constant x : positive) return natural is
        variable i : natural;
    begin
        i := 0;
        while (2**i < x) and i < 31 loop
            i := i + 1;
        end loop;
        return i;
    end function;

--! Logarithms: log*ceil*
--! ==========================================================================
--! return log2; always rounded up
--! From https://github.com/VLSI-EDA/PoC/blob/master/src/common/utils.vhdl
--! Removed this function because it was not synthesis friendly...
--!  function log2ceil(arg : positive) return natural is
--!      variable tmp : positive;
--!      variable log : natural;
--!  begin
--!      if arg = 1 then return 0; end if;
--!      tmp := 1;
--!      log := 0;
--!      while arg > tmp loop
--!          tmp := tmp * 2;
--!          log := log + 1;
--!      end loop;
--!      return log;
--!  end function;

--! Divisions: div_*
--! ===========================================================================
--! integer division; always round-up
--! calculates: ceil(a / b)
    function div_ceil(a : natural; b : positive) return natural is
    begin
        return (a + (b - 1)) / b;
    end function;

end FELIX_package;
