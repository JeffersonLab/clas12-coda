/*----------------------------------------------------------------------------*
 *  Copyright 2022, Jefferson Science Associates, LLC.
 *  Subject to the terms in the LICENSE file found in the top-level directory.
 *
 *    Authors: Bryan Moffit
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3
 *             Phone: (757) 269-5660             12000 Jefferson Ave.
 *             Fax:   (757) 269-5800             Newport News, VA 23606
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Primitive trigger control for Intel CPUs running Linux using the TJNAF
 *     Trigger Interface (TI) PCIexpress card Ultrascale+ version.
 *
 *----------------------------------------------------------------------------*/
#ifndef TIPCIEUS_H
#define TIPCIEUS_H

#define STATUS int
#define TRUE  1
#define FALSE 0
#define OK    0
#define ERROR -1
#ifndef _ROLDEFINED
typedef void            (*VOIDFUNCPTR) ();
typedef int             (*FUNCPTR) ();
#endif
typedef char            BOOL;

#include <pthread.h>

/*sergey pthread_mutex_t tiISR_mutex=PTHREAD_MUTEX_INITIALIZER;*/

#define INTLOCK
#define INTUNLOCK

struct TIPCIEUS_RegStruct
{
  /** 0x00000 */ volatile unsigned int boardID;
  /** 0x00004 */ volatile unsigned int fiber;
  /** 0x00008 */ volatile unsigned int intsetup;
  /** 0x0000C */ volatile unsigned int trigDelay;
  /** 0x00010 */ volatile unsigned int __adr32;      // NOT HERE
  /** 0x00014 */ volatile unsigned int blocklevel;
  /** 0x00018 */ volatile unsigned int dataFormat;
  /** 0x0001C */ volatile unsigned int vmeControl;
  /** 0x00020 */ volatile unsigned int trigsrc;
  /** 0x00024 */ volatile unsigned int sync;
  /** 0x00028 */ volatile unsigned int busy;
  /** 0x0002C */ volatile unsigned int clock;
  /** 0x00030 */ volatile unsigned int trig1Prescale;
  /** 0x00034 */ volatile unsigned int blockBuffer;
  /** 0x00038 */ volatile unsigned int triggerRule;
  /** 0x0003C */ volatile unsigned int triggerWindow;
  /** 0x00040 */          unsigned int blank0;
  /** 0x00044 */ volatile unsigned int tsInput;
  /** 0x00048 */          unsigned int blank1;
  /** 0x0004C */ volatile unsigned int output;
  /** 0x00050 */ volatile unsigned int fiberSyncDelay;
  /** 0x00054 */ volatile unsigned int dmaSetting;
  /** 0x00058 */ volatile unsigned int dmaAddr;
  /** 0x0005C */ volatile unsigned int pcieConfigLink;
  /** 0x00060 */ volatile unsigned int pcieConfigStatus;
  /** 0x00064 */ volatile unsigned int inputPrescale;
  /** 0x00068 */ volatile unsigned int fifo;
  /** 0x0006C */ volatile unsigned int pcieConfig;
  /** 0x00070 */ volatile unsigned int pcieDevConfig;
  /** 0x00074 */ volatile unsigned int pulserEvType;
  /** 0x00078 */ volatile unsigned int syncCommand;
  /** 0x0007C */ volatile unsigned int syncDelay;
  /** 0x00080 */ volatile unsigned int syncWidth;
  /** 0x00084 */ volatile unsigned int triggerCommand;
  /** 0x00088 */ volatile unsigned int randomPulser;
  /** 0x0008C */ volatile unsigned int fixedPulser1;
  /** 0x00090 */ volatile unsigned int fixedPulser2;
  /** 0x00094 */ volatile unsigned int nblocks;
  /** 0x00098 */ volatile unsigned int syncHistory;
  /** 0x0009C */ volatile unsigned int runningMode;
  /** 0x000A0 */ volatile unsigned int fiberLatencyMeasurement;
  /** 0x000A4 */ volatile unsigned int __fiberAlignment; // NOT HERE
  /** 0x000A8 */ volatile unsigned int livetime;
  /** 0x000AC */ volatile unsigned int busytime;
  /** 0x000B0 */ volatile unsigned int GTPStatusA;
  /** 0x000B4 */ volatile unsigned int GTPStatusB;
  /** 0x000B8 */ volatile unsigned int GTPtriggerBufferLength;
  /** 0x000BC */ volatile unsigned int inputCounter;
  /** 0x000C0 */ volatile unsigned int blockStatus[4];
  /** 0x000D0 */ volatile unsigned int adr24; // CHANGE NAME
  /** 0x000D4 */ volatile unsigned int syncEventCtrl;
  /** 0x000D8 */ volatile unsigned int eventNumber_hi;
  /** 0x000DC */ volatile unsigned int eventNumber_lo;
  /** 0x000E0 */          unsigned int blank4[(0xEC-0xE0)/4];
  /** 0x000EC */ volatile unsigned int rocEnable;
  /** 0x000F0 */          unsigned int blank5[(0xFC-0xF0)/4];
  /** 0x000FC */ volatile unsigned int blocklimit;
  /** 0x00100 */ volatile unsigned int reset;
  /** 0x00104 */ volatile unsigned int fpDelay[2];
  /** 0x0010C */          unsigned int blank6[(0x110-0x10C)/4];
  /** 0x00110 */          unsigned int __busy_scaler1[7]; // NOT HERE
  /** 0x0012C */          unsigned int blank7[(0x138-0x12C)/4];
  /** 0x00138 */ volatile unsigned int triggerRuleMin;
  /** 0x0013C */          unsigned int blank8;
  /** 0x00140 */ volatile unsigned int trigTable[(0x180-0x140)/4];
  /** 0x00180 */ volatile unsigned int ts_scaler[6];
  /** 0x00198 */          unsigned int blank9;
  /** 0x0019C */ volatile unsigned int __busy_scaler2[9]; // NOT HERE
  /** 0x001C0 */          unsigned int blank10[(0x1D0-0x1C0)/4];
  /** 0x001D0 */ volatile unsigned int hfbr_tiID[8];
  /** 0x001F0 */ volatile unsigned int master_tiID;
};

