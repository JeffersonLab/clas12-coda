/* vscmLib.h */

#ifndef VSCM_H
#define VSCM_H

#ifdef VXWORKS
typedef unsigned int uintptr_t;
#else
#include <stdint.h>
#endif

#ifndef CODA3DMA
#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))
#endif

/* Macros to help with register spacers */
#define MERGE_(a,b)  a##b
#define LABELVSCM_(a) MERGE_(uint32_t vscmblank, a)
#define BLANKVSCM LABELVSCM_(__LINE__)


#define VSCM_MAX_BOARDS     20
#define VSCM_BOARD_ID       0x5653434D
#define VSCM_MAX_FIFO       0x800000 /* 8MB */
#define VSCM_MAX_A32MB_SIZE 0x800000

#define VSCM_SYS_CLK    125000000L

/* Need to update this to ensure FSSR BCO clock number 
   is deskewed to VSCM bco clock number */
#define FSSR_SCR_BCONUM_START 240

#define DATA_TYPE_BLKHDR    0x00
#define DATA_TYPE_BLKTLR    0x01
#define DATA_TYPE_EVTHDR    0x02
#define DATA_TYPE_TRGTIME   0x03
#define DATA_TYPE_BCOTIME   0x04
#define DATA_TYPE_FSSREVT   0x08
#define DATA_TYPE_DNV       0x0E
#define DATA_TYPE_FILLER    0x0F

#define FSSR_SEL_HFCB1_U1   0
#define FSSR_SEL_HFCB1_U2   1
#define FSSR_SEL_HFCB1_U3   2
#define FSSR_SEL_HFCB1_U4   3
#define FSSR_SEL_HFCB2_U1   4
#define FSSR_SEL_HFCB2_U2   5
#define FSSR_SEL_HFCB2_U3   6
#define FSSR_SEL_HFCB2_U4   7
#define FSSR_SEL_ALL        8

#define FSSR_CMD_READ     0x4
#define FSSR_CMD_WRITE    0x1
#define FSSR_CMD_SET      0x2
#define FSSR_CMD_RESET    0x5
#define FSSR_CMD_DEFAULT  0x6

#define FSSR_ADDR_REG_DISC_VTN    0x07
#define FSSR_ADDR_REG_DISC_THR0   0x08
#define FSSR_ADDR_REG_DISC_THR1   0x09
#define FSSR_ADDR_REG_DISC_THR2   0x0A
#define FSSR_ADDR_REG_DISC_THR3   0x0B
#define FSSR_ADDR_REG_DISC_THR4   0x0C
#define FSSR_ADDR_REG_DISC_THR5   0x0D
#define FSSR_ADDR_REG_DISC_THR6   0x0E
#define FSSR_ADDR_REG_DISC_THR7   0x0F
#define FSSR_ADDR_CAPSEL          0x0D
#define FSSR_ADDR_REG_ALINES      0x10
#define FSSR_ADDR_REG_KILL        0x11
#define FSSR_ADDR_REG_INJECT      0x12
#define FSSR_ADDR_REG_SENDDATA    0x13
#define FSSR_ADDR_REG_REJECTHITS  0x14
#define FSSR_ADDR_REG_WILDREG     0x15
#define FSSR_ADDR_REG_SPR         0x18
#define FSSR_ADDR_REG_DCR         0x1B
#define FSSR_ADDR_REG_SCR         0x1C
#define FSSR_ADDR_REG_AQBCO       0x1E

#define FSSR_ALINES_1     0x0
#define FSSR_ALINES_2     0x1
#define FSSR_ALINES_4     0x2
#define FSSR_ALINES_6     0x3

#define FSSR_DCR_MOD256   0x10

#define IO_MUX_0                  0
#define IO_MUX_1                  1
#define IO_MUX_PULSER             2
#define IO_MUX_FPINPUT0           3
#define IO_MUX_FPINPUT1           4
#define IO_MUX_FPINPUT2           5
#define IO_MUX_FPINPUT3           6
#define IO_MUX_SWB_SYNC           7
#define IO_MUX_SWB_TRIG1          8
#define IO_MUX_SWB_TRIG2          9
#define IO_MUX_BUSY               14
#define IO_MUX_FSSRHIT            15
#define IO_MUX_DACTRIGGERED       16
#define IO_MUX_DACTRIGGERED_DLY   17
#define IO_MUX_BCOCLK             18
#define IO_MUX_FSSRHIT_TRIG       21
#define IO_MUX_FSSRHIT_TBAND_TRIG 22

