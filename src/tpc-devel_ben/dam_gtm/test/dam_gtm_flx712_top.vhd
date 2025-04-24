library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use work.gtm_pkg.all;

library UNISIM;
use UNISIM.VComponents.all;

entity dam_gtm_flx_test is
    port (   
        SI5345_INSEL : out std_logic_vector(1 downto 0);
        SI5345_nLOL  : in  std_logic;
        SI5345_SEL   : out std_logic;
        SI5345_OE    : out std_logic;
        SI5345_A     : out std_logic_vector(1 downto 0);
        SI5345_nRST  : out std_logic;

        I2C_SW_SCL : inout std_logic;
        I2C_SW_SDA : inout std_logic;
        
        si5345_ref_clk_p : out std_logic;
        si5345_ref_clk_n : out std_logic;
    
        gtm_ref_clk_p : in std_logic;
        gtm_ref_clk_n : in std_logic;
 
        gtm_tx_n : out std_logic;
        gtm_tx_p : out std_logic;
        gtm_rx_n : in std_logic;
        gtm_rx_p : in std_logic;
        SFP_TX_DISABLE : out std_logic;

        SYSCLK_P : in std_logic;
        SYSCLK_N : in std_logic);
end dam_gtm_flx_test;

architecture behv of dam_gtm_flx_test is

    signal clk_25MHz, clk_40MHz, clk_10MHz : std_logic;
    signal locked, reset, si5345_i2c_busy: std_logic;
    signal si_en, vio_reset, si_rst : std_logic;
    signal gtm_clk, gtm_freq_valid : std_logic;
    signal gtm_freq : std_logic_vector(31 downto 0);

    attribute mark_debug : string;
    attribute mark_debug of gtm_freq : signal is "true";
    attribute mark_debug of gtm_freq_valid : signal is "true";
    
begin

    SFP_TX_DISABLE <= '0';

    vio_0: entity work.vio_0
    PORT MAP (
        clk => clk_40MHz,
        probe_in0(0) => si5345_i2c_busy,
        probe_in1(0) => SI5345_nLOL,
        probe_in2(0) => '0',
        probe_in3(0) => '0',
        probe_in4(0) => '0',
        probe_out0(0) => vio_reset,
        probe_out1(0) => si_en,
        probe_out2(0) => si_rst,
        probe_out3(0) => open,
        probe_out4(0) => open 
    );
    
    clk0 : entity work.clk_wiz_200_0
    port map ( 
        -- Clock in ports
        clk_in1_p => SYSCLK_P,
        clk_in1_n => SYSCLK_N,
        -- Clock out ports  
        clk40  => clk_40MHz,
        clk10  => clk_10MHz,
        clk25  => clk_25MHz,
        -- Status and control signals                
        reset => '0',
        locked => locked
        );
        
    reset <= not locked;
    
    si5345_init_0 : entity work.si5345_pll_init
    port map (
        clk_50MHz  => clk_40MHz,
        rst        => reset or si_rst,
        
        en => si_en,
        busy => si5345_i2c_busy,
        
        status_strb => '0',
        status      => open,

        in_sel   => SI5345_INSEL,
        sel      => SI5345_SEL,
        output_en => SI5345_OE,
        i2c_addr  => SI5345_A,
        n_dev_reset => SI5345_nRST,

        i2c_sda => I2C_SW_SDA,
        i2c_scl => I2C_SW_SCL
    );
    
    si5345_clk_buffer : OBUFDS
    generic map (
        IOSTANDARD => "DEFAULT",
        SLEW => "FAST"
    )
    port map (
        O   => si5345_ref_clk_p,
        OB  => si5345_ref_clk_n,
        I   => clk_40MHz
    );

    gtm_recvr_i: entity work.gtm_recvr
    port map (
        -- Reset Logic (50 MHz) --
        clk   => clk_25Mhz,
        reset => reset or vio_reset,
        pll_locked_n => SI5345_nLOL,
        
        -- MGT Reference Clock (400 MHz) --
        mgt_ref_clk_p => gtm_ref_clk_p,
        mgt_ref_clk_n => gtm_ref_clk_n,
        
        gtm_clk => gtm_clk,
        gtm_recv => open,
        
        -- SFP TX --
        gth_tx_n => gtm_tx_n,
        gth_tx_p => gtm_tx_p,
        
        -- SFP RX --
        gth_rx_n => gtm_rx_n,
        gth_rx_p => gtm_rx_p
    );


    freq_meter_i : entity work.gc_frequency_meter 
  generic map (
    g_WITH_INTERNAL_TIMEBASE => TRUE,
    g_CLK_SYS_FREQ           => 40e6,
    g_SYNC_OUT               => TRUE,
    g_COUNTER_BITS           => 32)
  port map (
    clk_sys_i    => clk_40MHz,
    clk_in_i     => gtm_clk,
    rst_n_i      => '0',
    pps_p1_i     => '0',
    -- synced to clk_in_i or clk_sys_i, depending on g_SYNC_OUT value
    freq_o       => gtm_freq, 
    -- synced to clk_sys_i, always
    freq_valid_o => gtm_freq_valid);

end behv;
