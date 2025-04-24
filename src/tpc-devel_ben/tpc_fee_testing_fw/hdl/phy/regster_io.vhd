library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

Library UNISIM;
use UNISIM.vcomponents.all;

entity register_io is
	port (
        clk    : in std_logic;
        reset  : in std_logic;
        
        reg_addr    : in std_logic_vector(15 downto 0);
        reg_data    : in std_logic_vector(15 downto 0);
        reg_write   : in std_logic;
        reg_read    : in std_logic;
        
        reg_rd_dval : out std_logic;
        reg_ack     : out std_logic;
        reg_rd_data : out std_logic_vector(15 downto 0);

        temp     : in std_logic_vector(11 downto 0);
        vccint   : in std_logic_vector(11 downto 0);
        vccaux   : in std_logic_vector(11 downto 0);
        vccbram  : in std_logic_vector(11 downto 0);
        
        sampa_i2c_addr     : out std_logic_vector(3 downto 0);
        sampa_i2c_reg_addr : out std_logic_vector(5 downto 0);
        sampa_i2c_reg_write_data : out std_logic_vector(7 downto 0);
        sampa_i2c_reg_read_data  : in std_logic_vector(7 downto 0);
        sampa_i2c_opcode : out std_logic_vector(1 downto 0);
        sampa_i2c_en     : out std_logic;
        sampa_i2c_busy   : in std_logic;
        sampa_i2c_error  : in std_logic;
        
        sampa_trigger : out std_logic;

        telem_trigs     : out std_logic_vector(15 downto 0);        
        v_P4V           : in std_logic_vector(15 downto 0);       
        v_P2V_fpga      : in std_logic_vector(15 downto 0);
        v_P2V_sampa_dig : in std_logic_vector(15 downto 0);
        v_P2V_sampa_ana : in std_logic_vector(15 downto 0);
        i_P4V           : in std_logic_vector(15 downto 0);
        i_P2V_fpga      : in std_logic_vector(15 downto 0);
        i_P2V_sampa_dig : in std_logic_vector(15 downto 0);
        i_P2V_sampa_ana : in std_logic_vector(15 downto 0);
        pcb_temp_0      : in std_logic_vector(15 downto 0);
        pcb_temp_1      : in std_logic_vector(15 downto 0);
        pcb_temp_2      : in std_logic_vector(15 downto 0);
        pcb_temp_3      : in std_logic_vector(15 downto 0);
        
        test_pattern_trig : out std_logic;
        test_pattern_length : out std_logic_vector(15 downto 0);
        
        sampa_shape_gain : out std_logic_vector(2 downto 0);
        
        sampa_elink_enable : out std_logic_vector(31 downto 0)
    );
end register_io;

