library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;
use work.phy_pkg.all;

entity data_packer is
    port (
        clk        : in  std_logic;
        rst        : in  std_logic;
        block_size : in  std_logic_vector(9 downto 0);
        
        data_in    : in data_16b_arr(NUMBER_OF_FEEs-1 downto 0);
        read_en    : out std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        priority   : in std_logic_vector(NUMBER_OF_FEEs-1 downto 0);
        avail      : in std_logic_vector(NUMBER_OF_FEEs-1 downto 0);

        write_256b_data : out std_logic_vector(255 downto 0);
        write_256b_en   : out std_logic;
        write_256b_full : in std_logic
    );
end data_packer;

architecture behv of data_packer is

    type state_t is (IDLE, HEADER_0, HEADER_1, HEADER_2, HEADER_3, DATA, WAIT_UNTIL_WRITE_FIFO_EMPTY, ROTATE_OFFSETS);
    signal state : state_t;

    signal data_out : std_logic_vector(31 downto 0);
    signal fifo_pending : std_logic_vector(avail'range);

    signal fifo_idx, fifo_cnt : integer;

    type slv_arr1 is array(avail'range) of unsigned(4 downto 0); 
    signal offsets : slv_arr1;

    signal dma_block_size : unsigned(block_size'range);
    type slv_arr2 is array(0 to NUMBER_OF_FEEs-1) of unsigned(block_size'range);
    signal read_cnt : slv_arr2;

    type SLV_32b_ARRAY_T is array(0 to 15) of std_logic_vector(15 downto 0);
    signal data_16b : SLV_32b_ARRAY_T;
    signal din_256b : std_logic_vector(255 downto 0);

    signal flipped : std_logic;

    signal last_offset_idx : unsigned(4 downto 0);
    signal offset_zero     : unsigned(4 downto 0);
    signal base       : unsigned(4 downto 0) := "00000";
    signal flip       : std_logic_vector(avail'range);
    
    signal read_256b_empty : std_logic;
    signal din_wr_en, din_full : std_logic;
    
    --attribute mark_debug : string;
    --attribute mark_debug of state : signal is "true";
    --attribute mark_debug of fifo_pending : signal is "true";
    --attribute mark_debug of read_cnt : signal is "true";
    --attribute mark_debug of base : signal is "true";
    --attribute mark_debug of data_16b : signal is "true";
    --attribute mark_debug of write_256b_full : signal is "true";
    --attribute mark_debug of din_wr_en : signal is "true";
    --attribute mark_debug of read_en : signal is "true";

begin

    din_256b(15 downto 0)    <= data_16b(0);
    din_256b(31 downto 16)   <= data_16b(1);
    din_256b(47 downto 32)   <= data_16b(2);
    din_256b(63 downto 48)   <= data_16b(3);
    din_256b(79 downto 64)   <= data_16b(4);
    din_256b(95 downto 80)   <= data_16b(5);
    din_256b(111 downto 96)  <= data_16b(6);
    din_256b(127 downto 112) <= data_16b(7);
    din_256b(143 downto 128) <= data_16b(8);
    din_256b(159 downto 144) <= data_16b(9);
    din_256b(175 downto 160) <= data_16b(10);
    din_256b(191 downto 176) <= data_16b(11);
    din_256b(207 downto 192) <= data_16b(12);
    din_256b(223 downto 208) <= data_16b(13);
    din_256b(239 downto 224) <= data_16b(14);
    din_256b(255 downto 240) <= data_16b(15);

    process(clk)
    begin
        if (rising_edge(clk)) then
            write_256b_data <= din_256b;
            write_256b_en   <= din_wr_en;
            din_full        <= write_256b_full;
            dma_block_size  <= unsigned(block_size);
        end if;
   end process;

    process(clk, rst)
        variable fifo_read  : std_logic_vector(avail'range);
        variable read_finished : std_logic_vector(avail'range);
        variable count      : unsigned(4 downto 0) := "00000";
        variable addr       : unsigned(4 downto 0) := "00000";
        variable offset_idx : unsigned(4 downto 0) := "00000";
        variable rot_count  : unsigned(4 downto 0) := "00000";
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                state <= IDLE;
                data_out <= (others => '0');
                din_wr_en <= '0';
                fifo_idx <= 0;
                fifo_cnt <= 0;
                fifo_read := (others => '0');
                fifo_pending <= (others => '0');
                base <= "00000";
                flip <= (others => '0');
                flipped <= '0';
                read_en <= (others => '0');
                
                for i in 0 to NUMBER_OF_FEEs-1 loop
                    read_cnt(i) <= (others => '0');
                end loop;
                
                read_finished := (others => '0');
                offset_idx := "00000";
                count := "00000";
                rot_count := (others => '0');
                for i in 0 to 15 loop
                    data_16b(i) <= (others => '0');
                end loop;
            else
            
                din_wr_en <= '0';

                case state is
                    when IDLE =>
                        fifo_idx <= 0;
                        read_en <= (others => '0');
                        read_finished := (others => '0');
                        fifo_read := (others => '0');
                        count := "00000"; 
                        offset_idx := "00000";
                        
                        -- Set our read length for each FIFO
                        for i in 0 to NUMBER_OF_FEEs-1 loop
                            read_cnt(i) <= dma_block_size; --to_unsigned(255, 10);
                        end loop;

                        -- Any FIFOs going to overflow, read them 1st
                        -- Otherwise do a normal read.
                        if (priority /= "00") then
                            fifo_pending <= priority;
                        else
                            fifo_pending <= avail;
                        end if;

                        if ((avail /= "00" or priority /= "00") and din_full = '0') then
                            state <= HEADER_0;
                        end if;

                    when HEADER_0 =>
                        -- Count how many FIFOs to read
                        -- Generate the offset address for each FIFO index
                        for i in 0 to fifo_pending'left loop 
                            count := count + ("0000" & fifo_pending(i));
                            if (fifo_pending(i) = '1') then
                                offsets(to_integer(offset_idx)) <= count - 1;
                                last_offset_idx <= offset_idx;
                                offset_idx := offset_idx + 1;
                            end if;
                        end loop;                   
                    
                        -- Packet marker 1                      
                        data_16b(to_integer(base)) <= x"feee";
                        if (base + 1 > 15) then
                            base <= "00000";
                            din_wr_en <= '1';
                        else
                            base <= base + 1;
                        end if;
                        state <= HEADER_1;

                    when HEADER_1 =>
                        -- Packet marker 2
                        data_16b(to_integer(base)) <= x"ba5e";
                        if (base + 1 > 15) then
                            base <= "00000";
                            din_wr_en <= '1';
                        else
                            base <= base + 1;
                        end if;
                        state <= HEADER_2;

                    when HEADER_2 =>
                        -- FIFO count
                        data_16b(to_integer(base)) <= (15 downto (read_cnt(0)'length + 4) => '0') & std_logic_vector(read_cnt(0)) & std_logic_vector(count(3 downto 0));
                        if (base + 1 > 15) then
                            base <= "00000";
                            din_wr_en <= '1';
                        else
                            base <= base + 1;
                        end if;
                        state <= HEADER_3;

                    when HEADER_3 =>
                        -- Which FIFOs we are reading
                        data_16b(to_integer(base)) <= (15 downto fifo_pending'length => '0') & fifo_pending; 
                        if (base + 1 > 15) then
                            base <= "00000";
                            din_wr_en <= '1';
                        else
                            base <= base + 1;
                        end if;

                        read_en <= fifo_pending;
                        state <= DATA;

                    when DATA =>
                        read_en   <= (others => '0');
                        flip      <= (others => '0');
                        fifo_read := (others => '0');

                        -- Generate read_enables which fit into our 256-bit word (16 slots)
                        -- Lookup the offset address (flat), if it fits, read it and mark it read;
                        offset_idx := "00000";
                        READ_FIFO_LOOP:
                        for idx in 0 to fifo_pending'left loop
                            if (fifo_read(idx) = '0' and fifo_pending(idx) = '1') then 
                                addr := base + offsets(to_integer(offset_idx));
                                offset_idx := offset_idx + 1;
                                if (addr <= 15 and read_finished(idx) = '0') then
                                    data_16b(to_integer(addr)) <= data_in(idx)(15 downto 0);
                                    read_en(idx)   <= '1';
                                    fifo_read(idx) := '1';
                                end if;

                            end if;
                        end loop READ_FIFO_LOOP;

                        -- Update our read counters for each FIFO
                        for i in 0 to fifo_pending'left loop
                            if (fifo_read(i) = '1') then
                                if (read_cnt(i) = "000") then
                                    read_finished(i) := '1';
                                    read_en(i) <= '0';
                                else
                                    read_cnt(i) <= read_cnt(i) - 1;
                                end if;
                            end if;
                        end loop;

                        -- Are we going to overflow the word?
                        if (base + count > 15) then
                            if (din_full = '0') then
                                -- write 256-bit word here
                                base <= "00000";
                                din_wr_en <= '1';
                                read_en <= fifo_pending;
                            else
                                read_en <= (others => '0');
                                state <= WAIT_UNTIL_WRITE_FIFO_EMPTY;
                            end if;
                        else
                            base <= base + count;

                            -- Catch if we need to rotate the offset lookup table
                            offset_idx := "00000";
                            for idx in 0 to fifo_pending'left loop
                                if (fifo_read(idx) = '1') then 
                                    addr := base + count + offsets(to_integer(offset_idx));
                                    offset_idx := offset_idx + 1;
                                    if (addr > 15) then
                                        read_en(idx)   <= '0';
                                        fifo_read(idx) := '0';
                                        flip(idx)      <= '1';
                                    end if;
                                end if;
                            end loop;

                            -- Count how many times we might need to shift/rotate the table.
                            rot_count := (others => '0');
                            for i in 0 to fifo_read'left loop 
                                rot_count := rot_count + ("0000" & fifo_read(i));
                            end loop;
                        end if;

                        -- Are we finshed?
                        if (read_finished = fifo_pending) then
                            -- We might need to adjust the base address before we return to IDLE
                            -- for the next DMA packet
                            if (rot_count /= count) then
                                if (base + rot_count > 15) then
                                    base <= "00000";
                                else
                                    base <= base + rot_count;
                                end if;
                            end if;
                            read_finished := (others => '0');
                            read_en <= (others => '0');
                            state <= IDLE;
                        end if;

                        -- Check if we need to adjust the offset lookup table
                        -- to keep the data in order.
                        if (flip /= "00") then
                            -- Shift all the offsets down one index
                            for i in 1 to avail'left loop 
                                offsets(i) <= offsets(i-1);
                            end loop;
                            -- Take last ENABLED FIFO and move that to the top
                            offsets(0) <= offsets(to_integer(last_offset_idx));

                            -- Any more rotates?
                            rot_count := rot_count - 1;
                            if (rot_count /= "00") then
                                read_en <= (others => '0');
                                state <= ROTATE_OFFSETS;
                            end if;
                        end if;

                    when ROTATE_OFFSETS =>
                            for i in 1 to avail'left loop 
                                offsets(i) <= offsets(i-1);
                            end loop;
                            offsets(0) <= offsets(to_integer(last_offset_idx));

                            rot_count := rot_count - 1;
                            if (rot_count = "00") then
                                read_en <= fifo_pending;
                                state <= DATA;
                            else
                                state <= ROTATE_OFFSETS;
                            end if;
                            
                    when WAIT_UNTIL_WRITE_FIFO_EMPTY =>
                        if (din_full = '0') then
                            -- write 256-bit word here
                            base <= "00000";
                            din_wr_en <= '1';
                            read_en <= fifo_pending;
                            state <= DATA;
                        else
                            state <= WAIT_UNTIL_WRITE_FIFO_EMPTY;
                        end if;
                        
                end case;
            end if;
        end if;
    end process;

end behv;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.phy_pkg.all;

entity data_counter is
    port (
        clk                : in  std_logic;
        rst                : in  std_logic;
        
        read_data_avail    : in std_logic;
        read_data_count    : in std_logic_vector(9 downto 0);
        
        avail_flg          : out std_logic;
        priority_flg       : out std_logic;
        read_threshold     : in std_logic_vector(9 downto 0)
        --priority_threshold : in std_logic_vector(9 downto 0)        
    );
end data_counter;

architecture behv of data_counter is

    signal read_thres, prio_thres : std_logic_vector(read_data_count'range);

   -- attribute mark_debug : string;
   -- attribute mark_debug of read_data_avail : signal is "true";
   -- attribute mark_debug of read_data_count : signal is "true";
   -- attribute mark_debug of avail_flg : signal is "true";

begin

    process(clk)
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                priority_flg <= '0';
                avail_flg <= '0';
                read_thres <= (others => '0');
                prio_thres <= (others => '0');
            else
                read_thres <= read_threshold;
                prio_thres <= read_threshold(8 downto 0) & "0";
                if (read_data_avail = '1' and read_data_count >= read_thres) then
                    avail_flg <= '1';
                else
                    avail_flg <= '0';
                end if;
    
                if (read_data_avail = '1' and read_data_count >= prio_thres) then
                    priority_flg <= '1';
                else
                    priority_flg <= '0';
                end if;
            end if;
        end if;
    end process;

end behv;


library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;
use work.phy_pkg.all;

entity data_builder is
    port (
        clk          : in std_logic;
        rst          : in std_logic;
        block_size   : in  std_logic_vector(9 downto 0);
        
        -- Streaming Inteface --
        stream_ctrl  : out stream_ctrl_t;
        stream       :  in stream_t;
    
        -- DMA Interface --
        dma_write_clk  : out std_logic;
        dma_write_en   : out std_logic;
        dma_write_data : out std_logic_vector(255 downto 0);
        dma_write_full : in std_logic
    );
end data_builder;

architecture behv of data_builder is

    signal stream_data : data_16b_arr(NUMBER_OF_FEEs-1 downto 0);
    signal stream_read_en, stream_priority_flg, stream_avail_flg : std_logic_vector(NUMBER_OF_FEEs-1 downto 0);

    --signal read_threshold     : std_logic_vector(9 downto 0) := std_logic_vector(to_unsigned(255, 10));
    signal priority_threshold : std_logic_vector(9 downto 0) := std_logic_vector(to_unsigned(510, 10));

begin

    dma_write_clk <= clk;

    data_packer_i : entity work.data_packer
    port map (
        clk        => clk,
        rst        => rst,
        block_size => block_size,
        
        data_in    => stream_data,
        read_en    => stream_read_en,
        priority   => stream_priority_flg,
        avail      => stream_avail_flg,

        write_256b_data => dma_write_data,
        write_256b_en   => dma_write_en,
        write_256b_full => dma_write_full
    );
    
    FEE_INTERFACES_LOOP : for i in 0 to NUMBER_OF_FEEs-1 generate
        
        stream_data(i)         <= stream(i).data;
        stream_ctrl(i).read_en <= stream_read_en(i);
        stream_ctrl(i).clk     <= clk;
    
        data_counter_i : entity work.data_counter
        port map (
            clk                => clk,
            rst                => rst,
            
            read_data_avail    => stream(i).avail,
            read_data_count    => stream(i).count,
            
            avail_flg          => stream_avail_flg(i),
            priority_flg       => open, --stream_priority_flg(i),
            read_threshold     => block_size
            --priority_threshold => priority_threshold
        );
    end generate FEE_INTERFACES_LOOP;

end behv;
