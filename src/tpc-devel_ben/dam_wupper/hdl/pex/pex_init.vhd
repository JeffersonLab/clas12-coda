--!-----------------------------------------------------------------------------
--!                                                                           --
--!           BNL - Brookhaven National Lboratory                             --
--!                       Physics Department                                  --
--!                         Omega Group                                       --
--!-----------------------------------------------------------------------------
--|
--! author:      Kai Chen    (kchen@bnl.gov)
--!
--!
--!-----------------------------------------------------------------------------
--
--
-- Create Date: 2016/01/05 04:43:14 PM
-- Design Name: FELIX BNL-711 PEX8732 Automatically initialization
-- Module Name: pex_init - Behavioral
-- Project Name:
-- Target Devices: KCU
-- Tool Versions: Vivado
-- Description:
--              The MODULE FOR PCIe switch initialization
-- Dependencies:
--
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- -- Update
-- -- 2020.11: add extra reset to support server like EPYC
-- --
-- Copyright: All rights reserved
----------------------------------------------------------------------------------



library ieee, UNISIM;
use ieee.numeric_std.all;
use UNISIM.VCOMPONENTS.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_1164.all;

entity pex_init is
  generic(
    CARD_TYPE     : integer := 712
  );
  port (
    I2C_SMB           : out    std_logic;
    I2C_SMBUS_CFG_nEN : out    std_logic;
    MGMT_PORT_EN      : out    std_logic;
    PCIE_PERSTn1      : out    std_logic;
    PCIE_PERSTn2      : out    std_logic;
    PEX_PERSTn        : out    std_logic;
    PEX_SCL           : out    std_logic;
    PEX_SDA           : inout  std_logic;
    PORT_GOOD         : in     std_logic_vector(7 downto 0);
    SHPC_INT          : out    std_logic;
    clk40             : in     std_logic;
    lnk_up0           : in     std_logic;
    lnk_up1           : in     std_logic;
    reset_pcie        : out    std_logic_vector(0 downto 0);
    sys_reset_n       : in     std_logic);
end entity pex_init;



architecture rtl of pex_init is
  component i2c_master_pex
    PORT(
      data_clk          : IN std_logic;
      no_data           : in std_logic;
      wr_mod            : in std_logic; --'1' WR, '0' RD
      reset_n           : in std_logic;
      special           : IN std_logic;
      ena               : IN std_logic;
      wr_data_update    : out std_logic;
      rd_data_refresh   : out std_logic;
      rd_number_in      : in std_logic_vector(3 downto 0);
      wr_number_in      : in std_logic_vector(3 downto 0);
      state_display     : out std_logic_vector(4 DOWNTO 0);
      addr              : IN std_logic_vector(6 DOWNTO 0);
      --  rw            : IN std_logic;
      data_wr           : IN std_logic_vector(31 DOWNTO 0);
      addr_wr           : IN std_logic_vector(31 DOWNTO 0);
      data_rd           : OUT std_logic_vector(31 DOWNTO 0);
      i2c_process_finished : out std_logic;
      ack_error_o       : OUT std_logic;
      sda               : INOUT std_logic;
      scl               : out std_logic;
      sda_o_debug       : out std_logic;
      sda_i_debug       : OUT  std_logic
      );
  END component;

  signal clk400k        : std_logic:='0';

  TYPE machine          IS(IDLE, PHASE1,PHASE2,PHASE3,PHASE_p); --needed states
  SIGNAL PEX_STATE      :  machine:=IDLE;
  signal link_up        : std_logic;
  signal link_up1       : std_logic;
  signal sys_rst_n_r    : std_logic := '1';
  signal sys_rst_n_2r   : std_logic:='1';
  signal trig_en        : std_logic;
  signal PEX_AUTO_CFG   : std_logic := '1';
  signal PEX_AUTO_CFG_r : std_logic := '1';
  signal PEX_AUTO_CFG_2r: std_logic := '1';
  signal PEX_AUTO_CFG_3r: std_logic := '1';

  signal cnt            : std_logic_vector(10 downto 0);
  signal CMD_4B         : std_logic_vector(31 downto 0);
  signal DATA_WR_4B     : std_logic_vector(31 downto 0);

  signal i2c_wr         : std_logic;
  signal I2C_TRIG       : std_logic;
  signal trig_st        : std_logic;
  signal i2c_done       : std_logic;

  signal sys_rst_n_dly  : std_logic:='1';
  signal sys_reset_n_r  : std_logic:='1';
  signal FSM_RST        : std_logic:='1';
  signal extra_rst      : std_logic:='1';
  signal sys_reset_n_all        : std_logic:='1';
  signal reset_width_cnt        : std_logic_vector(4 downto 0);
  CONSTANT divider              :  INTEGER := 52;
  signal check_wait_cnt         : std_logic_vector(17 downto 0):="000000000000000000";
  signal data_clk       : std_logic;
  signal FSM_RST_ORIG   : std_logic;
  signal FSM_RST_ORIG_r : std_logic;
  signal manual_rst     : std_logic_vector(0 downto 0);
  signal lnk_up0_vec    : std_logic_vector(0 downto 0);
  signal lnk_up1_vec    : std_logic_vector(0 downto 0);
  signal PCIE_PERSTn1_vec       : std_logic_vector(0 downto 0);
  signal PCIE_PERSTn2_vec       : std_logic_vector(0 downto 0);
  signal i2c_address    : std_logic_vector(6 downto 0);
  signal Counter_extra_rst:std_logic_vector(23 downto 0):=x"000000";
  signal Counter_extra_rst_hold:std_logic_vector(18 downto 0);
    function To_Std_Logic(L: BOOLEAN) return std_ulogic is
    begin
        if L then
            return('1');
        else
            return('0');
        end if;
    end function To_Std_Logic;

