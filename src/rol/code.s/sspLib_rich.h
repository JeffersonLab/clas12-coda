#ifndef __SSPLIB_RICH_H 
#define __SSPLIB_RICH_H 
/****************************************************************************** 
 * 
 *  sspLib_rich.h -  Header for Driver library for JLAB config of JLAB Subsystem  
 *                   Processor (SSP) - rich extensions. 
 * 
 */ 

/* Macros to help with register spacers */
#define MERGE_(a,b)  a##b
#define LABEL_(a) MERGE_(unsigned int sspblank, a)
#define BLANK LABEL_(__LINE__)

/******************/
/* RICH registers */
/******************/

#define RICH_FIBER_NUM            32
#define RICH_CHAN_NUM             192

#define RICH_CLK_SOFTRESET_MASK   0x00000001
#define RICH_CLK_BOARDID          0x52494348


#define RICH_FLASH_CMD_WRPAGE     0x02
#define RICH_FLASH_CMD_RD         0x03
#define RICH_FLASH_CMD_GETSTATUS  0x05
#define RICH_FLASH_CMD_WREN       0x06
#define RICH_FLASH_CMD_GETID      0x9F
#define RICH_FLASH_CMD_ERASE64K   0xD8
#define RICH_FLASH_CMD_4BYTE_EN   0xB7
#define RICH_FLASH_CMD_4BYTE_DIS  0xE9

/*
#define RICH_FLASH_CMD_WRPAGE     0x12
#define RICH_FLASH_CMD_RD         0x13
#define RICH_FLASH_CMD_GETSTATUS  0x05
#define RICH_FLASH_CMD_WREN       0x06
#define RICH_FLASH_CMD_GETID      0x9F
#define RICH_FLASH_CMD_ERASE64K   0xDC
*/

#define RICH_FLASH_BYTE_LENGTH    32*1024*1024
#define RICH_FLASH_MFG_MICRON     0x20
#define RICH_FLASH_DEVID_N25Q256A 0xBB19

typedef struct
{
/* 0x0000-0x0003 */ volatile unsigned int    Ctrl;
/* 0x0004-0x0007 */ BLANK[(0x0008-0x0004)/4];
/* 0x0008-0x000B */ volatile unsigned int    SpiCtrl;
/* 0x000C-0x000F */ volatile unsigned int    SpiStatus;
/* 0x0010-0x0013 */ volatile unsigned int    BoardId;
/* 0x0014-0x0017 */ volatile unsigned int    ICAP;
/* 0x0018-0x001B */ volatile unsigned int    FWRev;
/* 0x001C-0x00FF */ BLANK[(0x0100-0x001C)/4];
} RICH_clk;

