/****************************************************************************
 *
 *  dsc2Config.h  -  configuration library header file for dsc2 board 
 *
 *  Author: Serguei Pozdniakov
 *          Jefferson Lab Online Group
 *          September 2009
 */

/*
Register		Description			Offset	Reset_State
---------------------------------------------------------------------------
A_THRESHOLD_CH0		Threshold Control Ch0		0x0000	0x0FFF0000
...			Threshold Control ChX		...	...
A_THRESHOLD_CH15	Threshold Control Ch15		0x003C	0x0FFF0000
A_PULSEWIDTH		Pulse Width Control		0x0080	0x01A501A5
A_CH_ENABLE		Channel Control			0x0088	0xFFFFFFFF
A_OR_MASK		OR Output Control		0x008C
A_DELAY			Input/Output Delays		0x0090	0x00080008
A_TEST			Test Input Control		0x0094	0x00000001
A_VME_LATCH		VME Scaler Latch		0x0098	0xXXXXXXXX
A_LATCH			Gated Scaler Latch		0x009C	0xXXXXXXXX
A_TRG_SCALER_CH0	Ext. Gate Trigger Scaler Ch0	0x0100	0xXXXXXXXX
...			Ext. Gate Trigger Scaler ChX	...	0xXXXXXXXX
A_TRG_SCALER_CH15	Ext. Gate Trigger Scaler Ch15	0x013C	0xXXXXXXXX
A_TDC_SCALER_CH0	Ext. Gate TDC Scaler Ch0	0x0140	0xXXXXXXXX
...			Ext. Gate TDC Scaler ChX	...	0xXXXXXXXX
A_TDC_SCALER_CH15	Ext. Gate TDC Scaler Ch15	0x017C	0xXXXXXXXX
A_TRG_VME_SCALER_CH0	VME Gate Trigger Scaler Ch0	0x0180	0xXXXXXXXX
...			VME Gate Trigger Scaler ChX	...	0xXXXXXXXX
A_TRG_VME_SCALER_CH15	VME Gate Trigger Scaler Ch15	0x01BC	0xXXXXXXXX
A_TDC_VME_SCALER_CH0	VME Gate TDC Scaler Ch0		0x01C0	0xXXXXXXXX
...			VME Gate TDC Scaler ChX		...	0xXXXXXXXX
A_TDC_VME_SCALER_CH15	VME Gate TDC Scaler Ch15	0x01FC	0xXXXXXXXX
A_REF_SCALER		VME Gate Ref Scaler		0x0200	0xXXXXXXXX
A_REF_SCALER_GATE	Ext. Gate Ref Scaler		0x0204	0xXXXXXXXX
A_FIRMWARE_REV		Firmware Revision		0x0400	0xXXXXXXXX
A_BOARDID		Board Identifier		0x0404	0x44534332 = "DSC2" in ASCII
*/

#ifndef __DSC2CONFIG__
#define __DSC2CONFIG__





#define DSC2_BOARD_ID     0x44534332
#define DSC2_A32_ADDR     0x08000000


#define DSC2_TRG_THRESHOLD         0x03ff0000
#define DSC2_TDC_THRESHOLD         0x000003ff

#define DSC2_TRG_OUTPUT_WIDTH      0xf0000000
#define DSC2_TRG_PULSER_WIDTH      0x003f0000
#define DSC2_TDC_PULSER_WIDTH      0x0000003f

#define DSC2_TRG_MASK              0xffff0000
#define DSC2_TDC_MASK              0x0000ffff

#define DSC2_TRG_OUTPUT_DELAY      0x007f0000
#define DSC2_SCALER_INPUT_DELAY    0x0000007f

#define DSC2_FIRMWARE_REV_MAJ      0x0000ff00
#define DSC2_FIRMWARE_REV_MIN      0x000000ff


#ifndef INT16
#define INT16  short
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef INT32
#define INT32  int
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef STATUS
#define STATUS int
#endif

/* Function Prototypes */


int dsc2GetNdsc_daq();
int dsc2Slot_daq(unsigned int id);
int dsc2Id_daq(unsigned int slot);
int dsc2GetNdsc_tcp();
int dsc2Slot_tcp(unsigned int id);
int dsc2Id_tcp(unsigned int slot);
void dsc2SetExpid(char *string);

int  dsc2Config(char *fname);
void dsc2InitGlobals();
int  dsc2ReadConfigFile(char *filename);
int  dsc2DownloadAll();
void dsc2Mon(int slot);
int  dsc2UploadAll(char *string, int length);
int  dsc2UploadAllPrint();

int    dsc2DownloadConfig ();
int    dsc2SetConfig      (char *fname);
void   dsc2MonAll         ();
void   dsc2MonConfig      ();
void   dsc2MonBoard       (int board_number);

