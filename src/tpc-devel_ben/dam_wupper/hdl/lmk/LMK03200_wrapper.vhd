------------------------------------------
---- Designed by Kai -------------
---- For LTDB test ---------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;

library UNISIM;
use UNISIM.VComponents.all;
LIBRARY work;


entity LMK03200_wrapper is
    generic (
        R0  : std_logic_vector(27 downto 0) := x"1001000";
        R1  : std_logic_vector(27 downto 0) := x"0001000";
        R2  : std_logic_vector(27 downto 0) := x"0001000";
        R3  : std_logic_vector(27 downto 0) := x"0001000";
        R4  : std_logic_vector(27 downto 0) := x"0001000";
        R5  : std_logic_vector(27 downto 0) := x"0001000";
        R6  : std_logic_vector(27 downto 0) := x"0001000";
        R7  : std_logic_vector(27 downto 0) := x"0001000";
        R13 : std_logic_vector(27 downto 0) := x"028A000";
        R14 : std_logic_vector(27 downto 0) := x"0830080";
        R15 : std_logic_vector(27 downto 0) := x"1400300"); --2000200 for 160M
    port (
        rst_lmk     : in std_logic;
        hw_rst      : in std_logic;
        LMK_locked  : out std_logic;
        clk40m_in   : in std_logic;
        clk10m_in   : in std_logic;
        CLK40_FPGA2LMK_P : out std_logic;
        CLK40_FPGA2LMK_N : out std_logic;
        LMK_DATA    : out std_logic;
        LMK_CLK     : out std_logic;
        LMK_LE      : out std_logic;
        LMK_GOE     : out std_logic;
        LMK_LD      : in std_logic;
        LMK_SYNCn   : out std_logic);
end LMK03200_wrapper;

architecture rtl of LMK03200_wrapper is

    type fsmtype is (IDLE, CFG0, CFG1, CFG2, CFG3, CFG4, CFG5, CFG6,
    CFG7, CFG8, CFG9, CFG10, CFG11, CFG12, CFG13, CFG14, CFG15, CFG16, CFGREG7, CFGREG4, CFGREG3);
    signal lmk_status : fsmtype:= IDLE;

    signal LMK03200_SPI_update: std_logic;

    signal lmk_cfg, LMK_WR_finished: std_logic;

    signal LMK_ADDR_wr: std_logic_vector(3 downto 0);
    signal LMK03200_SPI_data: std_logic_vector(27 downto 0);

    signal reset: std_logic;

begin

    LMK_locked <= LMK_LD;
    reset <= rst_lmk or hw_rst;

    LMK_inst: entity work.LMK03200_spi
    port map (
        LOGICRST        => reset,
        LMK_SPI_data    => LMK03200_SPI_data,
        LMK_ADDR_wr     => LMK_ADDR_wr,
        LMK_SPI_wr_in   => LMK03200_SPI_update,
        LMK_WR_finished => LMK_WR_finished,

        LMK_SPI_DATA_O  => LMK_DATA,
        LMK_SPI_CLK     => LMK_CLK,
        LMK_SPI_LE      => LMK_LE,

        LMK_SPI_CLK_top => clk10m_in);


    process(rst_lmk, hw_rst, clk10m_in)
    begin
      if (rst_lmk = '1' or hw_rst = '1') then
        lmk_status <= CFG0;
      elsif (clk10m_in'event and clk10m_in='1') then
        case lmk_status is
          when IDLE =>
            LMK_SYNCn <= '1';
            LMK_GOE <= '1';
            lmk_status <= IDLE;
          when CFG0 =>
            LMK_SYNCn <= '1';
            LMK_GOE <= '0';
            lmk_status <= CFG1;
          when CFG1 =>
            LMK_SYNCn <= '1';
            LMK_GOE <= '0';
            lmk_cfg <='1';
            lmk_status <= CFG2;
          when CFG2 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= x"8000000";
              LMK_ADDR_wr <= x"0";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG3;
              else
                lmk_status <= CFG2;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG3 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R0; --x"1001000";
              LMK_ADDR_wr <= x"0";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG4;
              else
                lmk_status <= CFG3;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG4 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R1; --x"0001000";
              LMK_ADDR_wr <= x"1";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFGREG3;
              else
                lmk_status <= CFG4;
                lmk_cfg <='0';
              end if;
            end if;
          when CFGREG3 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R3; --x"0001000";
              LMK_ADDR_wr <= x"3";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFGREG4;
              else
                lmk_status <= CFGREG3;
                lmk_cfg <='0';
              end if;
            end if;
          when CFGREG4 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R4; --x"0001000";
              LMK_ADDR_wr <= x"4";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFGREG7;
              else
                lmk_status <= CFGREG4;
                lmk_cfg <='0';
              end if;
            end if;
          when CFGREG7 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R7; --x"0001000";
              LMK_ADDR_wr <= x"7";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG5;
              else
                lmk_status <= CFGREG7;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG5 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R2; --x"0001000";
              LMK_ADDR_wr <= x"2";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG6;
              else
                lmk_status <= CFG5;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG6 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R5; --x"0001000";
              LMK_ADDR_wr <= x"5";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG7;
              else
                lmk_status <= CFG6;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG7 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R6; --x"0001000";
              LMK_ADDR_wr <= x"6";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG8;
              else
                lmk_status <= CFG7;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG8 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= x"1000090";
              LMK_ADDR_wr <= x"8";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG9;
              else
                lmk_status <= CFG8;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG9 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= x"0082000";
              LMK_ADDR_wr <= x"B";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG10;
              else
                lmk_status <= CFG9;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG10 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R13; --x"028A000";
              LMK_ADDR_wr <= x"D";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG11;
              else
                lmk_status <= CFG10;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG11 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R14; --x"0830080";
              LMK_ADDR_wr <= x"E";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG12;
              else
                lmk_status <= CFG11;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG12 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R15; --x"1400300"; --2000200 for 160M
              LMK_ADDR_wr <= x"F";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG13;
              else
                lmk_status <= CFG12;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG13 =>
            LMK_SYNCn <= '1';
            LMK_GOE <= '1';
            lmk_status <= CFG14;
          when CFG14 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= x"1801000";
              LMK_ADDR_wr <= x"0";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG15;
              else
                lmk_status <= CFG14;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG15 =>
            if lmk_cfg='1' then
              lmk_cfg <='0';
              LMK03200_SPI_data <= R15; --x"1400300";
              LMK_ADDR_wr <= x"F";
              LMK03200_SPI_update <='1';
            else
              LMK03200_SPI_update <='0';
              if LMK_WR_finished = '1' then
                lmk_cfg <='1';
                lmk_status <= CFG16;
              else
                lmk_status <= CFG15;
                lmk_cfg <='0';
              end if;
            end if;
          when CFG16 =>
            LMK_SYNCn <= '0';
            LMK_GOE <= '1';
            lmk_status <= IDLE;
          when others =>
            lmk_status <= IDLE;
        end case;
      end if;
    end process;


    lmk40m : OBUFDS
      generic map (
        IOSTANDARD => "DEFAULT",
        SLEW => "FAST"
        )
      port map (
        O   => CLK40_FPGA2LMK_P,
        OB  => CLK40_FPGA2LMK_N,
        I   => clk40m_in
        );


end architecture rtl;