/* Define TI Modes of operation:     Ext trigger - Interrupt mode   0
                                     TS  trigger - Interrupt mode   1
                                     Ext trigger - polling  mode    2
                                     TS  trigger - polling  mode    3  */
#define TIPUS_READOUT_EXT_INT    0
#define TIPUS_READOUT_TS_INT     1
#define TIPUS_READOUT_EXT_POLL   2
#define TIPUS_READOUT_TS_POLL    3

/* Supported firmware version */
#define TIPUS_SUPPORTED_FIRMWARE 0x092
#define TIPUS_SUPPORTED_TYPE     3

/* Firmware Masks */
#define TIPUS_FIRMWARE_ID_MASK              0xFFFF0000
#define TIPUS_FIRMWARE_TYPE_MASK            0x0000F000
#define TIPUS_FIRMWARE_TYPE_REV2            0
#define TIPUS_FIRMWARE_TYPE_PROD            1
#define TIPUS_FIRMWARE_TYPE_MODTI           2
#define TIPUS_FIRMWARE_TYPE_PROD2           3
#define TIPUS_FIRMWARE_MAJOR_VERSION_MASK   0x00000FF0
#define TIPUS_FIRWMARE_MINOR_VERSION_MASK   0x0000000F

/* 0x0 boardID bits and masks */
#define TIPUS_BOARDID_TYPE_TIDS         0x71D5
#define TIPUS_BOARDID_TYPE_TI           0x7100
#define TIPUS_BOARDID_TYPE_TS           0x7500
#define TIPUS_BOARDID_TYPE_TD           0x7D00
#define TIPUS_BOARDID_TYPE_MASK     0xFF000000
#define TIPUS_BOARDID_PROD_MASK     0x00FF0000
#define TIPUS_BOARDID_GEOADR_MASK   0x00001F00
#define TIPUS_BOARDID_CRATEID_MASK  0x000000FF

/* 0x4 fiber bits and masks */
#define TIPUS_FIBER_1                        (1<<0)
#define TIPUS_FIBER_2                        (1<<1)
#define TIPUS_FIBER_3                        (1<<2)
#define TIPUS_FIBER_4                        (1<<3)
#define TIPUS_FIBER_5                        (1<<4)
#define TIPUS_FIBER_6                        (1<<5)
#define TIPUS_FIBER_7                        (1<<6)
#define TIPUS_FIBER_8                        (1<<7)
#define TIPUS_FIBER_ENABLE_P0                (1<<8)
#define TIPUS_FIBER_ENABLED(x)           (1<<(x+1))
#define TIPUS_FIBER_MASK                 0x000000FF
#define TIPUS_FIBER_CONNECTED_1             (1<<16)
#define TIPUS_FIBER_CONNECTED_2             (1<<17)
#define TIPUS_FIBER_CONNECTED_3             (1<<18)
#define TIPUS_FIBER_CONNECTED_4             (1<<19)
#define TIPUS_FIBER_CONNECTED_5             (1<<20)
#define TIPUS_FIBER_CONNECTED_6             (1<<21)
#define TIPUS_FIBER_CONNECTED_7             (1<<22)
#define TIPUS_FIBER_CONNECTED_8             (1<<23)
#define TIPUS_FIBER_CONNECTED_TI(x)     (1<<(x+15))
#define TIPUS_FIBER_CONNECTED_MASK       0x00FF0000
#define TIPUS_FIBER_TRIGSRC_ENABLED_1       (1<<24)
#define TIPUS_FIBER_TRIGSRC_ENABLED_2       (1<<25)
#define TIPUS_FIBER_TRIGSRC_ENABLED_3       (1<<26)
#define TIPUS_FIBER_TRIGSRC_ENABLED_4       (1<<27)
#define TIPUS_FIBER_TRIGSRC_ENABLED_5       (1<<28)
#define TIPUS_FIBER_TRIGSRC_ENABLED_6       (1<<29)
#define TIPUS_FIBER_TRIGSRC_ENABLED_7       (1<<30)
#define TIPUS_FIBER_TRIGSRC_ENABLED_8       (1<<31)
#define TIPUS_FIBER_TRIGSRC_ENABLED_TI(x) (1<<(x+23))
#define TIPUS_FIBER_TRIGSRC_ENABLED_MASK 0xFF000000

/* 0x8 intsetup bits and masks */
#define TIPUS_INTSETUP_VECTOR_MASK   0x000000FF
#define TIPUS_INTSETUP_LEVEL_MASK    0x00000F00
#define TIPUS_INTSETUP_ENABLE        (1<<16)

/* 0xC trigDelay bits and masks */
#define TIPUS_TRIGDELAY_TRIG1_DELAY_MASK 0x000000FF
#define TIPUS_TRIGDELAY_TRIG1_WIDTH_MASK 0x0000FF00
#define TIPUS_TRIGDELAY_TRIG2_DELAY_MASK 0x00FF0000
#define TIPUS_TRIGDELAY_TRIG2_WIDTH_MASK 0xFF000000
#define TIPUS_TRIGDELAY_TRIG1_64NS_STEP  (1<<7)
#define TIPUS_TRIGDELAY_TRIG2_64NS_STEP  (1<<23)

/* 0x10 adr32 bits and masks */
#define TIPUS_ADR32_MBLK_ADDR_MAX_MASK  0x000003FE
#define TIPUS_ADR32_MBLK_ADDR_MIN_MASK  0x003FC000
#define TIPUS_ADR32_BASE_MASK       0xFF800000

