#ifndef __SSPLIB_H 
#define __SSPLIB_H 
/****************************************************************************** 
 * 
 *  sspLib.h    -  Header for Driver library for JLAB config of JLAB Subsystem  
 *                 Processor (SSP). 
 * 
 */ 

#include "sspLib_rich.h"

/* Macros to help with register spacers */
#define MERGE_(a,b)  a##b
#define LABEL_(a) MERGE_(unsigned int sspblank, a)
#define BLANK LABEL_(__LINE__)

#ifndef MAX_VME_SLOTS 
#define MAX_VME_SLOTS    21 
#endif

#define SSP_MAX_FIFO       0x800000   /* 0.5 Meg */
#define SSP_MAX_A32MB_SIZE 0x800000

#define SSP_SUPPORTED_FIRMWARE 0x0103

/* Config Peripheral: Board information, fpga flash update */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int BoardId;
  /* 0x0004-0x0007 */ volatile unsigned int FirmwareRev;
  /* 0x0008-0x000B */ volatile unsigned int SpiCtrl;
  /* 0x000C-0x000F */ volatile unsigned int SpiStatus;
  /* 0x0010-0x0013 */ volatile unsigned int ICapCtrl;
  /* 0x0014-0x0017 */ volatile unsigned int ICapDataWr;
  /* 0x0018-0x001B */ volatile unsigned int ICapDataRd;
  /* 0x001C-0x001F */ volatile unsigned int ICapStatus;
  /* 0x0020-0x0023 */ volatile unsigned int Reset;
  /* 0x0024-0x00FF */          unsigned int Reserved0[(0x0100-0x0024)/4];
} SspCfg_regs;

/* Clock Peripheral: Clock configuration interface */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x0004-0x0007 */ volatile unsigned int Status;
  /* 0x0008-0x00FF */          unsigned int Reserved0[(0x0100-0x0008)/4];
} Clk_regs;

/* SD Peripheral: Internal signal muxing, scalers, pulser */
typedef struct
{
  /* 0x0000-0x0047 */ volatile unsigned int SrcSel[18];
  /* 0x0048-0x007F */          unsigned int Reserved0[(0x0080-0x0048)/4];
  /* 0x0080-0x0083 */ volatile unsigned int PulserPeriod;
  /* 0x0084-0x0087 */ volatile unsigned int PulserLowCycles;
  /* 0x0088-0x008B */ volatile unsigned int PulserNPulses;
  /* 0x008C-0x008F */ volatile unsigned int PulserStart;
  /* 0x0090-0x0093 */ volatile unsigned int PulserDone;
  /* 0x0094-0x00FF */          unsigned int Reserved1[(0x0100-0x0094)/4];
  /* 0x0100-0x0103 */ volatile unsigned int ScalerLatch;
  /* 0x0104-0x018F */ volatile unsigned int Scalers[35];
  /* 0x0190-0x01FF */          unsigned int Reserved2[(0x0200-0x0190)/4];
} Sd_regs;

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
  /* 0x0034-0x0037 */ volatile unsigned int Latency;
  /* 0x0038-0x003F */          unsigned int Reserved1[(0x0040-0x0038)/4];
  /* 0x0040-0x004B */ volatile unsigned int MonMask[3];
  /* 0x004C-0x005F */          unsigned int Reserved2[(0x0060-0x004C)/4];
  /* 0x0060-0x006B */ volatile unsigned int MonVal[3];
  /* 0x006C-0x007F */          unsigned int Reserved3[(0x0080-0x006C)/4];
  /* 0x0080-0x0083 */ volatile unsigned int MonThr[1];
  /* 0x0084-0x008F */          unsigned int Reserved4[(0x0090-0x0084)/4];
  /* 0x0090-0x009B */ volatile unsigned int MonData[3];
  /* 0x009C-0x00FF */          unsigned int Reserved5[(0x0100-0x009C)/4];
} Serdes_regs;

/* HPS Trigger Peripheral */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Latency;
  /* 0x0004-0x00FF */          unsigned int Reserved0[(0x0100-0x0004)/4];
} Trigger_regs;

/* HPS Cosmic Peripheral */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int TimeCoincidence;
  /* 0x0004-0x0007 */ volatile unsigned int TriggerPattern;
  /* 0x0008-0x000F */          unsigned int Reserved0[(0x0010-0x0008)/4];
  /* 0x0010-0x001B */ volatile unsigned int ScalerScintillatorTop[3];
  /* 0x001C-0x001F */ volatile unsigned int ScalerCosmicTop;
  /* 0x0020-0x002B */ volatile unsigned int ScalerScintillatorBot[3];
  /* 0x002C-0x002F */ volatile unsigned int ScalerCosmicBot;
  /* 0x0030-0x00FF */          unsigned int Reserved1[(0x0100-0x0030)/4];
} HpsCosmic_regs;

/* HPS Singles Peripheral */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x0004-0x0017 */ volatile unsigned int Reserved0[(0x0018-0x0004)/4];
  /* 0x0018-0x001B */ volatile unsigned int ClusterEmin;
  /* 0x001C-0x001F */ volatile unsigned int ClusterEmax;
  /* 0x0020-0x0023 */ volatile unsigned int ClusterNHitsmin;
  /* 0x0024-0x002F */          unsigned int Reserved1[(0x0030-0x0024)/4];
  /* 0x0030-0x004B */ volatile unsigned int Prescale[7];
  /* 0x004C-0x007F */          unsigned int Reserved2[(0x0080-0x004C)/4];
  /* 0x0080-0x0083 */ volatile unsigned int ScalerSinglesPass;
  /* 0x0084-0x0087 */ volatile unsigned int ScalerSinglesTot;
  /* 0x0088-0x00FF */          unsigned int Reserved3[(0x0100-0x0088)/4];
} Hps_regs;

