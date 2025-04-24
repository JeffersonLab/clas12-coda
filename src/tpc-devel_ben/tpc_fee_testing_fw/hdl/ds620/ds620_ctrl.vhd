library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

entity ds620_ctrl is
    port (
        clk_50MHz  : in std_logic;
        rst        : in std_logic;

        en   : in std_logic;
        busy : out std_logic;
        
        valid : out std_logic;
        temp_0 : out std_logic_vector(15 downto 0);
        temp_1 : out std_logic_vector(15 downto 0);
        temp_2 : out std_logic_vector(15 downto 0);
        temp_3 : out std_logic_vector(15 downto 0);

        i2c_sda : inout std_logic;
        i2c_scl : inout std_logic
    );
end ds620_ctrl;

architecture behv of ds620_ctrl is

    constant N_NUMBER_OF_DEVICES : integer := 4;

    type SLV_7b_ARRAY_T is array (0 to N_NUMBER_OF_DEVICES-1) of std_logic_vector(6 downto 0);
    type SLV_16b_ARRAY_T is array (0 to N_NUMBER_OF_DEVICES-1) of std_logic_vector(15 downto 0);
    
    signal dev_temperatures : SLV_16b_ARRAY_T;
    
    constant I2C_DEVICE_ADDRESS : SLV_7b_ARRAY_T := (
        0 => "1001" & "000",
        1 => "1001" & "001",
        2 => "1001" & "010",
        3 => "1001" & "011"
    );
    
    constant WRITE_CMD : std_logic := '0';
    constant READ_CMD  : std_logic := '1';
    
    constant RAW_WRITE  : std_logic_vector(1 downto 0) := "00";
    constant RAW_READ   : std_logic_vector(1 downto 0) := "01";
    constant REG_WRITE  : std_logic_vector(1 downto 0) := "10";
    constant REG_READ   : std_logic_vector(1 downto 0) := "11";
    
    type state_t is (IDLE, SETUP_OUT_OF_RESET, GET_ALL_TEMPERATURE_VALUES, I2C_READ, I2C_WAIT, I2C_WAIT_WRITE, SLEEP);
    signal last_state, state : state_t;

    signal dev_addr    : std_logic_vector(6 downto 0);
    
    signal i2c_data_rd : std_logic_vector(7 downto 0);
    signal read_word   : std_logic_vector(7 downto 0);
    
    signal reg_addr       : std_logic_vector(7 downto 0);
    signal reg_write_data : std_logic_vector(7 downto 0);
    signal reg_read_data  : std_logic_vector(7 downto 0);
    
    signal i2c_en, i2c_wr, i2c_busy, i2c_prev_busy, i2c_err, en_0, cmd_valid : std_logic;
    signal prgm_seq : std_logic_vector(15 downto 0);
    signal sleep_cnt : std_logic_vector(23 downto 0);
    
    signal dev_ptr : integer := 0;
    
    signal i2c_opcode : std_logic_vector(1 downto 0);
    
