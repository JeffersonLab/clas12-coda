
--!------------------------------------------------------------------------------
--!                                                             
--!           NIKHEF - National Institute for Subatomic Physics 
--!
--!                       Electronics Department                
--!                                                             
--!-----------------------------------------------------------------------------
--! @class clock_and_reset
--! 
--!
--! @author      Andrea Borga    (andrea.borga@nikhef.nl)<br>
--!              Frans Schreuder (frans.schreuder@nikhef.nl)
--!
--!
--! @date        07/01/2015    created
--!
--! @version     1.0
--!
--! @brief 
--! Clock and Reset instantiates an MMCM. It generates clocks of 40,
--! 80, 160 and 320 MHz.
--! Additionally a reset signal is issued when the MMCM is not locked.
--! Reset_out is synchronous to 40MHz
--! 
--!
--!-----------------------------------------------------------------------------
--! @TODO
--!  
--!
--! ------------------------------------------------------------------------------
--! Virtex7 PCIe Gen3 DMA Core
--! 
--! \copyright GNU LGPL License
--! Copyright (c) Nikhef, Amsterdam, All rights reserved. <br>
--! This library is free software; you can redistribute it and/or
--! modify it under the terms of the GNU Lesser General Public
--! License as published by the Free Software Foundation; either
--! version 3.0 of the License, or (at your option) any later version.
--! This library is distributed in the hope that it will be useful,
--! but WITHOUT ANY WARRANTY; without even the implied warranty of
--! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
--! Lesser General Public License for more details.<br>
--! You should have received a copy of the GNU Lesser General Public
--! License along with this library.
--! 
-- 
--! @brief ieee



library ieee, UNISIM, work;
use ieee.numeric_std.all;
use UNISIM.VCOMPONENTS.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_1164.all;
use work.pcie_package.all;

entity clock_and_reset is
  generic(
    APP_CLK_FREQ           : integer := 200;
    USE_BACKUP_CLK         : boolean := true; -- true to use 100/200 mhz board crystal, false to use TTC clock
    AUTOMATIC_CLOCK_SWITCH : boolean := false);
  port (
    MMCM_Locked_out      : out    std_logic;
    MMCM_OscSelect_out   : out    std_logic;
    app_clk_in_n         : in     std_logic;
    app_clk_in_p         : in     std_logic;
    cdrlocked_in         : in     std_logic;
    clk10_xtal           : out    std_logic;
    clk160               : out    std_logic;
    clk240               : out    std_logic;
    clk250               : out    std_logic;
    clk320               : out    std_logic;
    clk40                : out    std_logic;
    clk40_xtal           : out    std_logic;
    clk80                : out    std_logic;
    clk_adn_160          : in     std_logic;
    clk_adn_160_out_n    : out    std_logic;
    clk_adn_160_out_p    : out    std_logic;
    clk_ttc_40           : in     std_logic;
    clk_ttcfx_mon1       : out    std_logic;
    clk_ttcfx_mon2       : out    std_logic;
    clk_ttcfx_ref1_in_n  : in     std_logic;
    clk_ttcfx_ref1_in_p  : in     std_logic;
    clk_ttcfx_ref2_in_n  : in     std_logic;
    clk_ttcfx_ref2_in_p  : in     std_logic;
    clk_ttcfx_ref2_out_n : out    std_logic;
    clk_ttcfx_ref2_out_p : out    std_logic;
    clk_ttcfx_ref_out_n  : out    std_logic;
    clk_ttcfx_ref_out_p  : out    std_logic;
    register_map_control : in     register_map_control_type;
    reset_out            : out    std_logic; --! Active high reset out (synchronous to clk40)
    sys_reset_n          : in     std_logic); --! Active low reset input.
end entity clock_and_reset;


architecture rtl of clock_and_reset is


component clk_wiz_40_0
port
 (-- Clock in ports
  clk_40_in           : in     std_logic;
  clk_in2             : in     std_logic;
  clk_in_sel          : in     std_logic;
  -- Clock out ports
  clk40           : out    std_logic;
  clk80           : out    std_logic;
  clk160          : out    std_logic;
  clk320          : out    std_logic;
  clk240          : out    std_logic;
  clk250          : out    std_logic;
  -- Status and control signals
  reset             : in     std_logic;
  locked            : out    std_logic
 );
end component;

component clk_wiz_250
port
 (-- Clock in ports
  clk_in40_1           : in     std_logic;
  clk_in40_2           : in     std_logic;
  clk_in_sel           : in     std_logic;
  -- Clock out ports
  clk250          : out    std_logic;
  -- Status and control signals
  reset             : in     std_logic;
  locked            : out    std_logic
 );