/*============================================================*/
int    dsc2GetBoardID          (UINT32 addr);
int    dsc2GetFirmwareRevMin   (UINT32 addr);
int    dsc2GetFirmwareRevMaj   (UINT32 addr);
char  *dsc2GetBoardRev         (UINT32 addr);
int    dsc2GetBoardSN          (UINT32 addr);
/*============================================================*/
void   dsc2MonAllThreshold     (UINT32 addr);
int    dsc2SetAllThreshold     (int TDC_thresh, int TRG_thresh);
int    dsc2GetTDCThreshold     (UINT32 addr, int channel);
int    dsc2GetTRGThreshold     (UINT32 addr, int channel);
int    dsc2SetTDCThreshold     (UINT32 addr, int channel, int thresh);
int    dsc2SetTRGThreshold     (UINT32 addr, int channel, int thresh);
int    dsc2SetUniTDCThreshold  (UINT32 addr, int thresh);
int    dsc2SetUniTRGThreshold  (UINT32 addr, int thresh);
/*============================================================*/
void   dsc2MonAllWidth         (UINT32 addr);
int    dsc2GetTDCPulserWidth   (UINT32 addr);
int    dsc2GetTRGPulserWidth   (UINT32 addr);
int    dsc2GetTRGOutputWidth   (UINT32 addr);
int    dsc2SetTDCPulserWidth   (UINT32 addr, int width);
int    dsc2SetTRGPulserWidth   (UINT32 addr, int width);
int    dsc2SetTRGOutputWidth   (UINT32 addr, int width);
/*============================================================*/
int    dsc2SetTRGandTDCmask    (UINT32 addr, UINT32 mask);
int    dsc2GetTRGandTDCmask    (UINT32 addr);
int    dsc2GetTDCmask          (UINT32 addr);
int    dsc2GetTRGmask          (UINT32 addr);
int    dsc2EnableTDCch         (UINT32 addr, int channel);
int    dsc2EnableTRGch         (UINT32 addr, int channel);
int    dsc2EnableAllTDCch      (UINT32 addr);
int    dsc2EnableAllTRGch      (UINT32 addr);
int    dsc2EnableAllAllch      (UINT32 addr);
int    dsc2DisableTDCch        (UINT32 addr, int channel);
int    dsc2DisableTRGch        (UINT32 addr, int channel);
int    dsc2DisableAllTDCch     (UINT32 addr);
int    dsc2DisableAllTRGch     (UINT32 addr);
int    dsc2DisableAllAllch     (UINT32 addr);
/*============================================================*/
int    dsc2SetORmask           (UINT32 addr, UINT32 mask);
int    dsc2GetORmask           (UINT32 addr);
int    dsc2GetORmaskTDC        (UINT32 addr);
int    dsc2GetORmaskTRG        (UINT32 addr);
int    dsc2EnableORchTDC       (UINT32 addr, int channel);
int    dsc2EnableORchTRG       (UINT32 addr, int channel);
int    dsc2EnableORAllchTDC    (UINT32 addr);
int    dsc2EnableORAllchTRG    (UINT32 addr);
int    dsc2EnableORAll         (UINT32 addr);
int    dsc2DisableORchTDC      (UINT32 addr, int channel);
int    dsc2DisableORchTRG      (UINT32 addr, int channel);
int    dsc2DisableORAllchTDC   (UINT32 addr);
int    dsc2DisableORAllchTRG   (UINT32 addr);
int    dsc2DisableORAll        (UINT32 addr);
/*============================================================*/
int    dsc2GetScalerInputDelay (UINT32 addr);
int    dsc2GetTRGOutputDelay   (UINT32 addr);
int    dsc2SetScalerInputDelay (UINT32 addr, int delay);
int    dsc2SetTRGOutputDelay   (UINT32 addr, int delay);
/*============================================================*/
int    dsc2LatchVMEScalers     (UINT32 addr);
int    dsc2LatchGatedScalers   (UINT32 addr);
UINT32 dsc2GetTRGscaler        (UINT32 addr, int channel);
UINT32 dsc2GetTDCscaler        (UINT32 addr, int channel);
UINT32 dsc2GetTRGscalerVME     (UINT32 addr, int channel);
UINT32 dsc2GetTDCscalerVME     (UINT32 addr, int channel);
UINT32 dsc2GetREFscalerVME     (UINT32 addr);
UINT32 dsc2GetREFscalerGATE    (UINT32 addr);
/*============================================================*/
UINT32 dsc2GetValidAddr    (UINT32 addr);
int    dsc2GetNumberByAddr (UINT32 addr);
UINT32 dsc2GetAddrByNumber (int board_number);


#endif /* __DSC2CONFIG__ */