/* HPS Cluster Peripheral */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int HistCtrl;
  /* 0x0004-0x000F */ volatile unsigned int Reserved0[(0x0010-0x0004)/4];
  /* 0x0010-0x0013 */ volatile unsigned int HistLatency;
  /* 0x0014-0x0017 */ volatile unsigned int HistPosition;
  /* 0x0018-0x001B */ volatile unsigned int HistEnergy;
  /* 0x001C-0x001F */ volatile unsigned int HistNHits;
  /* 0x0020-0x00FF */          unsigned int Reserved1[(0x0100-0x0020)/4];
} Fiber_regs;

/* HPS Pairs Peripheral */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x0004-0x000B */          unsigned int Reserved0[(0x000C-0x0004)/4];
  /* 0x000C-0x000F */ volatile unsigned int ClusterTimeCoincidence;
  /* 0x0010-0x0013 */ volatile unsigned int ClusterSummax;
  /* 0x0014-0x0017 */ volatile unsigned int ClusterDiffmax;
  /* 0x0018-0x001B */ volatile unsigned int ClusterEmin;
  /* 0x001C-0x001F */ volatile unsigned int ClusterEmax;
  /* 0x0020-0x0023 */ volatile unsigned int ClusterNHitsmin;
  /* 0x0024-0x0027 */ volatile unsigned int ClusterCoplanarTol;
  /* 0x0028-0x0028 */ volatile unsigned int ClusterEDFactor;
  /* 0x002C-0x002F */ volatile unsigned int ClusterEDmin;
  /* 0x0030-0x0033 */ volatile unsigned int ClusterSummin;
  /* 0x0034-0x007F */          unsigned int Reserved1[(0x0080-0x0034)/4];
  /* 0x0080-0x0083 */ volatile unsigned int ScalerPairsPass;
  /* 0x0084-0x0087 */ volatile unsigned int ScalerSumPass;
  /* 0x0088-0x008B */ volatile unsigned int ScalerDiffPass;
  /* 0x008C-0x008F */ volatile unsigned int ScalerEDPass;
  /* 0x0090-0x0093 */ volatile unsigned int ScalerCoplanarPass;
  /* 0x0094-0x0097 */ volatile unsigned int ScalerTriggerPass;
  /* 0x0098-0x00FF */          unsigned int Reserved2[(0x0100-0x0098)/4];
} HpsPair_regs;

/* Event Builder */
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
  /* 0x002C-0x00FF */          unsigned int Reserved1[(0x0100-0x002C)/4];
} EB_regs;


/******************************************************************/
/*** GT Structures ************************************************/
/******************************************************************/

#define GT_SSEC_DELAY_ESUM_MASK             0x000003FF
#define GT_SSEC_DELAY_CLUSTER_MASK          0x000003FF
#define GT_SSEC_WIDTHINT_ESUM_MASK          0x0000003F
#define GT_SSEC_DELAY_COSMIC_MASK           0x000003FF

/* GT ecal subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_esum;
  /* 0x0004-0x0007 */ volatile unsigned int Delay_cluster;
  /* 0x0008-0x000B */ volatile unsigned int Delay_cosmic;
  /* 0x000C-0x000F */ BLANK[(0x0010-0x000C)/4];
  /* 0x0010-0x0013 */ volatile unsigned int WidthInt_esum;
  /* 0x0014-0x001F */ BLANK[(0x020-0x0014)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Scaler_cluster;
  /* 0x0024-0x0028 */ volatile unsigned int Scaler_inner_cosmic;
  /* 0x0028-0x002B */ volatile unsigned int Scaler_outer_cosmic;
  /* 0x002C-0x00FF */ BLANK[(0x0100-0x002C)/4];
} GT_ssec_regs;

#define GT_SSPC_DELAY_ESUM_MASK             0x000003FF
#define GT_SSPC_DELAY_CLUSTER_MASK          0x000003FF
#define GT_SSPC_WIDTHINT_ESUM_MASK          0x0000003F
#define GT_SSPC_DELAY_COSMIC_MASK           0x000003FF

/* GT pcal subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_esum;
  /* 0x0004-0x0007 */ volatile unsigned int Delay_cluster;
  /* 0x0008-0x000B */ volatile unsigned int Delay_cosmic;
  /* 0x000C-0x000F */ BLANK[(0x0010-0x000C)/4];
  /* 0x0010-0x0013 */ volatile unsigned int WidthInt_esum;
  /* 0x0014-0x001F */ BLANK[(0x020-0x0014)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Scaler_cluster;
  /* 0x0024-0x0027 */ volatile unsigned int Scaler_cosmic;
  /* 0x0028-0x00FF */ BLANK[(0x0100-0x0028)/4];
} GT_sspc_regs;

/* GT drift chamber subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_seg;
  /* 0x0004-0x00FF */ BLANK[(0x0100-0x0004)/4];
} GT_ssdc_regs;

#define GT_SSHTCC_DELAY_MASK               0x000003FF

/* GT htcc subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_htcc;
  /* 0x0004-0x001F */ BLANK[(0x0020-0x0004)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Scaler_htcc;
  /* 0x0024-0x00FF */ BLANK[(0x0100-0x0024)/4];
} GT_sshtcc_regs;

#define GT_SSFTOF_DELAY_MASK               0x000003FF

/* GT ftof subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_ftof;
  /* 0x0004-0x001F */ BLANK[(0x0020-0x0004)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Scaler_ftof;
  /* 0x0024-0x00FF */ BLANK[(0x0100-0x0024)/4];
} GT_ssftof_regs;

#define GT_SSCTOF_DELAY_MASK               0x000003FF

/* GT ctof subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_ctof;
  /* 0x0004-0x001F */ BLANK[(0x0020-0x0004)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Scaler_ctof;
  /* 0x0024-0x00FF */ BLANK[(0x0100-0x0024)/4];
} GT_ssctof_regs;

#define GT_SSCND_DELAY_MASK               0x000003FF

/* GT cnd subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_cnd;
  /* 0x0004-0x001F */ BLANK[(0x0020-0x0004)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Scaler_cnd;
  /* 0x0024-0x00FF */ BLANK[(0x0100-0x0024)/4];
} GT_sscnd_regs;

