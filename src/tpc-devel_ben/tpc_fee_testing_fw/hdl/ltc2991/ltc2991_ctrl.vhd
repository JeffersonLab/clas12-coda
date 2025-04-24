library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

entity ltc2991_ctrl is
    port (
        clk_50MHz  : in std_logic;
        rst        : in std_logic;

        en   : in std_logic;
        busy : out std_logic;
        
        valid : out std_logic;
        voltage_a : out std_logic_vector(15 downto 0);
        voltage_b : out std_logic_vector(15 downto 0);
        voltage_c : out std_logic_vector(15 downto 0);
        voltage_d : out std_logic_vector(15 downto 0);
        current_a : out std_logic_vector(15 downto 0);
        current_b : out std_logic_vector(15 downto 0);
        current_c : out std_logic_vector(15 downto 0);
        current_d : out std_logic_vector(15 downto 0);
        
        device_temp : out std_logic_vector(15 downto 0);
        device_vcc  : out std_logic_vector(15 downto 0);

        i2c_sda : inout std_logic;
        i2c_scl : inout std_logic
    );
end ltc2991_ctrl;

architecture behv of ltc2991_ctrl is

    type DATA_REC_T is record
        reg   : std_logic_vector(7 downto 0);
        value : std_logic_vector(15 downto 0);
    end record;
    type DATA_ARRAY_T is array (0 to 9) of DATA_REC_T;
    
    signal device_data : DATA_ARRAY_T := (
        -- Voltage
        0 => (reg => x"0a", value => x"0000"), 
        1 => (reg => x"0e", value => x"0000"),
        2 => (reg => x"12", value => x"0000"),
        3 => (reg => x"16", value => x"0000"),
        -- Current
        4 => (reg => x"0c", value => x"0000"),
        5 => (reg => x"10", value => x"0000"),
        6 => (reg => x"14", value => x"0000"),
        7 => (reg => x"18", value => x"0000"),
        -- Internal Temperature
        8 => (reg => x"1a", value => x"0000"),
        -- Internal Vcc
        9 => (reg => x"1c", value => x"0000")
    );
    
    constant I2C_DEVICE_ADDRESS : std_logic_vector(6 downto 0) := "1001" & "000";
    
    constant WRITE_CMD : std_logic := '0';
    constant READ_CMD  : std_logic := '1';
    
    signal i2c_opcode : std_logic_vector(1 downto 0);
    constant RAW_WRITE  : std_logic_vector(1 downto 0) := "00";
    constant RAW_READ   : std_logic_vector(1 downto 0) := "01";
    constant REG_WRITE  : std_logic_vector(1 downto 0) := "10";
    constant REG_READ   : std_logic_vector(1 downto 0) := "11";
    
    type state_t is (IDLE, CONFIGURE_VOLTAGE, READ_VOLTAGE, CONFIGURE_CURRENT, 
                    READ_CURRENT, I2C_WAIT, SLEEP, WAIT_FOR_CONVERT, WAIT_FOR_NEW_DATA);
    signal last_state, state, next_state : state_t;
   
    signal read_word   : std_logic_vector(7 downto 0);   
    signal reg_addr       : std_logic_vector(7 downto 0);
    signal reg_write_data : std_logic_vector(7 downto 0);
    signal reg_read_data  : std_logic_vector(7 downto 0);
    
    signal i2c_en, i2c_busy, i2c_prev_busy, i2c_err, en_0 : std_logic;
    signal prgm_seq : std_logic_vector(15 downto 0);
    
    signal ptr : integer := 0;
    signal flush : std_logic;
    
    --attribute mark_debug : string;
    --attribute mark_debug of state        : signal is "true";
    --attribute mark_debug of last_state : signal is "true";
    --attribute mark_debug of reg_addr : signal is "true";
    --attribute mark_debug of reg_write_data : signal is "true";
    --attribute mark_debug of reg_read_data : signal is "true";
    --attribute mark_debug of i2c_err : signal is "true";
    --attribute mark_debug of prgm_seq : signal is "true";
    --attribute mark_debug of i2c_opcode : signal is "true";
    --attribute mark_debug of ptr : signal is "true";
    
