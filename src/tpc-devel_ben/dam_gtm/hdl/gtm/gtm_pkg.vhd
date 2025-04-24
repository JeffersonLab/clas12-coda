library ieee;
use ieee.numeric_std.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_1164.all;

package gtm_pkg is
    -- K Code Table --
    constant K_28_0_C : std_logic_vector(7 downto 0) := "00011100";  -- K28.0, 0x1C
    constant K_28_1_C : std_logic_vector(7 downto 0) := "00111100";  -- K28.1, 0x3C (Comma)
    constant K_28_2_C : std_logic_vector(7 downto 0) := "01011100";  -- K28.2, 0x5C
    constant K_28_3_C : std_logic_vector(7 downto 0) := "01111100";  -- K28.3, 0x7C
    constant K_28_4_C : std_logic_vector(7 downto 0) := "10011100";  -- K28.4, 0x9C
    constant K_28_5_C : std_logic_vector(7 downto 0) := "10111100";  -- K28.5, 0xBC (Comma)
    constant K_28_6_C : std_logic_vector(7 downto 0) := "11011100";  -- K28.6, 0xDC
    constant K_28_7_C : std_logic_vector(7 downto 0) := "11111100";  -- K28.7, 0xFC (Comma)
    
    -- D Codes Table -- 
    constant D_10_2_C : std_logic_vector(7 downto 0) := "01001010";  -- D10.2, 0x4A
    constant D_21_5_C : std_logic_vector(7 downto 0) := "10110101";  -- D21.5, 0xB5
    constant D_05_6_C : std_logic_vector(7 downto 0) := "11000101";  -- D05.6, 0xC5
    constant D_16_2_C : std_logic_vector(7 downto 0) := "01010000";  -- D16.2, 0x50
    constant D_12_6_C : std_logic_vector(7 downto 0) := "11001100";  -- D12.6, 0xCC
    constant D_10_7_C : std_logic_vector(7 downto 0) := "11101010";  -- D10.7, 0xEA
    constant D_11_1_C : std_logic_vector(7 downto 0) := "00101011";  -- D11.1, 0x2B
    constant D_11_4_C : std_logic_vector(7 downto 0) := "10001011";  -- D11.4, 0xAB
    constant D_13_4_C : std_logic_vector(7 downto 0) := "10001101";  -- D13.4, 0x8D
    
    constant GTM_IDLE_CODE  : std_logic_vector(15 downto 0) := D_16_2_C & K_28_5_C;
 
    type gtm_stats_rec is record
        beam_clk_cnt : std_logic_vector(31 downto 0);
    end record;
    
    type gtm_recvr_out_rec is record
        fem_user_bits : std_logic_vector(2 downto 0);
        modebits_enb  : std_logic_vector(0 downto 0);
        fem_endat     : std_logic_vector(1 downto 0);
        lvl1_accept   : std_logic_vector(0 downto 0);
        modebit_n_bco : std_logic_vector(0 downto 0);
        modebits      : std_logic_vector(7 downto 0);
        modebits_val  : std_logic;
        bco_count     : std_logic_vector(39 downto 0);
        bco_count_val : std_logic;
    end record;
    

end package gtm_pkg;
