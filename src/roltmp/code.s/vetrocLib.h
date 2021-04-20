/******************************************************************************
*
*  vetrocLib.h  - Driver library header file for readout of the vetroc boards
*
*  Author: Ben Raydo
*          Jefferson Lab Data Acquisition Group
*
*/

#ifndef __VETROCLIB__
#define __VETROCLIB__

#define VETROC_PULSER_FREQ_MIN        0.01
#define VETROC_PULSER_FREQ_MAX        62.5E6
#define VETROC_NS_PER_CLOCK           8

#define VETROC_MAX_LOOKBACK     8191
#define VETROC_MAX_WINDOW       8191
#define VETROC_MAX_DEADTIME      255
#define VETROC_DEFAULT_LOOKBACK 2000
#define VETROC_DEFAULT_WINDOW   1000
#define VETROC_DEFAULT_DEADTIME   32

#define VETROC_BOARD_ID              0x56695452
#define VETROC_MAX_BOARDS            20
#define VETROC_MAX_TDC_CHANNELS      128
#define VETROC_MAX_DATA_PER_CHANNEL  512
#define VETROC_MAX_A32_MEM           0x800000  /* 8 Meg */
#define VETROC_MAX_A32MB_SIZE        0x800000  /* 8 MB */
#define VETROC_VME_INT_LEVEL         3     
#define VETROC_VME_INT_VEC           0xDC

/* Define Init Flag bits */
#define VETROC_SOURCE_MASK           0xFFFF
#define VETROC_SOURCE_VXS            (VETROC_CLK_SOURCE_VXS | VETROC_SYNC_SOURCE_VXS | VETROC_TRIG_SOURCE_VXS)
#define VETROC_SOURCE_INT            (VETROC_CLK_SOURCE_INT | VETROC_SYNC_SOURCE_SW | VETROC_TRIG_SOURCE_FP1)

#define VETROC_CLK_SOURCE_INT        0x0000
#define VETROC_CLK_SOURCE_VXS        0x0100
#define VETROC_CLK_SOURCE_MASK       0x0100

#define VETROC_SYNC_SOURCE_SW         0x0000
#define VETROC_SYNC_SOURCE_VXS        0x0001
#define VETROC_SYNC_SOURCE_MASK       0x000F

#define VETROC_TRIG_SOURCE_SW         0x0000
#define VETROC_TRIG_SOURCE_VXS        0x0010
#define VETROC_TRIG_SOURCE_FP1        0x0020
#define VETROC_TRIG_SOURCE_MASK       0x00F0

/* Define Data Types and Masks */
#define VETROC_DATA_TYPE_DEFINE       0x80000000
#define VETROC_DATA_TYPE_MASK         0x78000000
#define VETROC_DATA_SLOT_MASK         0x07c00000
#define VETROC_BLOCK_LEVEL_MASK       0x7ff

#define VETROC_DATA_BLOCK_HEADER      0x00000000
#define VETROC_DATA_BLOCK_TRAILER     0x08000000
#define VETROC_DATA_EVENT_HEADER      0x10000000
#define VETROC_DATA_TRIGGER_TIME      0x18000000
#define VETROC_DATA_INVALID           0x70000000
#define VETROC_DATA_FILLER            0x78000000
#define VETROC_DUMMY_DATA             0xf800fafa
#define VETROC_DATA_TDCEVT            0x40000000

#define VETROC_DATA_BLKNUM_MASK       0x0000003f
#define VETROC_DATA_WRDCNT_MASK       0x003fffff
#define VETROC_DATA_TRIGNUM_MASK      0x07ffffff

typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int EnableN;
  /* 0x0004-0x0007 */ volatile unsigned int TriggerWidth;
  /* 0x0008-0x007F */          unsigned int Reserved0[(0x0080-0x0008)/4];
  /* 0x0080-0x00BF */ volatile unsigned int Scalers[16];
  /* 0x00C0-0x00FF */          unsigned int Reserved1[(0x0100-0x00C0)/4];
} vetrocTdc_regs;

