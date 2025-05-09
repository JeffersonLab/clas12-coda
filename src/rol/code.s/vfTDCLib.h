/*----------------------------------------------------------------------------*
 *  Copyright (c) 2015        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Authors: Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Jefferson Lab VXS FPGA-Based Time to Digital Converter module library.
 *
 *----------------------------------------------------------------------------*/
#ifndef VFTDCLIB_H
#define VFTDCLIB_H

/* Define default Interrupt vector and level */
#define VFTDC_INT_VEC      0xec
#define VFTDC_INT_LEVEL    5

#define VFTDC_MAX_BOARDS             20
#define VFTDC_MAX_TDC_CHANNELS      192
#define VFTDC_MAX_DATA_PER_CHANNEL    8
#define VFTDC_MAX_A32_MEM      0x800000   /* 8 Meg */
#define VFTDC_MAX_A32MB_SIZE   0x800000  /*  8 MB */
#define VFTDC_VME_INT_LEVEL           3
#define VFTDC_VME_INT_VEC          0xFA

#define VFTDC_SUPPORTED_FIRMWARE_OLD 0x328
#define VFTDC_SUPPORTED_FIRMWARE     0x329

#ifndef VXWORKS
#include <pthread.h>

#else
/* #include <intLib.h> */
extern int intLock();
extern int intUnlock();
#endif

#ifdef VXWORKS
int intLockKeya;
#define INTLOCK {				\
    intLockKeya = intLock();			\
}

#define INTUNLOCK {				\
    intUnlock(intLockKeya);			\
}
#else
#define INTLOCK {				\
    vmeBusLock();				\
}
#define INTUNLOCK {				\
    vmeBusUnlock();				\
}
#endif

struct vfTDC_struct
{
  /** 0x0000 */ volatile unsigned int boardID;
  /** 0x0004 */ volatile unsigned int ptw;
  /** 0x0008 */ volatile unsigned int intsetup;
  /** 0x000C */ volatile unsigned int pl;
  /** 0x0010 */ volatile unsigned int adr32;
  /** 0x0014 */ volatile unsigned int blocklevel;
  /** 0x0018 */          unsigned int blank0;
  /** 0x001C */ volatile unsigned int vmeControl;
  /** 0x0020 */ volatile unsigned int trigsrc;
  /** 0x0024 */ volatile unsigned int sync;
  /** 0x0028 */ volatile unsigned int busy;
  /** 0x002C */ volatile unsigned int clock;
  /** 0x0030 */ volatile unsigned int trig1_scaler;
  /** 0x0034 */          unsigned int blank1[(0x4C-0x34)/4];
  /** 0x004C */ volatile unsigned int blockBuffer;
  /** 0x0050 */ volatile unsigned int trig2_scaler;
  /** 0x0054 */ volatile unsigned int sync_scaler;
  /** 0x0058 */ volatile unsigned int berr_scaler;
  /** 0x005C */ volatile unsigned int status;
  /** 0x0060 */ volatile unsigned int input_enable[6];
  /** 0x0078 */ volatile unsigned int ref_input;
  /** 0x007C */          unsigned int blank2[(0x9C-0x7C)/4];
  /** 0x009C */ volatile unsigned int runningMode;
  /** 0x00A0 */          unsigned int blank3[(0xA8-0xA0)/4];
  /** 0x00A8 */ volatile unsigned int livetime;
  /** 0x00AC */ volatile unsigned int busytime;
  /** 0x00B0 */          unsigned int blank4[(0xD8-0xB0)/4];
  /** 0x00D8 */ volatile unsigned int eventNumber_hi;
  /** 0x00DC */ volatile unsigned int eventNumber_lo;
  /** 0x00E0 */          unsigned int blank5[(0xEC-0xE0)/4];
  /** 0x00EC */ volatile unsigned int rocEnable;
  /** 0x00F0 */          unsigned int blank6[(0x100-0xF0)/4];
  /** 0x0100 */ volatile unsigned int reset;

};

/* 0x0 boardID bits and masks */
#define VFTDC_BOARDID_TYPE_VFTDC        0xF7DC
#define VFTDC_BOARDID_TYPE_MASK     0xFFFF0000
#define VFTDC_BOARDID_GEOADR_MASK   0x00001F00
#define VFTDC_BOARDID_CRATEID_MASK  0x000000FF

/* 0x4 ptw bits and masks */
#define VFTDC_PTW_MASK  0x000000FF

/* 0x8 intsetup bits and masks */
#define VFTDC_INTSETUP_VECTOR_MASK   0x000000FF
#define VFTDC_INTSETUP_LEVEL_MASK    0x00000F00
#define VFTDC_INTSETUP_ENABLE        (1<<16)

