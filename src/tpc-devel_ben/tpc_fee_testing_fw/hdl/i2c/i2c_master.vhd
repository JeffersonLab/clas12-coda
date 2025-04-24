library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity i2c_master is 
    generic (
        INPUT_CLK_FREQ : integer := 50_000_000; 
        BUS_CLK_FREQ   : integer := 400_000
    );   
    port (
        clk       : in std_logic;
        reset     : in std_logic;
    
        slave_addr     : in  std_logic_vector(6 downto 0); 
        reg_addr       : in  std_logic_vector(7 downto 0); 
        reg_write_data : in  std_logic_vector(7 downto 0);
        reg_read_data  : out std_logic_vector(7 downto 0);
        
        opcode : in std_logic_vector(1 downto 0);
        en     : in std_logic;
    
        busy      : out    std_logic;
        ack_error : buffer std_logic;
    
        sda       : inout std_logic;
        scl       : inout std_logic
    );
end i2c_master;

architecture behv of i2c_master is
    constant RAW_WRITE  : std_logic_vector(1 downto 0) := "00";
    constant RAW_READ   : std_logic_vector(1 downto 0) := "01";
    constant REG_WRITE  : std_logic_vector(1 downto 0) := "10";
    constant REG_READ   : std_logic_vector(1 downto 0) := "11";
    
    constant I2C_WRITE_CMD : std_logic := '0';
    constant I2C_READ_CMD  : std_logic := '1';
    
    constant DIVIDER :  integer := (INPUT_CLK_FREQ / BUS_CLK_FREQ)/4; 
    type state_t is (IDLE, I2C_START, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS_ACK,
                    I2C_WRITE, I2C_READ, I2C_SLAVE_WRITE_ACK, I2C_MASTER_ACK, I2C_STOP); 
    signal state      : state_t;
    
    signal data_clk   : std_logic_vector(1 downto 0);
    signal scl_clk    : std_logic;
    signal scl_ena    : std_logic:= '0';
    signal sda_int    : std_logic:= '1';
    signal sda_ena_n  : std_logic;
    signal addr_rw    : std_logic_vector(7 downto 0);
    signal data_tx    : std_logic_vector(7 downto 0);
    signal data_rx    : std_logic_vector(7 downto 0);
    signal bit_cnt    : integer range 0 to 7 := 7;
    signal stretch    : std_logic := '0';
    signal write_data : std_logic := '0';
    
    signal tx_cnt     : integer range 0 to 7 := 0;
    signal rx_cnt     : integer range 0 to 7 := 0;
    
    signal reg_read_done : std_logic := '0';
    
    signal en_edge : std_logic_vector(1 downto 0);
    signal en_i, en_clear : std_logic;
       
