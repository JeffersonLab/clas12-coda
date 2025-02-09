/******************************************************************************
*
*  dcrbLib.h  - Driver library header file for readout of the dcrb boards
*
*  Author: Ben Raydo
*          Jefferson Lab Data Acquisition Group
*
*/

#ifndef __DCRBLIB__
#define __DCRBLIB__

#define DCRB_PULSER_FREQ_MIN		0.01
#define DCRB_PULSER_FREQ_MAX		62.5E6
#define DCRB_NS_PER_CLOCK           8

#define DCRB_MAX_LOOKBACK     16383
#define DCRB_MAX_WINDOW       16383
#define DCRB_MAX_DEADTIME      (255*8)
#define DCRB_DEFAULT_LOOKBACK 2000
#define DCRB_DEFAULT_WINDOW   1000
#define DCRB_DEFAULT_DEADTIME   32

#define DCRB_BOARD_ID       0x44438242
#define DCRB_MAX_BOARDS             21
#define DCRB_MAX_TDC_CHANNELS       96
#define DCRB_MAX_DATA_PER_CHANNEL  512
#define DCRB_MAX_A32_MEM      0x800000   /* 8 Meg */
#define DCRB_MAX_A32MB_SIZE   0x800000  /*  8 MB */
#define DCRB_VME_INT_LEVEL           3     
#define DCRB_VME_INT_VEC          0xDC
#define DCRB_MAX_DAC_VAL			 175

/* Define Init Flag bits for Clock Source */
#define DCRB_SOURCE_INT         0x00
#define DCRB_SOURCE_VXS         0x01
#define DCRB_SOURCE_MASK        0x01

/* Define ADC Data Types and Masks */
#define DCRB_DATA_TYPE_DEFINE       0x80000000
#define DCRB_DATA_TYPE_MASK         0x78000000
#define DCRB_DATA_SLOT_MASK         0x07c00000
#define DCRB_BLOCK_LEVEL_MASK 		0x7ff

#define DCRB_DATA_BLOCK_HEADER      0x00000000
#define DCRB_DATA_BLOCK_TRAILER     0x08000000
#define DCRB_DATA_EVENT_HEADER      0x10000000
#define DCRB_DATA_TRIGGER_TIME      0x18000000
#define DCRB_DATA_INVALID           0x70000000
#define DCRB_DATA_FILLER            0x78000000
#define DCRB_DUMMY_DATA             0xf800fafa
#define DCRB_DATA_TDCEVT            0x40000000

#define DCRB_DATA_BLKNUM_MASK       0x0000003f
#define DCRB_DATA_WRDCNT_MASK       0x003fffff
#define DCRB_DATA_TRIGNUM_MASK      0x07ffffff

#define FLASH_CMD_WRPAGE				0x02
#define FLASH_CMD_RD						0x03
#define FLASH_CMD_GETSTATUS			0x05
#define FLASH_CMD_WREN					0x06
#define FLASH_CMD_GETID					0x9F
#define FLASH_CMD_ERASE64K				0xD8
#define FLASH_CMD_WRBUF1				0x84
#define FLASH_CMD_PGBUF1ERASE			0x83
#define FLASH_CMD_GETSTATUS2			0xD7

#define FLASH_BYTE_LENGTH				8*1024*1024
#define FLASH_DEV_M25P64				0x10172020


#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */

/* Config Peripheral: Board information, fpga flash update */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int EnableN;
  /* 0x0004-0x0007 */ volatile unsigned int DeadCycles;
  /* 0x0008-0x000B */          unsigned int Reserved0[(0x000C-0x0008)/4];
  /* 0x000C-0x000F */ volatile unsigned int TriggerWidth;
  /* 0x0010-0x00FF */          unsigned int Reserved1[(0x0100-0x0010)/4];
  /* 0x0100-0x013F */ volatile unsigned int Scalers[16];
  /* 0x0140-0x01FF */          unsigned int Reserved2[(0x0200-0x0140)/4];
} dcrbTdc_regs;

/* Config Peripheral: Board information, fpga flash update */