/* 0x14 blocklevel bits and masks */
#define TIPUS_BLOCKLEVEL_MASK           0x000000FF
#define TIPUS_BLOCKLEVEL_CURRENT_MASK   0x00FF0000
#define TIPUS_BLOCKLEVEL_RECEIVED_MASK  0xFF000000


/* 0x18 dataFormat bits and masks */
#define TIPUS_DATAFORMAT_TWOBLOCK_PLACEHOLDER (1<<0)
#define TIPUS_DATAFORMAT_TIMING_WORD          (1<<1)
#define TIPUS_DATAFORMAT_HIGHERBITS_WORD      (1<<2)
#define TIPUS_DATAFORMAT_FPINPUT_READOUT         (1<<3)
#define TIPUS_DATAFORMAT_BCAST_BUFFERLEVEL_MASK  0xFF000000


/* 0x1C vmeControl bits and masks */
#define TIPUS_VMECONTROL_BERR           (1<<0)
#define TIPUS_VMECONTROL_TOKEN_TESTMODE (1<<1)
#define TIPUS_VMECONTROL_MBLK           (1<<2)
#define TIPUS_VMECONTROL_A32M           (1<<3)
#define TIPUS_VMECONTROL_A32            (1<<4)
#define TIPUS_VMECONTROL_ERROR_INT      (1<<7)
#define TIPUS_VMECONTROL_I2CDEV_HACK    (1<<8)
#define TIPUS_VMECONTROL_TOKENOUT_HI    (1<<9)
#define TIPUS_VMECONTROL_FIRST_BOARD    (1<<10)
#define TIPUS_VMECONTROL_LAST_BOARD     (1<<11)
#define TIPUS_VMECONTROL_BUFFER_DISABLE (1<<15)
#define TIPUS_VMECONTROL_BLOCKLEVEL_UPDATE (1<<21)
#define TIPUS_VMECONTROL_USE_LOCAL_BUFFERLEVEL (1<<22)
#define TIPUS_VMECONTROL_BUSY_ON_BUFFERLEVEL   (1<<23)
#define TIPUS_VMECONTROL_DMA_DATA_ENABLE  (1<<26)
#define TIPUS_VMECONTROL_SLOWER_TRIGGER_RULES (1<<31)
#define TIPUS_VMECONTROL_DMASETTING_MASK 0x01c00000

/* 0x20 trigsrc bits and masks */
#define TIPUS_TRIGSRC_SOURCEMASK       0x0000F3FF
#define TIPUS_TRIGSRC_P0               (1<<0)
#define TIPUS_TRIGSRC_HFBR1            (1<<1)
#define TIPUS_TRIGSRC_LOOPBACK         (1<<2)
#define TIPUS_TRIGSRC_FPTRG            (1<<3)
#define TIPUS_TRIGSRC_VME              (1<<4)
#define TIPUS_TRIGSRC_TSINPUTS         (1<<5) /*controls input 19/20 (Trigger_in), NOT 6 TS# inputs; for those, see register 0x48 below */
#define TIPUS_TRIGSRC_TSREV2           (1<<6)
#define TIPUS_TRIGSRC_PULSER           (1<<7)
#define TIPUS_TRIGSRC_HFBR5            (1<<10)
#define TIPUS_TRIGSRC_TRIG21           (1<<11)
#define TIPUS_TRIGSRC_PART_1           (1<<12)
#define TIPUS_TRIGSRC_PART_2           (1<<13)
#define TIPUS_TRIGSRC_PART_3           (1<<14)
#define TIPUS_TRIGSRC_PART_4           (1<<15)
#define TIPUS_TRIGSRC_MONITOR_MASK     0xFFFF0000
#define TIPUS_TRIGSRC_FORCE_SEND       0x00FC0000

/* 0x24 sync bits and masks */
#define TIPUS_SYNC_SOURCEMASK              0x000000FF
#define TIPUS_SYNC_P0                      (1<<0)
#define TIPUS_SYNC_HFBR1                   (1<<1)
#define TIPUS_SYNC_HFBR5                   (1<<2)
#define TIPUS_SYNC_FP                      (1<<3)
#define TIPUS_SYNC_LOOPBACK                (1<<4)
#define TIPUS_SYNC_USER_SYNCRESET_ENABLED  (1<<7)
#define TIPUS_SYNC_HFBR1_CODE_MASK         0x00000F00
#define TIPUS_SYNC_HFBR5_CODE_MASK         0x0000F000
#define TIPUS_SYNC_LOOPBACK_CODE_MASK      0x000F0000
#define TIPUS_SYNC_HISTORY_FIFO_MASK       0x00700000
#define TIPUS_SYNC_HISTORY_FIFO_EMPTY      (1<<20)
#define TIPUS_SYNC_HISTORY_FIFO_HALF_FULL  (1<<21)
#define TIPUS_SYNC_HISTORY_FIFO_FULL       (1<<22)
#define TIPUS_SYNC_MONITOR_MASK            0xFF000000