#define GT_GTPIF_LATENCY_MASK               0x0000007FF

/* GT GTP interface */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Latency;
  /* 0x0004-0x00FF */ BLANK[(0x0100-0x0004)/4];
} GT_gtpif_regs;

#define GT_STRG_CTRL_EN                     0x00000001
#define GT_STRG_CTRL_PCCLUSTER_EMIN_EN      0x00000002
#define GT_STRG_CTRL_ECCLUSTER_EMIN_EN      0x00000004
#define GT_STRG_CTRL_PCESUM_EMIN_EN         0x00000008
#define GT_STRG_CTRL_ECESUM_EMIN_EN         0x00000010
#define GT_STRG_CTRL_DC_MULT_EN             0x00000020
#define GT_STRG_CTRL_ECOCOSMIC_EN           0x00000040
#define GT_STRG_CTRL_ECICOSMIC_EN           0x00000080
#define GT_STRG_CTRL_PCCOSMIC_EN            0x00000100
#define GT_STRG_CTRL_HTCC_EN                0x00000200
#define GT_STRG_CTRL_FTOF_EN                0x00000400
#define GT_STRG_CTRL_ECPCCLUSTER_EMIN_EN    0x00000800
#define GT_STRG_CTRL_CTOF_EN                0x00001000
#define GT_STRG_CTRL_CND_EN                 0x00002000

#define GT_STRG_ECCTRL_ESUM_EMIN_MASK       0x00003FFF
#define GT_STRG_ECCTRL_ESUM_WIDTH_MASK      0x00FF0000

#define GT_STRG_PCCTRL_ESUM_EMIN_MASK       0x00003FFF
#define GT_STRG_PCCTRL_ESUM_WIDTH_MASK      0x00FF0000

#define GT_STRG_ECCTRL_CLUSTER_EMIN_MASK    0x0000FFFF
#define GT_STRG_ECCTRL_CLUSTER_WIDTH_MASK   0x00FF0000

#define GT_STRG_PCCTRL_CLUSTER_EMIN_MASK    0x0000FFFF
#define GT_STRG_PCCTRL_CLUSTER_WIDTH_MASK   0x00FF0000

#define GT_STRG_COSMIC_WIDTH_MASK           0x00FF0000

#define GT_STRG_DCCTRL_MULT_MIN_MASK        0x00000007
#define GT_STRG_DCCTRL_MULT_WIDTH_MASK      0x01FF0000

#define GT_STRG_HTCC_CTRL_WIDTH_MASK        0x00FF0000
#define GT_STRG_HTCC_MASK0                  0xFFFFFFFF
#define GT_STRG_HTCC_MASK1                  0x0000FFFF

#define GT_STRG_FTOF_CTRL_WIDTH_MASK        0x00FF0000
#define GT_STRG_FTOF_MASK0                  0xFFFFFFFF
#define GT_STRG_FTOF_MASK1                  0x3FFFFFFF

#define GT_STRG_CTOF_CTRL_WIDTH_MASK        0x00FF0000
#define GT_STRG_CTOF_CTRL_HIT_MASK          0x000000FF

#define GT_STRG_CND_CTRL_WIDTH_MASK         0x00FF0000
#define GT_STRG_CND_CTRL_HIT_MASK           0x000000FF

#define GT_STRG_ECPCCTRL_CLUSTER_EMIN_MASK  0x0000FFFF
#define GT_STRG_ECPCCTRL_CLUSTER_WIDTH_MASK 0x00FF0000

/* GT sector trigger */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x0004-0x0007 */ volatile unsigned int Cosmic;
  /* 0x0008-0x000B */ volatile unsigned int ECPCCtrl_cluster;
  /* 0x000C-0x000F */ BLANK[(0x0010-0x000C)/4];
  /* 0x0010-0x0013 */ volatile unsigned int ECCtrl_esum;
  /* 0x0014-0x0017 */ volatile unsigned int PCCtrl_esum;
  /* 0x0018-0x001B */ volatile unsigned int ECCtrl_cluster;
  /* 0x001C-0x001F */ volatile unsigned int CtofCtrl;
  /* 0x0020-0x0023 */ volatile unsigned int PCCtrl_cluster;
  /* 0x0024-0x0027 */ volatile unsigned int DCCtrl;
  /* 0x0028-0x0027 */ volatile unsigned int HtccCtrl;
  /* 0x002C-0x002F */ volatile unsigned int HtccMask0;
  /* 0x0030-0x0033 */ volatile unsigned int HtccMask1;
  /* 0x0034-0x0037 */ volatile unsigned int FtofCtrl;
  /* 0x0038-0x003B */ volatile unsigned int Scaler_trigger;
  /* 0x003C-0x003F */ volatile unsigned int FtofMask0;
  /* 0x0040-0x0043 */ volatile unsigned int FtofMask1;
  /* 0x0044-0x0047 */ volatile unsigned int CndCtrl;
  /* 0x0048-0x007F */ BLANK[(0x0080-0x0048)/4];
} GT_strg_regs;

/******************************************************************/
/*** GTC Structures ***********************************************/
/******************************************************************/

#define GTC_SSFT_DELAY_ESUM_MASK             0x000003FF
#define GTC_SSFT_DELAY_CLUSTER_MASK          0x000003FF
#define GTC_SSFT_WIDTHINT_ESUM_MASK          0x0000003F

/* GTC ft subsystem */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Delay_esum;
  /* 0x0004-0x0007 */ volatile unsigned int Delay_cluster;
  /* 0x0008-0x000F */ BLANK[(0x0010-0x0008)/4];
  /* 0x0010-0x0013 */ volatile unsigned int WidthInt_esum;
  /* 0x0014-0x001F */ BLANK[(0x020-0x0014)/4];
  /* 0x0020-0x0023 */ volatile unsigned int Scaler_cluster;
  /* 0x0024-0x00FF */ BLANK[(0x0100-0x0024)/4];
} GTC_ssft_regs;

