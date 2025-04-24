library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_unsigned.all;
use ieee.numeric_Std.all;
use work.sampa_pkg.all;
use work.phy_pkg.all;

library UNISIM;
use UNISIM.VCOMPONENTS.ALL;

entity sphx_fee_top is
    port ( 
        -- System Clock --
        sys_clk_n     : in   std_logic;
        sys_clk_p     : in   std_logic;
        
        -- GTX Reference Clock (9.382 MHz) --
        mgt0_ref_clk_n : in std_logic;
        mgt0_ref_clk_p : in std_logic;
        
--        mgt1_ref_clk_n : in std_logic;
--        mgt1_ref_clk_p : in std_logic;
        
        mgt_recovered_clk_n : in std_logic;
        mgt_recovered_clk_p : in std_logic;

        -- SFP --
        gth_tx_n : out std_logic;
        gth_tx_p : out std_logic;
        gth_rx_n : in std_logic;
        gth_rx_p : in std_logic;
        
        -- SAMPA Reset --
        sampa_hard_rst_n : out std_logic;
        sampa_hard_rst_p : out std_logic;
        
        -- SAMPA I2C --
        sampa_i2c_scl : inout std_logic;
        sampa_i2c_sda : inout std_logic;
        
        -- SAMPA Serial Link Clock (<= 320 MHz) --
        sampa_sdo_clk_n : out std_logic_vector(7 downto 0);
        sampa_sdo_clk_p : out std_logic_vector(7 downto 0);
        
        -- SAMPA Hi-Speed Serial Data --
        sampa0_sdo_n : in std_logic_vector(10 downto 0);
        sampa0_sdo_p : in std_logic_vector(10 downto 0);
        sampa1_sdo_n : in std_logic_vector(10 downto 0);
        sampa1_sdo_p : in std_logic_vector(10 downto 0);
        sampa2_sdo_n : in std_logic_vector(10 downto 0);
        sampa2_sdo_p : in std_logic_vector(10 downto 0);
        sampa3_sdo_n : in std_logic_vector(10 downto 0);
        sampa3_sdo_p : in std_logic_vector(10 downto 0);
        sampa4_sdo_n : in std_logic_vector(10 downto 0);
        sampa4_sdo_p : in std_logic_vector(10 downto 0);
        sampa5_sdo_n : in std_logic_vector(10 downto 0);
        sampa5_sdo_p : in std_logic_vector(10 downto 0);
        sampa6_sdo_n : in std_logic_vector(10 downto 0);
        sampa6_sdo_p : in std_logic_vector(10 downto 0);
        sampa7_sdo_n : in std_logic_vector(10 downto 0);
        sampa7_sdo_p : in std_logic_vector(10 downto 0);
        
        -- SAMPA ADC Clock (<= 20 MHz) --
        sampa_adc_clk_n : out std_logic;
        sampa_adc_clk_p : out std_logic;
        
        -- SAMPA Bunch Crossing Clock (<= 40 MHz) --
        sampa_bx_clk_n : out std_logic;
        sampa_bx_clk_p : out std_logic;
        
        -- SAMPA Bunch Crossing Sync --
        sampa_bx_sync_trig_n : out std_logic;
        sampa_bx_sync_trig_p : out std_logic;
        
        -- SAMPA Event Trigger --
        sampa_trig_n : out std_logic;
        sampa_trig_p : out std_logic;
        
        -- SAMPA Heartbeat Trigger --
        sampa_hbeat_trig_n : out std_logic;
        sampa_hbeat_trig_p : out std_logic;
        
        -- SAMPA Config --
        sampa_pwr_enable_ana : out std_logic;
        sampa_pwr_enable_dig : out std_logic;
        sampa_cg             : out std_logic_vector(1 downto 0);
        sampa_cts            : out std_logic;
        sampa_pol            : out std_logic;
        sampa_clk_config     : out std_logic_vector(6 downto 0);
        sampa_por            : out std_logic;
        sampa_por_valid      : in  std_logic;

        -- SFP I2C --
        sfp_i2c_scl : inout std_logic;
        sfp_i2c_sda : inout std_logic;
        
        -- External PLL I2C --
        si5338_i2c_scl : inout std_logic;
        si5338_i2c_sda : inout std_logic;
        
        -- PCB Temperature I2C --
        temperature_i2c_scl : inout std_logic;
        temperature_i2c_sda : inout std_logic;
        
        -- Voltage/Current Monitor I2C --
        vimon_i2c_scl : inout std_logic;
        vimon_i2c_sda : inout std_logic;
        
        -- TLK2501 Interface --
        tlk_tx : out std_logic_vector(1 downto 0);
        tlk_rx : in  std_logic;
        
        -- GPIO --
        led      : out std_logic_vector(3 downto 0);
        lemo_out : out std_logic_vector(1 downto 0)
	);
