
/* petirocLib.h */

#ifndef PETIROC_LIB_H
#define PETIROC_LIB_H

#define MERGE_(a,b)  a##b
#define LABELPETIROC_(a) MERGE_(unsigned int petirocblank, a)
#define BLANKPETIROC LABELPETIROC_(__LINE__)

#pragma pack(push,4)
typedef struct
{
  union
  {
    unsigned int Data[20];

    struct
    {
      // Analog probe
      unsigned int out_inpDAC_probe           : 32;
      unsigned int out_vth_discri             : 32;
      unsigned int out_time                   : 32;
      unsigned int out_time_dummy             : 1;
      unsigned int out_ramp_tdc               : 32;
      // Digital probe
      unsigned int out_discri_charge          : 32;
      unsigned int out_charge                 : 32;
      unsigned int startRampbADC_int          : 1;
      unsigned int holdb                      : 1;
    } Probes;

    struct
    {
      unsigned int mask_discri_charge         : 32;
      unsigned int inputDAC_ch0               : 8;
      unsigned int inputDAC_en_ch0            : 1;
      unsigned int inputDAC_ch1               : 8;
      unsigned int inputDAC_en_ch1            : 1;
      unsigned int inputDAC_ch2               : 8;
      unsigned int inputDAC_en_ch2            : 1;
      unsigned int inputDAC_ch3               : 8;
      unsigned int inputDAC_en_ch3            : 1;
      unsigned int inputDAC_ch4               : 8;
      unsigned int inputDAC_en_ch4            : 1;
      unsigned int inputDAC_ch5               : 8;
      unsigned int inputDAC_en_ch5            : 1;
      unsigned int inputDAC_ch6               : 8;
      unsigned int inputDAC_en_ch6            : 1;
      unsigned int inputDAC_ch7               : 8;
      unsigned int inputDAC_en_ch7            : 1;
      unsigned int inputDAC_ch8               : 8;
      unsigned int inputDAC_en_ch8            : 1;
      unsigned int inputDAC_ch9               : 8;
      unsigned int inputDAC_en_ch9            : 1;
      unsigned int inputDAC_ch10              : 8;
      unsigned int inputDAC_en_ch10           : 1;
      unsigned int inputDAC_ch11              : 8;
      unsigned int inputDAC_en_ch11           : 1;
      unsigned int inputDAC_ch12              : 8;
      unsigned int inputDAC_en_ch12           : 1;
      unsigned int inputDAC_ch13              : 8;
      unsigned int inputDAC_en_ch13           : 1;
      unsigned int inputDAC_ch14              : 8;
      unsigned int inputDAC_en_ch14           : 1;
      unsigned int inputDAC_ch15              : 8;
      unsigned int inputDAC_en_ch15           : 1;
      unsigned int inputDAC_ch16              : 8;
      unsigned int inputDAC_en_ch16           : 1;
      unsigned int inputDAC_ch17              : 8;
      unsigned int inputDAC_en_ch17           : 1;
      unsigned int inputDAC_ch18              : 8;
      unsigned int inputDAC_en_ch18           : 1;
      unsigned int inputDAC_ch19              : 8;
      unsigned int inputDAC_en_ch19           : 1;
      unsigned int inputDAC_ch20              : 8;
      unsigned int inputDAC_en_ch20           : 1;
      unsigned int inputDAC_ch21              : 8;
      unsigned int inputDAC_en_ch21           : 1;
      unsigned int inputDAC_ch22              : 8;
      unsigned int inputDAC_en_ch22           : 1;
      unsigned int inputDAC_ch23              : 8;
      unsigned int inputDAC_en_ch23           : 1;
      unsigned int inputDAC_ch24              : 8;
      unsigned int inputDAC_en_ch24           : 1;
      unsigned int inputDAC_ch25              : 8;
      unsigned int inputDAC_en_ch25           : 1;
      unsigned int inputDAC_ch26              : 8;
      unsigned int inputDAC_en_ch26           : 1;
      unsigned int inputDAC_ch27              : 8;
      unsigned int inputDAC_en_ch27           : 1;
      unsigned int inputDAC_ch28              : 8;
      unsigned int inputDAC_en_ch28           : 1;
      unsigned int inputDAC_ch29              : 8;
      unsigned int inputDAC_en_ch29           : 1;
      unsigned int inputDAC_ch30              : 8;
      unsigned int inputDAC_en_ch30           : 1;
      unsigned int inputDAC_ch31              : 8;
      unsigned int inputDAC_en_ch31           : 1;
      unsigned int inputDACdummy              : 8;
      unsigned int mask_discri_time           : 32;
      unsigned int DAC6b_ch0                  : 6;
      unsigned int DAC6b_ch1                  : 6;
      unsigned int DAC6b_ch2                  : 6;
      unsigned int DAC6b_ch3                  : 6;
      unsigned int DAC6b_ch4                  : 6;
      unsigned int DAC6b_ch5                  : 6;
      unsigned int DAC6b_ch6                  : 6;
      unsigned int DAC6b_ch7                  : 6;
      unsigned int DAC6b_ch8                  : 6;
      unsigned int DAC6b_ch9                  : 6;
      unsigned int DAC6b_ch10                 : 6;
      unsigned int DAC6b_ch11                 : 6;
      unsigned int DAC6b_ch12                 : 6;
      unsigned int DAC6b_ch13                 : 6;
      unsigned int DAC6b_ch14                 : 6;
      unsigned int DAC6b_ch15                 : 6;
      unsigned int DAC6b_ch16                 : 6;
      unsigned int DAC6b_ch17                 : 6;
      unsigned int DAC6b_ch18                 : 6;
      unsigned int DAC6b_ch19                 : 6;
      unsigned int DAC6b_ch20                 : 6;
      unsigned int DAC6b_ch21                 : 6;
      unsigned int DAC6b_ch22                 : 6;
      unsigned int DAC6b_ch23                 : 6;
      unsigned int DAC6b_ch24                 : 6;
      unsigned int DAC6b_ch25                 : 6;
      unsigned int DAC6b_ch26                 : 6;
      unsigned int DAC6b_ch27                 : 6;
      unsigned int DAC6b_ch28                 : 6;
      unsigned int DAC6b_ch29                 : 6;
      unsigned int DAC6b_ch30                 : 6;
      unsigned int DAC6b_ch31                 : 6;
      unsigned int EN_10b_DAC                 : 1;
      unsigned int PP_10b_DAC                 : 1;
      unsigned int vth_discri_charge          : 10; // note: bit reversal required
      unsigned int vth_time                   : 10; // note: bit reversal required
      unsigned int EN_ADC                     : 1;
      unsigned int PP_ADC                     : 1;
      unsigned int sel_startb_ramp_ADC_ext    : 1;
      unsigned int usebcompensation           : 1;
      unsigned int ENbiasDAC_delay            : 1;
      unsigned int PPbiasDAC_delay            : 1;
      unsigned int ENbiasramp_delay           : 1;
      unsigned int PPbiasramp_delay           : 1;
      unsigned int DACdelay                   : 8;
      unsigned int EN_discri_delay            : 1;
      unsigned int PP_discri_delay            : 1;
      unsigned int EN_temp_sensor             : 1;
      unsigned int PP_temp_sensor             : 1;
      unsigned int EN_bias_pa                 : 1;
      unsigned int PP_bias_pa                 : 1;
      unsigned int EN_bias_discri             : 1;
      unsigned int PP_bias_discri             : 1;
      unsigned int cmd_polarity               : 1;
      unsigned int LatchDiscri                : 1;
      unsigned int EN_bias_6b_DAC             : 1;
      unsigned int PP_bias_6b_DAC             : 1;
      unsigned int EN_bias_tdc                : 1;
      unsigned int PP_bias_tdc                : 1;
      unsigned int ON_input_DAC               : 1;
      unsigned int EN_bias_charge             : 1;
      unsigned int PP_bias_charge             : 1;
      unsigned int Cf                         : 4;  // note: bit reversal required
      unsigned int EN_bias_sca                : 1;
      unsigned int PP_bias_sca                : 1;
      unsigned int EN_bias_discri_charge      : 1;
      unsigned int PP_bias_discri_charge      : 1;
      unsigned int EN_bias_discri_ADC_time    : 1;
      unsigned int PP_bias_discri_ADC_time    : 1;
      unsigned int EN_bias_discri_ADC_charge  : 1;
      unsigned int PP_bias_discri_ADC_charge  : 1;
      unsigned int DIS_razchn_int             : 1;
      unsigned int DIS_razchn_ext             : 1;
      unsigned int SEL_80M                    : 1;
      unsigned int EN_80M                     : 1;
      unsigned int EN_slow_lvds_rec           : 1;
      unsigned int PP_slow_lvds_rec           : 1;
      unsigned int EN_fast_lvds_rec           : 1;
      unsigned int PP_fast_lvds_rec           : 1;
      unsigned int EN_transmitter             : 1;
      unsigned int PP_transmitter             : 1;
      unsigned int ON_1mA                     : 1;
      unsigned int ON_2mA                     : 1;
      unsigned int NC                         : 1;
      unsigned int ON_ota_mux                 : 1;
      unsigned int ON_ota_probe               : 1;
      unsigned int DIS_trig_mux               : 1;
      unsigned int EN_NOR32_time              : 1;
      unsigned int EN_NOR32_charge            : 1;
      unsigned int DIS_triggers               : 1;
      unsigned int EN_dout_oc                 : 1;
      unsigned int EN_transmit                : 1;
    } SlowControl;
  };
} PETIROC_Regs;
#pragma pack(pop)