#define GTC_GTPIF_LATENCY_MASK               0x0000007FF

/* GTC GTP interface */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Latency;
  /* 0x0004-0x00FF */ BLANK[(0x0100-0x0004)/4];
} GTC_gtpif_regs;

#define GTC_CTRG_CTRL_EN                          0x00000001
#define GTC_CTRG_CTRL_FTESUM_EMIN_EN              0x00000002
#define GTC_CTRG_CTRL_FTCLUSTER_EN                0x00000004

#define GTC_CTRG_FT_ESUM_CTRL_EMIN_MASK           0x00003FFF
#define GTC_CTRG_FT_ESUM_CTRL_WIDTH_MASK          0x00FF0000

#define GTC_CTRG_FT_CLUSTER_CTRL0_EMIN_MASK       0x00003FFF
#define GTC_CTRG_FT_CLUSTER_CTRL0_EMAX_MASK       0x3FFF0000

#define GTC_CTRG_FT_CLUSTER_CTRL1_HODO_NMIN_MASK  0x00000003
#define GTC_CTRG_FT_CLUSTER_CTRL1_NMIN_MASK       0x00000F00
#define GTC_CTRG_FT_CLUSTER_CTRL1_WIDTH_MASK      0x00FF0000

/* GTC sector trigger */
typedef struct
{
  /* 0x0000-0x0003 */ volatile unsigned int Ctrl;
  /* 0x0004-0x0007 */ volatile unsigned int FtCtrl_esum;
  /* 0x0008-0x000B */ volatile unsigned int FtCtrl_Cluster0;
  /* 0x000C-0x000F */ volatile unsigned int FtCtrl_Cluster1;
  /* 0x0010-0x007F */ BLANK[(0x0080-0x0010)/4];
  /* 0x0080-0x0083 */ volatile unsigned int Scaler_trigger;
  /* 0x0084-0x00FF */ BLANK[(0x0100-0x0084)/4];
} GTC_ctrg_regs;

/************************/
/* SSP memory structure */
/************************/
typedef struct
{
  /* 0x0000-0x00FF */ SspCfg_regs       Cfg;
  /* 0x0100-0x01FF */ Clk_regs          Clk;
  /* 0x0200-0x03FF */ Sd_regs           Sd;
  /* 0x0400-0x04FF */ BLANK[(0x0500-0x0400)/4];
  
  struct
  {
    /* 0x0500-0x05FF */ Fiber_regs      FiberTop;
    /* 0x0600-0x06FF */ Fiber_regs      FiberBot;
    /* 0x0700-0x08FF */ Hps_regs        HpsSingles[2];
    /* 0x0900-0x0AFF */ HpsPair_regs    HpsPairs[2];
    /* 0x0B00-0x0BFF */ HpsCosmic_regs  HpsCosmic;
  } hps;
  
  /* 0x0C00-0x0FFF */ BLANK[(0x1000-0x0C00)/4];
  /* 0x1000-0x19FF */ Serdes_regs       Ser[10];
  /* 0x1a00-0x1FFF */ BLANK[(0x2000-0x1a00)/4];
  /* 0x2000-0x20FF */ EB_regs           EB;
  /* 0x2100-0x21FF */ Trigger_regs      Trigger;
  
  struct
  {
  /* 0x2200-0x22FF */ GT_ssec_regs    ssec;
  /* 0x2300-0x23FF */ GT_sspc_regs    sspc;
  /* 0x2400-0x24FF */ GT_gtpif_regs   gtpif;
  /* 0x2500-0x25FF */ GT_ssdc_regs    ssdc;
  /* 0x2600-0x27FF */ BLANK[(0x2800-0x2600)/4];
  /* 0x2800-0x28FF */ GT_sshtcc_regs  sshtcc;
  /* 0x2900-0x29FF */ GT_ssftof_regs  ssftof;
  /* 0x2A00-0x2AFF */ GT_ssctof_regs  ssctof;
  /* 0x2B00-0x2BFF */ GT_sscnd_regs   sscnd;
  /* 0x2C00-0x2FFF */ BLANK[(0x3000-0x2C00)/4];
  /* 0x3000-0x33FF */ GT_strg_regs    strigger[8];
  } gt;

  /* 0x3400 */ BLANK[(0x4000-0x3400)/4];

  struct
  {
  /* 0x4000-0x41FF */ GTC_ssft_regs   ssft[2];
  /* 0x4200-0x42FF */ BLANK[(0x4300-0x4200)/4];
  /* 0x4300-0x43FF */ GTC_gtpif_regs  gtpif;
  /* 0x4400-0x4FFF */ BLANK[(0x5000-0x4400)/4];
  /* 0x5000-0x53FF */ GTC_ctrg_regs   ctrigger[4];
  } gtc;

  /* 0x5400-0xFFFF */ BLANK[(0x10000-0x5400)/4];
  
  struct
  {
  /* 0x10000-0x2F000 */ RICH_regs     fiber[32];
  } rich;
} SSP_regs;


/* Sd_regs->SrcSel[] IDs */
#define SD_SRC_LVDSOUT0			0
#define SD_SRC_LVDSOUT1			1
#define SD_SRC_LVDSOUT2			2
#define SD_SRC_LVDSOUT3			3
#define SD_SRC_LVDSOUT4			4
#define SD_SRC_GPIO0			5
#define SD_SRC_GPIO1			6
#define SD_SRC_P2_LVDSOUT0		7
#define SD_SRC_P2_LVDSOUT1		8
#define SD_SRC_P2_LVDSOUT2		9
#define SD_SRC_P2_LVDSOUT3		10
#define SD_SRC_P2_LVDSOUT4		11
#define SD_SRC_P2_LVDSOUT5		12
#define SD_SRC_P2_LVDSOUT6		13
#define SD_SRC_P2_LVDSOUT7		14
#define SD_SRC_TRIG			15
#define SD_SRC_SYNC			16
#define SD_SRC_TRIG2    17

#define SD_SRC_NUM			18

