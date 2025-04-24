library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_unsigned.all;

entity checksum is
    port (
        clk : in std_logic;
        rst : in std_logic;
        dval : in std_logic;
        
        din  : in std_logic_vector(15 downto 0);
        dout : out std_logic_vector(15 downto 0)
    );
end checksum;

architecture behv of checksum is
begin
    process(clk, rst, dval)
        variable sum : unsigned(16 downto 0);
    begin
        if (rst = '1') then
            sum := (others => '0');
        elsif (rising_edge(clk)) then
            if (dval = '1') then
                sum := sum + unsigned(din);
                if (sum(16) = '1') then
                    sum := '0' & sum(15 downto 0) + "1";
                end if;
                for i in 0 to 15 loop
                    dout(i) <= not(std_logic(sum(i)));
                end loop;
            end if;
        end if;
    end process;
end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity register_rx is
    port (
        clk : in std_logic;
        rst : in std_logic;
        
        reg_addr    : out std_logic_vector(15 downto 0);
        reg_data    : out std_logic_vector(31 downto 0);
        reg_read    : out std_logic;
        reg_write   : out std_logic;
        reg_request : out std_logic;
        reg_ack     : out std_logic;
        reg_fail    : out std_logic;
        reg_in_recv : out std_logic;
        
        rx_data_clk   : out std_logic;
        rx_data_avail : in std_logic;
        rx_data_rden  : out std_logic;
        rx_data_out   : in std_logic_vector(15 downto 0);
        rx_data_cnt   : in std_logic_vector(9 downto 0)
    );
end register_rx;

architecture behv of register_rx is
    type RX_STATE_T is (READ_REQUEST, READ_REPLY, READ_FAIL, 
                            WRITE_REQUEST, WRITE_ACK, WRITE_FAIL, IDLE, HEADER, WAIT_FOR_AVAIL, VALIDATE);
    signal state, last_state, payload_state : RX_STATE_T;
    
    constant OP_READ_REQUEST_C  : std_logic_vector(15 downto 0) := x"fee0";
    constant OP_READ_REPLY_C    : std_logic_vector(15 downto 0) := x"fee1";
    constant OP_READ_FAIL_C     : std_logic_vector(15 downto 0) := x"fee2";
    constant OP_WRITE_REQUEST_C : std_logic_vector(15 downto 0) := x"fee3";
    constant OP_WRITE_ACK_C     : std_logic_vector(15 downto 0) := x"fee4";
    constant OP_WRITE_FAIL_C    : std_logic_vector(15 downto 0) := x"fee5";
    
    signal cnt : std_logic_vector(7 downto 0) := (others => '0');
    signal seq_number : std_logic_vector(15 downto 0) := (others => '0');
    signal rx_seq_number : std_logic_vector(15 downto 0) := (others => '0');
    
    signal tbd : std_logic_vector(15 downto 0);
    signal fee_id : std_logic_vector(15 downto 0);
    signal rx_reg_addr : std_logic_vector(15 downto 0);
    signal rx_reg_data : std_logic_vector(31 downto 0);
    signal checksum : std_logic_vector(15 downto 0);
    signal opcode : std_logic_vector(15 downto 0);
    signal timeout : std_logic_vector(7 downto 0);
    
     --attribute mark_debug : string;
     --attribute mark_debug of state : signal is "true";
     --attribute mark_debug of seq_number : signal is "true";
     --attribute mark_debug of rx_seq_number : signal is "true";
     --attribute mark_debug of fee_id : signal is "true";
     --attribute mark_debug of rx_reg_addr : signal is "true";
     --attribute mark_debug of rx_reg_data : signal is "true";
     --attribute mark_debug of checksum : signal is "true";
     --attribute mark_debug of opcode : signal is "true";
     --attribute mark_debug of cnt : signal is "true";
     --attribute mark_debug of rx_data_avail : signal is "true";
     --attribute mark_debug of rx_data_rden : signal is "true";
     --attribute mark_debug of rx_data_out : signal is "true";
     --attribute mark_debug of rx_data_cnt : signal is "true";
     --attribute mark_debug of timeout  : signal is "true";

