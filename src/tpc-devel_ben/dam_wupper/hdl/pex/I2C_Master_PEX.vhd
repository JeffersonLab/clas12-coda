-- based on a version from opencore

LIBRARY ieee;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;

ENTITY i2c_master_pex IS
PORT(
    data_clk            : IN std_logic;
    no_data             : in std_logic;
    wr_mod              : in std_logic; 
    reset_n             : in std_logic;
    special             : IN STD_LOGIC;                   
    ena                 : IN STD_LOGIC;                   
    wr_data_update      : out std_logic;
    rd_data_refresh     : out std_logic;
    rd_number_in        : in std_logic_vector(3 downto 0);
    wr_number_in        : in std_logic_vector(3 downto 0);
    state_display       : out STD_LOGIC_VECTOR(4 DOWNTO 0);
    addr                : IN STD_LOGIC_VECTOR(6 DOWNTO 0); 
    --  rw              : IN STD_LOGIC;                    
    data_wr             : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
    addr_wr             : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
    data_rd             : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    i2c_process_finished: out std_logic;
    ack_error_o         : OUT STD_LOGIC;                    
    sda                 : INOUT  STD_LOGIC;                    
    scl                 : out STD_LOGIC;
    sda_o_debug         : out STD_LOGIC;
    sda_i_debug         : OUT  STD_LOGIC
    );                 
END i2c_master_pex;

ARCHITECTURE logic OF i2c_master_pex IS

  TYPE machine          IS(ready, start, command, wr_ack, stop1, stop2, restart2, restart3,
                           restartp1, restartp2, restart1, restart, rd_ack_final, rd_ack, cmd_ack,
                           wr_addr_state, rd_state, wr_state_ack, data_write, data_write_buf, stop);
  
  SIGNAL  state         :  machine;                          

  SIGNAL  adr_rw        :  STD_LOGIC_VECTOR(7 DOWNTO 0);     
  SIGNAL  data_tx       :  STD_LOGIC_VECTOR(31 DOWNTO 0);     
  SIGNAL  data_rx       :  STD_LOGIC_VECTOR(7 DOWNTO 0);    
  SIGNAL  bit_cnt       :  INTEGER RANGE 0 TO 7 := 7;        

  signal sda_o          : STD_LOGIC:='0';
  signal ena_i          : STD_LOGIC:='0';
  signal ena_r          : STD_LOGIC:='0';
  signal ack_error      : std_logic:='0';
  signal scl_o          : std_logic:='0';
  signal sda_i          : std_logic:='0';
  signal start_cnt      : std_logic:='0';
  signal stop_cnt       : std_logic:='0';
  signal sda_sel        : std_logic:='0';
  signal busy           : std_logic:='0';
  signal command_cnt    : std_logic_vector(1 downto 0):="00";
  signal ack_cnt        : std_logic_vector(1 downto 0):="00";
  signal rd_cnt         : std_logic_vector(1 downto 0):="00";
  signal wr_cnt         : std_logic_vector(1 downto 0):="00";
  signal rd_number      : std_logic_vector(3 downto 0):="0000";
  signal wr_number      : std_logic_vector(3 downto 0):="0000";
  signal iread          : integer :=0;
  signal iwrite         : integer :=0;
  signal addr_cnt       : integer :=0;