/* Sd_regs->SrcSel[] values */
#define SD_SRC_SEL_0		0
#define SD_SRC_SEL_1		1
#define SD_SRC_SEL_SYNC		2
#define SD_SRC_SEL_TRIG1	3
#define SD_SRC_SEL_TRIG2	4
#define SD_SRC_SEL_LVDSIN0	5
#define SD_SRC_SEL_LVDSIN1	6
#define SD_SRC_SEL_LVDSIN2	7
#define SD_SRC_SEL_LVDSIN3	8
#define SD_SRC_SEL_LVDSIN4	9
#define SD_SRC_SEL_P2LVDSIN0	10
#define SD_SRC_SEL_P2LVDSIN1	11
#define SD_SRC_SEL_P2LVDSIN2	12
#define SD_SRC_SEL_P2LVDSIN3	13
#define SD_SRC_SEL_P2LVDSIN4	14
#define SD_SRC_SEL_P2LVDSIN5	15
#define SD_SRC_SEL_P2LVDSIN6	16
#define SD_SRC_SEL_P2LVDSIN7	17
#define SD_SRC_SEL_PULSER	18
#define SD_SRC_SEL_BUSY		19
#define SD_SRC_SEL_TRIGGER0	20
#define SD_SRC_SEL_TRIGGER1	21
#define SD_SRC_SEL_TRIGGER2	22
#define SD_SRC_SEL_TRIGGER3	23
#define SD_SRC_SEL_TRIGGER4	24
#define SD_SRC_SEL_TRIGGER5	25
#define SD_SRC_SEL_TRIGGER6	26
#define SD_SRC_SEL_TRIGGER7	27

#define SD_SRC_SEL_NUM		28

#define SD_SRC_SEL_MASK         0x0000001F

/* Sd_regs->Scalers[] IDs */
#define SD_SCALER_SYSCLK	0
#define SD_SCALER_GCLK		1
#define SD_SCALER_SYNC		2
#define SD_SCALER_TRIG1		3
#define SD_SCALER_TRIG2		4
#define SD_SCALER_GPIO0		5
#define SD_SCALER_GPIO1		6
#define SD_SCALER_LVDSIN0	7
#define SD_SCALER_LVDSIN1	8
#define SD_SCALER_LVDSIN2	9
#define SD_SCALER_LVDSIN3	10
#define SD_SCALER_LVDSIN4	11
#define SD_SCALER_LVDSOUT0	12
#define SD_SCALER_LVDSOUT1	13
#define SD_SCALER_LVDSOUT2	14
#define SD_SCALER_LVDSOUT3	15
#define SD_SCALER_LVDSOUT4	16
#define SD_SCALER_BUSY		17
#define SD_SCALER_BUSYCYCLES	18
#define SD_SCALER_P2_LVDSIN0	19
#define SD_SCALER_P2_LVDSIN1	20
#define SD_SCALER_P2_LVDSIN2	21
#define SD_SCALER_P2_LVDSIN3	22
#define SD_SCALER_P2_LVDSIN4	23
#define SD_SCALER_P2_LVDSIN5	24
#define SD_SCALER_P2_LVDSIN6	25
#define SD_SCALER_P2_LVDSIN7	26
#define SD_SCALER_P2_LVDSOUT0	27
#define SD_SCALER_P2_LVDSOUT1	28
#define SD_SCALER_P2_LVDSOUT2	29
#define SD_SCALER_P2_LVDSOUT3	30
#define SD_SCALER_P2_LVDSOUT4	31
#define SD_SCALER_P2_LVDSOUT5	32
#define SD_SCALER_P2_LVDSOUT6	33
#define SD_SCALER_P2_LVDSOUT7	34

#define SD_SCALER_NUM			35

#define SD_PULSER_DONE			0x1
#define SD_PULSER_FREQ_MIN		0.00
#define SD_PULSER_FREQ_MAX		25E6

#define CLK_CTRL_DRPDWE				0x00200000
#define CLK_CTRL_DRPDEN				0x00400000
#define CLK_CTRL_SERDES_MASK                    0x03000000
#define CLK_CTRL_SERDES_DISABLED                (0<<24)
#define CLK_CTRL_SERDES_VXS                     (1<<24)
#define CLK_CTRL_SERDES_P2                      (2<<24)
#define CLK_CTRL_SERDES_LOCAL                   (3<<24)
#define CLK_CTRL_LOGIC_MASK                     0x0C000000
#define CLK_CTRL_LOGIC_DISABLED                 (0<<26)
#define CLK_CTRL_LOGIC_VXS                      (1<<26)
#define CLK_CTRL_LOGIC_P2                       (2<<26)
#define CLK_CTRL_LOGIC_LOCAL                    (3<<26)
#define CLK_CTRL_GCLKRST			0x80000000

#define CLK_STATUS_DRPRDY			0x00010000
#define CLK_STATUS_GCLKLOCKED   		0x00020000

#define SSPCFG_SPI_NCSSET			0x00000100
#define SSPCFG_SPI_NCSCLR			0x00000200
#define SSPCFG_SPI_START			0x00000400
#define SSPCFG_SPI_DONE				0x00000800

#define TRG_CTRL_FIBER_EN0			0x00000001
#define TRG_CTRL_FIBER_EN1			0x00000002
#define TRG_CTRL_FIBER_EN2			0x00000004
#define TRG_CTRL_FIBER_EN3			0x00000008
#define TRG_CTRL_FIBER_EN4			0x00000010
#define TRG_CTRL_FIBER_EN5			0x00000020
#define TRG_CTRL_FIBER_EN6			0x00000040
#define TRG_CTRL_FIBER_EN7			0x00000080