typedef struct
{
/* 0x0000-0x0003 */ unsigned int    Ctrl;
/* 0x0004-0x0007 */ unsigned int    Status;
/* 0x0008-0x000B */ unsigned int    SpiCtrl;
/* 0x000C-0x000F */ unsigned int    SpiStatus;
/* 0x0010-0x0013 */ unsigned int    BoardId;
/* 0x0014-0x0017 */ unsigned int    Reserved0[(0x0018-0x0014)/4];
/* 0x0018-0x001B */ unsigned int    FirmwareRev;
/* 0x001C-0x001F */ unsigned int    Timestamp;
/* 0x0020-0x00FF */ unsigned int    Reserved1[(0x0100-0x0020)/4];
} CLK_regs;

typedef struct
{
/* 0x0000-0x0003 */ unsigned int    Ctrl;
/* 0x0004-0x0007 */ unsigned int    Status;
/* 0x0008-0x000F */ unsigned int    Reserved0[(0x0010-0x0008)/4];
/* 0x0010-0x005F */ unsigned int    SerData0[20];
/* 0x0060-0x00AF */ unsigned int    SerData1[20];
/* 0x00B0-0x00FF */ unsigned int    Reserved1[(0x0100-0x00B0)/4];
} PETIROC_CFG_regs;

