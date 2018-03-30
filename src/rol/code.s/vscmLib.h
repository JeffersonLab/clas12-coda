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
#define IO_MUX_SWA_GPIO0          10
#define IO_MUX_SWA_GPIO1          11
#define IO_MUX_SWB_GPIO0          12
#define IO_MUX_SWB_GPIO1          13
#define IO_MUX_BUSY               14
#define IO_MUX_FSSRHIT            15
#define IO_MUX_DACTRIGGERED       16
#define IO_MUX_DACTRIGGERED_DLY   17
#define IO_MUX_BCOCLK             18
#define IO_MUX_TOKENOUT           19
#define IO_MUX_TOKENIN            20
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

typedef struct {
/* 0x0000-0x0003 */ uint32_t LastStatusWord;
/* 0x0004-0x0007 */ uint32_t ScalerStatusWord;
/* 0x0008-0x000B */ uint32_t ScalerEvent;
/* 0x000C-0x000F */ uint32_t ScalerWords;
/* 0x0010-0x0013 */ uint32_t ScalerIdle;
/* 0x0014-0x0017 */ uint32_t ScalerAqBco;
/* 0x0018-0x001B */ uint32_t ScalerMarkErr;
/* 0x001C-0x001F */ uint32_t ScalerEncErr;
/* 0x0020-0x0023 */ uint32_t ScalerChipIdErr;
/* 0x0024-0x0027 */ uint32_t LatencyMax;
/* 0x0028-0x002B */ uint32_t ScalerGotHit;
/* 0x002C-0x002F */ uint32_t ScalerStrip;
/* 0x0030-0x0033 */ uint32_t ScalerRef;
/* 0x0034-0x0037 */ uint32_t ScalerStripRef;
/* 0x0038-0x003B */ uint32_t ScalerCoreTalking;
/* 0x003C-0x003F */ uint32_t LastDataWord;
/* 0x0040-0x00FF */ uint32_t Dummy0[(0x0100-0x0040)/4];
} FssrStatus;