#define TRG_CTRL_GTPSRC_FIBER0	(0 << 24)
#define TRG_CTRL_GTPSRC_FIBER1	(1 << 24)
#define TRG_CTRL_GTPSRC_FIBER2	(2 << 24)
#define TRG_CTRL_GTPSRC_FIBER3	(3 << 24)
#define TRG_CTRL_GTPSRC_FIBER4	(4 << 24)
#define TRG_CTRL_GTPSRC_FIBER5	(5 << 24)
#define TRG_CTRL_GTPSRC_FIBER6	(6 << 24)
#define TRG_CTRL_GTPSRC_FIBER7	(7 << 24)
#define TRG_CTRL_GTPSRC_SUM	(8 << 24)



#define TRG_CTRL_GTPSRC_NUM	9

#define TRG_SUMHISTCTRL_EN	0x00000001

#define TRG_SUMHISTWINDOW_NSA_MASK  0x00FF0000
#define TRG_SUMHISTWINDOW_NSB_MASK  0x000000FF

#define SSP_SER_FIBER0		0
#define SSP_SER_FIBER1		1
#define SSP_SER_FIBER2		2
#define SSP_SER_FIBER3		3
#define SSP_SER_FIBER4		4
#define SSP_SER_FIBER5		5
#define SSP_SER_FIBER6		6
#define SSP_SER_FIBER7		7
#define SSP_SER_VXS0		8
#define SSP_SER_VXSGTP		9

#define SSP_SER_NUM		10

#define SER_CRATEID_MASK        0x0000FFFF

#define SSP_SER_CTRL_POWERDN	0x00000001
#define SSP_SER_CTRL_GTXRST	0x00000002
#define SSP_SER_CTRL_LINKRST	0x00000200
#define SSP_SER_CTRL_ERRCNT_RST	0x00000400
#define SSP_SER_CTRL_ERRCNT_EN	0x00000800

#define SSP_SER_STATUS_HARDERR(x)       (1<<x)
#define SSP_SER_STATUS_LANEUP(x)        (1<<(x+4))
#define SSP_SER_STATUS_POL_REVERSED(x)  (1<<(x+8))
#define SSP_SER_STATUS_CHUP             (1<<0)
#define SSP_SER_STATUS_SRCRDYN          (1<<14)

#define SYSCLK_FREQ   50000000
#define GCLK_FREQ     250000000

#define SSP_CLKSRC_DISABLED             0
#define SSP_CLKSRC_SWB			1
#define SSP_CLKSRC_P2			2
#define SSP_CLKSRC_LOCAL		3
#define SSP_CLKSRC_NUM                  4

#define SSP_CFG_BOARDID			0x53535020	// "SSP "

#define SSP_CFG_SSPTYPE_COMMON    0x00
#define SSP_CFG_SSPTYPE_HALLD     0x01
#define SSP_CFG_SSPTYPE_HALLB     0x02
#define SSP_CFG_SSPTYPE_HPS       0x03
#define SSP_CFG_SSPTYPE_HALLBGT   0x04
#define SSP_CFG_SSPTYPE_HALLBRICH 0x05
#define SSP_CFG_SSPTYPE_HALLBGTC  0x06

#define SSP_CFG_FIRMWAREREV_MASK           0x0000FFFF
#define SSP_CFG_FIRMWAREREV_MAJOR_MASK     0x0000FF00
#define SSP_CFG_FIRMWAREREV_MINOR_MASK     0x000000FF
#define SSP_CFG_SSPTYPE_MASK               0x00FF0000
#define SSP_CFG_SLOTID_MASK                0x1F000000

#define SPI_CMD_RD		       	0x03
#define SPI_CMD_GETID			0x9F
#define SPI_CMD_GETSTATUS		0xD7
#define SPI_CMD_WRBUF1			0x84
#define SPI_CMD_PGBUF1ERASE	0x83

#define SPI_BYTE_LENGTH			8*1024*1024
#define SPI_MFG_ATMEL			0x1F
#define SPI_DEVID_AT45DB642D	0x2800

/* sspInit iFlag options */
#define SSP_INIT_MODE_DISABLED         0
#define SSP_INIT_MODE_P2               1
#define SSP_INIT_MODE_FP               2
#define SSP_INIT_MODE_VXS              3
#define SSP_INIT_MODE_MASK             0x3
#define SSP_INIT_SKIP_SOURCE_SETUP     (1<<12)
#define SSP_INIT_SKIP_FIRMWARE_CHECK   (1<<13)
#define SSP_INIT_NO_INIT               (1<<14)
#define SSP_INIT_USE_ADDRLIST          (1<<15)
#define SSP_INIT_FIBER0_ENABLE         (1<<16)
#define SSP_INIT_FIBER1_ENABLE         (1<<17)
#define SSP_INIT_FIBER2_ENABLE         (1<<18)
#define SSP_INIT_FIBER3_ENABLE         (1<<19)
#define SSP_INIT_FIBER4_ENABLE         (1<<20)
#define SSP_INIT_FIBER5_ENABLE         (1<<21)
#define SSP_INIT_FIBER6_ENABLE         (1<<22)
#define SSP_INIT_FIBER7_ENABLE         (1<<23)
#define SSP_INIT_FIBER_ENABLE_MASK     0x00FF0000

#define SSP_INIT_GTP_FIBER0_ENABLE     (0<<24)
#define SSP_INIT_GTP_FIBER1_ENABLE     (1<<24)
#define SSP_INIT_GTP_FIBER2_ENABLE     (2<<24)
#define SSP_INIT_GTP_FIBER3_ENABLE     (3<<24)
#define SSP_INIT_GTP_FIBER4_ENABLE     (4<<24)
#define SSP_INIT_GTP_FIBER5_ENABLE     (5<<24)
#define SSP_INIT_GTP_FIBER6_ENABLE     (6<<24)
#define SSP_INIT_GTP_FIBER7_ENABLE     (7<<24)
#define SSP_INIT_GTP_FIBER_ENABLE_MASK 0xFF000000


/* sspStatus rflag options */
#define SSP_STATUS_SHOWREGS   (1<<0)