end component;

component clk_wiz_100_0
port
 (-- Clock in ports
  clk_in1_p      : in    std_logic;
  clk_in1_n      : in    std_logic;
  
  -- Clock out ports
  clk40          : out    std_logic;
  clk10          : out    std_logic;
  -- Status and control signals
  reset             : in     std_logic;
  locked            : out    std_logic
 );
end component;

component clk_wiz_200_0
port
 (-- Clock in ports
  clk_in1_p      : in    std_logic;
  clk_in1_n      : in    std_logic;
  -- Clock out ports
  clk40          : out    std_logic;
  clk10          : out    std_logic;
  -- Status and control signals
  reset          : in     std_logic;
  locked         : out    std_logic
 );
end component;

component clk_wiz_156_0
port
 (-- Clock in ports
  clk_in1_p      : in    std_logic;
  clk_in1_n      : in    std_logic;
  -- Clock out ports
  clk40          : out    std_logic;
  clk10          : out    std_logic;
  -- Status and control signals
  reset             : in     std_logic;
  locked            : out    std_logic
 );
end component;

component BUFGMUX 
  port (
    I0   : in std_logic;
    I1   : in std_logic;
    S    : in std_logic;
    O    : out std_logic
  ); 
end component;

   signal reset_in        : std_logic; 
   
   signal clk40_s         : std_logic;
   signal clk160_s        : std_logic;
   signal locked_s        : std_logic;
   signal clk_xtal_40_s   : std_logic;
   signal clk160_out_s    : std_logic;
   signal clk_ttcfx_ref_s : std_logic;

   signal ttc_locked       : std_logic;
   signal ttcCounter       : std_logic_vector(7 downto 0);
   signal ttcCounterXtal   : std_logic_vector(7 downto 0);
   signal ttcCounterXtalP1 : std_logic_vector(7 downto 0);
   signal xtalCounter      : std_logic_vector(7 downto 0);
   signal OscSelect        : std_logic;
   constant TTC_SRC        : std_logic  := '1';
   constant XTAL_SRC       : std_logic  := '0';
   
   signal unused_app_clk : std_logic;
   attribute dont_touch : string;
   attribute dont_touch of unused_app_clk : signal is "true";
   