#define FSSR_H1_U1_IDX    0
#define FSSR_H1_U2_IDX    1
#define FSSR_H1_U3_IDX    2
#define FSSR_H1_U4_IDX    3
#define FSSR_H2_U1_IDX    4
#define FSSR_H2_U2_IDX    5
#define FSSR_H2_U3_IDX    6
#define FSSR_H2_U4_IDX    7

#define FNLEN     128       /* length of config. file name */
#define ROCLEN     80       /* length of ROC_name */

typedef struct
{
/* 0x0000 */ volatile uint32_t FirmwareRev;
/* 0x0004 */ volatile uint32_t BoardID;
/* 0x0008 */ volatile uint32_t SpiCtrl;
/* 0x000C */ volatile uint32_t SpiStatus;
/* 0x0010 */ volatile uint32_t Reboot;
/* 0x0014 */ volatile uint32_t SpiRev2;
/* 0x0018 */ BLANKVSCM[(0x20-0x18)/4];
/* 0x0020 */ volatile uint32_t SemHeartbeatCnt;
/* 0x0024 */ volatile uint32_t SemErrorCnt;
/* 0x0028 */ BLANKVSCM[(0x100-0x28)/4];
} VSCM_CFG_REGS;

typedef struct
{
/* 0x0000 */ volatile uint32_t Ctrl;
/* 0x0004 */ volatile uint32_t Status;
/* 0x0008 */ volatile uint32_t DrpCtrl;
/* 0x000C */ volatile uint32_t DrpStatus;
/* 0x0010 */ BLANKVSCM[(0x100-0x10)/4];
} VSCM_CLKRST_REGS;

#define VSCM_SCALER_SYSCLK50      0
#define VSCM_SCALER_GCLK125       1
#define VSCM_SCALER_SYNC          2
#define VSCM_SCALER_TRIG1         3
#define VSCM_SCALER_TRIG2         4
#define VSCM_SCALER_TOKENIN       5
#define VSCM_SCALER_BUSY          6
#define VSCM_SCALER_FP_INPUT(n)   (7+n)
#define VSCM_SCALER_FP_OUTPUT(n)  (11+n)

typedef struct
{
/* 0x0000 */ volatile uint32_t TrgHitCtrl;
/* 0x0004 */ volatile uint32_t DacTrigCtrl;
/* 0x0008 */ volatile uint32_t TrigCtrl;
/* 0x000C */ volatile uint32_t SyncCtrl;
/* 0x0010 */ volatile uint32_t TrigoutCtrl;
/* 0x0014 */ volatile uint32_t FpOutputCtrl[4];
/* 0x0024 */ BLANKVSCM[(0x80-0x24)/4];
/* 0x0080 */ volatile uint32_t PulserPeriod;
/* 0x0084 */ volatile uint32_t PulserLowCycles;
/* 0x0088 */ volatile uint32_t PulserNPulses;
/* 0x008C */ volatile uint32_t PulserStart;
/* 0x0090 */ volatile uint32_t PulserDone;
/* 0x0094 */ BLANKVSCM[(0x100-0x94)/4];
/* 0x0104 */ volatile uint32_t ScalerLatch;
/* 0x0108 */ volatile uint32_t Scalers[15];
/* 0x0140 */ BLANKVSCM[(0x200-0x140)/4];
} VSCM_SD_REGS;

/* Event Builder */
#define VSCM_A32_ENABLE        0x00000001
#define VSCM_AMB_ENABLE        0x02000000
#define VSCM_A32_ADDR_MASK     0x0000ff80   /* 8 MB chunks */
#define VSCM_AMB_MIN_MASK      0x0000ff80
#define VSCM_AMB_MAX_MASK      0xff800000

//#define DCRB_ENABLE_BLKLVL_INT      0x40000
#define VSCM_ENABLE_BERR               0x01
#define VSCM_ENABLE_MULTIBLOCK    0x2000000
#define VSCM_FIRST_BOARD          0x4000000
#define VSCM_LAST_BOARD           0x8000000