architecture Behavioral of register_io is
    type  state_t is (IDLE, REGISTER_READ, REGISTER_WRITE);
    signal state : state_t;
    
    signal reg_data_i : std_logic_vector(reg_data'range);
    signal reg_addr_i : std_logic_vector(reg_addr'range);
    
    signal scratch_pad : std_logic_vector(reg_data'range);
    
    signal device_id : std_logic_vector(31 downto 0);
    
    signal sampa_i2c_en_i : std_logic;

begin

   EFUSE_USR_inst : EFUSE_USR
   generic map (
      SIM_EFUSE_VALUE => X"00000000"  -- Value of the 32-bit non-volatile value used in simulation
   )
   port map (
      EFUSEUSR => device_id  -- 32-bit output: User eFUSE register value output
   );

   sampa_i2c_en <= sampa_i2c_en_i;
   
	process(clk, reset)
	begin
        if (reset = '1') then
            state <= IDLE;
            reg_rd_data <= (others => '0');
            reg_addr_i <= (others => '0');
            reg_data_i <= (others => '0'); 
            sampa_i2c_en_i <= '0';
            sampa_elink_enable <= (others => '0');
            sampa_shape_gain <= (others => '0');
        elsif (rising_edge(clk)) then
            case state is
                when IDLE =>
                    reg_ack <= '0';
                    reg_rd_dval <= '0';
                    sampa_i2c_en_i <= '0';
                    test_pattern_trig   <= '0';
                    sampa_trigger <=  '0';
                    
                    if (reg_write = '1') then
                        reg_addr_i <= reg_addr;
                        reg_data_i <= reg_data;
                        state <= REGISTER_WRITE;
                    elsif (reg_read = '1') then
                        reg_addr_i <= reg_addr;
                        state <= REGISTER_READ;
                    end if;
                    
                when REGISTER_READ =>
                    reg_ack <= '1';
                    reg_rd_dval <= '1';
                    state <= IDLE;
                    
                    case reg_addr_i is
                        when x"0000" => reg_rd_data <= device_id(31 downto 16);
                        when x"0001" => reg_rd_data <= device_id(15 downto 0);
                        when x"0100" => reg_rd_data <= scratch_pad;
                        when x"0200" => reg_rd_data <= sampa_elink_enable(31 downto 16);
                        when x"0201" => reg_rd_data <= sampa_elink_enable(15 downto 0);
                        
                        when x"0300" => reg_rd_data <= (reg_rd_data'high downto sampa_shape_gain'length => '0') & sampa_shape_gain;
                        
                        when x"0600" => reg_rd_data <= (reg_rd_data'high downto 10 => '0') & sampa_i2c_busy & sampa_i2c_error & sampa_i2c_reg_read_data;
                        
                        when x"a000" => reg_rd_data <= (reg_rd_data'high downto temp'length => '0')    & temp;
                        when x"a001" => reg_rd_data <= (reg_rd_data'high downto vccint'length => '0')  & vccint;
                        when x"a002" => reg_rd_data <= (reg_rd_data'high downto vccaux'length => '0')  & vccaux;
                        when x"a003" => reg_rd_data <= (reg_rd_data'high downto vccbram'length => '0') & vccbram;
                        when x"a006" => reg_rd_data <= v_P4V;
                        when x"a007" => reg_rd_data <= v_P2V_fpga;
                        when x"a008" => reg_rd_data <= v_P2V_sampa_dig;
                        when x"a009" => reg_rd_data <= v_P2V_sampa_ana;
                        when x"a00a" => reg_rd_data <= i_P4V;
                        when x"a00b" => reg_rd_data <= i_P2V_fpga;
                        when x"a00c" => reg_rd_data <= i_P2V_sampa_dig;
                        when x"a00d" => reg_rd_data <= i_P2V_sampa_ana;
                        when x"a00e" => reg_rd_data <= pcb_temp_0;
                        when x"a00f" => reg_rd_data <= pcb_temp_1;
                        when x"a010" => reg_rd_data <= pcb_temp_2;
                        when x"a011" => reg_rd_data <= pcb_temp_3;
                        when OTHERS => reg_rd_data <= (others => '0');
                    end case;
                        
                when REGISTER_WRITE =>
                    reg_ack <= '1';
                    state <= IDLE;
                    
                    case reg_addr_i is
                        when x"0010" => test_pattern_trig   <= '1';
                                        test_pattern_length <= reg_data_i(15 downto 0);
                        when x"0100" => scratch_pad <= reg_data_i;
                        
                        when x"0200" => sampa_elink_enable(31 downto 16) <= reg_data_i;
                        when x"0201" => sampa_elink_enable(15 downto 0) <= reg_data_i;
                        
                        when x"0300" => sampa_shape_gain <= reg_data_i(2 downto 0);
                        
                        when x"0600" => sampa_i2c_addr   <= reg_data_i(3 downto 0);
                                        sampa_i2c_en_i   <= reg_data_i(4);
                                        sampa_i2c_opcode <= reg_data_i(6 downto 5);
                        when x"0601" => sampa_i2c_reg_addr       <= reg_data_i(sampa_i2c_reg_addr'range);
                        when x"0602" => sampa_i2c_reg_write_data <= reg_data_i(sampa_i2c_reg_write_data'range);
                        
                        when x"a000" => telem_trigs <= reg_data_i(telem_trigs'range);
                        when x"a001" => sampa_trigger <=  '1';
                        when OTHERS => 
                            reg_ack <= '0';
                    end case;
            end case;
        end if;
    end process;

end Behavioral;