typedef struct
{
  union
  {
    volatile unsigned int val;
    struct
    {
      volatile unsigned int cmd_fsu        : 1;
      volatile unsigned int cmd_ss         : 1;
      volatile unsigned int cmd_fsb        : 1;
      volatile unsigned int swb_buf_250f   : 1;
      volatile unsigned int swb_buf_500f   : 1;
      volatile unsigned int swb_buf_1p     : 1;
      volatile unsigned int swb_buf_2p     : 1;
      volatile unsigned int ONOFF_ss       : 1;
      volatile unsigned int sw_ss_300f     : 1;
      volatile unsigned int sw_ss_600f     : 1;
      volatile unsigned int sw_ss_1200f    : 1;
      volatile unsigned int EN_ADC         : 1;
      volatile unsigned int H1H2_choice    : 1;
      volatile unsigned int sw_fsu_20f     : 1;
      volatile unsigned int sw_fsu_40f     : 1;
      volatile unsigned int sw_fsu_25k     : 1;
      volatile unsigned int sw_fsu_50k     : 1;
      volatile unsigned int sw_fsu_100k    : 1;
      volatile unsigned int sw_fsb1_50k    : 1;
      volatile unsigned int sw_fsb1_100k   : 1;
      volatile unsigned int sw_fsb1_100f   : 1;
      volatile unsigned int sw_fsb1_50f    : 1;
      volatile unsigned int cmd_fsb_fsu    : 1;
      volatile unsigned int valid_dc_fs    : 1;
      volatile unsigned int sw_fsb2_50k    : 1;
      volatile unsigned int sw_fsb2_100k   : 1;
      volatile unsigned int sw_fsb2_100f   : 1;
      volatile unsigned int sw_fsb2_50f    : 1;
      volatile unsigned int valid_dc_fsb2  : 1;
      volatile unsigned int ENb_tristate   : 1;
      volatile unsigned int polar_discri   : 1;
      volatile unsigned int inv_discriADC  : 1;
    } bits;
  } Global0;

  union
  {
    volatile unsigned int val;
    struct
    {
      volatile unsigned int d1_d2              : 1;
      volatile unsigned int cmd_CK_mux         : 1;
      volatile unsigned int ONOFF_otabg        : 1;
      volatile unsigned int ONOFF_dac          : 1;
      volatile unsigned int small_dac          : 1;
      volatile unsigned int enb_outADC         : 1;
      volatile unsigned int inv_startCmptGray  : 1;
      volatile unsigned int ramp_8bit          : 1;
      volatile unsigned int ramp_10bit         : 1;
      volatile unsigned int Reserved0          : 23;
    } bits;
  } Global1;

  union
  {
    volatile unsigned int val;
    struct
    {
      volatile unsigned int DAC0       : 10;
      volatile unsigned int Reserved0  : 6;
      volatile unsigned int DAC1       : 10;
      volatile unsigned int Reserved1  : 6;
    } bits;
  } DAC;

  volatile unsigned int Reserved0;

  union
  {
    volatile unsigned int val;
    struct
    {
      volatile unsigned int Gain0      : 8;
      volatile unsigned int Sum0       : 1;
      volatile unsigned int CTest0     : 1;
      volatile unsigned int MaskOr0    : 2;
      volatile unsigned int Reserved0  : 4;
      volatile unsigned int Gain1      : 8;
      volatile unsigned int Sum1       : 1;
      volatile unsigned int CTest1     : 1;
      volatile unsigned int MaskOr1    : 2;
      volatile unsigned int Reserved1  : 4;
    } bits;
  } CH[32];
} MAROC_Regs;

typedef struct
{
  volatile unsigned int Ch0_31_Hold1;
  volatile unsigned int Ch32_63_Hold1;
  volatile unsigned int Ch0_31_Hold2;
  volatile unsigned int Ch32_63_Hold2;
} MAROC_DyRegs;

#define RICH_MAROCCFG_DAC_MAX     4095

typedef struct
{
/* 0x0000-0x0003 */ volatile unsigned int SerCtrl;
/* 0x0004-0x0007 */ volatile unsigned int SerStatus;
/* 0x0008-0x000B */ volatile unsigned int DACAmplitude;
/* 0x000C-0x000F */ BLANK[(0x0010-0x000C)/4];
/* 0x0010-0x009F */ MAROC_Regs            Regs;
/* 0x00A0-0x00AF */ MAROC_DyRegs          DyRegs_WrAll;
/* 0x00B0-0x00DF */ MAROC_DyRegs          DyRegs_Rd[3];
/* 0x00E0-0x00FF */ BLANK[(0x0100-0x00E0)/4];
} RICH_MAROC_Cfg;

typedef struct
{
/* 0x0000-0x000F */ volatile unsigned int    DisableCh[4];
/* 0x0010-0x0013 */ volatile unsigned int    HitOrMask0;
/* 0x0014-0x0017 */ volatile unsigned int    HitOrMask1;
/* 0x0018-0x00FF */ BLANK[(0x0100-0x0018)/4];
/* 0x0100-0x01FF */ volatile unsigned int    Scalers[64];
} RICH_Maroc_Proc;

typedef struct
{
/* 0x0000-0x0003 */ volatile unsigned int    AdcCtrl;
/* 0x0004-0x0007 */ BLANK[(0x0008-0x0004)/4];
/* 0x0008-0x000B */ volatile unsigned int    Hold1Delay;
/* 0x000C-0x000F */ volatile unsigned int    Hold2Delay;
/* 0x0010-0x00FF */ BLANK[(0x0100-0x0010)/4];
} RICH_MAROC_Adc;

#define RICH_EB_LOOKBACK_MAX      1023
#define RICH_EB_WINDOWWIDTH_MAX   1023

