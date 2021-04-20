/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    ConfigParams.h
-- Description:    Configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2011/12/16 IM
-- Revision:       1.0 2012/07/26 IM CmOffset parameter added
--                     2012/09/20 IM Triger generator memory configuration file added
--                     2012/09/21 IM UdpChan_Delay parameter added
--                     2013/11/26 IM Eeprom added
--                     2014/01/31 IM increase DEF_MAX_NB_OF_FEU to 16
--                     2014/02/25 IM Add DrDblSmpClk parameter
--                     2014/03/29 IM Common parameter propagation added
--                     2014/04/09 IM UdpChan_MultiPackEnb parameter added
--                                   Feu_InterPacket_Delay replaces UdpChan_Delay
--                     2014/04/15 IM UdpChan_MultiPackThr parameter added
--                     2014/04/22 IM Trigger Interface parameter added
--                     2014/04/30 IM Self Trigger parameter added
--                     2014/07/04 IM Self Trigger topology parameter added
--                     2014/09/03 IM Trigger Interface Bert parameter added
--                 1.1 2014/11/17 IM Feu_RunCtrl_EvTstExt parameter added
--                 1.1 2014/12/10 IM Dream clock parameters added
--                 1.2 2016/01/11 IM Trig_Conf_TrigVetoLen parameter added to TrigGen
--                 1.3 2016/02/24 IM Number of Topological trigger registers increased from 8 to 32
--                 1.4 2016/03/15 IM Max16031 registers added
--                 1.5 2016/03/15 IM Max16031 registers added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
//#include "Platform.h"
/*
#define LINE_SIZE	256
#define NBMAX_ARGV 	32

typedef char ArgvArray[NBMAX_ARGV][LINE_SIZE];
extern int        argc;
extern ArgvArray  argv;

extern void parse_line(char *s);
*/

#ifndef H_FeuConfigParams
#define H_FeuConfigParams

#ifndef DEF_MAX_NB_OF_FEU
#define DEF_MAX_NB_OF_FEU 64
#endif

#ifndef DEF_MAX_FEU_SN
#define DEF_MAX_FEU_SN 255
#endif

#ifndef DEF_MAX_NB_OF_DREAM
#define DEF_MAX_NB_OF_DREAM 8
#endif

/*
 * Dream Clock Parameters
 */
typedef struct _DrmClkParams
{
	double RdClk_Div;
	double WrClk_Div;
	int WrClk_Phase;
	int AdcClk_Phase;
} DrmClkParams;

/*
 * Dream registers
 */
#define D_DreamReg_NumOf16BitWords 4
typedef struct _DreamReg
{
	int reg[D_DreamReg_NumOf16BitWords];
} DreamReg;

#define D_DreamPar_NumOfRegs 13
typedef struct _DreamParams
{
	DreamReg dream_reg[D_DreamPar_NumOfRegs];
} DreamParams;

/*
 * Adc registers
 */
#define AdcRegFlag_Unset     -1
#define AdcRegFlag_Set        0
#define AdcRegFlag_SetAndUpd  1
typedef struct _AdcReg
{
	         char flg;
	unsigned char val;
} AdcReg;
#define D_AdcPar_NumOfRegs 34
typedef struct _AdcParams
{
	AdcReg adc_reg[D_AdcPar_NumOfRegs];
} AdcParams;

