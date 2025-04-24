library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

use work.si5345_register_pkg.all;

entity si5345_pll_init is
    port (
        clk_50MHz  : in std_logic;
        rst        : in std_logic;

        en   : in std_logic;
        busy : out std_logic;
        
        status_strb : in std_logic;
        status      : out std_logic_vector(7 downto 0);
        
        in_sel      : out std_logic_vector(1 downto 0);
        sel         : out std_logic;
        output_en   : out std_logic;
        i2c_addr    : out std_logic_vector(1 downto 0);
        n_dev_reset : out std_logic;

        i2c_sda : inout std_logic;
        i2c_scl : inout std_logic
    );
end si5345_pll_init;

architecture behv of si5345_pll_init is

    constant I2C_DEVICE_ADDRESS : std_logic_vector(6 downto 0) := "1101000"; 
    constant I2C_SWITCH_ADDRESS : std_logic_vector(6 downto 0) := "1110000"; 
    constant WRITE_CMD : std_logic := '0';
    constant READ_CMD  : std_logic := '1';
    
    constant RAW_WRITE  : std_logic_vector(1 downto 0) := "00";
    constant RAW_READ   : std_logic_vector(1 downto 0) := "01";
    constant REG_WRITE  : std_logic_vector(1 downto 0) := "10";
    constant REG_READ   : std_logic_vector(1 downto 0) := "11";
    
    constant PLL_LOL    : std_logic_vector(7 downto 0) := "00010000";
    constant LOS_FDBK   : std_logic_vector(7 downto 0) := "00001000";
    constant LOS_CLKIN  : std_logic_vector(7 downto 0) := "00000100";
    constant SYS_CAL    : std_logic_vector(7 downto 0) := "00000001";
    
    constant SI5345_PAGE_ADDR : std_logic_vector(7 downto 0) := x"01";
    
    constant PLL_LOCKED   : std_logic_vector(7 downto 0) := PLL_LOL or LOS_FDBK or SYS_CAL;
    constant INPUT_LOCKED : std_logic_vector(7 downto 0) := LOS_FDBK;
    
    type state_t is (IDLE, SETUP_I2C_SWITCH, WRITE, PREAMBLE_PROGRAM, PROGRAM, I2C_WAIT, SLEEP, GET_STATUS);
    signal return_state, state, caller_state : state_t;

    signal i2c_data_rd : std_logic_vector(7 downto 0);
    signal i2c_slave_addr : std_logic_vector(6 downto 0);
    signal read_word   : std_logic_vector(7 downto 0);
    signal i2c_data_wr : std_logic_vector(7 downto 0);
    
    signal reg_addr       : std_logic_vector(7 downto 0);
    signal reg_write_data : std_logic_vector(7 downto 0);
    signal reg_read_data  : std_logic_vector(7 downto 0);
    signal reg_mask       : std_logic_vector(7 downto 0);
    
    signal seq : std_logic_vector(7 downto 0);
    signal si_page_addr, si_reg_addr, si_reg_data : std_logic_vector(7 downto 0);
    
    signal i2c_en, i2c_wr, i2c_busy, i2c_prev_busy, i2c_err, en_0, status_strb_0 : std_logic;
    signal sleep_cnt : std_logic_vector(23 downto 0);
    
    signal i2c_opcode : std_logic_vector(1 downto 0);
    
    signal idx : integer := 0;
    
    attribute mark_debug : string;
    attribute mark_debug of state        : signal is "true";
    attribute mark_debug of return_state : signal is "true";
    attribute mark_debug of reg_addr : signal is "true";
    attribute mark_debug of reg_write_data : signal is "true";
    attribute mark_debug of reg_read_data : signal is "true";
    attribute mark_debug of reg_mask : signal is "true";
    attribute mark_debug of sleep_cnt : signal is "true";
    attribute mark_debug of i2c_opcode : signal is "true";