end sphx_fee_top;

architecture behv of sphx_fee_top is

    signal clk_40MHz : std_logic;
    signal clk_200MHz : std_logic;
    signal reset_40MHz : std_logic;
    signal user_reset  : std_logic;
    signal pll_locked  : std_logic;
    
    signal sampa_clk_320MHz : std_logic;
    signal sampa_clk_160MHz : std_logic;
    signal sampa_clk_40MHz  : std_logic;
    signal sampa_clk_20MHz  : std_logic;
    
    signal sampa_sdo_clk : std_logic_vector(sampa_sdo_clk_n'range);
    signal sampa_adc_clk : std_logic; 
    signal sampa_bx_clk :  std_logic; 

    signal sampa0_sdo : std_logic_vector(sampa0_sdo_n'range);
    signal sampa1_sdo : std_logic_vector(sampa1_sdo_n'range);
    signal sampa2_sdo : std_logic_vector(sampa2_sdo_n'range);
    signal sampa3_sdo : std_logic_vector(sampa3_sdo_n'range);
    signal sampa4_sdo : std_logic_vector(sampa4_sdo_n'range);
    signal sampa5_sdo : std_logic_vector(sampa5_sdo_n'range);
    signal sampa6_sdo : std_logic_vector(sampa6_sdo_n'range);
    signal sampa7_sdo : std_logic_vector(sampa7_sdo_n'range);
    signal delay_out : std_logic_vector(31 downto 0);

    signal sampa_bx_sync_trig : std_logic;
    signal sampa_trig         : std_logic;
    signal sampa_hbeat_trig   : std_logic;
    signal sampa_hard_rst     : std_logic;
    signal sampa_reg_pwr_en   : std_logic;
    signal sampa_clk_en       : std_logic;
    signal sampa_por_valid_i  : std_logic;
    signal sampa_init         : std_logic;
    signal sampa_reset        : std_logic;
    signal sampa_ext_reset    : std_logic;
    signal sampa_ext_reset_i  : std_logic;
    signal idelay_ready : std_logic;

    signal idelay_sel : std_logic_vector(31 downto 0) := (others => '0');
    
    signal reg_addr    : std_logic_vector(15 downto 0);
    signal reg_write, reg_read   :  std_logic;
        
    signal reg_rd_dval :  std_logic;
    signal reg_ack, reg_fail     :  std_logic;
    signal reg_rd_data :  std_logic_vector(15 downto 0);

    signal reg_data_read, reg_data_write : std_logic_vector(31 downto 0);

    signal telem_trigs : std_logic_vector(15 downto 0);

    signal temp     : std_logic_vector(11 downto 0);
    signal vccint   : std_logic_vector(11 downto 0);
    signal vccaux   : std_logic_vector(11 downto 0);
    signal vccbram  : std_logic_vector(11 downto 0);

    signal iv_mon_en, iv_mon_busy, iv_mon_valid       : std_logic;
    signal v_P4V, v_P2V_fpga, v_P2V_sampa_dig, v_P2V_sampa_ana : std_logic_vector(15 downto 0);
    signal i_P4V, i_P2V_fpga, i_P2V_sampa_dig, i_P2V_sampa_ana : std_logic_vector(15 downto 0);
     
    signal temp_mon_en, temp_mon_busy, temp_mon_valid : std_logic;
    signal temp_mon_0, temp_mon_1, temp_mon_2, temp_mon_3: std_logic_vector(15 downto 0);

    signal sampa_i2c_addr : std_logic_vector(3 downto 0);
    signal sampa_i2c_reg_addr : std_logic_vector(5 downto 0);
    signal sampa_i2c_reg_write_data : std_logic_vector(7 downto 0);
    signal sampa_i2c_reg_read_data : std_logic_vector(7 downto 0);
    signal sampa_i2c_opcode : std_logic_vector(1 downto 0);
    signal sampa_i2c_en : std_logic;
    signal sampa_i2c_busy : std_logic;
    signal sampa_i2c_error : std_logic;
    
    signal sampa_trig_en  : std_logic;
    
    
    signal sampa_data_wren, sampa_sob, sampa_eob : std_logic;
    signal sampa_data_stream : std_logic_vector(15 downto 0);
    
    signal test_pattern_trig : std_logic;
    signal test_pattern_length : std_logic_vector(15 downto 0);
    
    signal gth_tx_clk, gth_rx_clk, data_clk : std_logic;
    signal phy_reset : std_logic;
    
    signal sampa_data          : sampa_array;
    signal sampa_rd_en         : std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);
    signal sampa_rd_available  : std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);
    signal sampa_elink_enable  : std_logic_vector(NUMBER_OF_ELINKS-1 downto 0);
    
    signal sampa_shape_gain : std_logic_vector(2 downto 0);
    
    signal gtm_recv : gtm_recv_rec;
    
    signal sampa_trig_i, sampa_trig_gtm : std_logic;
    
begin

    sampa_hard_rst       <= sampa_ext_reset or sampa_ext_reset_i; 
    sampa_por            <= sampa_ext_reset or sampa_ext_reset_i;
    
    sampa_por_valid_i <= sampa_por_valid;
    lemo_out(0) <= not sampa_trig;
    lemo_out(1) <= '1';
    
    sampa_cg <= sampa_shape_gain(1 downto 0);
    sampa_cts <= sampa_shape_gain(2);
    sampa_pol <= '1';
    sampa_clk_config <= "0100110"; --  0100110 (20 MHz ADC), 0110110 (10 MHz ADC)

    sampa_i2c_master_0 : entity work.sampa_i2c_master
    generic map (
        INPUT_CLK_FREQ => 40_000_000, 
        BUS_CLK_FREQ   => 100_000
    )
    port map (
        clk       => clk_40MHz,
        reset     => reset_40MHz,
    
        slave_addr     => sampa_i2c_addr,
        reg_addr       => sampa_i2c_reg_addr,
        reg_write_data => sampa_i2c_reg_write_data,
        reg_read_data  => sampa_i2c_reg_read_data,
        
        opcode => sampa_i2c_opcode,
        en     => sampa_i2c_en,
    
        busy      => sampa_i2c_busy,
        ack_error => sampa_i2c_error,
    
        sda       => sampa_i2c_sda,
        scl       => sampa_i2c_scl
    );
        
    iv_mon_i : entity work.ltc2991_ctrl
    port map (
        clk_50MHz  => clk_40MHz,
        rst        => reset_40MHz,

        en   => telem_trigs(1),
        busy => iv_mon_busy,
        
        valid     => iv_mon_valid,
        voltage_a => v_P4V,
        voltage_b => v_P2V_fpga,
        voltage_c => v_P2V_sampa_dig,
        voltage_d => v_P2V_sampa_ana,
        
        current_a => i_P4V,
        current_b => i_P2V_fpga,
        current_c => i_P2V_sampa_dig,
        current_d => i_P2V_sampa_ana,

        i2c_sda => vimon_i2c_sda,
        i2c_scl => vimon_i2c_scl 
    );
    
    temp_mon_i : entity work.ds620_ctrl
    port map (
        clk_50MHz  => clk_40MHz,
        rst        => reset_40MHz,

        en   => telem_trigs(0),
        busy => temp_mon_busy,
        
        valid => temp_mon_valid,
        temp_0 => temp_mon_0,
        temp_1 => temp_mon_1,
        temp_2 => temp_mon_2,
        temp_3 => temp_mon_3,

        i2c_sda => temperature_i2c_sda,
        i2c_scl => temperature_i2c_scl 
    );

    -- XADC --
    xadc_0 : entity work.xadc_monitor 
    port map (
        clk => clk_40MHz,
        reset => reset_40MHz, 
        temp  => temp,
        vccint   => vccint,
        vccaux   => vccaux,
        vccbram  => vccbram
    );

    -- Register Controller --
    register_io_0 : entity work.register_io
    port map (
        clk   => clk_40MHz,
        reset => reset_40MHz,
        
        reg_addr    => reg_addr,
        reg_read    => reg_read,
        reg_data    => reg_data_write(15 downto 0),
        reg_write   => reg_write,
        reg_rd_dval => reg_rd_dval,
        reg_ack     => reg_ack,
        reg_rd_data => reg_data_read(15 downto 0),

        temp     => temp,
        vccint   => vccint,
        vccaux   => vccaux,
        vccbram  => vccbram,
        
        sampa_i2c_addr           => sampa_i2c_addr,
        sampa_i2c_reg_addr       => sampa_i2c_reg_addr,
        sampa_i2c_reg_write_data => sampa_i2c_reg_write_data,
        sampa_i2c_reg_read_data  => sampa_i2c_reg_read_data,
        sampa_i2c_opcode => sampa_i2c_opcode,
        sampa_i2c_en     => sampa_i2c_en,
        sampa_i2c_busy   => sampa_i2c_busy,
        sampa_i2c_error  => sampa_i2c_error,
        
        sampa_trigger => sampa_trig_en,
        
        telem_trigs     => telem_trigs,
        v_P4V           => v_P4V,
        v_P2V_fpga      => v_P2V_fpga,
        v_P2V_sampa_dig => v_P2V_sampa_dig,
        v_P2V_sampa_ana => v_P2V_sampa_ana,
        i_P4V           => i_P4V,
        i_P2V_fpga      => i_P2V_fpga,
        i_P2V_sampa_dig => i_P2V_sampa_dig,
        i_P2V_sampa_ana => i_P2V_sampa_ana,
        pcb_temp_0      => temp_mon_0,
        pcb_temp_1      => temp_mon_1,
        pcb_temp_2      => temp_mon_2,
        pcb_temp_3      => temp_mon_3,
        
        test_pattern_trig => test_pattern_trig,
        test_pattern_length => test_pattern_length, 
        
        sampa_shape_gain => sampa_shape_gain, 
        sampa_elink_enable =>  sampa_elink_enable
    );
    
    -- PHY --
    phy_io_0 : entity work.phy_io
    port map ( 
        -- Reset Logic --
        reset_clk   => clk_40MHz,
        reset_in => not pll_locked,
        reset_out => phy_reset,
        
        -- MGT Reference Clock --
        mgt_ref_clk_p => mgt0_ref_clk_p,
        mgt_ref_clk_n => mgt0_ref_clk_n,
        
        -- SFP TX --
        gth_tx_n => gth_tx_n,
        gth_tx_p => gth_tx_p,
        
        -- SFP RX --
        gth_rx_n => gth_rx_n,
        gth_rx_p => gth_rx_p,
        
        -- Register Interface --
        reg_clk        => clk_40MHz,
        reg_rst        => reset_40MHz,
        reg_addr       => reg_addr,
        reg_read       => reg_read,
        reg_write      => reg_write,
        reg_data_write => reg_data_write,
        reg_ack        => reg_ack,
        reg_fail       => reg_fail,
        reg_data_read  => reg_data_read,
        
        -- Streaming Data Interface --
        data_clk   => data_clk,
        data_wr_en => sampa_data_wren,
        data_sob   => sampa_sob,
        data_eob   => sampa_eob,
        data_in    => sampa_data_stream,
        
        trigger_clk => '0',        
        trigger_ack => '0',
        trigger_en  => '0',
        trigger     => open,
        trigger_freq => open,
        
        gtm_recv => gtm_recv,
        
        broadcast_recv => open,
        
        tx_clk_out => gth_tx_clk,
        rx_clk_out => gth_rx_clk,
        
        tx_activity => led(3),
        rx_activity => led(2),
        
        -- Status --
        status(0) => open,
        status(1) => open
    );
    
    tx_led_act : entity work.led_activity
    port map ( 
        clk     => gth_rx_clk,
        reset   => phy_reset,
    	act_in  => gtm_recv.lvl1_accept(0),
        led_out => led(1)
    );
--    led_i : entity work.led_heartbeat
--    port map ( 
--        clk     => clk_40MHz,
--        reset   => reset_40MHz,
--        led_out => led
--    );
    
    reset_manager_i : entity work.reset_manager
    port map (
        clk        => clk_40MHz,
        user_reset => user_reset,
        pll_locked => pll_locked and idelay_ready,
        
        reset_out  => reset_40MHz,
        sampa_init => sampa_init
    );
    
    sampa_rst_man_i : entity work.sampa_reset_manager 
    port map (
        clk         => clk_40MHz,
        reset       => reset_40MHz,
        sampa_init  => sampa_init,
        
        sampa_por    => sampa_por_valid_i,

        sampa_pwr_dig_en => sampa_pwr_enable_dig,
        sampa_pwr_ana_en => sampa_pwr_enable_ana,
        sampa_ext_reset => sampa_ext_reset,
        sampa_reset  => sampa_reset,
        sampa_clk_en => sampa_clk_en
     );
    
    vio_i : entity work.vio_0
    port map (
        clk          => clk_40MHz,
        probe_in0(0) => sampa_por_valid_i,
        probe_in1(0) => '0',
        
        probe_out0(0) => user_reset, 
        probe_out1(0) => sampa_ext_reset_i, 
        probe_out2(0) => open,
        probe_out3 => idelay_sel
    );

    
    -- System Clock PLL --
    sys_clk_0 : entity work.sys_pll
    port map (
        clk_in1_p  => sys_clk_p,
        clk_in1_n  => sys_clk_n,
        clk_320    => sampa_clk_320MHz,
        clk_160    => sampa_clk_160MHz,
        clk_40     => sampa_clk_40MHz,
        clk_20     => sampa_clk_20MHz,
        
        clk_40i    => clk_40MHz,
        clk_200    => clk_200MHz,
        
        clk_320_ce => sampa_clk_en,
        clk_160_ce => sampa_clk_en,
        clk_40_ce  => sampa_clk_en,
        clk_20_ce  => sampa_clk_en,
        reset      => '0',
        locked     => pll_locked
     );
     
     -- SAMPA Data Stream Multiplexer
    sampa_stream_i : entity work.sampa_stream 
    generic map (
        NUMBER_OF_ELINKS => NUMBER_OF_ELINKS
    )
    port map (
        clk  => gth_tx_clk,
        rst  => phy_reset,
        
        sampa_data          => sampa_data,
        sampa_rd_en         => sampa_rd_en,
        sampa_rd_available  => sampa_rd_available,
        
        data_clk   => data_clk,
        data_wr_en => sampa_data_wren,
        data_sob   => sampa_sob,
        data_eob   => sampa_eob,
        data_out   => sampa_data_stream
    );
    
    sampa_trigger_i : entity work.delay_counter
    port map (
           clk => sampa_clk_40MHz,
           rst => sampa_reset,
           cnt => x"000f",
           en  => '1',
           ack => open,
           trig_in => sampa_trig_en,
           trig_out => sampa_trig_i
    );
    
    gtm_trig_sync_i : entity work.gc_pulse_synchronizer2
    port map (
        clk_in_i => gth_rx_clk,
        rst_in_n_i => '1',
        clk_out_i => sampa_clk_40MHz,
        rst_out_n_i => '1',
        d_ready_o => open,
        d_ack_p_o => open,
        d_p_i => gtm_recv.lvl1_accept(0),
        q_p_o => sampa_trig_gtm);
    
    sampa_trig <= sampa_trig_i or sampa_trig_gtm;
    
    -- Buffers --
    HRST_OBUF : OBUFDS
    port map (
        I => sampa_hard_rst,
        O => sampa_hard_rst_p,
        OB => sampa_hard_rst_n 
    );

    BXTRIG_OBUF : OBUFDS
    port map (
        I => '0',
        O => sampa_bx_sync_trig_p,
        OB => sampa_bx_sync_trig_n
    );
    
    HBTRIG_OBUF : OBUFDS
    port map (
        I => '0',
        O => sampa_hbeat_trig_p,
        OB => sampa_hbeat_trig_n 
    );
    
    TRIG_OBUF : OBUFDS
    port map (
        I => sampa_trig,
        O => sampa_trig_p,
        OB => sampa_trig_n
    );
    
    SAMPA_SCLK_CLK_G : for idx in 0 to sampa_sdo_clk_n'length-1 generate
          SCLK_IBUFDS : OBUFDS
          generic map (
            SLEW => "FAST"
          )
          port map (
            O  => sampa_sdo_clk_p(idx),
            OB => sampa_sdo_clk_n(idx),
            I  => sampa_sdo_clk(idx) 
          );
          sampa_sdo_clk(idx) <= sampa_clk_160MHz;
    end generate SAMPA_SCLK_CLK_G;
    
    ADC_IBUFDS : OBUFDS
    generic map (
        SLEW => "FAST"
    )
    port map (
        O  => sampa_adc_clk_p,
        OB => sampa_adc_clk_n,
        I  => sampa_adc_clk 
    );
    sampa_adc_clk <= sampa_clk_20MHz;
    
    BX_IBUFDS : OBUFDS
    generic map (
        SLEW => "FAST"
    )
    port map (
        O  => sampa_bx_clk_p,
        OB => sampa_bx_clk_n,
        I  => sampa_bx_clk 
    );
    sampa_bx_clk <= sampa_clk_40MHz;

    SAMPA_DECODE_0_G : for idx in 0 to 3 generate
        sampa_decoder_i: entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa0_sdo_p(idx),
            sdi_n => sampa0_sdo_n(idx),
            en    => sampa_elink_enable(idx),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx),
            rd_en         => sampa_rd_en(idx),
            rd_available  => sampa_rd_available(idx)
        );
    end generate;

    SAMPA_DECODE_1_G : for idx in 0 to 3 generate
        sampa_decoder_i : entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa1_sdo_p(idx),
            sdi_n => sampa1_sdo_n(idx),
            en    => sampa_elink_enable(idx+(4*1)),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx+(4*1)),
            rd_en         => sampa_rd_en(idx+(4*1)),
            rd_available  => sampa_rd_available(idx+(4*1))
        );
    end generate;

    SAMPA_DECODE_2_G : for idx in 0 to 3 generate
        sampa_decoder_i : entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa2_sdo_p(idx),
            sdi_n => sampa2_sdo_n(idx),
            en    => sampa_elink_enable(idx+(4*2)),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx+(4*2)),
            rd_en         => sampa_rd_en(idx+(4*2)),
            rd_available  => sampa_rd_available(idx+(4*2))
        );
    end generate;

    SAMPA_DECODE_3_G : for idx in 0 to 3 generate
        sampa_decoder_i : entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa3_sdo_p(idx),
            sdi_n => sampa3_sdo_n(idx),
            en    => sampa_elink_enable(idx+(4*3)),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx+(4*3)),
            rd_en         => sampa_rd_en(idx+(4*3)),
            rd_available  => sampa_rd_available(idx+(4*3))
        );
    end generate;

    SAMPA_DECODE_4_G : for idx in 0 to 3 generate
        sampa_decoder_i : entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa4_sdo_p(idx),
            sdi_n => sampa4_sdo_n(idx),
            en    => sampa_elink_enable(idx+(4*4)),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx+(4*4)),
            rd_en         => sampa_rd_en(idx+(4*4)),
            rd_available  => sampa_rd_available(idx+(4*4))
        );
    end generate;

    SAMPA_DECODE_5_G : for idx in 0 to 3 generate
        sampa_decoder_i : entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa5_sdo_p(idx),
            sdi_n => sampa5_sdo_n(idx),
            en    => sampa_elink_enable(idx+(4*5)),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx+(4*5)),
            rd_en         => sampa_rd_en(idx+(4*5)),
            rd_available  => sampa_rd_available(idx+(4*5))
        );
    end generate;

    SAMPA_DECODE_6_G : for idx in 0 to 3 generate
        sampa_decoder_i : entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa6_sdo_p(idx),
            sdi_n => sampa6_sdo_n(idx),
            en    => sampa_elink_enable(idx+(4*6)),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx+(4*6)),
            rd_en         => sampa_rd_en(idx+(4*6)),
            rd_available  => sampa_rd_available(idx+(4*6))
        );
    end generate;

    SAMPA_DECODE_7_G : for idx in 0 to 3 generate
        sampa_decoder_i : entity work.sampa_decoder
        port map (
            clk   => sampa_clk_160MHz,
            rst   => sampa_reset,
            sdi_p => sampa7_sdo_p(idx),
            sdi_n => sampa7_sdo_n(idx),
            en    => sampa_elink_enable(idx+(4*7)),
            rd_clk        => gth_tx_clk,
            rd_data       => sampa_data(idx+(4*7)),
            rd_en         => sampa_rd_en(idx+(4*7)),
            rd_available  => sampa_rd_available(idx+(4*7))
        );
    end generate;
    
    IDELAYCTRL_inst : IDELAYCTRL
    port map (
      RDY => idelay_ready,  -- 1-bit output: Ready output
      REFCLK => clk_200MHz, -- 1-bit input: Reference clock input
      RST => not pll_locked -- 1-bit input: Active high reset input
   );

end behv;