typedef struct
{
/* 0x0000-0x0003 */ unsigned int    Ctrl;
/* 0x0004-0x00FF */ unsigned int    Reserved0[(0x0100-0x0004)/4];
} PETIROC_ADC_regs;

typedef struct
{
/* 0x0000-0x0003 */ unsigned int Ctrl;
/* 0x0004-0x007F */ unsigned int Reserved0[(0x0080-0x0004)/4];
/* 0x0080-0x0083 */ unsigned int Period;
/* 0x0084-0x0087 */ unsigned int LowCycles;
/* 0x0088-0x008B */ unsigned int NCycles;
/* 0x008C-0x008F */ unsigned int Start;
/* 0x0090-0x0093 */ unsigned int Status;
/* 0x0094-0x00FF */ unsigned int Reserved1[(0x0100-0x0094)/4];
} Pulser_regs;

typedef struct
{
/* 0x0000-0x0003 */ unsigned int Ctrl;
/* 0x0004-0x0007 */ unsigned int NumEntriesMin;
/* 0x0008-0x000F */ unsigned int TOF_TdcEn[2];
/* 0x0010-0x0013 */ unsigned int CAL_TdcEn;
/* 0x0014-0x0017 */ unsigned int CAL_Ctrl;
/* 0x0018-0x001B */ unsigned int CAL_Status;
/* 0x001C-0x001F */ unsigned int Reserved0[(0x0020-0x001C)/4];
/* 0x0020-0x00EF */ unsigned int Scalers[52];
/* 0x00F0-0x00FF */ unsigned int Reserved1[(0x0100-0x00F0)/4];
} TDC_regs;