/* 0x28 busy bits and masks */
#define TIPUS_BUSY_SOURCEMASK      0x0000FFFF
#define TIPUS_BUSY_SWA              (1<<0)
#define TIPUS_BUSY_SWB              (1<<1)
#define TIPUS_BUSY_P2               (1<<2)
#define TIPUS_BUSY_FP_FTDC          (1<<3)
#define TIPUS_BUSY_FP_FADC          (1<<4)
#define TIPUS_BUSY_FP               (1<<5)
#define TIPUS_BUSY_TRIGGER_LOCK     (1<<6)
#define TIPUS_BUSY_LOOPBACK         (1<<7)
#define TIPUS_BUSY_HFBR1            (1<<8)
#define TIPUS_BUSY_HFBR2            (1<<9)
#define TIPUS_BUSY_HFBR3            (1<<10)
#define TIPUS_BUSY_HFBR4            (1<<11)
#define TIPUS_BUSY_HFBR5            (1<<12)
#define TIPUS_BUSY_HFBR6            (1<<13)
#define TIPUS_BUSY_HFBR7            (1<<14)
#define TIPUS_BUSY_HFBR8            (1<<15)
#define TIPUS_BUSY_HFBR_MASK        0x0000FF00
#define TIPUS_BUSY_MONITOR_MASK     0xFFFF0000
#define TIPUS_BUSY_MONITOR_SWA      (1<<16)
#define TIPUS_BUSY_MONITOR_SWB      (1<<17)
#define TIPUS_BUSY_MONITOR_P2       (1<<18)
#define TIPUS_BUSY_MONITOR_FP_FTDC  (1<<19)
#define TIPUS_BUSY_MONITOR_FP_FADC  (1<<20)
#define TIPUS_BUSY_MONITOR_FP       (1<<21)
#define TIPUS_BUSY_MONITOR_TRIG_LOST (1<<22)
#define TIPUS_BUSY_MONITOR_LOOPBACK (1<<23)
#define TIPUS_BUSY_MONITOR_FIBER_BUSY(x) (1<<(x+23))
#define TIPUS_BUSY_MONITOR_HFBR1    (1<<24)
#define TIPUS_BUSY_MONITOR_HFBR2    (1<<25)
#define TIPUS_BUSY_MONITOR_HFBR3    (1<<26)
#define TIPUS_BUSY_MONITOR_HFBR4    (1<<27)
#define TIPUS_BUSY_MONITOR_HFBR5    (1<<28)
#define TIPUS_BUSY_MONITOR_HFBR6    (1<<29)
#define TIPUS_BUSY_MONITOR_HFBR7    (1<<30)
#define TIPUS_BUSY_MONITOR_HFBR8    (1<<31)

/* 0x2C clock bits and mask  */
#define TIPUS_CLOCK_INTERNAL    (0)
#define TIPUS_CLOCK_HFBR5       (1)
#define TIPUS_CLOCK_HFBR1       (2)
#define TIPUS_CLOCK_FP          (3)
#define TIPUS_CLOCK_BRIDGE      (0x50000)
#define TIPUS_CLOCK_MASK        0x3/*sergey: was 0x0000000F*/
/*sergey on William's update:*/
#define TIPUS_CLOCK_OUTPUT_SET_16MHZ   0x800
#define TIPUS_CLOCK_OUTPUT_SET_25MHZ   0x900
#define TIPUS_CLOCK_OUTPUT_SET_41MHZ   0xA00
#define TIPUS_CLOCK_OUTPUT_SET_125MHZ  0xB00
#define TIPUS_CLOCK_OUTPUT_ENABLE_MASK 0x0C00      /*bits [11:10]*/
#define TIPUS_CLOCK_OUTPUT_GET_MASK    0x3000      /*bits [13:12]*/


/* 0x30 trig1Prescale bits and masks */
#define TIPUS_TRIG1PRESCALE_MASK          0x0000FFFF

/* 0x34 blockBuffer bits and masks */
#define TIPUS_BLOCKBUFFER_BUFFERLEVEL_MASK      0x000000FF
#define TIPUS_BLOCKBUFFER_BLOCKS_READY_MASK     0x0000FF00
#define TIPUS_BLOCKBUFFER_TRIGGERS_NEEDED_IN_BLOCK 0x00FF0000
#define TIPUS_BLOCKBUFFER_RO_NEVENTS_MASK       0x07000000
#define TIPUS_BLOCKBUFFER_BLOCKS_NEEDACK_MASK   0x7F000000
#define TIPUS_BLOCKBUFFER_BREADY_INT_MASK       0x0F000000
#define TIPUS_BLOCKBUFFER_BUSY_ON_BLOCKLIMIT    (1<<28)
#define TIPUS_BLOCKBUFFER_SYNCRESET_REQUESTED   (1<<30)
#define TIPUS_BLOCKBUFFER_SYNCEVENT             (1<<31)

/* 0x38 triggerRule bits and masks */
#define TIPUS_TRIGGERRULE_RULE1_MASK 0x000000FF
#define TIPUS_TRIGGERRULE_RULE2_MASK 0x0000FF00
#define TIPUS_TRIGGERRULE_RULE3_MASK 0x00FF0000
#define TIPUS_TRIGGERRULE_RULE4_MASK 0xFF000000

/* 0x3C triggerWindow bits and masks */
#define TIPUS_TRIGGERWINDOW_COINC_MASK   0x000000FF
#define TIPUS_TRIGGERWINDOW_INHIBIT_MASK 0x0000FF00
#define TIPUS_TRIGGERWINDOW_TRIG21_MASK  0x01FF0000
#define TIPUS_TRIGGERWINDOW_LEVEL_LATCH  (1<<31)

/* 0x48 tsInput bits and masks (6 front panel trigger inputs, TS#1 .. TS#6) */
#define TIPUS_TSINPUT_MASK      0x0000003F
#define TIPUS_TSINPUT_1         (1<<0)
#define TIPUS_TSINPUT_2         (1<<1)
#define TIPUS_TSINPUT_3         (1<<2)
#define TIPUS_TSINPUT_4         (1<<3)
#define TIPUS_TSINPUT_5         (1<<4)
#define TIPUS_TSINPUT_6         (1<<5)
#define TIPUS_TSINPUT_ALL       (0x3F)


/* 0x4C output bits and masks */
#define TIPUS_OUTPUT_MASK                 0x0000FFFF
#define TIPUS_OUTPUT_BLOCKS_READY_MASK    0x00FF0000
#define TIPUS_OUTPUT_EVENTS_IN_BLOCK_MASK 0xFF000000