begin

  link_up <= lnk_up0 and lnk_up1;

  process(clk40)
    variable count : integer range 0 to divider*2-1;
  begin
    if clk40'event and clk40 = '1' then
      if count = divider*2-1 then
        count := 0;
      else
        count := count + 1;
      end if;
      case count is
        when 0 to divider*1-1 =>
          data_clk <= '0';
        when others =>
          data_clk <= '1';
      end case;
    end if;
  end process;

  bufg_i2c : BUFG
    port map
    (
      O   => clk400k,
      I   => data_clk
      );


  lnk_up0_vec(0)        <= lnk_up0;
  lnk_up1_vec(0)        <= lnk_up1;
  I2C_SMB               <= '0';
  I2C_SMBUS_CFG_nEN     <= '0';

  process(clk400k)
  begin
    if clk400k'event and clk400k='1' then
      if sys_reset_n='0' or sys_rst_n_dly='0' then
        check_wait_cnt <="000000000000000001";
      else
        check_wait_cnt <= check_wait_cnt+ '1';
      end if;
      if check_wait_cnt="00000000000000000" then
        if PORT_GOOD=x"CE" and lnk_up0='1' and lnk_up1='1' then
          FSM_RST_ORIG <= FSM_RST_ORIG;
        else
          FSM_RST_ORIG <= not FSM_RST_ORIG;
        end if;
      else
        FSM_RST_ORIG <= FSM_RST_ORIG;
      end if;
      FSM_RST_ORIG_r <= FSM_RST_ORIG;
      if FSM_RST_ORIG /= FSM_RST_ORIG_r then
        FSM_RST <='0';
      else
        FSM_RST <= '1';
      end if;
    end if;
  end process;
  
  
  process(clk400k)
  begin
    if clk400k'event and clk400k='1' then
      sys_reset_n_r <= sys_reset_n;
      if sys_reset_n='1' and sys_reset_n_r='0' then
        Counter_extra_rst <= Counter_extra_rst+'1';
      elsif Counter_extra_rst=x"000000" then
        Counter_extra_rst <= Counter_extra_rst;
      else 
        Counter_extra_rst <= Counter_extra_rst+'1';
      end if;
      if Counter_extra_rst(23 downto 5)=Counter_extra_rst_hold  then
        extra_rst <='0';
      else
        extra_rst <='1';
      end if;
  end if;
  end process;
  
  Counter_extra_rst_hold <= "0111" & "111111111111111";

  sys_reset_n_all       <= sys_reset_n and FSM_RST;-- and manual_rst(0);

  PEX_AUTO_CFG          <= sys_reset_n_all and extra_rst;-- sys_rst_n_dly;
  PEX_PERSTn            <= PEX_AUTO_CFG;

  PCIE_PERSTn1          <= '1';
  PCIE_PERSTn2          <= PEX_AUTO_CFG;-- ()'0';
  MGMT_PORT_EN          <= '1';
  SHPC_INT              <='1';


  process(clk400k)
  begin
    if clk400k'event and clk400k='1' then
      sys_rst_n_r <= sys_reset_n_all;
      sys_rst_n_2r <=sys_rst_n_r;
      link_up1<= link_up;
      if sys_rst_n_2r='0' and sys_rst_n_r='1' then
        trig_en <='1';
      elsif link_up1='0' and link_up='1' then
        trig_en <='0';
      else
        trig_en <=trig_en;
      end if;
      if trig_en='1' and link_up1='0' and link_up='1' then
        sys_rst_n_dly <='0';
        reset_width_cnt <="00000";
      elsif reset_width_cnt(4) = '1' then
        sys_rst_n_dly <='1';
        reset_width_cnt <=reset_width_cnt;
      else
        reset_width_cnt <=reset_width_cnt+'1';
      end if;
    end if;
  end process;

  process(clk400k)
  begin
    if clk400k'event and clk400k='1' then
      PEX_AUTO_CFG_r    <= PEX_AUTO_CFG;
      PEX_AUTO_CFG_2r   <= PEX_AUTO_CFG_r;
      PEX_AUTO_CFG_3r   <= PEX_AUTO_CFG_2r;
      case PEX_STATE is
        when IDLE =>
          if PEX_AUTO_CFG_2r='1' and PEX_AUTO_CFG_3r='0' then
            PEX_STATE   <= PHASE_p;
            cnt         <= "00000000000";
            CMD_4B      <= x"D83C0003";
            DATA_WR_4B  <= x"00000000";
            i2c_wr      <= '1';
            I2C_TRIG    <= '0';
            trig_st     <= '1';
          else
            PEX_STATE   <= IDLE;
          end if;
        when PHASE_p =>
          cnt           <= cnt +'1';
          if cnt(10)='1' then
            PEX_STATE   <= PHASE1;
          else
            PEX_STATE   <= PHASE_p;
          end if;
        when PHASE1 =>
          if trig_st='1' then
            I2C_TRIG    <= '1';
            trig_st     <= '0';
            PEX_STATE   <= PHASE1;
          else
            if i2c_done ='1' then
              PEX_STATE <= PHASE2;
              CMD_4B    <= x"EB3C0003";
              DATA_WR_4B<= x"01000000";
              i2c_wr    <= '1';
              I2C_TRIG  <= '0';
              trig_st   <= '1';
            else
              PEX_STATE <= PHASE1;
              I2C_TRIG  <= '0';
              trig_st   <= '0';
            end if;
          end if;
        when PHASE2 =>
          if trig_st='1' then
            I2C_TRIG    <= '1';
            trig_st     <= '0';
            PEX_STATE   <= PHASE2;
          else
            if i2c_done ='1' then
              PEX_STATE <= PHASE3;
              CMD_4B    <= x"EB3C0003";
              DATA_WR_4B<= x"01000000";
              i2c_wr    <= '1';
              I2C_TRIG  <= '0';
              trig_st   <= '1';
            else
              PEX_STATE <= PHASE2;
              I2C_TRIG  <= '0';
              trig_st   <= '0';
            end if;
          end if;
        when PHASE3 =>
          PEX_STATE     <= IDLE;
        when others =>
          PEX_STATE     <= IDLE;
      end case;
    end if;
  end process;

g_i2c_address_2_0: if CARD_TYPE = 712 generate
i2c_address <= "0111010";
end generate;

g_i2c_address_1_5: if CARD_TYPE = 711 generate
i2c_address <= "0111000";
end generate;


  i2c_pex : entity work.i2c_master_pex
    PORT MAP(
      data_clk          => clk400k,
      no_data           => '0',
      wr_mod            => i2c_wr,
      reset_n           => '1',--I2C_RSTn,
      special           => '0',
      ena               => I2C_TRIG,
      wr_data_update    => OPEN,
      rd_data_refresh   => OPEN,
      rd_number_in      => "0011",
      wr_number_in      => "0011",
      state_display     => open,
      addr              => i2c_address,--"0111000", --010 when ADDR0=HIGH, 000 when ADDR0=LOW
      --  rw            : IN     std_logic;
      data_wr           => DATA_WR_4B,
      addr_wr           => CMD_4B,
      data_rd           => open,
      i2c_process_finished      => i2c_done,
      ack_error_o       => open,
      sda               => PEX_SDA,
      scl               => PEX_SCL,
      sda_o_debug       => open,
      sda_i_debug       => open
      );

end architecture rtl ;