typedef struct
{
/* 0x0000-0x000F */ unsigned int Temp[4];
/* 0x0010-0x00FF */ unsigned int Reserved0[(0x0100-0x0010)/4];
} TempMon_regs;

typedef struct
{
/* 0x0000-0x0003 */ unsigned int Trig;
/* 0x0004-0x0007 */ unsigned int Sync;
/* 0x0008-0x000B */ unsigned int Busy;
/* 0x000C-0x000F */ unsigned int LedG;
/* 0x0010-0x0013 */ unsigned int LedY;
/* 0x0014-0x0017 */ unsigned int Status;
/* 0x0018-0x003F */ unsigned int Reserved0[(0x0040-0x0018)/4];
/* 0x0040-0x0043 */ unsigned int Delay;
/* 0x0044-0x0047 */ unsigned int ErrCtrl;
/* 0x0048-0x004B */ unsigned int ErrStatus;
/* 0x004C-0x009F */ unsigned int Reserved1[(0x00A0-0x004C)/4];
/* 0x00A0-0x00A3 */ unsigned int Latch;
/* 0x00A4-0x00C7 */ unsigned int Scalers[9];
/* 0x00C8-0x00FF */ unsigned int Reserved2[(0x0100-0x00C8)/4];
} Sd_regs;

typedef struct
{
/* 0x0000-0x0003 */ unsigned int    Blocksize;
/* 0x0004-0x0007 */ unsigned int    TrigBusyThr;
/* 0x0008-0x000B */ unsigned int    Lookback;
/* 0x000C-0x000F */ unsigned int    WindowWidth;
/* 0x0010-0x0013 */ unsigned int    DeviceID;
/* 0x0014-0x0017 */ unsigned int    TrigDelay;
/* 0x0018-0x00FF */ unsigned int    Reserved3[(0x0100-0x0018)/4];
} PETIROC_Eb;

typedef struct
{
/* 0x0000-0x0003 */ unsigned int    ErrCtrl;
/* 0x0004-0x0007 */ unsigned int    ErrAddrL;
/* 0x0008-0x000B */ unsigned int    ErrAddrH;
/* 0x000C-0x000F */ BLANKPETIROC[(0x0010-0x000C)/4];
/* 0x0010-0x0013 */ unsigned int    HeartBeatCnt;
/* 0x0014-0x001B */ BLANKPETIROC[(0x001C-0x0014)/4];
/* 0x001C-0x001F */ unsigned int    CorrectionCnt;
/* 0x005C-0x005F */ BLANKPETIROC[(0x0060-0x0020)/4];
/* 0x0060-0x0063 */ unsigned int    XAdcCtrl;
/* 0x0064-0x0067 */ unsigned int    XAdcStatus;
/* 0x0068-0x006F */ BLANKPETIROC[(0x0070-0x0068)/4]; 
/* 0x0070-0x0073 */ unsigned int    FiberCtrl;
/* 0x0074-0x0077 */ unsigned int    FiberStatus;
/* 0x0078-0x007F */ BLANKPETIROC[(0x0080-0x0078)/4];
/* 0x0080-0x0083 */ unsigned int    FpgaRebootCtrl;
/* 0x0084-0x0087 */ unsigned int    FpgaRebootStatus;
/* 0x0088-0x00FF */ BLANKPETIROC[(0x0100-0x0088)/4];
} PETIROC_Testing;