/* Clock Peripheral: Clock configuration interface */
#define VETROC_VERSION_MASK        0x0000ffff
#define VETROC_BOARD_MASK          0xffffffff
#define VETROC_SLOT_ID_MASK        0x1f000000
#define VETROC_SLOT_ID_SHIFT       24

#define VETROC_SPI_NCS_SET         0x100
#define VETROC_SPI_NCS_CLEAR       0x200
#define VETROC_SPI_TRANSFER        0x400
#define VETROC_SPI_READY           0x800

#define VETROC_FLASH_CMD_WRPAGE    0x02
#define VETROC_FLASH_CMD_RD        0x03
#define VETROC_FLASH_CMD_GETSTATUS 0x05
#define VETROC_FLASH_CMD_WREN      0x06
#define VETROC_FLASH_CMD_GETID     0x9F
#define VETROC_FLASH_CMD_ERASE64K  0xD8

/* actual length is 32*1024*1024, but driver currently only supports half, which is ok */
#define VETROC_FLASH_BYTE_LENGTH    16*1024*1024
#define VETROC_FLASH_DEVID_N25Q256A 0x1019BA20

#define VETROC_REF_CLK_RESET       0x80000000
#define VETROC_CLK_CTRL_REFA_SHIFT 22
#define VETROC_CLK_CTRL_REFB_SHIFT 24
#define VETROC_CLK_CTRL_FPGA_SHIFT 26
#define VETROC_CLK_CTRL_TD_SHIFT   28
#define VETROC_CLK_SRC_TI          0
#define VETROC_CLK_SRC_DISABLED    1
#define VETROC_CLK_SRC_LOCAL       2
#define VETROC_CLK_SRC_VXS         3

#define VETROC_CLK_SOFT_RESET      1

typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int BoardId;
  /* 0x0004-0x0007 */ volatile unsigned int FirmwareRev;
  /* 0x0008-0x000B */ volatile unsigned int Ctrl;
  /* 0x000C-0x000F */ volatile unsigned int Status;
  /* 0x0010-0x0013 */ volatile unsigned int SpiCtrl;
  /* 0x0014-0x0017 */ volatile unsigned int SpiStatus;
  /* 0x0018-0x001B */ volatile unsigned int Reset;
  /* 0x001C-0x00FF */          unsigned int Reserved0[(0x0100-0x001C)/4];
} vetrocClk_regs;

#define VETROC_SCALER_GCLK125		0
#define VETROC_SCALER_SYNC			1
#define VETROC_SCALER_TRIG			2
#define VETROC_SCALER_BUSYCYCLES	3
#define VETROC_SCALER_FPIN1		4
#define VETROC_SCALER_FPIN2		5
#define VETROC_SCALER_FPIN3		6
#define VETROC_SCALER_FPIN4		7
#define VETROC_SCALER_FPIN5		8
#define VETROC_SCALER_FPIN6		9
#define VETROC_SCALER_FPIN7		10
#define VETROC_SCALER_FPIN8		11
#define VETROC_SCALER_TOKENFI		12
#define VETROC_SCALER_SYNCFI		13
#define VETROC_SCALER_TRIG1F		14
#define VETROC_SCALER_TRIG2F		15
#define VETROC_SCALER_STATAIN		16
#define VETROC_SCALER_STATBIN		17
#define VETROC_SCALER_FPOUT1		18
#define VETROC_SCALER_FPOUT2		19
#define VETROC_SCALER_FPOUT3		20
#define VETROC_SCALER_FPOUT4		21
#define VETROC_SCALER_FPOUT5		22
#define VETROC_SCALER_FPOUT6		23
#define VETROC_SCALER_FPOUT7		24
#define VETROC_SCALER_FPOUT8		25
#define VETROC_SCALER_TOKENFO		26
#define VETROC_SCALER_TRIGFO		27
#define VETROC_SCALER_SDLINKF		28
#define VETROC_SCALER_STATAOUT	29
#define VETROC_SCALER_BUSYOUT		30