/* 0x50 fiberSyncDelay bits and masks */
#define TIPUS_FIBERSYNCDELAY_HFBR1_SYNCPHASE_MASK    0x000000FF
#define TIPUS_FIBERSYNCDELAY_HFBR1_SYNCDELAY_MASK    0x0000FF00
#define TIPUS_FIBERSYNCDELAY_LOOPBACK_SYNCDELAY_MASK 0x00FF0000
#define TIPUS_FIBERSYNCDELAY_HFBR5_SYNCDELAY_MASK    0xFF000000

/* 0x54 dmaSetting bits and masks */
#define TIPUS_DMASETTING_PHYS_ADDR_HI_MASK    0x0000FFFF
#define TIPUS_DMASETTING_DMA_SIZE_MASK        0x03000000
#define TIPUS_DMASETTING_MAX_PACKET_SIZE_MASK 0x70000000
#define TIPUS_DMASETTING_ADDR_MODE_MASK       0x80000000

/* 0x58 dmaAddr masks */
#define TIPUS_DMAADDR_PHYS_ADDR_LO_MASK       0xFFFFFFFF

/* 0x74 inputPrescale bits and masks */
#define TIPUS_INPUTPRESCALE_FP1_MASK   0x0000000F
#define TIPUS_INPUTPRESCALE_FP2_MASK   0x000000F0
#define TIPUS_INPUTPRESCALE_FP3_MASK   0x00000F00
#define TIPUS_INPUTPRESCALE_FP4_MASK   0x0000F000
#define TIPUS_INPUTPRESCALE_FP5_MASK   0x000F0000
#define TIPUS_INPUTPRESCALE_FP6_MASK   0x00F00000
#define TIPUS_INPUTPRESCALE_FP_MASK(x) (0xF<<4*((x-1)))

/* 0x78 syncCommand bits and masks */
#define TIPUS_SYNCCOMMAND_VME_CLOCKRESET      0x11
#define TIPUS_SYNCCOMMAND_CLK250_RESYNC       0x22
#define TIPUS_SYNCCOMMAND_AD9510_RESYNC       0x33
#define TIPUS_SYNCCOMMAND_GTP_STATUSB_RESET   0x44
#define TIPUS_SYNCCOMMAND_TRIGGERLINK_ENABLE  0x55
#define TIPUS_SYNCCOMMAND_TRIGGERLINK_DISABLE 0x77
#define TIPUS_SYNCCOMMAND_SYNCRESET_HIGH      0x99
#define TIPUS_SYNCCOMMAND_TRIGGER_READY_RESET 0xAA
#define TIPUS_SYNCCOMMAND_RESET_EVNUM         0xBB
#define TIPUS_SYNCCOMMAND_SYNCRESET_LOW       0xCC
#define TIPUS_SYNCCOMMAND_SYNCRESET           0xDD
#define TIPUS_SYNCCOMMAND_SYNCRESET_4US       0xEE
#define TIPUS_SYNCCOMMAND_SYNCCODE_MASK       0x000000FF

/* 0x7C syncDelay bits and masks */
#define TIPUS_SYNCDELAY_MASK              0x0000007F

/* 0x80 syncWidth bits and masks */
#define TIPUS_SYNCWIDTH_MASK              0x7F
#define TIPUS_SYNCWIDTH_LONGWIDTH_ENABLE  (1<<7)

/* 0x84 triggerCommand bits and masks */
#define TIPUS_TRIGGERCOMMAND_VALUE_MASK     0x000000FF
#define TIPUS_TRIGGERCOMMAND_CODE_MASK      0x00000F00
#define TIPUS_TRIGGERCOMMAND_TRIG1          0x00000100
#define TIPUS_TRIGGERCOMMAND_TRIG2          0x00000200
#define TIPUS_TRIGGERCOMMAND_SYNC_EVENT     0x00000300
#define TIPUS_TRIGGERCOMMAND_SET_BLOCKLEVEL 0x00000800
#define TIPUS_TRIGGERCOMMAND_SET_BUFFERLEVEL 0x00000C00

/* 0x88 randomPulser bits and masks */
#define TIPUS_RANDOMPULSER_TRIG1_RATE_MASK 0x0000000F
#define TIPUS_RANDOMPULSER_TRIG1_ENABLE    (1<<7)
#define TIPUS_RANDOMPULSER_TRIG2_RATE_MASK 0x00000F00
#define TIPUS_RANDOMPULSER_TRIG2_ENABLE    (1<<15)

/* 0x8C fixedPulser1 bits and masks */
#define TIPUS_FIXEDPULSER1_NTRIGGERS_MASK 0x0000FFFF
#define TIPUS_FIXEDPULSER1_PERIOD_MASK    0x7FFF0000
#define TIPUS_FIXEDPULSER1_PERIOD_RANGE   (1<<31)

/* 0x90 fixedPulser2 bits and masks */
#define TIPUS_FIXEDPULSER2_NTRIGGERS_MASK 0x0000FFFF
#define TIPUS_FIXEDPULSER2_PERIOD_MASK    0x7FFF0000
#define TIPUS_FIXEDPULSER2_PERIOD_RANGE   (1<<31)

/* 0x94 nblocks bits and masks */
#define TIPUS_NBLOCKS_COUNT_MASK           0x00FFFFFF
#define TIPUS_NBLOCKS_EVENTS_IN_BLOCK_MASK 0xFF000000

/* 0x98 syncHistory bits and masks */
#define TIPUS_SYNCHISTORY_HFBR1_CODE_MASK     0x0000000F
#define TIPUS_SYNCHISTORY_HFBR1_CODE_VALID    (1<<4)
#define TIPUS_SYNCHISTORY_HFBR5_CODE_MASK     0x000001E0
#define TIPUS_SYNCHISTORY_HFBR5_CODE_VALID    (1<<9)
#define TIPUS_SYNCHISTORY_LOOPBACK_CODE_MASK  0x00003C00
#define TIPUS_SYNCHISTORY_LOOPBACK_CODE_VALID (1<<14)
#define TIPUS_SYNCHISTORY_TIMESTAMP_OVERFLOW  (1<<15)
#define TIPUS_SYNCHISTORY_TIMESTAMP_MASK      0xFFFF0000