begin
  

  -- Main MMCM
  clk0 : clk_wiz_40_0
  port map ( 
   -- Clock in ports
   clk_40_in => clk_ttc_40,
   clk_in2   => clk_xtal_40_s,
   clk_in_sel=> OscSelect,
  -- Clock out ports  
   clk40  => clk40_s,
   clk80  => clk80,
   clk160 => clk160_s,
   clk320 => clk320,
   clk240  => clk240,
   clk250 => open,
  -- Status and control signals                
   reset => reset_in,
   locked => locked_s
   );
   
  clk250_0 : clk_wiz_250
   port map ( 

   -- Clock in ports
   clk_in40_1 => clk_ttc_40,
   clk_in40_2 => clk_xtal_40_s,
   clk_in_sel => OscSelect,
  -- Clock out ports  
   clk250 => clk250,
  -- Status and control signals                
   reset => reset_in,
   locked => open            
   );
 
 -- FLX-709 Si570 @ 156.25MHz
  g_156M: if(APP_CLK_FREQ = 156) generate
    clk0 : clk_wiz_156_0
      port map ( 
        -- Clock in ports
        clk_in1_p => app_clk_in_p,
        clk_in1_n => app_clk_in_n,
        -- Clock out ports  
        clk40  => clk_xtal_40_s,
        clk10  => clk10_xtal,
        -- Status and control signals                
        reset => '0',
        locked => open
        );
  end generate;
 
  -- FLX-711 local clock
  g_200M: if(APP_CLK_FREQ = 200) generate
    clk0 : clk_wiz_200_0
      port map ( 
        -- Clock in ports
        clk_in1_p => app_clk_in_p,
        clk_in1_n => app_clk_in_n,
        -- Clock out ports  
        clk40  => clk_xtal_40_s,
        clk10  => clk10_xtal,
        -- Status and control signals                
        reset => '0',
        locked => open
        );
  end generate;

  -- FLX-710 local clock
  g_100M: if(APP_CLK_FREQ = 100) generate
    clk0 : clk_wiz_100_0
      port map ( 
        -- Clock in ports
        clk_in1_p => app_clk_in_p,
        clk_in1_n => app_clk_in_n,
        -- Clock out ports  
        clk40  => clk_xtal_40_s,
        clk10  => clk10_xtal,
        -- Status and control signals                
        reset => '0',
        locked => open
        );
  end generate;

  --instantiate the buffer for the clock, in order to pass bitgen
  g1b: if(USE_BACKUP_CLK = false and AUTOMATIC_CLOCK_SWITCH = false) generate
    buf0: IBUFGDS
      port map (
        I => app_clk_in_p,
        Ib => app_clk_in_n,
        O => unused_app_clk
        );
  end generate;

  MMCM_Locked_out <= locked_s;
  MMCM_OscSelect_out <= OscSelect;
 
  reset_in <= not sys_reset_n;

  clk40_xtal <= clk_xtal_40_s;
  clk40  <= clk40_s;
  clk160 <= clk160_s;
  clk_ttcfx_ref_s <= clk40_s;

  
  buf1: IBUFGDS
  port map (
      I => clk_ttcfx_ref1_in_p,
      Ib => clk_ttcfx_ref1_in_n,
      O => clk_ttcfx_mon1
      );

  buf2: IBUFGDS
  port map (
      I => clk_ttcfx_ref2_in_p,
      Ib => clk_ttcfx_ref2_in_n,
      O => clk_ttcfx_mon2
      ); 
  
  
 --Save one BUFG by using the clock switch in the MMCM to create the 40 MHz reference clock for the Si5345
 --mux0: BUFGMUX
 --port map
 --(
 --   I0 => clk_xtal_40_s,
 --   I1 => clk_ttc_40,
 --   O  => clk_ttcfx_ref_s,
 --   S  => OscSelect
 --);

  -- input to the Si5345 from Main MMCM 40 MHz
  OBUFDS_ttcfx_ref_out : OBUFDS
  generic map
    ( IOSTANDARD => "DEFAULT")
  port map (
    O  => clk_ttcfx_ref_out_p,
    OB => clk_ttcfx_ref_out_n,
    I  => clk_ttcfx_ref_s --clk_ttc_40    -- clock from the TTC decoder module (phase aligned)
    );

  -- extra input to the Si5345
  OBUFDS_ttcfx_ref2_out : OBUFDS
  generic map
    ( IOSTANDARD => "DEFAULT")
  port map (
    O  => clk_ttcfx_ref2_out_p,
    OB => clk_ttcfx_ref2_out_n,
    I  => clk_xtal_40_s                -- 40 MHz crystal generated clock
    );

  
  g2a: if (AUTOMATIC_CLOCK_SWITCH = false) generate
    g2c: if(USE_BACKUP_CLK = false) generate
      OscSelect <= TTC_SRC;
    end generate;
    g2d: if(USE_BACKUP_CLK = true) generate
      OscSelect <= XTAL_SRC;
    end generate;
  end generate;

  -- autoclock switch turned into manual select
--  g2b: if (AUTOMATIC_CLOCK_SWITCH = true) generate
--    ttc_locked <= cdrlocked_in;
--    OscSelect <= XTAL_SRC when register_map_control.MMCM_MAIN.LCLK_SEL = "1" else  TTC_SRC; --ttc_locked;
--  end generate;
 
 
  reset_out <= not locked_s;
  
  -- 160 Mhz to the Si5324 (FLX-709 only)
  g_160_out_bufgmux0: if(AUTOMATIC_CLOCK_SWITCH = true) generate
    bufgmux_160_0: BUFGMUX
    port map (
      I0     => clk160_s, -- insert clock input used when select (S) is Low 
      I1     => clk_adn_160, -- insert clock input used when select (S) is High
      S      => OscSelect, -- insert Mux-Select input
      O      => clk160_out_s  -- insert clock output
      );
  end generate;

  g_160_out_xtal: if(USE_BACKUP_CLK = true and AUTOMATIC_CLOCK_SWITCH = false) generate
    clk160_out_s <= clk160_s;
  end generate;

  g_160_out_adn: if(USE_BACKUP_CLK = false and AUTOMATIC_CLOCK_SWITCH = false) generate
    clk160_out_s <= clk_adn_160;
  end generate;

  --OBUFDS to route the 160 MHz clock into the Si5324 jitter cleaner (VC709) to create the GBT Reference clock
  OBUF160: OBUFDS 
  generic map (
    IOSTANDARD => "LVDS",
    SLEW       => "FAST")
  port map(
    I => clk160_out_s,
    O => clk_adn_160_out_p,
    OB => clk_adn_160_out_n);
 
end architecture rtl ; -- of clock_and_reset