#define VETROC_OUTPUT_FPOUT1		0
#define VETROC_OUTPUT_FPOUT2		1
#define VETROC_OUTPUT_FPOUT3		2
#define VETROC_OUTPUT_FPOUT4		3
#define VETROC_OUTPUT_FPOUT5		4
#define VETROC_OUTPUT_FPOUT6		5
#define VETROC_OUTPUT_FPOUT7		6
#define VETROC_OUTPUT_FPOUT8		7
#define VETROC_OUTPUT_TOKENFO		8
#define VETROC_OUTPUT_TRIGFO		9
#define VETROC_OUTPUT_SDLINKF		10
#define VETROC_OUTPUT_STATAOUT	11
#define VETROC_OUTPUT_BUSYOUT		12
#define VETROC_OUTPUT_TRIG			13
#define VETROC_OUTPUT_SYNC			14

#define VETROC_SD_MUX_0				0
#define VETROC_SD_MUX_1				1
#define VETROC_SD_MUX_FPIN1		2
#define VETROC_SD_MUX_FPIN2		3
#define VETROC_SD_MUX_FPIN3		4
#define VETROC_SD_MUX_FPIN4		5
#define VETROC_SD_MUX_FPIN5		6
#define VETROC_SD_MUX_FPIN6		7
#define VETROC_SD_MUX_FPIN7		8
#define VETROC_SD_MUX_FPIN8		9
#define VETROC_SD_MUX_TOKENFI		10
#define VETROC_SD_MUX_SYNCFI		11
#define VETROC_SD_MUX_TRIG1F		12
#define VETROC_SD_MUX_TRIG2F		13
#define VETROC_SD_MUX_STATAIN		14
#define VETROC_SD_MUX_STATBIN		15
#define VETROC_SD_MUX_PULSER		18
#define VETROC_SD_MUX_BUSY			19

/* SD Peripheral: Internal signal muxing, scalers, pulser */
typedef struct
{
  /* 0x0000-0x0007 */          unsigned int Reserved0[(0x0008-0x0000)/4];
  /* 0x0008-0x0043 */ volatile unsigned int SrcSel[15];
  /* 0x0044-0x007F */          unsigned int Reserved1[(0x0080-0x0044)/4];
  /* 0x0080-0x0083 */ volatile unsigned int PulserPeriod;
  /* 0x0084-0x0087 */ volatile unsigned int PulserLowCycles;
  /* 0x0088-0x008B */ volatile unsigned int PulserNPulses;
  /* 0x008C-0x008F */ volatile unsigned int PulserStart;
  /* 0x0090-0x0093 */ volatile unsigned int PulserDone;
  /* 0x0094-0x00FF */          unsigned int Reserved2[(0x0100-0x0094)/4];
  /* 0x0100-0x0103 */ volatile unsigned int ScalerLatch;
  /* 0x0104-0x0107 */          unsigned int Reserved3[(0x0108-0x0104)/4];
  /* 0x0108-0x0183 */ volatile unsigned int Scalers[31];
  /* 0x0184-0x01FF */          unsigned int Reserved4[(0x0200-0x0184)/4];
} vetrocSd_regs;

/* Serdes Peripheral: Fiber & VXS serdes controls and monitors */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x000C-0x0004 */          unsigned int Reserved0[(0x000C-0x0004)/4];
  /* 0x000C-0x000F */ volatile unsigned int DrpCtrl;
  /* 0x0010-0x0013 */ volatile unsigned int Status;
  /* 0x0014-0x0017 */ volatile unsigned int DrpStatus;
  /* 0x0018-0x00FF */          unsigned int Reserved1[(0x0100-0x0018)/4];
} vetrocSerdes_regs;

/* Event Builder */
#define VETROC_A32_ENABLE        0x00000001
#define VETROC_AMB_ENABLE        0x02000000
#define VETROC_A32_ADDR_MASK     0x0000ff80   /* 8 MB chunks */
#define VETROC_AMB_MIN_MASK      0x0000ff80
#define VETROC_AMB_MAX_MASK      0xff800000