typedef struct
{
/* 0x0000-0x00FF */ CLK_regs          Clk;
/* 0x0100-0x01FF */ Sd_regs           Sd;
/* 0x0200-0x02FF */ PETIROC_Eb        Eb;
/* 0x0300-0x04FF */ unsigned int      Reserved0[(0x0500-0x0300)/4];
/* 0x0500-0x05FF */ PETIROC_CFG_regs  PetirocCfg;
/* 0x0600-0x07FF */ PETIROC_ADC_regs  PetirocAdc[2];
/* 0x0800-0x08FF */ Pulser_regs       Pulser;
/* 0x0900-0x0BFF */ unsigned int      Reserved1[(0x0C00-0x0900)/4];
/* 0x0C00-0x0CFF */ PETIROC_Testing   Testing;
/* 0x0D00-0x0DFF */ TempMon_regs      TempMon;
/* 0x0E00-0x0FFF */ unsigned int      Reserved2[(0x1000-0x0E00)/4];
/* 0x1000-0x10FF */ TDC_regs          Tdc;
} PETIROC_regs;

typedef struct
{
  // Temps units: mC
  struct
  {
    int fpga;
    int sipm[4]; // arb units
  } temps;
  
  // Temps units: mV
  struct
  {
    int pcb_lv1;
    int pcb_lv2;
    int pcb_3_3va;
    int pcb_3_3v;
    int pcb_2_5v;
    int fpga_vccint_1v;
    int fpga_vccaux_1_8v;
    int fpga_mgt_1v;
    int fpga_mgt_1_2v;
  } voltages;

  // SEM
  struct
  {
    int heartbeat;
    int seu_cnt;
  } sem;

} petiroc_monitor_t;

#define OK                0
#define ERROR             -1

#define PETIROC_MAX_NUM          15 //sergey: was 32

// FPGA IP addresses start at: PETIROC_SUBNET.PETIROC_IP_START
// Last device ends at:        PETIROC_SUBNET.(PETIROC_IP_START+PETIROC_MAX_NUM-1)
#define PETIROC_IP_START        10
#define PETIROC_SUBNET          "192.168.0."
#define PETIROC_REG_SOCKET      6103
#define PETIROC_SLOWCON_SOCKET  6104
#define PETIROC_EVT_SOCKET      6105

// SPI Flash memory constants
#define PETIROC_FLASH_CMD_WRPAGE      0x12
#define PETIROC_FLASH_CMD_RD          0x13
#define PETIROC_FLASH_CMD_GETSTATUS   0x05
#define PETIROC_FLASH_CMD_WREN        0x06
#define PETIROC_FLASH_CMD_GETID       0x9F
#define PETIROC_FLASH_CMD_ERASE64K    0xDC
#define PETIROC_FLASH_CMD_4BYTE_EN    0xB7
#define PETIROC_FLASH_CMD_4BYTE_DIS   0xE9

#define PETIROC_FLASH_BYTE_LENGTH     32*1024*1024
#define PETIROC_FLASH_MFG_MICRON      0x20
#define PETIROC_FLASH_DEVID_N25Q256A  0xBB19

#define PETIROC_SPI_MFG_WINBOND       0xEF
#define PETIROC_SPI_DEVID_W25Q256JVIQ 0x4019

#define PETIROC_SPI_MFG_ATMEL         0x12
#define PETIROC_SPI_DEVID_AT45DB642D  0x3456