BEGIN

  -- command_cnt_o      <= command_cnt;
  ack_error_o           <= ack_error;

  process(data_clk)
  begin
    if data_clk'event and data_clk='1' then
      ena_r <= ena;
      ena_i <= ena and (not ena_r);
    end if;
  end process;

  process(data_clk, reset_n)
  begin
    if reset_n = '0' then
      sda_o     <= '1';
      sda_sel   <= '1'; --out
      scl_o     <= '1';
      bit_cnt   <= 7;
      ack_cnt   <= "00";
      state     <= ready;
      ack_error <= '0';
      iread     <= 0;
      iwrite    <= 0;
      busy      <= '0';
      state_display             <= "00000";
      wr_number                 <= "0000";
      rd_number                 <="0000";
      i2c_process_finished      <= '0';
    elsif data_clk'event and data_clk='1' then
      case state is
        when ready =>
          iwrite        <= 0;
          iread         <= 0;
          if ena_i = '1' then
            i2c_process_finished        <= '0';
            state                       <= start;
            state_display               <= "00001";
            busy                        <= '1';
            adr_rw                      <= addr & '0';--(not wr_mod);
            --if special='1' then
            --  adr_rw <= addr & '1';
            --else
            --  adr_rw <= addr & '0';
            --end if;
            data_tx                     <= data_wr;
            scl_o                       <= '1';
            sda_o                       <= '1';
            sda_sel                     <= '1';
            start_cnt                   <='0';
          else
            state                       <= ready;
            state_display               <= "00000";
            i2c_process_finished        <= '0';
            start_cnt                   <='0';
            sda_o                       <= '1';
            sda_sel                     <= '1'; --out
            scl_o                       <= 'Z';
            bit_cnt                     <= 7;
          end if;
        when start =>
          ack_error                     <= '0';
          command_cnt                   <= "00";
          if start_cnt = '0' then
            start_cnt                   <= '1';
            sda_o                       <= '0';
            scl_o                       <= '1';
            sda_sel                     <= '1';
            state                       <= start;
            bit_cnt                     <= 7;
            state_display               <= "00001";
          else
            start_cnt                   <= '0';
            sda_o                       <= '0';
            scl_o                       <= '0';
            sda_sel                     <= '1';
            state                       <= command;
            bit_cnt                     <= 7;
            state_display               <= "00010";
          end if;
        when command =>
          state_display                 <= "00010";
          sda_o                         <= adr_rw(bit_cnt);
          case command_cnt is
            when "00" =>
              command_cnt               <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              state                     <= command;
            when "01" =>
              command_cnt               <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              state                     <= command;
            when "10" =>
              command_cnt               <= "11";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              state                     <= command;
            when others =>
              command_cnt               <= "00";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              if bit_cnt = 0 then
                bit_cnt                 <= 7;
                state                   <= cmd_ack;
                ack_cnt                 <= "00";
              else
                bit_cnt                 <= bit_cnt-1;
                state                   <= command;
              end if;
          end case;
        when cmd_ack =>
          state_display                 <= "00011";
          case ack_cnt is
            when "00" =>
              ack_cnt                   <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= cmd_ack;
            when "01" =>
              ack_cnt                   <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= cmd_ack;
            when "10" =>
              scl_o                     <= '1';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= cmd_ack;
              ack_cnt                   <= "11";
              ack_error                 <= sda_i or ack_error;
            when others =>
              scl_o                     <= '0';
              ack_error                 <= sda_i or ack_error;
              ack_cnt                   <= "00";
              if special ='1' then
                state                   <= wr_addr_state;
                addr_cnt                <= 0;
                wr_cnt                  <= "00";
                rd_number               <= "0000";
                sda_sel                 <= '1';
                sda_o                   <= 'Z';
                bit_cnt                 <= 7;
              elsif adr_rw(0) = '0' then  -- 0: write
                state                   <= wr_addr_state;
                addr_cnt                <= 0;
                wr_cnt                  <= "00";
                rd_number               <= "0000";
                sda_sel                 <= '1';
                sda_o                   <= 'Z';
                bit_cnt                 <= 7;

              else
                state                   <= rd_state;
                rd_number               <= rd_number_in;
                rd_cnt                  <= "00";
                sda_sel                 <= '1';
                sda_o                   <= 'Z';
                bit_cnt                 <= 7;
              end if;
          end case;
        when wr_addr_state =>
          state_display                 <= "00100";
          sda_o                         <= addr_wr(addr_cnt*8+bit_cnt);
          case wr_cnt is
            when "00" =>
              wr_cnt                    <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              state                     <= wr_addr_state;
            when "01" =>
              wr_cnt                    <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              state                     <= wr_addr_state;
            when "10" =>
              wr_cnt                    <= "11";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              state                     <= wr_addr_state;
            when others =>
              wr_cnt                    <= "00";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              if bit_cnt = 0 then
                bit_cnt                 <= 7;
                state                   <= wr_state_ack;
                ack_cnt                 <= "00";
              else
                bit_cnt                 <= bit_cnt-1;
                state                   <= wr_addr_state;
              end if;
          end case;
        when wr_state_ack =>
          state_display                 <= "00101";
          case ack_cnt is
            when "00" =>
              ack_cnt                   <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= wr_state_ack;
            when "01" =>
              ack_cnt                   <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= wr_state_ack;
            when "10" =>
              scl_o                     <= '1';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= wr_state_ack;
              ack_cnt                   <= "11";
              ack_error                 <= sda_i or ack_error;
            when others =>
              --	ack_error <= sda_i or ack_error;
              scl_o                     <= '0';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              ack_cnt                   <= "00";
              if addr_cnt =3 then
                if wr_mod = '0' then
                  state                 <= restartP2;
                elsif no_data = '1' then	 