#define DCRB_VERSION_MASK        0x0000ffff
#define DCRB_BOARD_MASK          0xffffffff
#define DCRB_SLOT_ID_MASK        0x1f000000
#define DCRB_SLOT_ID_SHIFT       24

#define DCRB_ICAP_WRITE           0x4
#define DCRB_ICAP_CLK             0x1
#define DCRB_ICAP_CE              0x2

#define DCRB_SPI_NCS_SET			0x100
#define DCRB_SPI_NCS_CLEAR			0x200
#define DCRB_SPI_TRANSFER			0x400
#define DCRB_SPI_READY				0x800

typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int BoardId;
  /* 0x0004-0x0007 */ volatile unsigned int FirmwareRev;
  /* 0x0008-0x000B */ volatile unsigned int SpiCtrl;
  /* 0x000C-0x000F */ volatile unsigned int SpiStatus;
  /* 0x0010-0x001F */          unsigned int Reserved0[(0x0020-0x0010)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Reset;
  /* 0x0024-0x0027 */ volatile unsigned int DacThreshold;
  /* 0x0028-0x002B */ volatile unsigned int DacLow;
  /* 0x002C-0x002F */ volatile unsigned int DacHigh;
  /* 0x0030-0x0033 */ volatile unsigned int DacOffset;
  /* 0x0034-0x003F */          unsigned int Reserved1[(0x0040-0x0034)/4];
  /* 0x0040-0x0043 */ volatile unsigned int Reboot;
  /* 0x0044-0x0047 */ volatile unsigned int HeartbeatCnt;
  /* 0x0048-0x004B */ volatile unsigned int SEUCnt;
  /* 0x004C-0x00FF */          unsigned int Reserved2[(0x0100-0x004C)/4];
} dcrbCfg_regs;

/* Clock Peripheral: Clock configuration interface */
#define DCRB_REF_CLK_INTERNAL        0x00000000
#define DCRB_REF_CLK_P0              0x40000000
#define DCRB_REF_CLK_MASK            0x40000000
#define DCRB_REF_CLK_RESET           0x80000000
#define DCRB_TDC_CLK_LOCKED          0x4
#define DCRB_VME_CLK_LOCKED          0x8

typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x0004-0x0007 */ volatile unsigned int Status;
  /* 0x0008-0x00FF */          unsigned int Reserved0[(0x0100-0x0008)/4];
} dcrbClk_regs;

#define DCRB_SCALER_SYSCLK50		0
#define DCRB_SCALER_GCLK125		1
#define DCRB_SCALER_SYNC			2
#define DCRB_SCALER_TRIG1			3
#define DCRB_SCALER_TRIG2			4
#define DCRB_SCALER_SWA_GPIO1		14
#define DCRB_SCALER_SWB_GPIO1		15
#define DCRB_SCALER_TOKENIN		16
#define DCRB_SCALER_BUSY			17

#define DCRB_OUTPUT_DCE0			0
#define DCRB_OUTPUT_DCE1			1
#define DCRB_OUTPUT_DCE2			2
#define DCRB_OUTPUT_FCE0			3
#define DCRB_OUTPUT_FCE1			4
#define DCRB_OUTPUT_FCE2			5
#define DCRB_OUTPUT_SWA_GPIO0		6
#define DCRB_OUTPUT_SWB_GPIO0		7
#define DCRB_OUTPUT_TRIGOUT		8
#define DCRB_OUTPUT_SDLINK			9
#define DCRB_OUTPUT_TOKENOUT		10
#define DCRB_OUTPUT_TOKENIN		11
#define DCRB_OUTPUT_TRIG			15
#define DCRB_OUTPUT_SYNC			16

#define DCRB_SD_MUX_0				0
#define DCRB_SD_MUX_1				1
#define DCRB_SD_MUX_SYNC			2
#define DCRB_SD_MUX_TRIG1			3
#define DCRB_SD_MUX_TRIG2			4
#define DCRB_SD_MUX_TOKENIN		5
#define DCRB_SD_MUX_SWAGPIO1		6
#define DCRB_SD_MUX_SWBGPIO1		7
#define DCRB_SD_MUX_SYSCLK_125	16
#define DCRB_SD_MUX_GCLK_125		17
#define DCRB_SD_MUX_PULSER			18
#define DCRB_SD_MUX_BUSY			19
#define DCRB_SD_MUX_TOKENOUT		20