/* Global arrays of strings of names of ports/signals */
extern const char *ssp_ioport_names[SD_SRC_NUM];
extern const char *ssp_signal_names[SD_SRC_SEL_NUM];
extern const char *ssp_gtpsrc_names[TRG_CTRL_GTPSRC_NUM];
extern const char *ssp_scaler_name[SD_SCALER_NUM];
extern const char *ssp_clksrc_name[SSP_CLKSRC_NUM];
extern const char *ssp_serdes_names[SSP_SER_NUM];

/* SSP configuration */
int  sspInit(unsigned int addr, unsigned int addr_inc, int nfind, int iFlag);
int sspGetGeoAddress(int id);
int  sspSlot(unsigned int id);
int  sspId(unsigned int slot);
int  sspSetMode(int id, int iFlag, int pflag);
int  sspStatus(int id, int rflag);
void sspGStatus(int rflag);
int  sspSetClkSrc(int id, int src);
int  sspGetClkStatus(int id);
int  sspGetClkSrc(int id, int pflag);
int  sspSetIOSrc(int id, int ioport, int signal);
void sspPrintIOSrc(int id, int pflag);
int  sspTriggerSetup(int id, int fiber_mask, int gtp_src, int pflag);
int  sspGetFirmwareType(int id);
int  sspPrintEbStatus(int id);
int  sspEbReset(int id, int reset);

/* HPS routines */
int  sspHps_SetLatency(int id, int latency);
int  sspHps_SetSinglesEmin(int id, int n, int min);
int  sspHps_SetSinglesEmax(int id, int n, int max);
int  sspHps_SetSinglesNHitsmin(int id, int n, int min);
int  sspHps_SetSinglesEnableEmin(int id, int n, int en);
int  sspHps_SetSinglesEnableEmax(int id, int n, int en);
int  sspHps_SetSinglesEnableNmin(int id, int n, int en);
int  sspHps_SetSinglePrescale(int id, int n, int region, int xmin, int xmax, int prescale);
int  sspHps_SetCosmicTimeCoincidence(int id, int ticks);
int  sspHps_SetCosmicCoincidencePattern(int id, int pattern);
int  sspHps_SetPairsEnableSum(int id, int n, int en);
int  sspHps_SetPairsEnableDiff(int id, int n, int en);
int  sspHps_SetPairsEnableCoplanar(int id, int n, int en);
int  sspHps_SetPairsEnableED(int id, int n, int en);
int  sspHps_SetPairsTimeCoincidence(int id, int n, int ticks);
int  sspHps_SetPairsSummax(int id, int n, int max);
int  sspHps_SetPairsSummin(int id, int n, int min);
int  sspHps_SetPairsDiffmax(int id, int n, int max);
int  sspHps_SetPairsEmin(int id, int n, int min);
int  sspHps_SetPairsEmax(int id, int n, int max);
int  sspHps_SetPairsNHitsmin(int id, int n, int min);
int  sspHps_SetPairsCoplanarTolerance(int id, int n, int tol);
int sspHps_SetPairsEDFactor(int id, int n, float f);
int  sspHps_SetPairsEDmin(int id, int n, int min);

void sspPrintHpsScalers(int id);
void sspPrintHpsConfig(int id);

/* GT routines */
int sspGt_SetLatency(int id, int latency);
int sspGt_GetLatency(int id);
int sspGt_SetEcal_EsumDelay(int id, int delay);
int sspGt_GetEcal_EsumDelay(int id);
int sspGt_SetEcal_ClusterDelay(int id, int delay);
int sspGt_GetEcal_ClusterDelay(int id);
int sspGt_SetEcal_EsumIntegrationWidth(int id, int width);
int sspGt_GetEcal_EsumIntegrationWidth(int id);
int sspGt_SetEcal_CosmicDelay(int id, int delay);
int sspGt_GetEcal_CosmicDelay(int id);
int sspGt_SetPcal_EsumDelay(int id, int delay);
int sspGt_GetPcal_EsumDelay(int id);
int sspGt_SetPcal_ClusterDelay(int id, int delay);
int sspGt_GetPcal_ClusterDelay(int id);
int sspGt_SetPcal_EsumIntegrationWidth(int id, int width);
int sspGt_GetPcal_EsumIntegrationWidth(int id);
int sspGt_SetPcal_CosmicDelay(int id, int delay);
int sspGt_GetPcal_CosmicDelay(int id);
int sspGt_SetDc_SegDelay(int id, int delay);
int sspGt_GetDc_SegDelay(int id);
int sspGt_SetHtcc_Delay(int id, int delay);
int sspGt_GetHtcc_Delay(int id);
int sspGt_SetFtof_Delay(int id, int delay);
int sspGt_GetFtof_Delay(int id);
int sspGt_SetCtof_Delay(int id, int delay);
int sspGt_GetCtof_Delay(int id);
int sspGt_SetCnd_Delay(int id, int delay);
int sspGt_GetCnd_Delay(int id);
int sspGt_SetTrigger_Enable(int id, int trg, int en_mask);
int sspGt_GetTrigger_Enable(int id, int trg);
int sspGt_SetTrigger_EcalEsumEmin(int id, int trg, int val);
int sspGt_GetTrigger_EcalEsumEmin(int id, int trg);
int sspGt_SetTrigger_EcalEsumWidth(int id, int trg, int val);
int sspGt_GetTrigger_EcalEsumWidth(int id, int trg);
int sspGt_SetTrigger_PcalEsumEmin(int id, int trg, int val);
int sspGt_GetTrigger_PcalEsumEmin(int id, int trg);
int sspGt_SetTrigger_PcalEsumWidth(int id, int trg, int val);
int sspGt_GetTrigger_PcalEsumWidth(int id, int trg);
int sspGt_SetTrigger_EcalClusterEmin(int id, int trg, int val);
int sspGt_GetTrigger_EcalClusterEmin(int id, int trg);
int sspGt_SetTrigger_EcalClusterWidth(int id, int trg, int val);
int sspGt_GetTrigger_EcalClusterWidth(int id, int trg);
int sspGt_SetTrigger_PcalClusterEmin(int id, int trg, int val);
int sspGt_GetTrigger_PcalClusterEmin(int id, int trg);
int sspGt_SetTrigger_PcalClusterWidth(int id, int trg, int val);
int sspGt_GetTrigger_PcalClusterWidth(int id, int trg);
int sspGt_SetTrigger_CosmicWidth(int id, int trg, int val);
int sspGt_GetTrigger_CosmicWidth(int id, int trg);
int sspGt_SetTrigger_DcMultMin(int id, int trg, int val);
int sspGt_GetTrigger_DcMultMin(int id, int trig);
int sspGt_SetTrigger_DcMultWidth(int id, int trg, int val);
int sspGt_GetTrigger_DcMultWidth(int id, int trg);
int sspGt_SetTrigger_HtccWidth(int id, int trg, int val);
int sspGt_GetTrigger_HtccWidth(int id, int trg);
int sspGt_SetTrigger_HtccMask(int id, int trg, long long mask);
long long sspGt_GetTrigger_HtccMask(int id, int trg);
int sspGt_SetTrigger_FtofWidth(int id, int trg, int val);
int sspGt_GetTrigger_FtofWidth(int id, int trg);
int sspGt_SetTrigger_FtofMask(int id, int trg, long long mask);
long long sspGt_GetTrigger_FtofMask(int id, int trg);
int sspGt_SetTrigger_CtofWidth(int id, int trg, int val);
int sspGt_GetTrigger_CtofWidth(int id, int trg);
int sspGt_SetTrigger_CtofMask(int id, int trg, int mask);
int sspGt_GetTrigger_CtofMask(int id, int trg);
int sspGt_SetTrigger_CndWidth(int id, int trg, int val);
int sspGt_GetTrigger_CndWidth(int id, int trg);
int sspGt_SetTrigger_CndMask(int id, int trg, int mask);
int sspGt_GetTrigger_CndMask(int id, int trg);
int sspGt_SetTrigger_ECPCClusterEmin(int id, int trg, int val);
int sspGt_GetTrigger_ECPCClusterEmin(int id, int trg);
int sspGt_SetTrigger_ECPCClusterWidth(int id, int trg, int val);
int sspGt_GetTrigger_ECPCClusterWidth(int id, int trg);