/* 0xC pl bits and masks (Sergey: 3FF->7FF for new firmware) */
#define VFTDC_PL_MASK   0x000007FF

/* 0x10 adr32 bits and masks */
#define VFTDC_ADR32_MBLK_ADDR_MAX_MASK  0x000003FE
#define VFTDC_ADR32_MBLK_ADDR_MIN_MASK  0x003FC000
#define VFTDC_ADR32_BASE_MASK           0xFF800000

/* 0x1C vmeControl bits and masks */
#define VFTDC_VMECONTROL_BERR           (1<<0)
#define VFTDC_VMECONTROL_TOKEN_TESTMODE (1<<1)
#define VFTDC_VMECONTROL_MBLK           (1<<2)
#define VFTDC_VMECONTROL_A32M           (1<<3)
#define VFTDC_VMECONTROL_A32            (1<<4)
#define VFTDC_VMECONTROL_ERROR_INT      (1<<7)
#define VFTDC_VMECONTROL_I2CDEV_HACK    (1<<8)
#define VFTDC_VMECONTROL_TOKENOUT_HI    (1<<9)
#define VFTDC_VMECONTROL_FIRST_BOARD    (1<<10)
#define VFTDC_VMECONTROL_LAST_BOARD     (1<<11)
#define VFTDC_VMECONTROL_BUFFER_DISABLE (1<<15)

/* 0x20 trigsrc bits and masks */
#define VFTDC_TRIGSRC_SOURCEMASK       0x0000FFFF
#define VFTDC_TRIGSRC_VXS              (1<<0)
#define VFTDC_TRIGSRC_HFBR1            (1<<1)
#define VFTDC_TRIGSRC_FP               (1<<3)
#define VFTDC_TRIGSRC_VME              (1<<4)
#define VFTDC_TRIGSRC_PULSER           (1<<7)
#define VFTDC_TRIGSRC_MONITOR_MASK     0xFFFF0000

/* 0x24 sync bits and masks */
#define VFTDC_SYNC_SOURCEMASK              0x0000FFFF
#define VFTDC_SYNC_VXS                     (1<<0)
#define VFTDC_SYNC_HFBR1                   (1<<1)
#define VFTDC_SYNC_FP                      (1<<3)
#define VFTDC_SYNC_VME                     (1<<4)
#define VFTDC_SYNC_MONITOR_MASK            0xFF000000

/* 0x28 busy bits and masks */
#define VFTDC_BUSY_SOURCEMASK      0x0000FFFF
#define VFTDC_BUSY_SWA              (1<<0)
#define VFTDC_BUSY_SWB              (1<<1)
#define VFTDC_BUSY_P2               (1<<2)
#define VFTDC_BUSY_FP_FTDC          (1<<3)
#define VFTDC_BUSY_FP_FADC          (1<<4)
#define VFTDC_BUSY_FP               (1<<5)
#define VFTDC_BUSY_LOOPBACK         (1<<7)
#define VFTDC_BUSY_HFBR1            (1<<8)
#define VFTDC_BUSY_HFBR2            (1<<9)
#define VFTDC_BUSY_HFBR3            (1<<10)
#define VFTDC_BUSY_HFBR4            (1<<11)
#define VFTDC_BUSY_HFBR5            (1<<12)
#define VFTDC_BUSY_HFBR6            (1<<13)
#define VFTDC_BUSY_HFBR7            (1<<14)
#define VFTDC_BUSY_HFBR8            (1<<15)
#define VFTDC_BUSY_MONITOR_FIFOFULL (1<<16)
#define VFTDC_BUSY_MONITOR_MASK     0xFFFF0000

/* 0x2C clock bits and mask  */
#define VFTDC_CLOCK_HFBR1        (0)
#define VFTDC_CLOCK_INTERNAL_250 (1)
#define VFTDC_CLOCK_INTERNAL_125 (2)
#define VFTDC_CLOCK_INTERNAL     VFTDC_CLOCK_INTERNAL_250
#define VFTDC_CLOCK_VXS          (3)
#define VFTDC_CLOCK_MASK         0x00000003

/* 0x34 (sergey: 0x4c ??) blockBuffer bits and masks */
#define VFTDC_BLOCKBUFFER_BLOCKS_READY_MASK     0x0000FF00
#define VFTDC_BLOCKBUFFER_BREADY_INT_MASK       0x00FF0000
#define VFTDC_BLOCKBUFFER_TRIGGERS_IN_BLOCK     0xFF000000