begin

    rx_data_clk <= clk;
    
    process(clk, rst)
    begin
        if (rst = '1') then
            cnt <= (others => '0');
            rx_data_rden <= '0';
            reg_addr <= (others => '0');
            reg_data <= (others => '0');
            timeout <= (others => '0');
            reg_read <= '0';
            reg_write <= '0';
            reg_request <= '0'; 
            reg_ack <= '0'; 
            reg_in_recv <= '0';
            reg_fail <= '0';
            state <= IDLE;
        elsif (rising_edge(clk)) then
            case (state) is
                when IDLE =>
                    reg_in_recv <= '0';
                    cnt <= (others => '0');
                    timeout <= (others => '0');
                    reg_addr <= (others => '0');
                    reg_data <= (others => '0');
                    reg_read <= '0';
                    reg_write <= '0';
                    reg_request <= '0'; 
                    reg_ack <= '0'; 
                    reg_fail <= '0';
                    rx_data_rden <= '0';
                    
                    if (rx_data_avail = '1') then
                        seq_number <= seq_number + 1;
                        rx_data_rden <= '1';
                        state <= HEADER;
                        case (rx_data_out) is
                            when OP_READ_REQUEST_C  => payload_state <= READ_REQUEST;
                            when OP_READ_REPLY_C    => payload_state <= READ_REPLY;
                            when OP_READ_FAIL_C     => payload_state <= READ_FAIL;
                            when OP_WRITE_REQUEST_C => payload_state <= WRITE_REQUEST;
                            when OP_WRITE_ACK_C     => payload_state <= WRITE_ACK;
                            when OP_WRITE_FAIL_C    => payload_state <= WRITE_FAIL;
                            when others => state <= IDLE;
                        end case;
                    end if;
                
                when HEADER =>
                    reg_in_recv <= '1';
                    if (rx_data_avail = '1') then
                        cnt <= cnt + 1;
                        rx_data_rden <= '1';
                        case (cnt) is
                            when x"00" =>        opcode <= rx_data_out; -- Opcode --
                            when x"01" =>        fee_id <= rx_data_out; -- FEE ID --
                            when x"02" => rx_seq_number <= rx_data_out; -- Sequence Number --
                            when x"03" =>           tbd <= rx_data_out; -- TBD --
                            state <= payload_state;
                            cnt <= (others => '0');
                        when others => state <= IDLE;
                    end case;
                    else
                        rx_data_rden <= '0';
                        last_state <= state;
                        state <= WAIT_FOR_AVAIL;
                    end if;
                    
                    
                    
                when READ_REQUEST | READ_FAIL | WRITE_ACK | WRITE_FAIL =>
                    reg_in_recv <= '1';
                    if (rx_data_avail = '1') then
                        cnt <= cnt + 1;
                        rx_data_rden <= '1';
                        case (cnt) is
                            when x"00" =>   rx_reg_addr <= rx_data_out; -- Register Addr --
                            when x"01" =>      checksum <= rx_data_out; -- Checksum --
                                           rx_data_rden <= '0';
                                                    cnt <= (others => '0');
                                                  state <= VALIDATE;
                            when others => state <= IDLE;
                        end case;
                    else
                        rx_data_rden <= '0';
                        last_state <= state;
                        state <= WAIT_FOR_AVAIL;
                    end if;
                    
                
                when WRITE_REQUEST | READ_REPLY =>
                    reg_in_recv <= '1';
                    if (rx_data_avail = '1') then
                        cnt <= cnt + 1;
                        rx_data_rden <= '1';
                        case (cnt) is
                            when x"00" =>   rx_reg_addr <= rx_data_out; -- Register Addr --
                            when x"01" =>   rx_reg_data(31 downto 16) <= rx_data_out;
                            when x"02" =>   rx_reg_data(15 downto 0)  <= rx_data_out;
                            when x"03" =>      checksum <= rx_data_out; -- Checksum --
                                           rx_data_rden <= '0';
                                                    cnt <= (others => '0');
                                                  state <= VALIDATE;
                            when others => state <= IDLE;
                        end case;
                    else
                        rx_data_rden <= '0';
                        last_state <= state;
                        state <= WAIT_FOR_AVAIL;
                    end if;
                    
                    
                when VALIDATE =>
                    reg_in_recv <= '1';
                    reg_addr <= rx_reg_addr;
                    reg_data <= rx_reg_data;
                    
                    case (payload_state) is
                        when READ_REQUEST => reg_read <= '1'; reg_write <= '0'; reg_request <= '1'; reg_ack <= '0'; reg_fail <= '0';
                        when READ_REPLY   => reg_read <= '1'; reg_write <= '0'; reg_request <= '0'; reg_ack <= '1'; reg_fail <= '0';
                        when READ_FAIL    => reg_read <= '1'; reg_write <= '0'; reg_request <= '0'; reg_ack <= '1'; reg_fail <= '1';
                        when WRITE_REQUEST=> reg_read <= '0'; reg_write <= '1'; reg_request <= '1'; reg_ack <= '0'; reg_fail <= '0';
                        when WRITE_ACK    => reg_read <= '0'; reg_write <= '1'; reg_request <= '0'; reg_ack <= '1'; reg_fail <= '0';
                        when WRITE_FAIL   => reg_read <= '0'; reg_write <= '1'; reg_request <= '0'; reg_ack <= '1'; reg_fail <= '1';
                        when others       => reg_read <= '0'; reg_write <= '0'; reg_request <= '0'; reg_ack <= '0'; reg_fail <= '0';
                    end case;
                    
                    state <= IDLE;
                    
                    
                when WAIT_FOR_AVAIL =>
                    reg_in_recv <= '0';
                    rx_data_rden <= '0';
                    timeout <= timeout + x"01";
                    if (rx_data_avail = '1') then
                        rx_data_rden <= '1';
                        timeout <= (others => '0');
                        state <= last_state;
                    elsif (timeout = x"ff") then
                        timeout <= (others => '0');
                        state <= IDLE;
                    end if;
            end case;
        end if;
    end process;