/* 0x9C runningMode settings */
#define TIPUS_RUNNINGMODE_ENABLE          0x71
#define TIPUS_RUNNINGMODE_DISABLE         0x0

/* 0xA0 fiberLatencyMeasurement bits and masks */
#define TIPUS_FIBERLATENCYMEASUREMENT_CARRYCHAIN_MASK 0x0000FFFF
#define TIPUS_FIBERLATENCYMEASUREMENT_IODELAY_MASK    0x007F0000
#define TIPUS_FIBERLATENCYMEASUREMENT_DATA_MASK       0xFF800000

/* 0xA4 fiberAlignment bits and masks */
#define TIPUS_FIBERALIGNMENT_HFBR1_IODELAY_MASK   0x000000FF
#define TIPUS_FIBERALIGNMENT_HFBR1_SYNCDELAY_MASK 0x0000FF00
#define TIPUS_FIBERALIGNMENT_HFBR5_IODELAY_MASK   0x00FF0000
#define TIPUS_FIBERALIGNMENT_HFBR5_SYNCDELAY_MASK 0xFF000000

/* 0xC0 blockStatus bits and masks */
#define TIPUS_BLOCKSTATUS_NBLOCKS_READY0    0x000000FF
#define TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK0  0x0000FF00
#define TIPUS_BLOCKSTATUS_NBLOCKS_READY1    0x00FF0000
#define TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK1  0xFF000000

/* 0xD0 adr24 bits and masks */
#define TIPUS_ADR24_ADDRESS_MASK         0x0000001F
#define TIPUS_ADR24_HARDWARE_SET_MASK    0x000003E0
#define TIPUS_ADR24_GEOADDR_MASK         0x00007C00
#define TIPUS_ADR24_TM_NBLOCKS_READY1    0x00FF0000
#define TIPUS_ADR24_TM_NBLOCKS_NEEDACK1  0xFF000000

/* 0xD4 syncEventCtrl bits and masks */
#define TIPUS_SYNCEVENTCTRL_NBLOCKS_MASK 0x00FFFFFF

/* 0xD8 eventNumber_hi bits and masks */
#define TIPUS_PROMPT_TRIG_WIDTH_MASK     0x0000007F
#define TIPUS_EVENTNUMBER_HI_MASK        0xFFFF0000


/* 0xEC rocEnable bits and masks */
#define TIPUS_ROCENABLE_MASK             0x000000FF
#define TIPUS_ROCENABLE_ROC(x)           (1<<(x))
#define TIPUS_ROCENABLE_FIFO_ENABLE      (1<<1)
#define TIPUS_ROCENABLE_SYNCRESET_REQUEST_ENABLE_MASK  0x0007FC00
#define TIPUS_ROCENABLE_SYNCRESET_REQUEST_MONITOR_MASK 0x1FF00000

/* 0x100 reset bits and masks */
#define TIPUS_RESET_I2C                  (1<<1)
#define TIPUS_RESET_JTAG                 (1<<2)
#define TIPUS_RESET_SFM                  (1<<3)
#define TIPUS_RESET_SOFT                 (1<<4)
#define TIPUS_RESET_SYNC_HISTORY         (1<<6)
#define TIPUS_RESET_BUSYACK              (1<<7)
#define TIPUS_RESET_CLK250               (1<<8)
//#define TIPUS_RESET_CLK200               (1<<8) ???
#define TIPUS_RESET_CLK125               (1<<9)
#define TIPUS_RESET_MGT                  (1<<10)
#define TIPUS_RESET_AUTOALIGN_HFBR1_SYNC (1<<11)
#define TIPUS_RESET_AUTOALIGN_HFBR5_SYNC (1<<12)
#define TIPUS_RESET_RAM_WRITE            (1<<12)
#define TIPUS_RESET_FIBER_AUTO_ALIGN     (1<<13)
#define TIPUS_RESET_IODELAY              (1<<14)
#define TIPUS_RESET_MEASURE_LATENCY      (1<<15)
#define TIPUS_RESET_TAKE_TOKEN           (1<<16)
#define TIPUS_RESET_BLOCK_READOUT        (1<<17)
#define TIPUS_RESET_FORCE_SYNCEVENT      (1<<20)
#define TIPUS_RESET_MGT_RECEIVER         (1<<22)
#define TIPUS_RESET_SYNCRESET_REQUEST    (1<<23)
#define TIPUS_RESET_SCALERS_LATCH        (1<<24)
#define TIPUS_RESET_SCALERS_RESET        (1<<25)
#define TIPUS_RESET_FILL_TO_END_BLOCK    (1<<31)

/* 0x104 fpDelay Masks */
#define TIPUS_FPDELAY_MASK(x) (0x1FF<<(10*(x%3)))

/* 0x138 triggerRuleMin bits and masks */
#define TIPUS_TRIGGERRULEMIN_MIN2_MASK  0x00007F00
#define TIPUS_TRIGGERRULEMIN_MIN2_EN    (1<<15)
#define TIPUS_TRIGGERRULEMIN_MIN3_MASK  0x007F0000
#define TIPUS_TRIGGERRULEMIN_MIN3_EN    (1<<23)
#define TIPUS_TRIGGERRULEMIN_MIN4_MASK  0x7F000000
#define TIPUS_TRIGGERRULEMIN_MIN4_EN    (1<<31)

/* 0x1D0-0x1F0 TI ID bits and masks */
#define TIPUS_ID_TRIGSRC_ENABLE_MASK     0x000000FF
#define TIPUS_ID_CRATEID_MASK            0x0000FF00
#define TIPUS_ID_BLOCKLEVEL_MASK         0x00FF0000