/* 0x5c Status bits and masks */
#define VFTDC_STATUS_BERR                         (1<<0)
#define VFTDC_STATUS_TOKEN                        (1<<1)
#define VFTDC_STATUS_BERR_N                       (1<<2)
#define VFTDC_STATUS_TAKE_TOKEN                   (1<<3)
#define VFTDC_STATUS_READ_TOKEN_OUT               (1<<4)
#define VFTDC_STATUS_DONE_BLOCK                   (1<<5)
#define VFTDC_STATUS_BERR_STATUS                  (1<<6)
#define VFTDC_STATUS_FIRST_BUFFER_FULL_A          (1<<8)
#define VFTDC_STATUS_FIRST_BUFFER_FULL_B          (1<<9)
#define VFTDC_STATUS_FIRST_BUFFER_EMPTY_A         (1<<10)
#define VFTDC_STATUS_FIRST_BUFFER_EMPTY_B         (1<<11)
#define VFTDC_STATUS_SECOND_BUFFER_FULL_A         (1<<12)
#define VFTDC_STATUS_SECOND_BUFFER_FULL_B         (1<<13)
#define VFTDC_STATUS_SECOND_BUFFER_ALMOST_FULL_B  (1<<14)
#define VFTDC_STATUS_SECOND_BUFFER_EMPTY_B        (1<<15)
#define VFTDC_STATUS_SECOND_BUFFER_ALMOST_FULL_A  (1<<16)
#define VFTDC_STATUS_FIRMWARE_REV_MASK            0x00F00000
#define VFTDC_STATUS_FIRMWARE_VERS_MASK           0x3F000000
#define VFTDC_STATUS_FIRMWARE_VERSION_MASK        0x3FF00000
#define VFTDC_STATUS_BOARD_TYPE_MASK              0xC0000000
#define VFTDC_BOARD_TYPE_192_CHANNELS             (3)
#define VFTDC_BOARD_TYPE_144_CHANNELS             (2)
#define VFTDC_BOARD_TYPE_96_CHANNELS              (1)
#define VFTDC_BOARD_TYPE_STREAM                   (0)

/* 0x78 ref_input bits and masks */
#define VFTDC_REFINPUT_INPUT5_LOGIC_ENABLE     (1<<4)
#define VFTDC_REFINPUT_LOGIC_WIDTH_MASK    0x0000FF00

/* 0x9C runningMode settings */
#define VFTDC_RUNNINGMODE_ENABLE          0xF7
#define VFTDC_RUNNINGMODE_CALIB_P2_AD     0xF8
#define VFTDC_RUNNINGMODE_CALIB_P2_CD     0xF9
#define VFTDC_RUNNINGMODE_CALIB_FP_A      0xFA
#define VFTDC_RUNNINGMODE_CALIB_FP_B      0xFB
#define VFTDC_RUNNINGMODE_CALIB_FP_C      0xFC
#define VFTDC_RUNNINGMODE_CALIB_FP_D      0xFD
#define VFTDC_RUNNINGMODE_DISABLE         0x00

/* 0xD8 eventNumber_hi bits and masks */
#define VFTDC_EVENTNUMBER_HI_MASK        0xFFFF0000

/* 0xEC rocEnable bits and masks */
#define VFTDC_ROCENABLE_MASK             0x000000FF
#define VFTDC_ROCENABLE_ROC(x)           (1<<(x))

/* 0x100 reset bits and masks */
#define VFTDC_RESET_I2C                  (1<<1)
#define VFTDC_RESET_SOFT                 (1<<4)
#define VFTDC_RESET_SYNCRESET            (1<<5)
#define VFTDC_RESET_BUSYACK              (1<<7)
#define VFTDC_RESET_CLK250               (1<<8)
#define VFTDC_RESET_MGT                  (1<<10)
#define VFTDC_RESET_AUTOALIGN_HFBR1_SYNC (1<<11)
#define VFTDC_RESET_TRIGGER              (1<<12)
#define VFTDC_RESET_IODELAY              (1<<14)
#define VFTDC_RESET_TAKE_TOKEN           (1<<16)
#define VFTDC_RESET_BLOCK_READOUT        (1<<17)
#define VFTDC_RESET_SCALERS_LATCH        (1<<24)
#define VFTDC_RESET_SCALERS_RESET        (1<<25)

/* faInit initialization flag bits */
#define VFTDC_INIT_SOFT_SYNCRESET      (0<<0)
#define VFTDC_INIT_HFBR1_SYNCRESET     (1<<0)
#define VFTDC_INIT_VXS_SYNCRESET       (2<<0)
#define VFTDC_INIT_SYNCRESETSRC_MASK   0x3