#define PETIROC_INIT_REGSOCKET        0x00000000
#define PETIROC_INIT_SLOWCONSOCKET    0x00000001

int petiroc_read_ip(int slot);
int petiroc_program_ip(int slot, unsigned int ip, unsigned int mac0, unsigned int mac1);
int petiroc_check_open_register_socket(int slot);
int petiroc_open_socket(char *ip, int port);
int petiroc_open_register_socket(int slot, int type);
int petiroc_open_event_socket(int slot);
void petiroc_close_register_socket(int slot);
void petiroc_close_event_socket(int slot);
int petiroc_read_event_socket(int slot, unsigned int *buf, int nwords_max);
int petirocEventBufferRead(int slot, unsigned int *buf, int nwords_max);
void petirocEventBufferWrite(int slot, unsigned int *buf, int nwords);
int petirocReadBlock(unsigned int *buf, int nwords_max);
int petirocGetNpetiroc();
int petirocSlot(int n);
int petirocInit(int devid_start, int n, int iFlag);
void petirocEnable();
void petirocEnd();
int petiroc_clear_scalers(int slot);
int petiroc_get_scaler(int slot, int ch);
int petiroc_status_all();
int petiroc_gstatus();
int petiroc_startb_adc(int slot, int val);
int petiroc_trig_ext(int slot);
int petiroc_hold_ext(int slot, int val);
int petiroc_enable(int slot, int enable);
int petiroc_val_evt(int slot, int sel, int en_dly, int dis_dly);
int petiroc_raz_chn(int slot, int sel);
int petiroc_soft_reset(int slot, int val);
int petiroc_set_tdc_enable(int slot, int en_mask[2]);
int petiroc_get_tdc_enable(int slot, int en_mask[2]);
int petiroc_get_fwrev(int slot);
int petiroc_get_fwtimestamp(int slot);
int petiroc_set_readout(int slot, int width, int offset, int busythr, int trigdelay);
int petiroc_get_readout(int slot, int *width, int *offset, int *busythr, int *trigdelay);
int petiroc_set_blocksize(int slot, int blocksize);
int petiroc_get_fw_timestamp(int slot);
int petiroc_get_fw_rev(int slot);
int petiroc_set_clk(int slot, int sel);
int petiroc_get_clk(int slot, int *sel);
int petiroc_start_conv(int slot, int sel);
int petiroc_force_conv(int slot);
int petiroc_cfg_pwr(int slot, int en_d, int en_a, int en_adc, int en_dac, int gain, int clk_en);
int petiroc_trig_setup(int slot, int trig, int sync);
int petiroc_status(int slot);
int petiroc_cfg_rst(int slot);
int petiroc_cfg_load(int slot);
int petiroc_cfg_select(int slot, int sel);
int petiroc_clken(int slot, int en);
unsigned int bit_flip(unsigned int val, unsigned int len);
int probe(int slot, int ana, int ana_bit, int dig, int dig_bit);
int petiroc_slow_control(int slot, PETIROC_Regs regs[2]);
int petiroc_shift_regs(int slot, PETIROC_Regs *regs, PETIROC_Regs *result);
void petiroc_print_regs(PETIROC_Regs regs, int opt);
int petiroc_set_pulser(int slot, int mask, int ncycles, float freq, float duty, int amp[4]);
int tdc_calibrate_start(int slot, int auto_cal_en, int min_entries);
int tdc_calibrate_stop(int slot, int auto_cal_en, int min_entries);
int petiroc_get_idelayerr(int slot);
int petiroc_set_idelay(int slot, int delay_trig1_p, int delay_trig1_n, int delay_sync_p, int delay_sync_n);
int petiroc_sendscalers(char *host);
int petiroc_printmonitor(int slot);

int petiroc_flash_GFirmwareUpdate(char *filename);
int petiroc_flash_GFirmwareVerify(char *filename);
int petiroc_set_blocksize_all(int blocksize);
int petiroc_Reboot(int id, int image);

#endif

