library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;

entity tx_data_link is
    port ( 
        -- Async Reset --
        reset        : in std_logic;

        -- TX PHY Encoder --
        tx_clk       : in STD_LOGIC;
        tx_data      : out STD_LOGIC_VECTOR(15 downto 0);
        tx_kcode     : out STD_LOGIC_VECTOR(1 downto 0);
        tx_disparity : out std_logic_vector(15 downto 0);
        tx_is_locked : in std_logic;

        -- Register Frame Data --
        tx_data_clk  : in std_logic;
        tx_data_wren : in std_logic;
        tx_data_in   : in std_logic_vector(16 downto 0);
        tx_data_full : out std_logic;
        
        -- Stream Frame Data --
        stream_clk   : in std_logic;
        stream_wren  : in std_logic;
        stream_data  : in std_logic_vector(16 downto 0);
        stream_full  : out std_logic;
        
        tx_activity : out std_logic
    );
end tx_data_link;

architecture behv of tx_data_link is

    type ACTIVITY_STATE_T is (IDLE, READ_REGISTER_FIFO, WAIT_FOR_REGISTER_DATA, READ_STREAM_FIFO, WAIT_FOR_STREAM_DATA);
    signal act_state : ACTIVITY_STATE_T;

    signal tx_empty_i : std_logic;
    signal tx_fifo_data : std_logic_vector(16 downto 0);
    signal tx_fifo_rden : std_logic;
    signal tx_fifo_avail : std_logic;
    
    signal tx_data_wren_i : std_logic;
    signal stream_wren_i  : std_logic;

    signal tx_data_in_i : std_logic_vector(tx_data_in'range);
    signal stream_data_i : std_logic_vector(stream_data'range);
        
    signal stream_fifo_rden : std_logic;
    signal stream_fifo_data : std_logic_vector(16 downto 0);
    signal stream_fifo_avail, stream_empty_i : std_logic;

    attribute mark_debug : string;    
    attribute mark_debug of tx_data : signal is "true";  
    attribute mark_debug of tx_kcode : signal is "true";  
    attribute mark_debug of tx_is_locked : signal is "true";
    attribute mark_debug of tx_fifo_rden : signal is "true";
    attribute mark_debug of tx_fifo_data : signal is "true";
    attribute mark_debug of tx_fifo_avail : signal is "true";
    attribute mark_debug of tx_empty_i : signal is "true";
    attribute mark_debug of act_state : signal is "true";
    attribute mark_debug of stream_data : signal is "true";
    attribute mark_debug of stream_full : signal is "true";
    
begin

    tx_led_act : entity work.led_activity
    port map ( 
        clk     => tx_clk,
        reset   => reset or not tx_is_locked,
    	act_in  => tx_data_wren_i or stream_wren_i,
        led_out => tx_activity
    );


    tx_cmd_crc_generator : entity work.crc_generator
    port map (
        clk => tx_data_clk,
        rst => reset or not tx_is_locked,
        
        data_in => tx_data_in,
        wren_in => tx_data_wren,
        
        data_out => tx_data_in_i,
        wren_out => tx_data_wren_i
    );


    tx_fifo_0 : entity work.fifo_17b_1024
    port map (
        rst => reset or not tx_is_locked,
        
        wr_clk => tx_data_clk,
        wr_en  => tx_data_wren_i,
        din    => tx_data_in_i,
        full   => tx_data_full,

        rd_clk => tx_clk,
        rd_en  => tx_fifo_rden,
        dout   => tx_fifo_data,
        empty  => tx_empty_i
    );    
    tx_fifo_avail <= not tx_empty_i;

    tx_stream_crc_generator : entity work.crc_generator
    port map (
        clk => stream_clk,
        rst => reset or not tx_is_locked,
        
        data_in => stream_data,
        wren_in => stream_wren,
        
        data_out => stream_data_i,
        wren_out => stream_wren_i
    );
    
    stream_fifo_0 : entity work.fifo_17b_1024
    port map (
        rst => reset or not tx_is_locked,
        
        wr_clk => stream_clk,
        wr_en  => stream_wren_i,
        din    => stream_data_i,
        full   => stream_full,

        rd_clk => tx_clk,
        rd_en  => stream_fifo_rden,
        dout   => stream_fifo_data,
        empty  => stream_empty_i
    );    
    stream_fifo_avail <= not stream_empty_i;

    tx_disparity <= (others => '0');

    process(tx_clk, tx_is_locked, reset)
        procedure tx_send_data(constant data_upper : std_logic_vector(7 downto 0);
                                constant data_lower : std_logic_vector(7 downto 0);
                                constant upper_isK : std_logic;
                                constant lower_isK : std_logic) is
        begin
                tx_data <= data_upper & data_lower;
                tx_kcode <= upper_isK & lower_isK;
        end procedure;

    begin
        if (reset = '1' or tx_is_locked = '0') then
            tx_data <= (others => '0');
            tx_kcode <= (others => '0');
            act_state <= IDLE;
            tx_fifo_rden <= '0';
            stream_fifo_rden <= '0';
        elsif (rising_edge(tx_clk)) then           
            tx_fifo_rden <= '0';
            stream_fifo_rden <= '0';
            
            case act_state is
                when IDLE =>
                    tx_send_data(D_16_2_C, K_28_5_C, '0', '1');
                    
                    if (tx_fifo_avail = '1') then
                        tx_fifo_rden <= '1';
                        if (tx_fifo_data = "1" & x"FEED") then
                            tx_send_data(START_OF_REG_C, K_28_5_C, '0', '1');
                        end if;
                        act_state <= WAIT_FOR_REGISTER_DATA;
                    elsif (stream_fifo_avail = '1') then
                        stream_fifo_rden <= '1';
                        if (stream_fifo_data = "1" & x"FEED") then
                            tx_send_data(START_OF_BLOCK_C, K_28_5_C, '0', '1');
                            act_state <= WAIT_FOR_STREAM_DATA;
                        else 
                            tx_send_data(RESUME_BLOCK_C, K_28_5_C, '0', '1');
                            act_state <= READ_STREAM_FIFO;
                        end if;
                    end if;
                    
                when READ_REGISTER_FIFO =>                       
                    if (tx_fifo_avail = '0') then
                        tx_send_data(WAIT_FOR_REG_C, K_28_5_C, '0', '1');
                        act_state <= WAIT_FOR_REGISTER_DATA;
                    else
                        if (tx_fifo_data = "1" & x"FACE") then
                            tx_send_data(END_OF_REG_C, K_28_5_C, '0', '1');
                            act_state <= IDLE;                                
                        else
                            tx_fifo_rden <= '1';
                            tx_data <= tx_fifo_data(15 downto 0);
                            tx_kcode <= b"00";
                        end if;
                    end if;          
                    
                when WAIT_FOR_REGISTER_DATA =>
                    tx_send_data(WAIT_FOR_REG_C, K_28_5_C, '0', '1');
                    if (tx_fifo_avail = '1') then
                        tx_fifo_rden <= '1';
                        act_state <= READ_REGISTER_FIFO;
                    end if;
                    
                when READ_STREAM_FIFO =>
                    if (stream_fifo_avail = '0') then 
                        act_state <= IDLE;
                        tx_send_data(PAUSED_BLOCK_C, K_28_5_C, '0', '1');
                    else
                        if (stream_fifo_data = "1" & x"FACE") then
                            tx_send_data(END_OF_BLOCK_C, K_28_5_C, '0', '1');
                            act_state <= IDLE;
                        else
                            stream_fifo_rden <= '1';
                            tx_data <= stream_fifo_data(15 downto 0);
                            tx_kcode <= b"00";
                        end if;
                    end if;
                                                                
                when WAIT_FOR_STREAM_DATA =>
                    tx_send_data(PAUSED_BLOCK_C, K_28_5_C, '0', '1');
                    if (stream_fifo_avail = '1') then
                        tx_send_data(RESUME_BLOCK_C, K_28_5_C, '0', '1');
                        stream_fifo_rden <= '1';
                        act_state <= READ_STREAM_FIFO;
                    end if;
            end case;
        end if;
    end process;
end behv;