/* Trigger Sources, used by tiSetTriggerSource  */
#define TIPUS_TRIGGER_P0        0
#define TIPUS_TRIGGER_HFBR1     1
#define TIPUS_TRIGGER_FPTRG     2
#define TIPUS_TRIGGER_TSINPUTS  3
#define TIPUS_TRIGGER_TSREV2    4
#define TIPUS_TRIGGER_RANDOM    5
#define TIPUS_TRIGGER_PULSER    5
#define TIPUS_TRIGGER_PART_1    6
#define TIPUS_TRIGGER_PART_2    7
#define TIPUS_TRIGGER_PART_3    8
#define TIPUS_TRIGGER_PART_4    9
#define TIPUS_TRIGGER_HFBR5    10
#define TIPUS_TRIGGER_TRIG21   11

/* Define default Interrupt vector and level */
#define TIPUS_INT_VEC      0xec
/* #define TIPUS_INT_VEC      0xc8 */
#define TIPUS_INT_LEVEL    5

/* i2c data masks - 16bit data default */
#define TIPUS_I2C_DATA_MASK             0x0000ffff
#define TIPUS_I2C_8BIT_DATA_MASK        0x000000ff

/* Data buffer bits and masks */
#define TIPUS_DATA_TYPE_DEFINE_MASK           0x80000000
#define TIPUS_WORD_TYPE_MASK                  0x78000000
#define TIPUS_FILLER_WORD_TYPE                0x78000000
#define TIPUS_BLOCK_HEADER_WORD_TYPE          0x00000000
#define TIPUS_BLOCK_TRAILER_WORD_TYPE         0x08000000
#define TIPUS_EMPTY_FIFO                      0xF0BAD0F0
#define TIPUS_BLOCK_HEADER_CRATEID_MASK       0xFF000000
#define TIPUS_BLOCK_HEADER_SLOTS_MASK         0x001F0000
#define TIPUS_BLOCK_TRAILER_WORD_COUNT_MASK   0x001FFFFF
#define TIPUS_BLOCK_TRAILER_SYNCEVENT_FLAG    (1 << 21)
#define TIPUS_BLOCK_TRAILER_CRATEID_MASK      0x00FF0000
#define TIPUS_BLOCK_TRAILER_SLOTS_MASK        0x1F000000
#define TIPUS_DATA_BLKNUM_MASK                0x0000FF00
#define TIPUS_DATA_BLKLEVEL_MASK              0x000000FF

/* tiInit initialization flag bits */
#define TIPUS_INIT_NO_INIT                 (1<<0)
#define TIPUS_INIT_SKIP_FIRMWARE_CHECK     (1<<1)
#define TIPUS_INIT_USE_DMA                 (1<<2)

/* Some pre-initialization routine prototypes */
int  tipusSetFiberLatencyOffset_preInit(int flo);
int  tipusSetCrateID_preInit(int cid);

/* Function prototypes */
int  tipusInit(unsigned int mode, int force);

int  tipusCheckAddresses();
void tipusStatus(int pflag);
int  tipusSetSlavePort(int port);
int  tipusGetSlavePort();
void tipusSlaveStatus(int pflag);
int  tipusReload();
unsigned int tipusGetSerialNumber(char **rSN);
int  tipusPrintTempVolt();
int  tipusClockResync();
int  tipusReset();
int  tipusGetFirmwareVersion();
int  tipusSetCrateID(unsigned int crateID);
int  tipusGetCrateID(int port);
int  tipusGetPortTrigSrcEnabled(int port);
int  tipusGetSlaveBlocklevel(int port);
int  tipusSetBlockLevel(int blockLevel);
int  tipusBroadcastNextBlockLevel(int blockLevel);
int  tipusGetNextBlockLevel();
int  tipusGetCurrentBlockLevel();
int  tipusSetInstantBlockLevelChange(int enable);
int  tipusGetInstantBlockLevelChange();
int  tipusSetTriggerSource(int trig);
int  tipusSetTriggerSourceMask(int trigmask);
int  tipusEnableTriggerSource();
int  tipusForceSendTriggerSourceEnable();
int  tipusDisableTriggerSource(int fflag);
int  tipusSetSyncSource(unsigned int sync);
int  tipusSetEventFormat(int format);
int  tipusSetFPInputReadout(int enable);
int  tipusSoftTrig(int trigger, unsigned int nevents, unsigned int period_inc, int range);
int  tipusSetRandomTrigger(int trigger, int setting);
int  tipusDisableRandomTrigger();
int  tipusFifoReadBlock(volatile unsigned int *data, int nwrds, int rflag);
int  tipusReadBlock(volatile unsigned int *data, int nwrds, int rflag);
int  tipusFakeTriggerBankOnError(int enable);
int  tipusGenerateTriggerBank(volatile unsigned int *data);
int  tipusReadTriggerBlock(volatile unsigned int *data);
int  tipusGetBlockSyncFlag();
int  tipusCheckTriggerBlock(volatile unsigned int *data);
int  tipusDecodeTriggerType(volatile unsigned int *data, int data_len, int event);
int  tipusEnableFiber(unsigned int fiber);
int  tipusDisableFiber(unsigned int fiber);
int  tipusSetBusySource(unsigned int sourcemask, int rFlag);
int  tipusSetTriggerLock(int enable);
int  tipusGetTriggerLock();
int  tipusSetPrescale(int prescale);
int  tipusGetPrescale();
int  tipusSetInputPrescale(int input, int prescale);
int  tipusGetInputPrescale(int input);
int  tipusSetTriggerPulse(int trigger, int delay, int width, int delay_step);
int  tipusSetPromptTriggerWidth(int width);
int  tipusGetPromptTriggerWidth();
void tipusSetSyncDelayWidth(unsigned int delay, unsigned int width, int widthstep);
void tipusTrigLinkReset();
int  tipusSetSyncResetType(int type);
void tipusSyncReset(int bflag);
void tipusSyncResetResync();
void tipusClockReset();
int  tipusResetEventCounter();
unsigned long long int tipusGetEventCounter();
int  tipusSetBlockLimit(unsigned int limit);
unsigned int  tipusGetBlockLimit();
unsigned int  tipusBReady();
int  tipusGetSyncEventFlag();
int  tipusGetSyncEventReceived();
int  tipusGetReadoutEvents();
int  tipusSetBlockBufferLevel(unsigned int level);
int  tipusGetBroadcastBlockBufferLevel();
int  tipusBusyOnBufferLevel(int enable);
int  tipusUseBroadcastBufferLevel(int enable);
int  tipusEnableTSInput(unsigned int inpMask);
int  tipusDisableTSInput(unsigned int inpMask);
int  tipusSetOutputPort(unsigned int set1, unsigned int set2, unsigned int set3, unsigned int set4);
int  tipusSetClockSource(unsigned int source);
int  tipusGetClockSource();
void  tipusSetFiberDelay(unsigned int delay, unsigned int offset);
int  tipusResetSlaveConfig();
int  tipusAddSlave(unsigned int fiber);
int  tipusSetTriggerHoldoff(int rule, unsigned int value, int timestep);
int  tipusGetTriggerHoldoff(int rule);
int  tipusPrintTriggerHoldoff(int dflag);
int  tipusSetTriggerHoldoffMin(int rule, unsigned int value);
int  tipusGetTriggerHoldoffMin(int rule, int pflag);

