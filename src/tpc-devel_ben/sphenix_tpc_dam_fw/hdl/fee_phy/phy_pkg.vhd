library ieee;
use ieee.numeric_std.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_1164.all;

package phy_pkg is
    constant NUMBER_OF_FEEs : integer := 8;

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

    constant RX_SYNC_C         : std_logic_vector(15 downto 0) := D_16_2_C & K_28_5_C;

    constant START_OF_BLOCK_C  : std_logic_vector(15 downto 0) := D_10_2_C & K_28_5_C;
    constant END_OF_BLOCK_C    : std_logic_vector(15 downto 0) := D_21_5_C & K_28_5_C;
    constant RESUME_BLOCK_C    : std_logic_vector(15 downto 0) := D_12_6_C & K_28_5_C;
    constant PAUSED_BLOCK_C    : std_logic_vector(15 downto 0) := D_05_6_C & K_28_5_C;
    
    constant START_OF_REG_C    : std_logic_vector(15 downto 0) := D_10_7_C & K_28_5_C;
    constant END_OF_REG_C      : std_logic_vector(15 downto 0) := D_11_1_C & K_28_5_C;
    constant WAIT_FOR_REG_C    : std_logic_vector(15 downto 0) := D_11_4_C & K_28_5_C;
 
    constant TX_START_OF_REG_C    : std_logic_vector(7 downto 0) := D_10_7_C; 
    constant TX_END_OF_REG_C      : std_logic_vector(7 downto 0) := D_11_1_C;
    constant TX_WAIT_FOR_REG_C    : std_logic_vector(7 downto 0) := D_11_4_C;
    
    constant TX_TRIGGER_C         : std_logic_vector(7 downto 0) := D_13_4_C;
 
    constant TX_GTM_CMD_C         : std_logic_vector(7 downto 0) := K_28_2_C;
    constant TX_BDC_CMD_C         : std_logic_vector(7 downto 0) := K_28_3_C;
  
    type phy_stats_rec is record
        rx_sob                 : std_logic_vector(31 downto 0);
        rx_eob                 : std_logic_vector(31 downto 0);
        rx_stream_bytes        : std_logic_vector(31 downto 0);
        crc_stream_errors      : std_logic_vector(15 downto 0);
        crc_reg_errors         : std_logic_vector(15 downto 0);
        stream_fifo_full_count : std_logic_vector(15 downto 0);
    end record;
    
    type reg_in_rec is record
        reg_clk        :  std_logic;
        reg_rst        :  std_logic;
        reg_addr       :  std_logic_vector(15 downto 0);
        reg_read       :  std_logic;
        reg_write      :  std_logic;
        reg_data_write :  std_logic_vector(31 downto 0);
    end record;
    
    type reg_out_rec is record
        reg_ack        : std_logic;
        reg_fail       : std_logic;
        rx_ready       : std_logic;
        reg_addr_read  : std_logic_vector(15 downto 0);
        reg_data_read  : std_logic_vector(31 downto 0);
    end record;
    
    type stream_out_rec is record
        data    : std_logic_vector(15 downto 0);
        count   : std_logic_vector(9 downto 0);
        avail   : std_logic;
    end record;
    
    type stream_ctrl_rec is record
        clk     : std_logic;
        rst     : std_logic;
        read_en : std_logic;
    end record;
    
    type broadcast_recv_rec is record
        heartbeat_trigger : std_logic; -- Bit 0
        bx_count_sync     : std_logic; -- Bit 1
        hard_reset        : std_logic; -- Bit 2
        valid             : std_logic;
    end record;

    
    type stream_t      is array(0 to NUMBER_OF_FEEs-1) of stream_out_rec;
    type stream_ctrl_t is array(0 to NUMBER_OF_FEEs-1) of stream_ctrl_rec;
    type reg_request_t is array(0 to NUMBER_OF_FEEs-1) of reg_in_rec;
    type reg_reply_t   is array(0 to NUMBER_OF_FEEs-1) of reg_out_rec;
    type phy_stats_t   is array(0 to NUMBER_OF_FEEs-1) of phy_stats_rec;
    
    type data_16b_arr is array (natural range <>) of std_logic_vector(15 downto 0);

end package phy_pkg;