typedef struct
{
/* 0x0000-0x0003 */ volatile unsigned int    Lookback;
/* 0x0004-0x0007 */ volatile unsigned int    WindowWidth;
/* 0x0008-0x000B */ volatile unsigned int    BlockCfg;
/* 0x000C-0x000F */ BLANK[(0x0010-0x000C)/4];
/* 0x0010-0x0013 */ volatile unsigned int    DeviceID;
/* 0x0014-0x0017 */ volatile unsigned int    TrigDelay;
/* 0x0018-0x0023 */ BLANK[(0x0024-0x0018)/4];
/* 0x0024-0x0027 */ volatile unsigned int    FifoWordCnt;
/* 0x0028-0x002B */ volatile unsigned int    FifoEventCnt;
/* 0x002C-0x007F */ BLANK[(0x0080-0x002C)/4];
/* 0x0080-0x0083 */ volatile unsigned int    FifoData;
/* 0x0084-0x00FF */ BLANK[(0x0100-0x0084)/4];
} RICH_EvtBuilder;

typedef struct
{
/* 0x0000-0x0003 */ volatile unsigned int    ErrCtrl;
/* 0x0004-0x0007 */ volatile unsigned int    ErrAddrL;
/* 0x0008-0x000B */ volatile unsigned int    ErrAddrH;
/* 0x000C-0x000F */ BLANK[(0x0010-0x000C)/4];
/* 0x0010-0x0013 */ volatile unsigned int    HeartBeatCnt;
/* 0x0014-0x0017 */ volatile unsigned int    InitializationCnt;
/* 0x0018-0x001B */ volatile unsigned int    ObservationCnt;
/* 0x001C-0x001F */ volatile unsigned int    CorrectionCnt;
/* 0x0020-0x0023 */ volatile unsigned int    ClassifactionCnt;
/* 0x0024-0x0027 */ volatile unsigned int    InjectionCnt;
/* 0x0028-0x002B */ volatile unsigned int    EssentialCnt;
/* 0x002C-0x002F */ volatile unsigned int    UncorrectableCnt;
/* 0x0030-0x0033 */ volatile unsigned int    RamAddr;
/* 0x0034-0x0037 */ volatile unsigned int    RamWrData;
/* 0x0038-0x003B */ volatile unsigned int    RamRdData;
/* 0x003C-0x003F */ BLANK[(0x0040-0x003C)/4];
/* 0x0040-0x0043 */ volatile unsigned int    RegData;
/* 0x0044-0x0047 */ volatile unsigned int    RegCtrl;
/* 0x0048-0x004F */ BLANK[(0x0050-0x0048)/4];
/* 0x0050-0x0053 */ volatile unsigned int    MonRd;
/* 0x0054-0x0057 */ volatile unsigned int    MonWr;
/* 0x0058-0x005B */ volatile unsigned int    MonStatus;
/* 0x005C-0x005F */ BLANK[(0x0060-0x005C)/4];
/* 0x0060-0x0063 */ volatile unsigned int    XAdcCtrl;
/* 0x0064-0x0067 */ volatile unsigned int    XAdcStatus;
/* 0x0068-0x006F */ BLANK[(0x0070-0x0068)/4]; 
/* 0x0070-0x0073 */ volatile unsigned int    FiberCtrl;
/* 0x0074-0x0077 */ volatile unsigned int    FiberStatus;
/* 0x0078-0x00FF */ BLANK[(0x0100-0x0078)/4];
} RICH_Testing;

// Mux signal selection for SD->*Src registers
#define RICH_SD_CTEST_SRC_SEL_0        0
#define RICH_SD_CTEST_SRC_SEL_1        1
#define RICH_SD_CTEST_SRC_SEL_SSP      2

typedef struct
{
/* 0x0000-0x0003 */ volatile unsigned int    ScalerLatch;
/* 0x0004-0x0007 */ volatile unsigned int    Scaler_GClk125;
/* 0x0008-0x000F */ BLANK[(0x0010-0x0008)/4];
/* 0x0010-0x0013 */ volatile unsigned int    CTestSrc;
/* 0x0014-0x00FF */ BLANK[(0x0100-0x0014)/4];
} RICH_sd;