int  tipusDisableDataReadout();
int  tipusEnableDataReadout();
void tipusResetBlockReadout();

int  tipusTriggerTableConfig(unsigned int *itable);
int  tipusGetTriggerTable(unsigned int *otable);
int  tipusTriggerTablePredefinedConfig(int mode);
int  tipusDefineEventType(int trigMask, int hwTrig, int evType);
int  tipusDefinePulserEventType(int fixed_type, int random_type);
int  tipusLoadTriggerTable(int mode);
void tipusrintTriggerTable(int showbits);
int  tipusSetTriggerWindow(int window_width);
int  tipusGetTriggerWindow();
int  tipusSetTriggerInhibitWindow(int window_width);
int  tipusGetTriggerInhibitWindow();
int  tipusSetTrig21Delay(int delay);
int  tipusGetTrig21Delay();
int  tipusSetTriggerLatchOnLevel(int enable);
int  tipusGetTriggerLatchOnLevel();
int  tipusLatchTimers();
unsigned int tipusGetLiveTime();
unsigned int tipusGetBusyTime();
int  tipusLive(int sflag);
unsigned int tipusGetTSscaler(int input, int latch);
unsigned int tipusBlockStatus(int fiber, int pflag);

int  tipusGetFiberLatencyMeasurement();
int  tipusSetUserSyncResetReceive(int enable);
int  tipusGetLastSyncCodes(int pflag);
int  tipusGetSyncHistoryBufferStatus(int pflag);
void tipusResetSyncHistory();
void tipusUserSyncReset(int enable, int pflag);
void tipusrintSyncHistory();
int  tipusSetSyncEventInterval(int blk_interval);
int  tipusGetSyncEventInterval();
int  tipusForceSyncEvent();
int  tipusSyncResetRequest();
int  tipusGetSyncResetRequest();
int  tipusEnableSyncResetRequest(unsigned int portMask, int self);
int  tipusSyncResetRequestStatus(int pflag);
void tipusTriggerReadyReset();
int  tipusFillToEndBlock();
int  tipusResetMGT();
int  tipusSetTSInputDelay(int chan, int delay);
int  tipusGetTSInputDelay(int chan);
int  tipusPrintTSInputDelay();
unsigned int tipusGetGTPBufferLength(int pflag);
int  tipusGetConnectedFiberMask();
int  tipusGetTrigSrcEnabledFiberMask();
int  tipusEnableFifo();
#ifdef OLDDMA
int  tipusDmaConfig(int packet_size, int adr_mode, int dma_size);
int  tipusDmaSetAddr(unsigned int phys_addr_lo, unsigned int phys_addr_hi);
int  tipusPCIEStatus(int pflag);
#endif /* OLDDMA */

/* Library Interrupt/Polling routine prototypes */
int  tipusDoLibraryPollingThread(int choice);
int  tipusIntConnect(unsigned int vector, VOIDFUNCPTR routine, unsigned int arg);
int  tipusIntDisconnect();
int  tipusAckConnect(VOIDFUNCPTR routine, unsigned int arg);
void tipusIntAck();
int  tipusIntEnable(int iflag);
void tipusIntDisable();
unsigned int  tipusGetIntCount();
unsigned int  tipusGetAckCount();

int  tipusReadFiberFifo(int fiber, volatile unsigned int *data, int maxwords);
int  tipusPrintFiberFifo(int fiber);

#ifdef NOTSUPPORTED
int  tipusRocEnable(int roc);
int  tipusRocEnableMask(int rocmask);
int  tipusGetRocEnableMask();
#endif /* NOTSUPPORTED */

unsigned int tipusRead(volatile unsigned int *reg);
int  tipusWrite(volatile unsigned int *reg, unsigned int value);
unsigned int tipusJTAGRead(unsigned int reg);
int  tipusJTAGWrite(unsigned int reg, unsigned int value);
int  tipusOpen();
int  tipusClose();

/*sergey*/
int tipusSetFiberIn_preInit(int port);
int tipusGetSlavePort();
int tipusBusy();
int tipusGetNumberOfBlocksInBuffer();
int tipusGetBlockBufferLevel();
int tipusGetTSInputMask();
int tipusGetRandomTriggerSetting(int trigger);
int tipusGetRandomTriggerEnable(int trigger);
int tipusEnableClockOutputSoftwareSetting();
int tipusDisableClockOutputSoftwareSetting();
int tipusSetClockOutput(unsigned int mode);
int tipusGetClockOutput();
/*sergey*/

#endif /* TIPCIEUS_H */