begin


    IN_SEL    <= (others => '0');
    SEL       <= '1';
    output_en <= '0';
    i2c_addr  <= (others => '0');

    i2c_master_0 : entity work.i2c_master
    port map (
        clk       => clk_50MHz,
        reset     => rst,
        
        opcode    => i2c_opcode,
        en        => i2c_en,
        busy      => i2c_busy,
        
        slave_addr     => i2c_slave_addr,
        reg_addr       => reg_addr,
        reg_write_data => reg_write_data,
        reg_read_data  => reg_read_data,
        
        ack_error => i2c_err,
        sda       => i2c_sda,
        scl       => i2c_scl
    );

    process(clk_50MHz, rst)
        procedure i2c_read_word_c(constant reg_rd_addr : std_logic_vector(reg_addr'range)) is
        begin
            i2c_en         <= '1';
            reg_addr       <= reg_rd_addr;
            i2c_opcode     <= REG_READ;
            state          <= I2C_WAIT;
        end procedure;
        
        procedure i2c_write_word_c(constant reg_wr_addr : std_logic_vector(reg_addr'range);
                                 constant reg_wr_data : std_logic_vector(reg_write_data'range)) is
        begin
            i2c_en         <= '1';
            reg_addr       <= reg_wr_addr;
            reg_write_data <= reg_wr_data; 
            i2c_opcode     <= REG_WRITE;
            state          <= I2C_WAIT;
        end procedure;
        
        procedure i2c_write_word_cs(constant reg_wr_addr : std_logic_vector(reg_addr'range);
                                 signal reg_wr_data : std_logic_vector(reg_write_data'range)) is
        begin
            i2c_en         <= '1';
            reg_addr       <= reg_wr_addr;
            reg_write_data <= reg_wr_data; 
            i2c_opcode     <= REG_WRITE;
            state          <= I2C_WAIT;
        end procedure;
        
        procedure i2c_read_word(signal reg_rd_addr : std_logic_vector(reg_addr'range)) is
        begin
            i2c_en         <= '1';
            reg_addr       <= reg_rd_addr;
            i2c_opcode     <= REG_READ;
            state          <= I2C_WAIT;
        end procedure;
        
        procedure i2c_write_word(signal reg_wr_addr : std_logic_vector(reg_addr'range);
                                 signal reg_wr_data : std_logic_vector(reg_write_data'range)) is
        begin
            i2c_en         <= '1';
            reg_addr       <= reg_wr_addr;
            reg_write_data <= reg_wr_data; 
            i2c_opcode     <= REG_WRITE;
            state          <= I2C_WAIT;
        end procedure;
        
        procedure i2c_write_command(constant cmd : std_logic_vector(reg_write_data'range)) is
        begin
            i2c_en         <= '1';
            reg_write_data <= cmd;
            i2c_opcode     <= RAW_WRITE;
            state          <= I2C_WAIT;
        end procedure;
        
        procedure si5345_write(constant si_r_addr : std_logic_vector(15 downto 0);
                               constant si_r_data : std_logic_vector(7 downto 0);
                               constant call_state : state_t) is
        begin
            state <= WRITE;
            caller_state <= call_state;
            si_page_addr <= si_r_addr(15 downto 8);
            si_reg_addr <= si_r_addr(7 downto 0);
            si_reg_data <= si_r_data;
        end procedure;
    begin
        if (rst = '1') then
            read_word   <= (others => '0');  
            i2c_data_wr <= (others => '0');  
            sleep_cnt   <= (others => '0');
            idx         <= 0;
            i2c_en      <= '0';
            i2c_wr      <= '0';
            en_0        <= '0';
            status_strb_0 <= '0';
            n_dev_reset  <= '0';
            seq <= (others => '0');
            return_state  <= IDLE;
            state       <= IDLE;
        elsif (rising_edge(clk_50MHz)) then
            n_dev_reset <= '1';
            case state is
                when IDLE =>
                    busy   <= '0';
                    i2c_en <= '0';
                    idx <= 0;

                    en_0 <= en;
                    if (en_0 = '0' and en = '1') then
                        busy <= '1';
                        state <= SETUP_I2C_SWITCH;
                    end if;
                    
                    if (status_strb_0 = '0' and status_strb = '1') then
                        busy <= '1';
                        return_state <= GET_STATUS;
                    end if;
                
                when GET_STATUS =>
                    status <= read_word;
                    state <= IDLE;

                when SETUP_I2C_SWITCH =>
                    i2c_slave_addr <= I2C_SWITCH_ADDRESS;
                    i2c_write_command("00000100");
                    return_state <= PREAMBLE_PROGRAM;
                
                when PREAMBLE_PROGRAM =>
                    i2c_slave_addr <= I2C_DEVICE_ADDRESS;
                    if (idx < 3) then
                        si5345_write(SI5345_ROM(idx).reg, SI5345_ROM(idx).data, PREAMBLE_PROGRAM);
                        idx <= idx + 1;
                    else
                        state <= SLEEP;
                        return_state <= PROGRAM;
                    end if;

                when PROGRAM =>
                    idx <= idx + 1;
                    if (idx < SI5345_ROM'length) then
                        si5345_write(SI5345_ROM(idx).reg, SI5345_ROM(idx).data, PROGRAM);
                    else
                        state <= IDLE;
                    end if;
                    
                when WRITE =>
                    seq <= seq + x"01";
                    case seq is
                        when x"00" =>
                            return_state <= WRITE;
                            i2c_write_word_cs(SI5345_PAGE_ADDR, si_page_addr);
                        when x"01" =>
                            seq <= (others => '0');
                            return_state <= caller_state;
                            i2c_write_word(si_reg_addr, si_reg_data);
                        when others =>
                            state <= IDLE;
                    end case;
                    
                    
                when SLEEP =>
                    sleep_cnt <= sleep_cnt + 1;
                    state <= SLEEP;
                    
                    if (sleep_cnt >= x"e4e1c0") then -- 300 ms
                        sleep_cnt <= (others => '0');
                        state <= return_state;
                    end if;
                    
                when I2C_WAIT =>
                    i2c_prev_busy <= i2c_busy;
                    if (i2c_prev_busy = '1' and i2c_busy = '0') then
                        i2c_en <= '0';
                        read_word <= reg_read_data;
                        state <= return_state;
                    end if;
                    
                when OTHERS =>
                    null;
            end case;
        end if;
    end process;
end behv;