begin

    -- Generate SCL
    process (clk, reset)
        variable count : integer range 0 to DIVIDER * 4;
    begin
        if (reset = '1') then
            stretch <= '0';
            count := 0;
            scl_clk     <= '0';
            data_clk(0) <= '0';
        elsif (rising_edge(clk)) then
        
            data_clk(1) <= data_clk(0);
            
            if (count = divider*4-1) then
                count := 0;
            elsif (stretch = '0') then
                count := count + 1;
            end if;
            
            if (count >= 0 and count <= divider-1) then
                scl_clk     <= '0';
                data_clk(0) <= '0';
            elsif (count >= divider and count <= divider*2-1) then
                scl_clk     <= '0';
                data_clk(0) <= '1';
            elsif (count >= divider*2 and count <= divider*3-1) then
                scl_clk     <= '1';
                data_clk(0) <= '1';
                if (scl = '0') then
                    stretch <= '1';
                else
                    stretch <= '0';
                end if;
            else
                scl_clk     <= '1';
                data_clk(0) <= '0';
            end if;
        end if;
    end process;
    
    process (clk, reset)
    begin
        if (reset = '1') then
            state         <= IDLE;
            busy          <= '0';
            scl_ena       <= '0';
            sda_int       <= '1';
            ack_error     <= '0';
            bit_cnt       <= 7;
            reg_read_data <= (others => '0');
            en_edge       <= (others => '0');
            en_i          <= '0';
            en_clear      <= '0';
        elsif (rising_edge(clk)) then
            en_edge(0) <= en;
            en_edge(1) <= en_edge(0);
            if (en_edge = "01") then
                en_i <= '1';
            elsif (en_clear = '1') then
                en_i <= '0';
            end if;
        
            -- Data clk rising edge
            if (data_clk = "01") then 
                case (state) is 
                    when IDLE =>
                        if (en_i = '1') then
                            state <= I2C_START;
                            reg_read_done <= '0';
                            write_data <= '0';
                            en_clear <= '1';
                            case opcode is
                                when RAW_WRITE =>
                                    -- [start] [addr][W][ack] [tx data] [stop]
                                    addr_rw <= slave_addr & I2C_WRITE_CMD;
                                    data_tx <= reg_write_data;
                                when RAW_READ =>
                                    -- [start] [addr][R][ack] [rx data] [stop]
                                    addr_rw <= slave_addr & I2C_READ_CMD;
                                when REG_WRITE =>
                                    -- [start] [addr][W][ack] [reg_addr][ack] [reg_write_data][ack] [stop]
                                    addr_rw <= slave_addr & I2C_WRITE_CMD;
                                    data_tx <= reg_addr;
                                when REG_READ =>
                                    -- [start] [addr][W][ack] [reg_addr][ack] [stop]
                                    -- [start] [addr][R][ack] [rx_data][ack] [stop]
                                    addr_rw <= slave_addr & I2C_WRITE_CMD;
                                    data_tx <= reg_addr;
                                when others =>
                                    null;
                            end case;
                        end if;
                    
                    when I2C_START =>
                        en_clear <= '0';
                        busy <= '1';
                        sda_int <= addr_rw(bit_cnt);
                        state <= I2C_SLAVE_ADDRESS;
                        
                    when I2C_SLAVE_ADDRESS =>
                        if (bit_cnt = 0) then
                            sda_int <= '1';
                            bit_cnt <= 7;
                            state <= I2C_SLAVE_ADDRESS_ACK;
                        else
                            bit_cnt <= bit_cnt - 1;
                            sda_int <= addr_rw(bit_cnt-1);
                        end if;
                        
                    when I2C_SLAVE_ADDRESS_ACK =>
                        if (addr_rw(0) = '0') THEN
                            sda_int <= data_tx(bit_cnt);
                            state <= I2C_WRITE;
                        else
                            sda_int <= '1';
                            state <= I2C_READ;
                        end if;
                        
                    when I2C_WRITE =>
                        if (bit_cnt = 0) then 
                            sda_int <= '1';
                            bit_cnt <= 7;
                            state <= I2C_SLAVE_WRITE_ACK;
                        else 
                            bit_cnt <= bit_cnt - 1;
                            sda_int <= data_tx(bit_cnt-1);
                        end if;
                        
                    when I2C_SLAVE_WRITE_ACK =>
                        if (opcode = REG_WRITE and write_data = '0') then
                            write_data <= '1';
                            data_tx <= reg_write_data;
                            sda_int <= reg_write_data(bit_cnt);
                            state <= I2C_WRITE;
                        elsif (opcode = REG_READ and reg_read_done = '0') then
                            reg_read_done <= '1';
                            addr_rw <= slave_addr & I2C_READ_CMD;
                            state <= I2C_START;
                        else
                            write_data <= '0';
                            state <= I2C_STOP;
                        end if;
                        
                    when I2C_READ =>
                        if (bit_cnt = 0) then 
                            sda_int <= '1';
                            bit_cnt <= 7;
                            reg_read_data <= data_rx;
                            state <= I2C_MASTER_ACK;
                        else
                            bit_cnt <= bit_cnt - 1;
                        end if;
                        
                    when I2C_MASTER_ACK =>
                        state <= I2C_STOP;
                        
                    when I2C_STOP =>
                        if (opcode = REG_READ and reg_read_done = '0') then
                            reg_read_done <= '1';
                            addr_rw <= slave_addr & I2C_READ_CMD;
                            state <= I2C_START;
                        else
                            busy <= '0';
                            state <= IDLE;
                        end if;
                        
                end case;
                
            -- Data clk falling edge
            elsif (data_clk = "10") then
                case state is 
                    when I2C_START =>
                        if (scl_ena = '0') then 
                            scl_ena <= '1';
                            ack_error <= '0';
                        end if;
                        
                    when I2C_SLAVE_ADDRESS_ACK =>
                        if (sda /= '0' or ack_error = '1') then 
                            ack_error <= '1';
                        end if;
                        
                    when I2C_READ =>
                        data_rx(bit_cnt) <= sda;
                        
                    when I2C_SLAVE_WRITE_ACK => 
                        if (sda /= '0' or ack_error = '1') then 
                            ack_error <= '1';
                        end if;
                        
                    when I2C_STOP =>
                        scl_ena <= '0';
                        
                    when others =>
                        null;
                end case;
            end if;
        end if;
    end process;
    
    with state select 
    sda_ena_n <=     data_clk(1) when I2C_START, -- generate start condition
                 not data_clk(1) when I2C_STOP,  -- generate stop condition
                         sda_int when others;    -- set to internal sda signal    
      
    -- Set scl and sda outputs
    scl <= '0' when (scl_ena = '1' and scl_clk = '0') else 'Z';
    sda <= '0' when sda_ena_n = '0' else 'Z';
    
end architecture;