--            elsif  wr_number_in = "0000" then
                  state                 <= stop;
                  WR_number             <= "0000";
                else
                  state                 <= data_write_buf;
                  wr_data_update        <= '1';
                  wr_cnt                <= "00";
                  iwrite                <= 0;
                  wr_number             <= wr_number_in;
                end if;
              else
                addr_cnt                <= addr_cnt+1;
                state                   <= wr_addr_state;
              end if;
          end case;

        when restartp2 =>
          state_display                 <= "11000";
          scl_o                         <= '0';
          sda_sel                       <= '1';
          sda_o                         <= '1';
          -- stop_cnt                   <= '1';
          state                         <= restartp1;

        when restartp1 =>
          state_display                 <= "11000";
          scl_o                         <= '0';
          sda_sel                       <= '1';
          sda_o                         <= '1';
          -- stop_cnt                   <= '1';
          state                         <= restart;

        when restart =>
          state_display                 <= "00110";
          scl_o                         <= '1';
          sda_sel                       <= '1';
          sda_o                         <= '1';
          -- stop_cnt                   <= '1';
          state                         <= restart1;

        when restart1 =>
          state_display                 <= "00111";
          scl_o                         <= '1';
          sda_sel                       <= '1';
          sda_o                         <= '1';
          -- stop_cnt                   <= '1';
          state                         <= restart2;

        when restart2 =>
          state_display                 <= "01000";
          scl_o                         <= '1';
          sda_sel                       <= '1';
          sda_o                         <= '0';
          -- stop_cnt                   <= '1';
          state                         <= restart3;

        when restart3 =>
          state_display                 <= "01001";
          adr_rw                        <= addr & '1';
          scl_o                         <= '0';
          sda_sel                       <= '1';
          sda_o                         <= '0';
          -- stop_cnt                   <= '1';
          state                         <= command;
          iread                         <= 0;

        when data_write_buf =>
          state                         <= data_write;
          wr_data_update                <= '0';
          bit_cnt                       <= 7;
          state_display                 <= "01010";

        when data_write =>
          state_display                 <= "01011";
          sda_o                         <= data_tx(iwrite*8+bit_cnt);
          case wr_cnt is
            when "00" =>
              wr_cnt                    <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              state                     <= data_write;
            when "01" =>
              wr_cnt                    <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              state                     <= data_write;
            when "10" =>
              wr_cnt                    <= "11";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              state                     <= data_write;
            when others =>
              wr_cnt                    <= "00";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              if bit_cnt = 0 then
                bit_cnt                 <= 7;
                state                   <= wr_ack;
                ack_cnt                 <= "00";
              else
                bit_cnt                 <= bit_cnt-1;
                state                   <= data_write;
              end if;
          end case;

        when rd_state =>
          state_display                 <= "01100";     
          case rd_cnt is
            when "00" =>
              rd_cnt                    <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '0';
              state                     <= rd_state;
              sda_o                     <= 'Z';
            when "01" =>
              rd_cnt                    <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '0';
              state                     <= rd_state;
              sda_o                     <= 'Z';
            when "10" =>
              rd_cnt                    <= "11";
              scl_o                     <= '1';
              sda_sel                   <= '0';
              data_rd(iread*8+bit_cnt)  <= sda_i;
              sda_o                     <= 'Z';
              state                     <= rd_state;
            when others =>
              rd_cnt                    <= "00";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              if bit_cnt = 0 then
                bit_cnt                 <= 7;
                sda_o                   <= '0';
                if rd_number = "0000" then
                  state                 <= rd_ack_final;
                else
                  state                 <= rd_ack;  
                  iread                 <= iread +1;
                end if;
                rd_data_refresh         <= '1';
                ack_cnt                 <= "00";
              else
                bit_cnt                 <= bit_cnt-1;
                state                   <= rd_state;
                sda_o                   <= 'Z';
              end if;
          end case;
        when rd_ack_final =>  -- jump to stop
          state_display                 <= "01101";
          rd_data_refresh               <= '0';
          case ack_cnt is
            when "00" =>
              ack_cnt                   <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              state                     <= rd_ack_final;
            when "01" =>
              ack_cnt                   <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              state                     <= rd_ack_final;
            when "10" =>
              scl_o                     <= '1';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              state                     <= rd_ack_final;
              ack_cnt                   <= "11";
              -- ack_error              <= sda_i or ack_error;
            when others =>
              scl_o                     <= '0';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              ack_cnt                   <= "00";
              stop_cnt                  <= '0';
              --    if rd_number = "0000" then
              --       rd_number <= "0000";
              state                     <= stop;
              --     else
              --    state <= stop;
              --    end if;
          end case;
        when rd_ack =>  -- jump to stop
          state_display                 <= "01110";
          rd_data_refresh               <= '0';
          case ack_cnt is
            when "00" =>
              ack_cnt                   <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              state                     <= rd_ack;
            when "01" =>
              ack_cnt                   <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              state                     <= rd_ack;
            when "10" =>
              scl_o                     <= '1';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              state                     <= rd_ack;
              ack_cnt                   <= "11";
              -- ack_error              <= sda_i or ack_error;
            when others =>
              scl_o                     <= '0';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              ack_cnt                   <= "00";
              -- stop_cnt               <= '0';

              if rd_number = "0000" then
                --  rd_number           <= "0000";
                state                   <= stop;
              else
                state                   <= rd_state;
                rd_number               <= rd_number -'1';
              end if;
          end case;

        when wr_ack =>  -- jump to stop
          state_display                 <= "01111";
          rd_data_refresh               <= '0';
          case ack_cnt is
            when "00" =>
              ack_cnt                   <= "01";
              scl_o                     <= '0';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= wr_ack;
            when "01" =>
              ack_cnt                   <= "10";
              scl_o                     <= '1';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= wr_ack;
            when "10" =>
              scl_o                     <= '1';
              sda_sel                   <= '0';
              sda_o                     <= 'Z';
              state                     <= wr_ack;
              ack_cnt                   <= "11";
              --ack_error               <= sda_i or ack_error;
            when others =>
              ack_error                 <= sda_i or ack_error;
              scl_o                     <= '0';
              sda_sel                   <= '1';
              sda_o                     <= '0';
              ack_cnt                   <= "00";
              stop_cnt                  <= '0';
              if wr_number = "0000" then
                state                   <= stop;
                WR_number               <= "0000";
              else
                wr_number               <= wr_number - '1';
                state                   <= data_write_buf;
                iwrite                  <= iwrite+1;
                wr_data_update          <= '1';
              end if;
          end case;

        when stop =>
          state_display                 <= "10000";
          i2c_process_finished          <= '1';
          if stop_cnt = '0' then
            scl_o                       <= '0';
            sda_sel                     <= '1';
            sda_o                       <= '0';
            stop_cnt                    <= '1';
            state                       <= stop;
          else
            scl_o                       <= '1';
            sda_sel                     <= '1';
            sda_o                       <= '0';
            stop_cnt                    <= '0';
            state                       <= stop1;
            busy                        <= '0';
          end if;

        when stop1 =>
          state_display                 <= "10001";
          i2c_process_finished          <= '1';
          scl_o                         <= '1';
          sda_sel                       <= '1';
          sda_o                         <= '1';
          state                         <= stop2;

        when stop2 =>
          state_display                 <= "10011";
          i2c_process_finished          <= '1';
          scl_o                         <= '1';
          sda_sel                       <= '1';
          sda_o                         <= '1';
          state                         <= ready;

        when others =>
          state_display                 <= "10010";
          i2c_process_finished          <= '1';
          state                         <= ready;
          busy                          <='0';
      end case;	         
    end if;
  end process;

  scl           <= scl_o;
  sda           <= sda_o when sda_sel = '1' else
                   'Z';
  sda_i         <= sda when sda_sel = '0' else
                   sda_o;

  sda_o_debug   <= sda_o;
  sda_i_debug   <= sda_i;

END logic;