end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity register_tx is
    port (
        clk : in std_logic;
        rst : in std_logic;
        busy : out std_logic;
        
        reg_addr : in std_logic_vector(15 downto 0);
        reg_data : in std_logic_vector(31 downto 0);
        reg_read : in std_logic;
        reg_write : in std_logic;
        reg_request : in std_logic;
        reg_ack : in std_logic;
        reg_fail : in std_logic;
        fee_id : in std_logic_vector(15 downto 0);
        
        tx_data_clk  : out std_logic;
        tx_data_wren : out std_logic;
        tx_data_in   : out std_logic_vector(16 downto 0);
        tx_data_cnt  : in std_logic_vector(9 downto 0);
        tx_data_full : in std_logic
    );
end register_tx;

architecture behv of register_tx is
    type TX_STATE_T is (READ_REQUEST, READ_REPLY, READ_FAIL, 
                            WRITE_REQUEST, WRITE_ACK, WRITE_FAIL, IDLE, HEADER, WAIT_FOR);
    signal state, payload_state : TX_STATE_T;
    
    constant OP_READ_REQUEST_C  : std_logic_vector(15 downto 0) := x"fee0";
    constant OP_READ_REPLY_C    : std_logic_vector(15 downto 0) := x"fee1";
    constant OP_READ_FAIL_C     : std_logic_vector(15 downto 0) := x"fee2";
    constant OP_WRITE_REQUEST_C : std_logic_vector(15 downto 0) := x"fee3";
    constant OP_WRITE_ACK_C     : std_logic_vector(15 downto 0) := x"fee4";
    constant OP_WRITE_FAIL_C    : std_logic_vector(15 downto 0) := x"fee5";
    
    type SLV_16b_ARRAY is array (0 to 5) of std_logic_vector(15 downto 0);
    signal OPCODES : SLV_16b_ARRAY := (
        TX_STATE_T'POS(READ_REQUEST)  => OP_READ_REQUEST_C,
        TX_STATE_T'POS(READ_REPLY)    => OP_READ_FAIL_C,
        TX_STATE_T'POS(READ_FAIL)     => OP_READ_FAIL_C,
        TX_STATE_T'POS(WRITE_REQUEST) => OP_WRITE_REQUEST_C,
        TX_STATE_T'POS(WRITE_ACK)     => OP_WRITE_ACK_C,
        TX_STATE_T'POS(WRITE_FAIL)    => OP_WRITE_FAIL_C
    );
    
    signal cnt : std_logic_vector(7 downto 0) := (others => '0');
    signal seq_number : std_logic_vector(15 downto 0) := (others => '0');
    signal tx_reg_addr : std_logic_vector(reg_addr'range);
    signal tx_reg_data : std_logic_vector(reg_data'range);
    signal tx_fee_id : std_logic_vector(fee_id'range);
    
    --attribute mark_debug : string;
    -- attribute mark_debug of tx_data_wren : signal is "true";
    -- attribute mark_debug of tx_data_in : signal is "true";
    -- attribute mark_debug of tx_data_cnt : signal is "true";
    -- attribute mark_debug of tx_data_full : signal is "true";
    -- attribute mark_debug of cnt : signal is "true";
    -- attribute mark_debug of seq_number : signal is "true";
    -- attribute mark_debug of tx_addr : signal is "true";
    -- attribute mark_debug of tx_data : signal is "true";

begin

    tx_data_clk <= clk;
    
--    chksum_i : entity work.checksum
--    port map (
--        clk => clk,
--        rst => chksum_rst,
--        dval => chksum_dval,
--        din => 
--        dout => chksum
--    );

    
    process(clk, rst)
    begin
        if (rst = '1') then
            cnt <= (others => '0');
            state <= IDLE;
            tx_data_wren <= '0';
            tx_data_in <= (others => '0');
            seq_number <= (others => '0');
            tx_reg_data <= (others => '0');
            tx_reg_addr <= (others => '0');
            busy <= '0';
        elsif (rising_edge(clk)) then
            case (state) is
                when IDLE =>
                    tx_data_wren <= '0';
                    cnt <= (others => '0');
                    tx_fee_id <= fee_id;
                    busy <= '0';
                    
                    if (reg_read = '1' and reg_request = '1') then
                        seq_number <= seq_number + 1;
                        tx_reg_addr <= reg_addr;
                        tx_reg_data <= (others => '0');
                        payload_state <= READ_REQUEST;
                        state <= HEADER;
                    elsif (reg_read = '1' and reg_ack = '1' and reg_fail = '0') then
                        seq_number <= seq_number + 1;
                        tx_reg_addr <= reg_addr;
                        tx_reg_data <= reg_data;
                        payload_state <= READ_REPLY;
                        state <= HEADER;
                    elsif (reg_read = '1' and reg_ack = '1' and reg_fail = '1') then
                        seq_number <= seq_number + 1;
                        tx_reg_addr <= reg_addr;
                        tx_reg_data <= (others => '0');
                        payload_state <= READ_FAIL;
                        state <= HEADER;
                    elsif (reg_write = '1' and reg_request = '1') then
                        seq_number <= seq_number + 1;
                        tx_reg_addr <= reg_addr;
                        tx_reg_data <= reg_data;
                        payload_state <= WRITE_REQUEST;
                        state <= HEADER;
                    elsif (reg_write = '1' and reg_ack = '1' and reg_fail = '0') then
                        seq_number <= seq_number + 1;
                        tx_reg_addr <= reg_addr;
                        tx_reg_data <= reg_data;
                        payload_state <= WRITE_ACK;
                        state <= HEADER;
                    elsif (reg_write = '1' and reg_ack = '1' and reg_fail = '1') then
                        seq_number <= seq_number + 1;
                        tx_reg_addr <= reg_addr;
                        tx_reg_data <= reg_data;
                        payload_state <= WRITE_FAIL;
                        state <= HEADER;
                    end if;
               
               when HEADER =>
                    busy <= '1';
                    cnt <= cnt + 1;
                    tx_data_wren <= '1';
                    
                    case (cnt) is
                        when x"00" => tx_data_in <= '1' & x"feed";
                        when x"01" => tx_data_in <= '0' & OPCODES(TX_STATE_T'POS(payload_state));    -- Op Code --
                        when x"02" => tx_data_in <= '0' & tx_fee_id;    -- FEE ID --
                        when x"03" => tx_data_in <= '0' & seq_number; -- Sequence Number --
                        when x"04" => tx_data_in <= '0' & x"ffff";    -- MBO --
                                        cnt <= (others => '0');
                                        state <= payload_state;
                        when others => state <= IDLE;
                    end case;
                
                
                when READ_REQUEST | READ_FAIL | WRITE_ACK | WRITE_FAIL =>
                    cnt <= cnt + 1;
                    tx_data_wren <= '1';
                    
                    case (cnt) is
                        when x"00" => tx_data_in <= '0' & tx_reg_addr;    -- Register Addr --
                        when x"01" => tx_data_in <= '0' & x"beef";    -- Checksum --
                        when x"02" => tx_data_in <= '1' & x"face";    -- Checksum --
                                     state <= IDLE;
                        when others => state <= IDLE;
                    end case;
                    
                    
                when WRITE_REQUEST | READ_REPLY =>
                    cnt <= cnt + 1;
                    tx_data_wren <= '1';
                    
                    case (cnt) is
                        when x"00" => tx_data_in <= '0' & tx_reg_addr;                 -- Register Addr --
                        when x"01" => tx_data_in <= '0' & tx_reg_data(31 downto 16);   -- Register Data Upper --
                        when x"02" => tx_data_in <= '0' & tx_reg_data(15 downto 0);    -- Register Data Lower --
                        when x"03" => tx_data_in <= '0' & x"beef";                 -- Checksum --
                        when x"04" => tx_data_in <= '1' & x"face";    -- Checksum --
                                     state <= IDLE;
                        when others => state <= IDLE;
                    end case;
                    
                when WAIT_FOR =>
                    state <= IDLE;
            end case;
        end if;
    end process;
    
end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity register_slave is
    port (
        reg_clk        :  in std_logic;
        reg_rst        :  in std_logic;
        
        reg_addr       : out std_logic_vector(15 downto 0);
        reg_read       : out std_logic;
        reg_write      : out std_logic;
        reg_data_write : out std_logic_vector(31 downto 0);
        reg_ack        :  in std_logic;
        reg_fail       :  in std_logic;
        reg_data_read  :  in std_logic_vector(31 downto 0);
        
        rx_data_clk   : out std_logic;
        rx_data_avail : in std_logic;
        rx_data_rden  : out std_logic;
        rx_data_out   : in std_logic_vector(15 downto 0);
        rx_data_cnt   : in std_logic_vector(9 downto 0);
        
        tx_data_clk  : out std_logic;
        tx_data_wren : out std_logic;
        tx_data_in   : out std_logic_vector(15 downto 0);
        tx_data_cnt  : in std_logic_vector(9 downto 0);
        tx_data_full : in std_logic
    );
end register_slave;

architecture behv of register_slave is
    type SLAVE_STATE_T is (IDLE, WAIT_FOR_READ, WAIT_FOR_WRITE);
    signal state : SLAVE_STATE_T;
    
    signal tx_reg_addr, rx_reg_addr, reg_addr_i : std_logic_vector(15 downto 0);
    signal tx_reg_data, rx_reg_data, reg_data_i : std_logic_vector(31 downto 0);
    
    signal tx_reg_write, tx_reg_read : std_logic;
    signal rx_reg_write, rx_reg_read : std_logic;
    signal rx_reg_req : std_logic;
    signal tx_reg_fail, tx_reg_ack : std_logic;
    
begin

    reg_addr <= reg_addr_i;
    reg_data_write <= reg_data_i;
    
    process(reg_clk, reg_rst)
    begin
        if (reg_rst = '1') then
            reg_read <= '0';
            reg_write <= '0';
            tx_reg_write <= '0';
            tx_reg_read <= '0';
            tx_reg_data <= (others => '0');
            tx_reg_addr <= (others => '0');
            tx_reg_ack <= '0';
            tx_reg_fail <= '0';
            state <= IDLE;
        elsif (rising_edge(reg_clk)) then
            case state is
                when IDLE =>
                    tx_reg_write <= '0';
                    tx_reg_read <= '0';
                    reg_read <= '0';
                    reg_write <= '0';
                    
                    if (rx_reg_req = '1' and rx_reg_read = '1') then
                        reg_read <= rx_reg_read;
                        reg_addr_i <= rx_reg_addr;
                        state <= WAIT_FOR_READ;
                    elsif (rx_reg_req = '1' and rx_reg_write = '1') then
                        reg_data_i <= rx_reg_data;
                        reg_addr_i <= rx_reg_addr;
                        reg_write <= rx_reg_write;
                        state <= WAIT_FOR_WRITE;
                    end if;
                when WAIT_FOR_READ =>
                    if (reg_ack = '1') then
                        tx_reg_read <= '1';
                        tx_reg_ack <= reg_ack;
                        tx_reg_fail <= reg_fail;
                        tx_reg_addr <= reg_addr_i;
                        tx_reg_data <= reg_data_read;
                        state <= IDLE;
                    end if;
                when WAIT_FOR_WRITE =>
                    if (reg_ack = '1') then
                        tx_reg_write <= '1';
                        tx_reg_ack <= reg_ack;
                        tx_reg_fail <= reg_fail;
                        tx_reg_addr <= reg_addr_i;
                        tx_reg_data <= (others => '0');
                        state <= IDLE;
                    end if;
            end case;
        end if;
    end process;
    

    register_rx_i : entity work.register_rx
    port map (
        clk => reg_clk,
        rst => reg_rst,
        
        reg_addr   => rx_reg_addr,
        reg_data   => rx_reg_data, 
        reg_read   => rx_reg_read, 
        reg_write  => rx_reg_write, 
        reg_request => rx_reg_req,
        reg_ack     => open,
        reg_fail    => open, 
        
        rx_data_clk   => rx_data_clk,
        rx_data_avail => rx_data_avail,
        rx_data_rden  => rx_data_rden,
        rx_data_out   => rx_data_out,
        rx_data_cnt   => rx_data_cnt
    );
    
    register_tx_i : entity work.register_tx 
    port map (
        clk => reg_clk,
        rst => reg_rst,
        busy => open,
        
        reg_addr => tx_reg_addr,
        reg_data => tx_reg_data, 
        reg_read => tx_reg_read,
        reg_write => tx_reg_write,
        reg_request => '0',
        reg_ack => tx_reg_ack,
        reg_fail => tx_reg_fail,
        fee_id => x"feee",
        
        tx_data_clk  => tx_data_clk,
        tx_data_wren => tx_data_wren,
        tx_data_in   => tx_data_in,
        tx_data_cnt  => tx_data_cnt,
        tx_data_full => tx_data_full
    );

end behv;


library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity register_master is
    port (
        reg_clk        : in std_logic;
        reg_rst        : in std_logic;
        
        reg_addr       : in std_logic_vector(15 downto 0);
        reg_read       : in std_logic;
        reg_write      : in std_logic;
        reg_data_write : in std_logic_vector(31 downto 0);
        reg_ack        : out std_logic;
        reg_fail       : out std_logic;
        reg_data_read  : out std_logic_vector(31 downto 0);
        reg_addr_read  : out std_logic_vector(15 downto 0);
        
        rx_data_clk   : out std_logic;
        rx_data_avail : in std_logic;
        rx_data_rden  : out std_logic;
        rx_data_out   : in std_logic_vector(15 downto 0);
        rx_data_cnt   : in std_logic_vector(9 downto 0);
        
        tx_data_clk  : out std_logic;
        tx_data_wren : out std_logic;
        tx_data_in   : out std_logic_vector(16 downto 0);
        tx_data_cnt  : in std_logic_vector(9 downto 0);
        tx_data_full : in std_logic
    );
end register_master;

architecture behv of register_master is
    type SLAVE_STATE_T is (IDLE, WAIT_FOR_READ, WAIT_FOR_WRITE);
    signal state : SLAVE_STATE_T;
    
    signal tx_reg_addr, rx_reg_addr : std_logic_vector(15 downto 0);
    signal tx_reg_data, rx_reg_data : std_logic_vector(31 downto 0);
    
    signal timeout : std_logic_vector(15 downto 0);
    
    signal tx_reg_write, tx_reg_read : std_logic;
    signal rx_reg_write, rx_reg_read : std_logic;
    signal tx_reg_req : std_logic;
    signal rx_reg_fail, rx_reg_ack : std_logic;
    signal reg_in_recv : std_logic;

    signal reg_read_q, reg_write_q : std_logic_vector(1 downto 0) := (others => '0');
    
    attribute ASYNC_REG : string;
    attribute ASYNC_REG of reg_read_q  : signal is "true";
    attribute ASYNC_REG of reg_write_q : signal is "true";

    --attribute mark_debug : string;
    --attribute mark_debug of  reg_read  : signal is "true";
    --attribute mark_debug of  reg_write : signal is "true";
    --attribute mark_debug of  state     : signal is "true";
    --attribute mark_debug of  reg_in_recv : signal is "true";

begin

    process(reg_clk)
    begin
        if (rising_edge(reg_clk)) then
            reg_read_q(0) <= reg_read;
            reg_read_q(1) <= reg_read_q(0);

            reg_write_q(0) <= reg_write;
            reg_write_q(1) <= reg_write_q(0);
        end if;
    end process;

    process(reg_clk, reg_rst)
    begin
        if (reg_rst = '1') then
            reg_ack <= '0';
            reg_fail <= '0';
            reg_data_read <= (others => '0');
            reg_addr_read <= (others => '0');
            tx_reg_addr <= (others => '0');
            tx_reg_data <= (others => '0');
            tx_reg_write <= '0';
            tx_reg_read <= '0';
            timeout <= (others => '0');
            state <= IDLE;
        elsif (rising_edge(reg_clk)) then
            case state is
                when IDLE =>
                    timeout <= (others => '0');
                    tx_reg_addr <= (others => '0');
                    tx_reg_data <= (others => '0');
                    tx_reg_write <= '0';
                    tx_reg_read <= '0';
                    tx_reg_req <= '0';
                    
                    if (reg_read_q = "01") then
                        tx_reg_addr <= reg_addr;
                        tx_reg_req <= '1';
                        tx_reg_read <= '1';
                        reg_ack <= '0';
                        reg_fail <= '0';
                        state <= WAIT_FOR_READ;
                    elsif (reg_write_q = "01") then
                        tx_reg_addr <= reg_addr;
                        tx_reg_data <= reg_data_write;
                        tx_reg_req <= '1';
                        tx_reg_write <= '1';
                        reg_ack <= '0';
                        reg_fail <= '0';
                        state <= WAIT_FOR_WRITE;
                    end if;
                    
                when WAIT_FOR_READ =>
                    tx_reg_read <= '0';
                    tx_reg_req <= '0';
                    if (reg_in_recv = '0') then
                        timeout <= timeout + 1;
                    else 
                        timeout <= (others => '0');
                    end if;
                    
                    if (rx_reg_read = '1' and rx_reg_ack = '1' and rx_reg_fail = '0') then
                        reg_ack <= '1';
                        reg_fail <= '0';
                        reg_data_read <= rx_reg_data;
                        reg_addr_read <= rx_reg_addr;
                        state <= IDLE;
                    elsif (rx_reg_read = '1'and rx_reg_ack = '1' and rx_reg_fail = '1') then
                        reg_ack <= '1';
                        reg_fail <= '1';
                        reg_addr_read <= rx_reg_addr;
                        reg_data_read <= (others => '0');
                        state <= IDLE;
                    elsif (timeout = x"ffff") then
                        reg_ack <= '0';
                        reg_fail <= '1';
                        reg_addr_read <= (others => '1');
                        reg_data_read <= (others => '0');
                        state <= IDLE;
                    end if;
                    
                when WAIT_FOR_WRITE =>
                    tx_reg_write <= '0';
                    tx_reg_req <= '0';
                    reg_data_read <= (others => '0');
                    if (reg_in_recv = '0') then
                        timeout <= timeout + 1;
                    else 
                        timeout <= (others => '0');
                    end if;
                    
                    if (rx_reg_write = '1' and rx_reg_ack = '1' and rx_reg_fail = '0') then
                        reg_ack <= '1';
                        reg_fail <= '0';
                        reg_addr_read <= rx_reg_addr;
                        state <= IDLE;
                    elsif (rx_reg_write = '1' and rx_reg_ack = '1' and rx_reg_fail = '1') then
                        reg_ack <= '1';
                        reg_fail <= '1';
                        reg_addr_read <= rx_reg_addr;
                        state <= IDLE;
                    elsif (timeout = x"ffff") then
                        reg_ack <= '0';
                        reg_fail <= '1';
                        reg_addr_read <= (others => '1');
                        reg_data_read <= (others => '0');
                        state <= IDLE;
                    end if;
            end case;
        end if;
    end process;
    

    register_rx_i : entity work.register_rx
    port map (
        clk => reg_clk,
        rst => reg_rst,
        
        reg_addr   => rx_reg_addr,
        reg_data   => rx_reg_data, 
        reg_read   => rx_reg_read, 
        reg_write  => rx_reg_write, 
        reg_request => open,
        reg_ack     => rx_reg_ack,
        reg_fail    => rx_reg_fail,
        reg_in_recv => reg_in_recv, 
        
        rx_data_clk   => rx_data_clk,
        rx_data_avail => rx_data_avail,
        rx_data_rden  => rx_data_rden,
        rx_data_out   => rx_data_out,
        rx_data_cnt   => rx_data_cnt
    );
    
    register_tx_i : entity work.register_tx 
    port map (
        clk => reg_clk,
        rst => reg_rst,
        busy => open,
        
        reg_addr => tx_reg_addr,
        reg_data => tx_reg_data, 
        reg_read => tx_reg_read,
        reg_write => tx_reg_write,
        reg_request => tx_reg_req,
        reg_ack => '0',
        reg_fail => '0',
        fee_id => x"feee",
        
        tx_data_clk  => tx_data_clk,
        tx_data_wren => tx_data_wren,
        tx_data_in   => tx_data_in,
        tx_data_cnt  => tx_data_cnt,
        tx_data_full => tx_data_full
    );

end behv;