typedef struct
{
/* 0x0000 */ volatile uint32_t Lookback;
/* 0x0004 */ volatile uint32_t WindowWidth;
/* 0x0008 */ volatile uint32_t BlockCfg;
/* 0x000C */ volatile uint32_t AD32;
/* 0x0010 */ volatile uint32_t Adr32M;
/* 0x0014 */ volatile uint32_t Interrupt;
/* 0x0018 */ volatile uint32_t ReadoutCfg;
/* 0x001C */ volatile uint32_t ReadoutStatus;
/* 0x0020 */ volatile uint32_t FifoBlockCnt;
/* 0x0024 */ volatile uint32_t FifoWordCnt;
/* 0x0028 */ volatile uint32_t FifoEventCnt;
/* 0x002C */ volatile uint32_t TrigCntBusyThr;
/* 0x0030 */ BLANKVSCM[(0x100-0x30)/4];
} VSCM_EB_REGS;

typedef struct
{
/* 0x0000 */ volatile uint32_t SerCtrl;
/* 0x0004 */ volatile uint32_t AddrH1;
/* 0x0008 */ volatile uint32_t AddrH2;
/* 0x000C */ BLANKVSCM[(0x10-0xC)/4];
/* 0x0010 */ volatile uint32_t SerData[4];
/* 0x0020 */ volatile uint32_t SerClk;
/* 0x0024 */ volatile uint32_t ClkCtrl;
/* 0x0028 */ volatile uint32_t Status;
/* 0x002C */ BLANKVSCM[(0x100-0x2C)/4];
} VSCM_FSSR_CTRL_REGS;

typedef struct
{
/* 0x0000 */ volatile uint32_t Ctrl;
/* 0x0004 */ volatile uint32_t HistCtrl;
/* 0x0008 */ volatile uint32_t HistCnt;
/* 0x000C */ volatile uint32_t Eye[7];
/* 0x0028 */ BLANKVSCM[(0x40-0x28)/4];
/* 0x0040 */ volatile uint32_t LastDataWord;
/* 0x0044 */ volatile uint32_t LastStatusWord;
/* 0x0048 */ volatile uint32_t ScalerStatusWord;
/* 0x004C */ volatile uint32_t ScalerEvent;
/* 0x0050 */ volatile uint32_t ScalerWords;
/* 0x0054 */ volatile uint32_t ScalerIdle;
/* 0x0058 */ volatile uint32_t ScalerAqBco;
/* 0x005C */ volatile uint32_t ScalerMarkErr;
/* 0x0060 */ volatile uint32_t ScalerEncErr;
/* 0x0064 */ volatile uint32_t ScalerChipIdErr;
/* 0x0068 */ volatile uint32_t ScalerGotHit;
/* 0x006C */ volatile uint32_t ScalerCoreTalking;
/* 0x0070 */ BLANKVSCM[(0x100-0x70)/4];
} VSCM_FSSR_REGS;

typedef struct
{
/* 0x0000 */ volatile uint32_t Ctrl;
/* 0x0004 */ volatile uint32_t Status;
/* 0x0008 */ volatile uint32_t Ch0;
/* 0x000C */ volatile uint32_t Ch1;
/* 0x0010 */ volatile uint32_t TrigCtrl;
/* 0x0014 */ BLANKVSCM[(0x100-0x14)/4];
} VSCM_DAC_REGS;

typedef struct
{
/* 0x0000 */ volatile uint32_t Ctrl;
/* 0x0004 */ volatile uint32_t DeadCycles;
/* 0x0008 */ BLANKVSCM[(0xC-0x8)/4];
/* 0x000C */ volatile uint32_t TrgHitWidth;
/* 0x0010 */ BLANKVSCM[(0x100-0x10)/4];
} VSCM_TDC_REGS;

typedef struct
{
/* 0x0000-0x00FF */ VSCM_CFG_REGS       Cfg;
/* 0x0100-0x01FF */ VSCM_CLKRST_REGS    Clk;
/* 0x0200-0x03FF */ VSCM_SD_REGS        Sd;
/* 0x0400-0x04FF */ VSCM_EB_REGS        Eb;
/* 0x0500-0x05FF */ VSCM_FSSR_CTRL_REGS FssrCtrl;
/* 0x0600-0x0FFF */ BLANKVSCM[(0x1000-0x0600)/4];
/* 0x1000-0x17FF */ VSCM_FSSR_REGS      Fssr[8];
/* 0x1800-0x1FFF */ BLANKVSCM[(0x2000-0x1800)/4];
/* 0x2000-0x20FF */ VSCM_DAC_REGS       Dac;
/* 0x2100-0x21FF */ VSCM_TDC_REGS       Tdc;
} VSCM_regs;