#define D_FeuPar_NumOfDreams (DEF_MAX_NB_OF_DREAM+1)
#define D_FeuPar_EeProm_Size 16
#define D_FeuPar_Max16031_Size 256
#define D_FeuPar_SelfTrigTopo_Size 32
typedef struct _FeuParams
{
	// Main module config parameters
	char Main_Conf_ClkSel[16];
	int  Main_Conf_AdcDtp;
	int  Main_Conf_DataPipeLen;
	int  Main_Conf_DreamMask;
	int  Main_Conf_Samples;
	// Main module Trigger logic parameters
	int  Main_Trig_TimeStamp;
	int  Main_Trig_OvrWrnLwm;
	int  Main_Trig_OvrWrnHwm;
	int  Main_Trig_OvrThersh;
	int  Main_Trig_LocThrot;

	// FEU PowerUp Register
	int  Feu_Pwr_Dream;
	int  Feu_Pwr_PrtFlt; 
	int  Feu_Pwr_Adc;
	// FEU Run Control parameters
	int  Feu_RunCtrl_Pd;
	int  Feu_RunCtrl_CM;
	int  Feu_RunCtrl_ZS;
	int  Feu_RunCtrl_DrOvr;
	int  Feu_RunCtrl_DrDblSmpClk;
	int  Feu_RunCtrl_ZsChkSmp;
	int  Feu_RunCtrl_Id;
	int  Feu_RunCtrl_AdcDatRdyDel;
	int  Feu_RunCtrl_EvTstExt;
	int  Feu_RunCtrl_RdDel;
	int  Feu_RunCtrl_CmOffset;
	char Feu_RunCtrl_PdFile[128];
	char Feu_RunCtrl_ZsFile[128];

	// Common Pulser parameters
	int  Feu_Pulser_Enable;
	// TbDream Pulser parameters
	int  Feu_Pulser_BaseVal;
	int  Feu_Pulser_TrigVal;
	int  Feu_Pulser_TestFun;
	// Feu Pulser parameters
	int  Feu_Pulser_DreamTst;
	int  Feu_Pulser_PulseWid;
	// FEU Prescale parameters
	int  Feu_PreScale_EvtData;
	int  Feu_InterPacket_Delay;

	// Trigger Interface/Generator parameters
	int  Trig_Conf_Rate;
	char Trig_Conf_Src[24];
	int  Trig_Conf_TrigPipeLen;
	int  Trig_Conf_TrigVetoLen;
	char Trig_Conf_File[128];

	// Auxiliari Trigger Interface
	int  TI_Ignore;
	int  TI_DcBal_Enc;
	int  TI_DcBal_Dec;
	int  TI_Bert;

	// Self Trigger parameters
	int  SelfTrig_DreamMask;
	int  SelfTrig_Mult;
	int  SelfTrig_CmbHitPropFb;
	int  SelfTrig_CmbHitPropOl;
	int  SelfTrig_DrmHitWid;
	int  SelfTrig_CmbHitWid;
	int  SelfTrig_TrigTopo;
	int  SelfTrig_Veto;
	unsigned int SelfTrig_Topology[D_FeuPar_SelfTrigTopo_Size];

	// Communication registers
	int  UdpChan_Enable;
	int  UdpChan_Delay;
	int  UdpChan_MultiPackEnb;
	int  UdpChan_MultiPackThr;
	int  ComChan_Enable;
	char NetChan_Ip[16];

	// Dream parameters
	DreamParams dream_params[D_FeuPar_NumOfDreams];
	// Dream clock parameters
	DrmClkParams dream_clk_params;

	// Adc parameters
	AdcParams adc_params;

	// EE Prom values
	short ee_prom[D_FeuPar_EeProm_Size];

	// EE Prom values
	short max16031[D_FeuPar_Max16031_Size];
} FeuParams;

#define D_FeuParamsCol_NumOfFeuParams (DEF_MAX_NB_OF_FEU+1)
typedef struct _FeuParamsCol
{
	FeuParams feu_params[D_FeuParamsCol_NumOfFeuParams];
} FeuParamsCol;
int FeuParamsCol_Init(          FeuParamsCol *feu_params_col );
int FeuParamsCol_Sprintf(       FeuParamsCol *feu_params_col, char *buf  );
int FeuParamsCol_Fprintf(       FeuParamsCol *feu_params_col, FILE *fptr );
int FeuParamsCol_Parse(         FeuParamsCol *feu_params_col, int line_num );
int FeuParamsCol_Fread(         FeuParamsCol *feu_params_col, FILE *fptr );
int FeuParamsCol_PropComParams( FeuParamsCol *feu_params_col );

#endif // #ifndef H_FeuConfigParams