#define SSP_RICH_GTXCTRL_FIBER_GT_RESET   0x00000001
#define SSP_RICH_GTXCTRL_FIBER_RESET      0x00000002

#define SSP_RICH_GTXSTATUS_CHANNELUP      0x00000001
#define SSP_RICH_GTXSTATUS_HARDERR        0x00000002
#define SSP_RICH_GTXSTATUS_FRAMEERR       0x00000004
#define SSP_RICH_GTXSTATUS_ERRCNT         0x0000FF00

#define SSP_RICH_EVT_ENABLE               0x00000001

typedef struct
{
/* 0x0000-0x00FF */ RICH_clk        Clk;
/* 0x0100-0x01FF */ RICH_MAROC_Cfg  MAROC_Cfg;
/* 0x0200-0x02FF */ RICH_sd         Sd;
/* 0x0300-0x03FF */ RICH_MAROC_Adc  MAROC_Adc;
/* 0x0400-0x09FF */ RICH_Maroc_Proc MAROC_Proc[3];
/* 0x0A00-0x0AFF */ BLANK[(0x0B00-0x0A00)/4];
/* 0x0B00-0x0BFF */ RICH_EvtBuilder EvtBuilder;
/* 0x0C00-0x0CFF */ RICH_Testing    Testing;
/* 0x0D00-0x0DFF */ BLANK[(0x0E00-0x0D00)/4];
/* 0x0E00-0x0E03 */ volatile unsigned int GtxCtrl;
/* 0x0E04-0x0E0F */ BLANK[(0x0E10-0x0E04)/4];
/* 0x0E10-0x0E13 */ volatile unsigned int GtxStatus;
/* 0x0E14-0x0E1F */ BLANK[(0x0E20-0x0E14)/4];
/* 0x0E20-0x0E23 */ volatile unsigned int EbCtrl;
/* 0x0E24-0x0E27 */ volatile unsigned int EbFullThreshold;
/* 0x0E28-0x0FFF */ BLANK[(0x1000-0x0E28)/4];
} RICH_regs;

enum maroc_reg_t {
  RICH_MAROC_REG_CMD_FSU = 1,
  RICH_MAROC_REG_CMD_SS,
  RICH_MAROC_REG_CMD_FSB,
  RICH_MAROC_REG_SWB_BUF_250F,
  RICH_MAROC_REG_SWB_BUF_500F,
  RICH_MAROC_REG_SWB_BUF_1P,
  RICH_MAROC_REG_SWB_BUF_2P,
  RICH_MAROC_REG_ONOFF_SS,
  RICH_MAROC_REG_SW_SS_300F,
  RICH_MAROC_REG_SW_SS_600F,
  RICH_MAROC_REG_SW_SS1200F,
  RICH_MAROC_REG_EN_ADC,
  RICH_MAROC_REG_H1H2_CHOICE,
  RICH_MAROC_REG_SW_FSU_20F,
  RICH_MAROC_REG_SW_FSU_40F,
  RICH_MAROC_REG_SW_FSU_25K,
  RICH_MAROC_REG_SW_FSU_50K,
  RICH_MAROC_REG_SW_FSU_100K,
  RICH_MAROC_REG_SW_FSB1_50K,
  RICH_MAROC_REG_SW_FSB1_100K,
  RICH_MAROC_REG_SW_FSB1_100F,
  RICH_MAROC_REG_SW_FSB1_50F,
  RICH_MAROC_REG_CMD_FSB_FSU,
  RICH_MAROC_REG_VALID_DC_FS,
  RICH_MAROC_REG_SW_FSB2_50K,
  RICH_MAROC_REG_SW_FSB2_100K,
  RICH_MAROC_REG_SW_FSB2_100F,
  RICH_MAROC_REG_SW_FSB2_50F,
  RICH_MAROC_REG_VALID_DC_FSB2,
  RICH_MAROC_REG_ENB_TRISTATE,
  RICH_MAROC_REG_POLAR_DISCRI,
  RICH_MAROC_REG_INV_DISCRIADC,
  RICH_MAROC_REG_D1_D2,
  RICH_MAROC_REG_CMD_CK_MUX,
  RICH_MAROC_REG_ONOFF_OTABG,
  RICH_MAROC_REG_ONOFF_DAC,
  RICH_MAROC_REG_SMALL_DAC,
  RICH_MAROC_REG_ENB_OUTADC,
  RICH_MAROC_REG_INV_STARTCMPTGRAY,
  RICH_MAROC_REG_RAMP_8BIT,
  RICH_MAROC_REG_RAMP_10BIT,
  RICH_MAROC_REG_DAC0,
  RICH_MAROC_REG_DAC1,
  RICH_MAROC_REG_GAIN,
  RICH_MAROC_REG_SUM,
  RICH_MAROC_REG_CTEST,
  RICH_MAROC_REG_MASKOR,
  RICH_MAROC_REG_GLOBAL0,
  RICH_MAROC_REG_GLOBAL1
};