void sspPrintGtConfig(int id);

/* GTC routines */
int sspGtc_SetLatency(int id, int latency);
int sspGtc_GetLatency(int id);
int sspGtc_SetFt_EsumDelay(int id, int delay);
int sspGtc_GetFt_EsumDelay(int id);
int sspGtc_SetFt_ClusterDelay(int id, int delay);
int sspGtc_GetFt_ClusterDelay(int id);
int sspGtc_SetFt_EsumIntegrationWidth(int id, int width);
int sspGtc_GetFt_EsumIntegrationWidth(int id);
int sspGtc_SetTrigger_Enable(int id, int trg, int en_mask);
int sspGtc_GetTrigger_Enable(int id, int trg);
int sspGtc_SetTrigger_FtEsumEmin(int id, int trg, int val);
int sspGtc_GetTrigger_FtEsumEmin(int id, int trg);
int sspGtc_SetTrigger_FtEsumWidth(int id, int trg, int val);
int sspGtc_GetTrigger_FtEsumWidth(int id, int trg);
int sspGtc_SetTrigger_FtClusterEmin(int id, int trg, int val);
int sspGtc_GetTrigger_FtClusterEmin(int id, int trg);
int sspGtc_SetTrigger_FtClusterEmax(int id, int trg, int val);
int sspGtc_GetTrigger_FtClusterEmax(int id, int trg);
int sspGtc_SetTrigger_FtClusterHodoNmin(int id, int trg, int val);
int sspGtc_GetTrigger_FtClusterHodoNmin(int id, int trg);
int sspGtc_SetTrigger_FtClusterNmin(int id, int trg, int val); 
int sspGtc_GetTrigger_FtClusterNmin(int id, int trg); 
int sspGtc_SetTrigger_FtClusterWidth(int id, int trg, int val);
int sspGtc_GetTrigger_FtClusterWidth(int id, int trg);

/* Pulser routines */
int  sspPulserStatus(int id);
void sspPulserStart(int id);
void sspPulserSetup(int id, float freq, float duty, unsigned npulses);
int sspGetPulserFreq(int id);

/* SSP serdes */
void sspPortEnable(int id, int mask, int pflag);
void sspPortResetErrorCount(int id, int mask);
int  sspPortGetErrorCount(int id, int port, int lane);
void sspPortPrintStatus(int id);
int  sspGetConnectedFiberMask(int id);
int  sspGetCrateID(int id, int port);
void sspSerdesEnable(int id, int mask, int pflag);
void sspSerdesPrintStatus(int id);

/* Scaler routines */
void sspPrintScalers(int id);

/* Firmware update routines */
int  sspFirmwareUpdateVerify(int id, const char *filename);
int  sspFirmwareUpdate(int id, const char *filename);
int  sspFirmwareRead(int id, const char *filename);
int  sspFirmwareVerify(int id, const char *filename);
int  sspGFirmwareUpdateVerify(const char *filename);


/* Firmware update utility routines */
int  sspGetSerialNumber(int id, char *mfg, int *sn);
unsigned int sspGetFirmwareVersion(int id);

int sspReadBlock(int id, unsigned int *data, int nwrds, int rflag);



int sspSetBlockLevel(int id, int block_level);
int sspGetBlockLevel(int id);
int sspEnableBusError(int id);
int sspDisableBusError(int id);
int sspGetBusError(int id);
int sspSetWindowWidth(int id, int window_width);
int sspGetWindowWidth(int id);
int sspSetWindowOffset(int id, int window_offset);
int sspGetWindowOffset(int id);
int sspBReady(int id);
int sspGetNssp();
unsigned int sspGBReady();


int sspGetFirmwareType_Shadow(int id);
unsigned int sspReadReg(volatile unsigned int *addr);
void sspWriteReg(volatile unsigned int *addr, unsigned int val);
void SSPLOCK();
void SSPUNLOCK();

#endif