#define VFTDC_INIT_SOFT_TRIG           (0<<2)
#define VFTDC_INIT_HFBR1_TRIG          (1<<2)
#define VFTDC_INIT_VXS_TRIG            (2<<2)
#define VFTDC_INIT_FP_TRIG             (3<<2)
#define VFTDC_INIT_TRIGSRC_MASK        0x1C

#define VFTDC_INIT_INT_CLKSRC_250      (0<<5)
#define VFTDC_INIT_HFBR1_CLKSRC        (1<<5)
#define VFTDC_INIT_VXS_CLKSRC          (2<<5)
#define VFTDC_INIT_INT_CLKSRC_125      (3<<5)
#define VFTDC_INIT_INT_CLKSRC          VFTDC_INIT_INT_CLKSRC_250
#define VFTDC_INIT_CLKSRC_MASK         0x60

#define VFTDC_INIT_SKIP                (1<<16)
#define VFTDC_INIT_USE_ADDRLIST        (1<<17)
#define VFTDC_INIT_SKIP_FIRMWARE_CHECK (1<<18)

/* vfTDCBlockError values */
#define VFTDC_BLOCKERROR_NO_ERROR          0
#define VFTDC_BLOCKERROR_TERM_ON_WORDCOUNT 1
#define VFTDC_BLOCKERROR_UNKNOWN_BUS_ERROR 2
#define VFTDC_BLOCKERROR_ZERO_WORD_COUNT   3
#define VFTDC_BLOCKERROR_DMADONE_ERROR     4
#define VFTDC_BLOCKERROR_NTYPES            5

/* Data types and masks */
#define VFTDC_DUMMY_DATA             0xf800f7dc
#define VFTDC_DATA_TYPE_DEFINE       0x80000000
#define VFTDC_DATA_TYPE_MASK         0x78000000

#define VFTDC_DATA_BLOCK_HEADER      0x00000000
#define VFTDC_DATA_BLOCK_TRAILER     0x08000000
#define VFTDC_DATA_BLKNUM_MASK       0x0000003f

struct vftdc_data_struct
{
  unsigned int new_type;
  unsigned int type;
  unsigned int slot_id_hd;
  unsigned int slot_id_tr;
  unsigned int slot_id_evh;
  unsigned int n_evts;
  unsigned int blk_num;
  unsigned int modID;
  unsigned int PL;
  unsigned int n_words;
  unsigned int evt_num_1;
  unsigned int evt_num_2;
  unsigned int time_now;
  unsigned int time_1;
  unsigned int time_2;
  unsigned int time_3;
  unsigned int time_4;
  unsigned int group;
  unsigned int chan;
  unsigned int edge_type;
  unsigned int time_coarse;
  unsigned int two_ns;
  unsigned int time_fine;
};

/* Function prototypes */
STATUS vfTDCInit(UINT32 addr, UINT32 addr_inc, int ntdc, int iFlag);
int  vfTDCCheckAddresses();
void vfTDCStatus(int id, int pflag);
int  vfTDCGetFirmwareVersion(int id);
int  vfTDCReset(int id);
int  vfTDCSetBlockLevel(int id, int blockLevel);
int  vfTDCSetTriggerSource(int id, unsigned int trigmask);
int  vfTDCSetSyncSource(int id, unsigned int sync);
int  vfTDCSoftTrig(int id);
int  vfTDCSetWindowParamters(int id, int latency, int width);
int  vfTDCReadBlockStatus(int pflag);
int  vfTDCReadBlock(int id, volatile UINT32 *data, int nwrds, int rflag);
int  vfTDCEnableBusError(int id);
int  vfTDCDisableBusError(int id);
int  vfTDCSyncReset(int id);
int  vfTDCSetAdr32(int id, unsigned int a32base);
int  vfTDCDisableA32(int id);
int  vfTDCResetEventCounter(int id);
unsigned long long int vfTDCGetEventCounter(int id);
unsigned int vfTDCBReady(int id);
int  vfTDCSetClockSource(int id, unsigned int source);
int  vfTDCGetClockSource(int id);
int  vfTDCGetGeoAddress(int id);

int  vfTDCSetLogicInputChannelMask(int id, int conn, unsigned int chanmask);
unsigned int vfTDCGetLogicRefChannelMask(int id, int conn);
int  vfTDCSetLogicOutputWidth(int id, unsigned int width);
int  vfTDCGetLogicOutputWidth(int id);
int  vfTDCSetExtraLogicInput(int id, int enable);
int  vfTDCGetExtraLogicInput(int id);

void vfTDCDataDecode(unsigned int data);

/*sergey*/
int vfTDCSlot(unsigned int i);
void vfTDCSetA32BaseAddress(unsigned int addr);


#endif /* VFTDCLIB_H */