/* Firmware Function Prototypes */
int vscmSlot(unsigned int id);
int vscmId(unsigned int slot);
int vscmGetSpiMode(int id);
void vscmSelectSpi(int id, int sel, int mode);
uint8_t vscmTransferSpi(int id, uint8_t data, int mode);
void vscmFlashGetID(int id, uint8_t *rsp, int mode);
uint8_t vscmFlashGetStatus(int id, int mode);
void vscmReloadFirmware(int id);
int vscmFirmwareUpdate(int id, const char *filename);
int vscmFirmwareVerify(int id, const char *filename);
int vscmFirmwareUpdateVerify(int id, const char *filename);
int vscmFirmwareRead(int id, const char *filename);
uint32_t vscmFirmwareRev(int id);
void vscmFirmware(char *filename, int slot);


int vscmInit(uintptr_t addr, uint32_t addr_inc, int numvscm, int flag);
int vscmIsNotInit(int *id, const char *func);
void vscmResetToken(int id);

void vscmSetClockSource(int id, int clock_int_ext);
int  vscmGetClockSource(int id);
int  vscmGetBCOFreq(int id);
int  vscmSetTriggerWindowWidth(int id);
int  vscmSetTriggerWindowOffset(int id);
void vscmInitGlobals();
int  vscmReadConfigFile(char *filename);
int  vscmDownloadAll();
void vscmSetExpid(char *string);
int  vscmConfig(char *fname);
void vscmMon(int slot);
int  vscmUploadAll(char *string, int length);
int  vscmUploadAllPrint();


/* Ready Functions */
uint32_t vscmDReady(int id);
int vscmBReady(int id);
uint32_t vscmGBReady();

int vscmGetSerial(int id);
int vscmConfigDownload(int id, char *fname);
void vscmStat(int id);
void vscmGStat();
uint32_t vscmGetInputTriggers(int id);
uint32_t vscmGetAcceptedTriggers(int id);
void vscmFifoClear(int id);
int vscmFifoStatus(int id);
void vscmSetHitMask(int id, uint8_t mask, uint8_t trig_width);
uint8_t vscmGetHitMask(int id);
uint8_t vscmGetHitMaskWidth(int id);

void vscmRebootFpga(int id);
void vscmGRebootFpga();



/* Pulser Functions */
void vscmSetPulserRate(int id, uint32_t freq);
uint32_t vscmGetPulserRate(int id);
void vscmPulser(int id, int ch, uint32_t amp, uint32_t num_pulses);
void vscmPulserStart(int id);
void vscmPulserStop(int id);
void vscmPulserDelay(int id, uint8_t delay);
void vscmPulserBCOSync(int id, uint8_t bco, int sync);
/* Don't call this externally */
uint8_t vscmSetDacCalibration(int id);

/* Scaler Functions */
int  vscmReadScalers(int id, volatile unsigned int *data, int nwrds, int rflag, int rmode);
void vscmEnableScaler(int id);
void vscmDisableScaler(int id);
void vscmClearStripScalers(int id, int chip);
int vscmReadStripScalers(int id, int chip, uint32_t *arr);
void vscmLatchScalers(int id, int latch);
uint32_t vscmReadVmeClk(int id);

void vscmSetBCOFreq(int id, uint32_t freq);
int vscmReadBlock(int id, volatile uintptr_t *data, int nwrds, int rflag);
void vscmSetTriggerWindow(int id, \
                          uint32_t windowSize, \
                          uint32_t windowLookback, \
                          uint32_t bcoFreq);
void vscmSetBlockLevel(int id, int block_level);
void vscmSWSync(int id);
void fssrMasterReset(int id);
char *readNormalizedScaler(char *buf, char *prefix, \
                                uint32_t ref, uint32_t scaler);
int  vscmGSendScalers();