typedef struct
{
  // Temps units: mC
  struct
  {
    int fpga;
    int regulator[2];
  } temps;
  
  // Temps units: mV
  struct
  {
    int pcb_5v;
    int pcb_3_3v;
    int fpga_vccint_1v;
    int fpga_vccaux_1_8v;
    int fpga_mgt_1v;
    int fpga_mgt_1_2v;
  } voltages;
} sspRich_Monitor;


int sspRich_IsFiberInvalid(int id, int fiber, const char *func);
int sspRich_ScanFibers(int id);
int sspRich_ScanFibers_NoInit(int id);

int sspRich_GetConnectedFibers(int id, int *fibers);
int sspRich_ReadScalers(int id, int fiber, unsigned int *ref, unsigned int maroc[RICH_CHAN_NUM]);
int sspRich_PrintScalers(int id, int fiber);
int sspRich_ReadMonitor(int id, int fiber, sspRich_Monitor *mon);
int sspRich_PrintMonitor(int id, int fiber);

int sspRich_SetCTestSource(int id, int fiber, int src);
int sspRich_GetCTestSource(int id, int fiber, int *src);
int sspRich_SetSoftReset(int id, int fiber, int reset);
int sspRich_SetLookback(int id, int fiber, int lookback);
int sspRich_GetLookback(int id, int fiber, int *lookback);
int sspRich_SetWindow(int id, int fiber, int window);
int sspRich_GetWindow(int id, int fiber, int *window);
int sspRich_SetCTestAmplitude(int id, int fiber, int dac);
int sspRich_GetCTestAmplitude(int id, int fiber, int *dac);
int sspRich_SetTDCEnableChannelMask(int id, int fiber,
      unsigned int mask0, unsigned int mask1,
      unsigned int mask2, unsigned int mask3,
      unsigned int mask4, unsigned int mask5);
int sspRich_GetTDCEnableChannelMask(int id, int fiber,
      unsigned int *mask0, unsigned int *mask1,
      unsigned int *mask2, unsigned int *mask3,
      unsigned int *mask4, unsigned int *mask5);
int sspRich_SetDeviceId(int id, int fiber, int devid);
int sspRich_GetBoardId(int id, int fiber, int *boardId);
int sspRich_ShiftMarocRegs(int id, int fiber, MAROC_Regs *regs_in, MAROC_Regs *regs_out);
int sspRich_SetMarocReg(int id, int fiber, int chip, int reg, int channel, int val);
int sspRich_GetMarocReg(int id, int fiber, int chip, int reg, int channel, int *val);

#define SSP_RICH_ASIC_TYPE_0MAROC         0
#define SSP_RICH_ASIC_TYPE_2MAROC         2
#define SSP_RICH_ASIC_TYPE_3MAROC         3
int sspRich_UpdateMarocRegs(int id, int fiber);

#define RICH_MAROC_REGS_WR      0
#define RICH_MAROC_REGS_RD      1
int sspRich_PrintMarocRegs(int id, int fiber, int chip, int type);

int sspRich_PrintFiberStatus(int id);
int sspRich_IsAsicInvalid(int id, int fiber);
int sspRich_PrintConnectedAsic_All();
int sspRich_ClearConnectedAsicList();
int sspRich_SaveConfig(const char * filename);
int sspRich_LoadConfig(const char * filename);

#endif