//#define VETROC_ENABLE_BLKLVL_INT      0x40000
#define VETROC_ENABLE_BERR               0x01
#define VETROC_ENABLE_MULTIBLOCK    0x2000000
#define VETROC_FIRST_BOARD          0x4000000
#define VETROC_LAST_BOARD           0x8000000

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
  /* 0x002C-0x002F */ volatile unsigned int TrigFifoBusyThreshold;
  /* 0x0030-0x0033 */ volatile unsigned int SramAddr;
  /* 0x0034-0x0037 */ volatile unsigned int SramDinL;
  /* 0x0038-0x003B */ volatile unsigned int SramDinH;
  /* 0x003C-0x003F */ volatile unsigned int SramDoutL;
  /* 0x0040-0x0043 */ volatile unsigned int SramDoutH;
  /* 0x0044-0x0047 */ volatile unsigned int SramCtrl;
  /* 0x0048-0x00FF */ volatile unsigned int Reserved1[(0x0100-0x0048)/4];
} vetrocEB_regs;

/* DCRB memory structure */
typedef struct
{
  /* 0x0000-0x00FF */ vetrocClk_regs		Clk;
  /* 0x0100-0x01FF */ unsigned int			Reserved0[(0x0200-0x0100)/4];
  /* 0x0200-0x03FF */ vetrocSd_regs			Sd;
  /* 0x0400-0x04FF */ vetrocSerdes_regs	Ser;
  /* 0x0500-0x0FFF */ unsigned int			Reserved1[(0x1000-0x0500)/4];
  /* 0x1000-0x17FF */ vetrocTdc_regs		Tdc[8];
  /* 0x1800-0x1FFF */ unsigned int			Reserved2[(0x2000-0x1800)/4];
  /* 0x2000-0x20FF */ vetrocEB_regs			EB;
} vetroc_regs;



// /* Function Prototypes */
// 
int vetrocSlot(unsigned int id);
int vetrocId(unsigned int slot);
int vetrocTriggerPulseWidth(int id, unsigned int width);
int vetrocLinkReset(int id);

STATUS vetrocInit (UINT32 addr, UINT32 addr_inc, int nvetroc, int iFlag);
int vetrocSetClockSource(int id, int clkSrc);
void vetrocStatus(int id, int sflag);
void vetrocGStatus(int sflag);
int vetrocSetProcMode(int id, unsigned int lookBack, unsigned int windowWidth);
void vetrocGSetProcMode(unsigned int lookBack, unsigned int windowWidth);
int vetrocReadBlock(int id, volatile UINT32 *data, int nwrds, int rflag);
int vetrocReadBlockStatus(int id, volatile UINT32 *data, int nwrds, int rflag);
int vetrocPrintBlock(int id, int rflag);
void vetrocClear(int id);
void vetrocReset(int id, int iFlag);
void vetrocChanDisable(int id, int group, int mask);
int vetrocDready(int id, int dflag);
int vetrocBready(int id);
unsigned int vetrocGBready();
unsigned int vetrocScanMask();
void vetrocEnableBusError(int id);
void vetrocGEnableBusError();
void vetrocDisableBusError(int id);
void vetrocEnableMultiBlock();
void vetrocDisableMultiBlock();
int vetrocSetBlockLevel(int id, int level);
void vetrocGSetBlockLevel(int level);
void vetrocDataDecode(unsigned int data);
void vetrocGFirmwareUpdateVerify(const char *filename);
int vetrocPulserSetup(int id, float freq, float duty, unsigned int npulses);
void vetrocSync(int id);
void vetrocGSync(int id);
void vetrocTrig(int id);
void vetrocGStatus(int sflag);

int vetrocFirmwareUpdateVerify(int id, const char *filename);
int vetrocFirmwareUpdate(int id, const char *filename);
int vetrocFirmwareVerify(int id, const char *filename);


#endif /* __VETROCLIB__ */