void fssrStatusAll();
void fssrStatus(int id, int chip);
uint32_t fssrReadLastStatusWord(int id, int chip);
uint32_t fssrReadScalerStatusWord(int id, int chip);
uint32_t fssrReadScalerEvent(int id, int chip);
uint32_t fssrReadScalerWords(int id, int chip);
uint32_t fssrReadScalerIdle(int id, int chip);
uint32_t fssrReadScalerAqBco(int id, int chip);
uint32_t fssrReadScalerMarkErr(int id, int chip);
uint32_t fssrReadScalerEncErr(int id, int chip);
uint32_t fssrReadScalerChipIdErr(int id, int chip);
uint32_t fssrReadLatencyMax(int id, int chip);
uint32_t fssrReadScalerGotHit(int id, int chip);
uint32_t fssrReadScalerStrip(int id, int chip);
uint32_t fssrReadScalerRef(int id);
uint32_t fssrReadScalerStripRef(int id, int chip);
uint32_t fssrReadScalerCoreTalking(int id, int chip);
uint32_t fssrReadLastDataWord(int id, int chip);

void fssrSetChipID(int id, \
                   unsigned int hfcb, \
                   unsigned int u1, \
                   unsigned int u2, \
                   unsigned int u3, \
                   unsigned int u4);
void fssrSetControl(int id, int chip, uint8_t mask);
uint8_t fssrGetControl(int id, int chip);
void fssrSetThreshold(int id, int chip, int idx, uint8_t thr);
uint8_t fssrGetThreshold(int id, int chip, uint8_t idx);
void fssrSetVtn(int id, int chip, uint8_t thr);
uint8_t fssrGetVtn(int id, int chip);
uint8_t fssrGetBCONum(int id, int chip);
uint8_t fssrGetBCONumOffset(int id, int chip, uint8_t offset);
uint8_t fssrGetBCONumNoSync(int id, int chip);

int fssrChipIDTest(int id, int chip);
int fssrRegisterTest(int id, int chip);
int fssrDiffLineTest(int id, int chip);

int fssrWaitReady(int id);
void fssrTransfer(int id, uint8_t chip, uint8_t reg, uint8_t cmd, \
                    uint8_t nBits, uint32_t *pData);
/** Mask Functions **/
/* Should only call these functions */
void fssrGetKillMask(int id, int chip, uint32_t *mask);
void fssrKillMaskDisableAllChips(int id);
void fssrKillMaskDisableAll(int id, int chip);
void fssrKillMaskEnableAll(int id, int chip);
void fssrKillMaskDisableSingle(int id, int chip, int chan);
void fssrKillMaskEnableSingle(int id, int chip, int chan);

void fssrGetInjectMask(int id, int chip, uint32_t *mask);
void fssrInjectMaskDisableAll(int id, int chip);
void fssrInjectMaskDisableAllChips(int id);
void fssrInjectMaskEnableSingle(int id, int chip, int chan);
/* These are helper functions that shouldn't be called externally */
int fssrSetMask(int id, int chip, int reg, uint32_t *mask);
void fssrGetMask(int id, int chip, int reg, uint32_t *mask);
void fssrMaskSingle(int id, int chip, int reg, int chan, int boolean);
int fssrMaskCompare(uint32_t *mask, uint32_t *readmask);
/** End of Mask Functions **/

void fssrSetActiveLines(int id, int chip, unsigned int lines);
void fssrRejectHits(int id, int chip, int reject);
void fssrSendData(int id, int chip, int send);
void fssrSCR(int id, int chip);
void fssrInternalPulserEnable(int id, int chip);
void fssrSetInternalPulserAmp(int id, int chip, uint8_t mask);
uint8_t fssrGetInternalPulserAmp(int id, int chip);

void fssrSetActiveLines_Asic(int id, int chip, unsigned int lines);
void vscmPrestart(char *fname);

void fssrGainScan(int id, char *filename,
                  int beg_chip, int end_chip,
                  int beg_chan, int end_chan,
				  int start_thr, int chan_mult);

int fssrParseControl(int id, int chip, char *s);
uint32_t fssrGetChipID(int id, int chip);

// FSSR2 Mclk phase control interface
void fssrEyeStatus(int id, int chip);
void fssrEyeSetup(int id, int chip);
void vscmMclkReset(int id, int rst);
int vscmMclkLocked(int id);
void vscmMclkDrpWrite(int id, int addr, int val);
int vscmMclkDrpRead(int id, int addr);

#endif