begin
    
    voltage_a <= device_data(0).value;
    voltage_b <= device_data(1).value;
    voltage_c <= device_data(2).value;
    voltage_d <= device_data(3).value;
    
    current_a <= device_data(4).value;
    current_b <= device_data(5).value;
    current_c <= device_data(6).value;
    current_d <= device_data(7).value;
    
    device_temp <= device_data(8).value;
    device_vcc  <= device_data(9).value;
    
    i2c_master_0 : entity work.i2c_master
    generic map (
        BUS_CLK_FREQ   => 100_000
    )
    port map (
        clk       => clk_50MHz,
        reset     => rst,
        
        opcode    => i2c_opcode,
        en        => i2c_en,
        busy      => i2c_busy,
        
        slave_addr     => I2C_DEVICE_ADDRESS,
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
        
    begin
        if (rst = '1') then
            prgm_seq    <= (others => '0');
            read_word   <= (others => '0');  
            i2c_en      <= '0';
            en_0        <= '0';
            ptr         <= 0;
            valid       <= '0';
            flush       <= '0';
            last_state  <= IDLE;
            state       <= IDLE;
            next_state  <= IDLE;
        elsif (rising_edge(clk_50MHz)) then
            case state is
                when IDLE =>
                    busy   <= '0';
                    prgm_seq <= (others => '0');
                    i2c_en <= '0';

                    en_0 <= en;
                    if (en_0 = '0' and en = '1') then
                        busy <= '1';
                        valid <= '0';
                        ptr <= 0;
                        state <= CONFIGURE_VOLTAGE;
                    end if;
                
                when CONFIGURE_VOLTAGE =>
                    last_state <= CONFIGURE_VOLTAGE;
                    prgm_seq <= prgm_seq + x"1";
                    
                    case prgm_seq is
                        when x"0000" =>
                            -- PWM, VCC and TINTERNAL CONTROL (08h) Register
                            i2c_write_word_c(x"08", x"00");
                        
                        when x"0001" =>
                            i2c_write_word_c(x"01", x"00");
                        
                        when x"0002" =>
                            -- V1, V2 and V3, V4 CONTROL (06h) Register
                            i2c_write_word_c(x"06", x"00");
                            
                        when x"0003" =>
                            -- V5, V6 and V7, V8 CONTROL (07h) Register 
                            i2c_write_word_c(x"07", x"00");
                            
                        when x"0004" =>
                            -- STATUS HIGH, CHANNEL ENABLE, Trigger (01h) Register
                            i2c_write_word_c(x"01", x"f8");
                        
                        when x"0005" =>
                            -- Trigger a convert
                            i2c_read_word_c(x"01");

                        when x"0006" =>
                            state <= WAIT_FOR_CONVERT;
                            next_state <= CONFIGURE_VOLTAGE;
                        
                        when x"0007" =>
                            i2c_read_word_c(x"00");

                        when x"0008" =>
                            prgm_seq <= (others => '0');
                            state <= WAIT_FOR_NEW_DATA;
                            next_state <= READ_VOLTAGE;
                        
                        when others =>
                            state <= IDLE;
                    
                    end case;
                
                when WAIT_FOR_CONVERT =>
                    last_state <= WAIT_FOR_CONVERT;
                    if ((read_word and "00000100") /= x"00") then
                        i2c_read_word_c(x"01");
                    else
                        state <= next_state;
                    end if;
                    
                when WAIT_FOR_NEW_DATA =>
                    last_state <= WAIT_FOR_NEW_DATA;
                    if ((read_word and x"0f") = x"0f") then
                        state <= next_state;
                    else
                        i2c_read_word_c(x"00");
                    end if;
                
                when READ_VOLTAGE =>
                    last_state <= READ_VOLTAGE;
                    prgm_seq <= prgm_seq + x"1";
                    
                    if (ptr < 4) then
                        case prgm_seq is
                            -- Read MSB 
                            when x"0000" =>
                                i2c_read_word_c(device_data(ptr).reg);
                                
                            -- Read LSB , store MSB
                            when x"0001" =>
                                device_data(ptr).value(15 downto 8) <= read_word;
                                i2c_read_word_c(device_data(ptr).reg+1);
                            -- Store LSB    
                            when x"0002" =>
                                device_data(ptr).value(7 downto 0) <= read_word;
                                ptr <= ptr + 1;
                                prgm_seq <= (others => '0');
                                
                            when others =>
                                state <= IDLE;
                        end case;
                    else
                        state <= CONFIGURE_CURRENT;
                    end if;
                    
                when CONFIGURE_CURRENT =>
                    last_state <= CONFIGURE_CURRENT;
                    prgm_seq <= prgm_seq + x"1";
                    
                    case prgm_seq is
                        when x"0000" =>
                            -- PWM, VCC and TINTERNAL CONTROL (08h) Register
                            i2c_write_word_c(x"08", x"00");
                        
                        when x"0001" =>
                            i2c_write_word_c(x"01", x"00");
                        
                        when x"0002" =>
                            -- V1, V2 and V3, V4 CONTROL (06h) Register
                            i2c_write_word_c(x"06", "00010001");
                            
                        when x"0003" =>
                            -- V5, V6 and V7, V8 CONTROL (07h) Register
                            i2c_write_word_c(x"07", "00010001");
                            
                        when x"0004" =>
                            -- STATUS HIGH, CHANNEL ENABLE, Trigger (01h) Register
                            i2c_write_word_c(x"01", "11111000");
                        
                        when x"0005" =>
                            -- Trigger a convert
                            i2c_read_word_c(x"01");

                        when x"0006" =>
                            state <= WAIT_FOR_CONVERT;
                            next_state <= CONFIGURE_CURRENT;
                        
                        when x"0007" =>
                            -- Wait for new data
                            i2c_read_word_c(x"00");

                        when x"0008" =>
                            prgm_seq <= (others => '0');
                            state <= WAIT_FOR_NEW_DATA;
                            next_state <= READ_CURRENT;
                            
                        when others =>
                            state <= IDLE;
                    end case;
                    
                when READ_CURRENT =>
                    last_state <= READ_CURRENT;
                    prgm_seq <= prgm_seq + x"1";
                    
                    if (ptr < device_data'length) then
                        case prgm_seq is
                            -- Read MSB 
                            when x"0000" =>
                                i2c_read_word_c(device_data(ptr).reg);
                                
                            -- Read LSB, store MSB 
                            when x"0001" =>
                                device_data(ptr).value(15 downto 8) <= read_word;
                                i2c_read_word_c(device_data(ptr).reg+1);
                            
                            -- Store LSB
                            when x"0002" =>
                                device_data(ptr).value(7 downto 0) <= read_word;
                                ptr <= ptr + 1;
                                prgm_seq <= (others => '0');
                                
                            when others =>
                                state <= IDLE;
                        end case;
                    else
                        ptr <= 0;
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