/* SD Peripheral: Internal signal muxing, scalers, pulser */
typedef struct
{
  /* 0x0000-0x0043 */ volatile unsigned int SrcSel[17];
  /* 0x003C-0x007F */          unsigned int Reserved0[(0x0080-0x0044)/4];
  /* 0x0080-0x0083 */ volatile unsigned int PulserPeriod;
  /* 0x0084-0x0087 */ volatile unsigned int PulserLowCycles;
  /* 0x0088-0x008B */ volatile unsigned int PulserNPulses;
  /* 0x008C-0x008F */ volatile unsigned int PulserStart;
  /* 0x0090-0x0093 */ volatile unsigned int PulserDone;
  /* 0x0094-0x00FF */          unsigned int Reserved1[(0x0100-0x0094)/4];
  /* 0x0100-0x0103 */ volatile unsigned int ScalerLatch;
  /* 0x0104-0x018F */ volatile unsigned int Scalers[35];
  /* 0x0190-0x01FF */          unsigned int Reserved2[(0x0200-0x0190)/4];
} dcrbSd_regs;

/* Serdes Peripheral: Fiber & VXS serdes controls and monitors */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x0004-0x0007 */ volatile unsigned int CtrlTile0;
  /* 0x0008-0x000B */ volatile unsigned int CtrlTile1;
  /* 0x000C-0x000F */ volatile unsigned int DrpCtrl;
  /* 0x0010-0x0013 */ volatile unsigned int Status;
  /* 0x0014-0x0017 */ volatile unsigned int DrpStatus;
  /* 0x0018-0x001B */ volatile unsigned int ErrTile0;
  /* 0x001C-0x001F */ volatile unsigned int ErrTile1;
  /* 0x0020-0x0023 */ volatile unsigned int CrateId;
  /* 0x0024-0x002F */          unsigned int Reserved0[(0x0030-0x0024)/4];
  /* 0x0030-0x0033 */ volatile unsigned int MonCtrl;
  /* 0x0034-0x0037 */ volatile unsigned int MonStatus;
  /* 0x0038-0x003F */          unsigned int Reserved1[(0x0040-0x0038)/4];
  /* 0x0040-0x004B */ volatile unsigned int MonMask[3];
  /* 0x004C-0x005F */          unsigned int Reserved2[(0x0060-0x004C)/4];
  /* 0x0060-0x006B */ volatile unsigned int MonVal[3];
  /* 0x006C-0x007F */          unsigned int Reserved3[(0x0080-0x006C)/4];
  /* 0x0080-0x0083 */ volatile unsigned int MonThr[1];
  /* 0x0084-0x008F */          unsigned int Reserved4[(0x0090-0x0084)/4];
  /* 0x0090-0x009B */ volatile unsigned int MonData[3];
  /* 0x009C-0x00FF */          unsigned int Reserved5[(0x0100-0x009C)/4];
} dcrbSerdes_regs;

/* Event Builder */
#define DCRB_A32_ENABLE        0x00000001
#define DCRB_AMB_ENABLE        0x02000000
#define DCRB_A32_ADDR_MASK     0x0000ff80   /* 8 MB chunks */
#define DCRB_AMB_MIN_MASK      0x0000ff80
#define DCRB_AMB_MAX_MASK      0xff800000

//#define DCRB_ENABLE_BLKLVL_INT      0x40000
#define DCRB_ENABLE_BERR               0x01
#define DCRB_ENABLE_MULTIBLOCK    0x2000000
#define DCRB_FIRST_BOARD          0x4000000
#define DCRB_LAST_BOARD           0x8000000

typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Lookback;
  /* 0x0004-0x0007 */ volatile unsigned int WindowWidth;
  /* 0x0008-0x000B */ volatile unsigned int BlockCfg;
  /* 0x000C-0x000F */ volatile unsigned int AD32;
  /* 0x0010-0x0013 */ volatile unsigned int Adr32M;
  /* 0x0014-0x0017 */ volatile unsigned int Interrupt;
  /* 0x0018-0x001B */ volatile unsigned int ReadoutCfg;
  /* 0x001C-0x001F */ volatile unsigned int ReadoutStatus;
  /* 0x0020-0x0023 */ volatile unsigned int FifoBlockCnt;
  /* 0x0024-0x0027 */ volatile unsigned int FifoWordCnt;
  /* 0x0028-0x002B */ volatile unsigned int FifoEventCnt;
  /* 0x002C-0x002F */ volatile unsigned int TrigCntBusyThr;
  /* 0x0030-0x00FF */ volatile unsigned int Reserved0[(0x0100-0x0030)/4];
} dcrbEB_regs;

/* DCRB memory structure */
#define DCRB_CFG_RESET			0x1

typedef struct
{
  /* 0x0000-0x00FF */ dcrbCfg_regs		Cfg;
  /* 0x0100-0x01FF */ dcrbClk_regs		Clk;
  /* 0x0200-0x03FF */ dcrbSd_regs		Sd;
  /* 0x0400-0x04FF */ dcrbEB_regs		EB;
  /* 0x0500-0x06FF */ dcrbSerdes_regs	Ser[2];
  /* 0x0700-0x0FFF */ unsigned int		Reserved0[(0x1000-0x0700)/4];
  /* 0x1000-0x11FF */ dcrbTdc_regs		Tdc[6];
} dcrb_regs;


// /* Function Prototypes */
// 

int dcrbSlot(unsigned int id);
int dcrbId(unsigned int slot);
int dcrbInit (unsigned int addr, unsigned int addr_inc, int ndcrb, int iFlag);
int dcrbHardReset(int id);
int dcrbSetClockSource(int id, int clkSrc);
void dcrbStatus(int id, int sflag);
void dcrbGStatus(int sflag);
int dcrbSetProcMode(int id, unsigned int lookBack, unsigned int windowWidth, unsigned int deadTime);
void dcrbGSetProcMode(unsigned int lookBack, unsigned int windowWidth, unsigned int deadTime);
int dcrbReadBlock(int id, volatile unsigned int *data, int nwrds, int rflag);
int dcrbReadBlockStatus(int id, volatile unsigned int *data, int nwrds, int rflag);
int dcrbPrintBlock(int id, int rflag);
void dcrbClear(int id);
void dcrbReset(int id, int iFlag);
void dcrbChanDisable(int id, unsigned int cmask0, unsigned int cmask1, unsigned int cmask2);
int dcrbDready(int id, int dflag);
int dcrbBready(int id);
unsigned int dcrbGBready();
unsigned int dcrbScanMask();
void dcrbEnableBusError(int id);
void dcrbGEnableBusError();
void dcrbDisableBusError(int id);
void dcrbEnableMultiBlock();
void dcrbDisableMultiBlock();
int dcrbSetBlockLevel(int id, int level);
void dcrbGSetBlockLevel(int level);
int dcrbSetDAC(int id, int dvalue);
void dcrbGSetDAC(unsigned int dvalue);
void dcrbDataDecode(unsigned int data);
void dcrbGFirmwareUpdateVerify(const char *filename);
int dcrbSetDAC_Pulser(int id, int grp_mask, float freq, int offset_mV, int low_mV, int high_mV, int width);
int dcrbPulserSetup(int id, float freq, float duty, unsigned int npulses);
int dcrbSetTriggerPulseWidth(int id, unsigned int width);
int dcrbGetTriggerPulseWidth(int id);
int dcrbLinkStatus(int id);
int dcrbLinkReset(int id);
int dcrbEnableLinkErrorCounts(int id);

int dcrbGetDAC(int id);
int dcrbFirmwareUpdate(int id, const char *filename);
int dcrbFirmwareVerify(int id, const char *filename);
int dcrbFirmwareUpdateVerify(int id, const char *filename);

void dcrbSetExpid(char *string);
void dcrbInitGlobals();
int  dcrbReadConfigFile(char *filename);
int  dcrbDownloadAll();
int  dcrbConfig(char *fname);
void dcrbMon(int slot);
int  dcrbUploadAll(char *string, int length);
int  dcrbUploadAllPrint();


#endif /* __DCRBLIB__ */
