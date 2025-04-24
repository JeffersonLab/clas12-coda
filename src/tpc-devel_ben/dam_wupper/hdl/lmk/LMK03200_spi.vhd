-- for spi_LMK03200
-- 
-- 
----------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;

library UNISIM;
use UNISIM.VComponents.all;

entity LMK03200_spi is
  port (     
    LOGICRST            : in std_logic;
    LMK_SPI_data        : in std_logic_vector(27 downto 0);
    LMK_ADDR_wr         : in std_logic_vector(3 downto 0);
    LMK_SPI_wr_in       : in std_logic;
    LMK_WR_finished     : OUT STD_LOGIC;
        
    LMK_SPI_DATA_O      : out std_logic;
    LMK_SPI_CLK         : out std_logic;
    LMK_SPI_LE          : out std_logic;
        
 --       fsm_clk : in std_logic;    
    LMK_SPI_CLK_top     : in std_logic --10MHz
    
    );
end LMK03200_spi;

architecture Behavioral of LMK03200_spi is


  type fsm_state_type is (FSM_IDLE, FSM_WR); --spi control state machine type
  signal LMK_STATE: fsm_state_type := FSM_IDLE;
  SIGNAL SPI_CLK : std_logic;
  signal LMK_DATA_wr32_shift :std_logic_vector(31 downto 0);
  signal LMK_SPI_wr_6, wr_cnt : std_logic_vector(5 downto 0):="000000";
  signal LMK_SPI_wr_l, LMK_SPI_wr, LMK_SPI_wr_r,LMK_SPI_wr_l_r, clk_en :std_logic:='0'; 

begin

  LMK_SPI_CLK <= (not LMK_SPI_CLK_top) WHEN clk_en='1' else '0';
  SPI_CLK <= LMK_SPI_CLK_top;


------stretch  the signal if FSM CLK is fast than SPI CLK
--process(FSM_CLK)
--   begin
--   if FSM_CLK'event and FSM_CLK='1' then
--       LMK_SPI_wr_6 <= LMK_SPI_wr_6(4 downto 0) & LMK_SPI_wr_in;
--       if LMK_SPI_wr_6(0)='1' then
--           LMK_SPI_wr_l <='1';
--       elsif LMK_SPI_wr_6(5)='1' then
--           LMK_SPI_wr_l <='0';
--       else
--           LMK_SPI_wr_l <= LMK_SPI_wr_l;
--       end if;   
--    end if;
--end process;       

  process(LOGICRST,SPI_CLK)
  begin
    if LOGICRST = '1' then
      LMK_STATE         <= FSM_IDLE; 
      LMK_WR_finished   <= '0';
      LMK_SPI_LE        <= '0';
      clk_en            <= '0';
      LMK_SPI_DATA_O    <='0';
      
    elsif SPI_CLK'event and SPI_CLK='1' then
 --   LMK_SPI_wr_l_r <= LMK_SPI_wr_l;
 --   LMK_SPI_wr <= LMK_SPI_wr_l and (not LMK_SPI_wr_l_r);
      LMK_SPI_wr_r <= LMK_SPI_wr; 
      LMK_SPI_wr <= LMK_SPI_wr_in;
    

      case LMK_STATE  is
        when FSM_IDLE =>
          LMK_WR_finished       <= '0';
          LMK_SPI_LE            <= '0';
          LMK_SPI_DATA_O        <='0';
          if LMK_SPI_wr = '1' then
            LMK_STATE   <= FSM_WR;
          else
            LMK_STATE   <= FSM_IDLE;           
          end if;
          clk_en        <= '0';
     
        when FSM_WR =>
          if LMK_SPI_wr_r = '1' then
            LMK_SPI_LE  <= '0';
            clk_en      <= '1';
            LMK_SPI_DATA_O      <= LMK_SPI_data(27);
            LMK_DATA_wr32_shift <=LMK_SPI_data(26 DOWNTO 0) & LMK_ADDR_wr(3 downto 0) & '0';
            wr_cnt              <= "000001";
            LMK_STATE           <= FSM_WR;
            LMK_WR_finished     <= '0';
        elsif wr_cnt(5) ='1' then 
            LMK_SPI_LE  <= '1';
            wr_cnt      <= "000000";
            clk_en      <= '0';
            LMK_STATE   <= FSM_IDLE;
            LMK_WR_finished     <= '1';
            LMK_SPI_DATA_O      <= '0';
        else
            wr_cnt <= wr_cnt + '1';
            LMK_SPI_DATA_O      <= LMK_DATA_wr32_shift(31);
            LMK_DATA_wr32_shift <= LMK_DATA_wr32_shift(30 downto 0) & '0';
            LMK_STATE           <= FSM_WR;
            LMK_WR_finished     <= '0';
            clk_en              <= '1';
        end if;
      
    when others=>
        LMK_STATE       <= FSM_IDLE; 
        LMK_WR_finished <= '0';
        LMK_SPI_LE      <= '1';
        clk_en          <= '0'; 
        LMK_SPI_DATA_O  <= '0';
    end case;
end if;
end process;     
               
end Behavioral;