struct VSCM_regs {
/* 0x0000-0x0003 */ volatile uint32_t FirmwareRev;
/* 0x0004-0x0007 */ volatile uint32_t BoardID;
/* 0x0008-0x000B */ volatile uint32_t Sem;
/* 0x000C-0x000F */ volatile uint32_t Dummy0[(0x0010-0x000C)/4];
/* 0x0010-0x0013 */ volatile uint32_t ICap;
/* 0x0014-0x0017 */ volatile uint32_t SpiFlash;
/* 0x0018-0x001B */ volatile uint32_t TokenInCfg;
/* 0x001C-0x001F */ volatile uint32_t TokenOutCfg;
/* 0x0020-0x0023 */ volatile uint32_t SdLinkCfg;
/* 0x0024-0x0027 */ volatile uint32_t TrigOutCfg;
/* 0x0028-0x002B */ volatile uint32_t BlockCfg;
/* 0x002C-0x002F */ volatile uint32_t Dummy2[(0x0030-0x002C)/4];
/* 0x0030-0x0033 */ volatile uint32_t ClockCfg;
/* 0x0034-0x0037 */ volatile uint32_t Adr32M;
/* 0x0038-0x003B */ volatile uint32_t TrigLatency;
/* 0x003C-0x003F */ volatile uint32_t TrigBusyThr;
/* 0x0040-0x0043 */ volatile uint32_t FifoBlockCnt;
/* 0x0044-0x0047 */ volatile uint32_t AD32;
/* 0x0048-0x004B */ volatile uint32_t Interrupt;
/* 0x004C-0x004F */ volatile uint32_t InterruptAck;
/* 0x0050-0x0053 */ volatile uint32_t Geo;
/* 0x0054-0x0057 */ volatile uint32_t FifoWordCnt;
/* 0x0058-0x005B */ volatile uint32_t FifoEventCnt;
/* 0x005C-0x005F */ volatile uint32_t ReadoutCfg;
/* 0x0060-0x0063 */ volatile uint32_t SramDbgAdr;
/* 0x0064-0x0067 */ volatile uint32_t SramDbgData;
/* 0x0068-0x006B */ volatile uint32_t Reset;
/* 0x006C-0x006F */ volatile uint32_t FssrClkCfg;
/* 0x0070-0x0077 */ volatile uint32_t Dummy4[(0x0078-0x0070)/4];
/* 0x0078-0x007B */ volatile uint32_t ScalerLatch;
/* 0x007C-0x007F */ volatile uint32_t Dummy5[(0x0080-0x007C)/4];
/* 0x0080-0x0083 */ volatile uint32_t FssrSerCfg;
/* 0x0084-0x0093 */ volatile uint32_t FssrSerData[4];
/* 0x0094-0x0097 */ volatile uint32_t DacCfg;
/* 0x0098-0x009B */ volatile uint32_t DacCh0;
/* 0x009C-0x009F */ volatile uint32_t DacCh1;
/* 0x00A0-0x00A3 */ volatile uint32_t FssrSerClk;
/* 0x00A4-0x00A7 */ volatile uint32_t FssrAddrH1;
/* 0x00A8-0x00AB */ volatile uint32_t FssrAddrH2;
/* 0x00AC-0x00AF */ volatile uint32_t FssrHitReg;
/* 0x00B0-0x00FF */ volatile uint32_t Dummy6[(0x0100-0x00B0)/4];
/* 0x0100-0x0103 */ volatile uint32_t Trigger;
/* 0x0104-0x0107 */ volatile uint32_t Sync;
/* 0x0108-0x010B */ volatile uint32_t SwAGpio;
/* 0x010C-0x010F */ volatile uint32_t SwBGpio;
/* 0x0110-0x011F */ volatile uint32_t Dummy7[(0x0120-0x0110)/4];
/* 0x0120-0x0123 */ volatile uint32_t PulserPeriod;
/* 0x0124-0x0127 */ volatile uint32_t PulserHigh;
/* 0x0128-0x0137 */ volatile uint32_t FpInput[4];
/* 0x0138-0x0147 */ volatile uint32_t FpOutput[4];
/* 0x0148-0x014B */ volatile uint32_t TriggerWindow;
/* 0x014C-0x014F */ volatile uint32_t DACTrigger;
/* 0x0150-0x0153 */ volatile uint32_t PulserN;
/* 0x0154-0x0157 */ volatile uint32_t PulserStart;
/* 0x0158-0x015B */ volatile uint32_t PulserStatus;
/* 0x015C-0x0FBF */ volatile uint32_t Dummy8[(0x0FC0-0x015C)/4];
/* 0x0FC0-0x0FC3 */ volatile uint32_t ScalerTrigger;
/* 0x0FC4-0x0FC7 */ volatile uint32_t ScalerTriggerAccepted;
/* 0x0FC8-0x0FD7 */ volatile uint32_t ScalerFpOutput[4];
/* 0x0FD8-0x0FE7 */ volatile uint32_t ScalerFpInput[4];
/* 0x0FE8-0x0FEB */ volatile uint32_t ScalerBusy;
/* 0x0FEC-0x0FEF */ volatile uint32_t ScalerBusyCycles;
/* 0x0FF0-0x0FF3 */ volatile uint32_t ScalerVmeClk;
/* 0x0FF4-0x0FF7 */ volatile uint32_t ScalerSync;
/* 0x0FF8-0x0FFB */ volatile uint32_t ScalerTrig1;
/* 0x0FFC-0x0FFF */ volatile uint32_t ScalerTrig2;
/* 0x1000-0x10FF */ volatile FssrStatus Fssr[8];
};

/* Declare Global Variables 
extern int nvscm;
extern const int vscmA32Base;
extern int vscmA32Offset;
extern int vscmA24Offset;
extern int vscmA16Offset;
extern volatile struct VSCM_regs *VSCMpr[VSCM_MAX_BOARDS + 1];
extern volatile uintptr_t *VSCMpf[VSCM_MAX_BOARDS + 1];
extern int vscmID[VSCM_MAX_BOARDS];
extern int vscmInited;
extern int minSlot;
extern int maxSlot;
*/

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
uint32_t vscmGetInputTriggers(int id);
uint32_t vscmGetAcceptedTriggers(int id);
void vscmFifoClear(int id);
int vscmFifoStatus(int id);
void vscmSetHitMask(int id, uint8_t mask, uint8_t trig_width);
uint8_t vscmGetHitMask(int id);
uint8_t vscmGetHitMaskWidth(int id);




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
void vscmEnableChipScaler(int id, int chip);
void vscmDisableChipScaler(int id, int chip);
void vscmLatchChipScaler(int id, int chip);
void vscmClearStripScalers(int id, int chip);
int vscmReadStripScalers(int id, int chip, uint32_t *arr);
void vscmLatchScalers(int id);
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
uint32_t fssrReadScalerRef(int id, int chip);
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
#endif