begin

    temp_0 <= dev_temperatures(0);
    temp_1 <= dev_temperatures(1);
    temp_2 <= dev_temperatures(2);
    temp_3 <= dev_temperatures(3);

    i2c_master_0 : entity work.i2c_master
    port map (
        clk       => clk_50MHz,
        reset     => rst,
        
        opcode    => i2c_opcode,
        en        => i2c_en,
        busy      => i2c_busy,
        
        slave_addr     => dev_addr,
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
        
        procedure i2c_write_command(constant cmd : std_logic_vector(reg_addr'range)) is
        begin
            i2c_en         <= '1';
            reg_write_data <= cmd;
            i2c_opcode     <= RAW_WRITE;
            state          <= I2C_WAIT;
        end procedure;
        
    begin
        if (rst = '1') then
            prgm_seq    <= (others => '0');
            read_word   <= (others => '0');  
            sleep_cnt   <= (others => '0');
            cmd_valid   <= '0';
            i2c_en      <= '0';
            i2c_wr      <= '0';
            en_0        <= '0';
            dev_ptr     <= 0;
            valid       <= '0';
            dev_addr    <= I2C_DEVICE_ADDRESS(0);
            last_state  <= IDLE;
            state       <= SETUP_OUT_OF_RESET;
        elsif (rising_edge(clk_50MHz)) then
            
            case state is
                when IDLE =>
                    busy   <= '0';
                    i2c_en <= '0';
                    prgm_seq <= (others => '0');
                    dev_ptr <= 0;

                    en_0 <= en;
                    if (en_0 = '0' and en = '1') then
                        busy <= '1';
                        valid <= '0';
                        dev_addr <= I2C_DEVICE_ADDRESS(0);
                        state <= GET_ALL_TEMPERATURE_VALUES;
                    end if;
                
                when SETUP_OUT_OF_RESET =>
                    busy <= '1';
                    last_state <= SETUP_OUT_OF_RESET;
                    prgm_seq <= prgm_seq + x"1";
                    
                    if (dev_ptr < I2C_DEVICE_ADDRESS'length) then
                        dev_addr <= I2C_DEVICE_ADDRESS(dev_ptr);
                        case prgm_seq is
                            -- Power On Reset Command (0x54)
                            when x"0000" =>
                               i2c_write_command(x"54");
                                
                            -- Send Start Command (0x51)
                            when x"0001" =>
                               i2c_write_command(x"51");
                               
                            -- Select configuration register MSB (0xAC)
                            -- Conversion resolution = 13 bits, temperature conversion, continuous mode (0x0E)
                            when x"0002" =>
                                i2c_write_word_c(x"AC", x"0E");
                                
                            -- Select configuration register LSB (0xAD)
                            -- Thermostat disabled (0x00)
                            when x"0003" =>
                                i2c_write_word_c(x"AD", x"00");
                            
                            when x"0004" =>
                               i2c_write_command(x"51");
                               prgm_seq <= (others => '0');
                               dev_ptr <= dev_ptr + 1;  
                                                              
                            when others =>
                                state <= IDLE;
                        end case;
                    else
                        state <= IDLE;
                    end if;
                
                when SLEEP =>
                    sleep_cnt <= sleep_cnt + 1;
                    state <= SLEEP;
                    
                    if (sleep_cnt >= x"16e360") then -- ~26 ms
                        sleep_cnt <= (others => '0');
                        state <= last_state;
                    end if;
                
                when GET_ALL_TEMPERATURE_VALUES =>
                    last_state <= GET_ALL_TEMPERATURE_VALUES;
                    prgm_seq <= prgm_seq + x"1";
                    
                    if (dev_ptr < I2C_DEVICE_ADDRESS'length) then
                        dev_addr <= I2C_DEVICE_ADDRESS(dev_ptr);
                        case prgm_seq is
                           when x"0000" =>
                                i2c_write_command(x"51");
                                                                                          
                            -- Read MSB 
                            when x"0001" =>
                                i2c_read_word_c(x"AA");
                                
                            -- Read LSB 
                            when x"0002" =>
                                dev_temperatures(dev_ptr)(15 downto 8) <= read_word;
                                i2c_read_word_c(x"AB");
                                
                            when x"0003" =>
                                dev_temperatures(dev_ptr)(7 downto 0) <= read_word;
                                dev_ptr <= dev_ptr + 1;
                                prgm_seq <= (others => '0');

                            when others =>
                                state <= IDLE;
                        end case;
                    else
                        dev_ptr <= 0;
                        valid <= '1';
                        state <= IDLE;
                    end if;
                
                when I2C_WAIT =>
                    i2c_prev_busy <= i2c_busy;
                    if (i2c_prev_busy = '1' and i2c_busy = '0') then
                        i2c_en <= '0';
                        read_word <= reg_read_data;
                        state <= last_state;
                    end if;
                    
                when OTHERS =>
                    null;
            end case;
        end if;
    end process;
end behv;
