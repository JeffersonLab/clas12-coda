//10 AOUT 2016
#ifndef BEUSSPLIB_H
#define BEUSSPLIB_H

// ------------------------------------------------------------------------------------------------------------------
// hardware desciption : the set of BEUSSP_VME_BASEADDR should match the values set by hardware on each BEUSSP board	
// ------------------------------------------------------------------------------------------------------------------
// VME base Addresses are given in the configuration file.
// These definitions are no longer used.
#define BEUSSP_VME_BASEADDR_0	0x00BE0000
#define BEUSSP_VME_BASEADDR_1	0x00CE0000


// -------------------------------------------------------------------------------------------------------------
//BEUSSP parameters needing configuration : filled up by software and exported to BEUSSP hardware when requested 
// -------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include "BeuConfigParams.h"

/*struct BEUSSP_Config
{
	//--
	//-- Configuration Registers
	//--
	//-- int base_adr_reg; -- A24 defined by on-board switches
	//-- These parameters must be consistent with the system configuration
	int base_adr_fifo;   //-- A32 defined by user; part of a shared A32 space; shall be unique
	int base_adr_common; //-- A32m defined by user; common among all VME boards involved in tocken passing

	int ClkSrc; //-- On-board 250 MHz (0); TI-VXS (1); (never used TI-VME(2))
	//-- Same register for the next two parameters
	int NbOfSamples;
	int NbofEventsPerBlock;

	int near_end_loop_enb; //-- 1-bit per channel (0-dis; 1-enable)
	int rol_enb; //-- 1-bit per readout link (0-ignored; 1-actibe, data to be received) (active GTX)
	int feu_emu_msk; //-- mask FEU emulators (0-emu active; 1-masked)

	//-- Same register for the next three parameters
	int TrgSrc; //-- 0 - soft; 1-Internal periodic; 2-external Trg1 of TI; 3 - external Trg2 of TI
	int BsySrc; //-- 0 - No busy; 1-In Busy constantly for test; 2-Internal TrigFifo; 3 - FEU bUsy; 4 - 2 or 3
	int SyncSrc; //-- 0 - Soft; 1-Ti Synch; governs RstEvtCntr, RstTstp end ReSync synchronous commands
	
	//-- Emulator configuration
	int emu_data_size;

	//-- Trigger FIFO low and high water mark for Busy generation
	//-- Same register for next two parameters
	int TrgFifo_Lwm;
	//-- if nb of triggers is above TrgFifo_Hwm either
	//-- a) assert Busy to TI and keep going
	//-- b) assert Busy to TI, do not send triggers to FEU, send empty packets
	//-- c) do not assert Busy, do no send triggers to FEU, respond by empty packets, notify by software
	//-- d) keep sending triggers to FEU but notify by software
	int TrgFifo_Hwm; //-- might be decided no to sent triggers 
	//-- If despite of TrgFifo_Hwm triggers are sent to FEUs,
	//-- above the TrgFifo_Full thershold error flag is set
	//-- to be decided what to do later
	int TrgFifo_Full;

	//-- Feu data packet concentration timeout
	int timeout; //-- number of 8ns clock cycles

	//-- Internal Trigger parameters
	int TrigRate; //-- 8ns clock cycles
} ;
*/

//int BeuSspConf_Init(   BEUSSP_Config * conf);
//int BeuSspConf_Fread(  BEUSSP_Config * conf, FILE * fptr);
//int BeuSspConf_Fprintf(BEUSSP_Config * conf, FILE * fptr);
//int BeuSspConf_Sprintf(BEUSSP_Config * conf, char * str);

// -------------------------------------------------------------------------------------------------------------
// VME register map of the BEUSSP board/firmware 
// -------------------------------------------------------------------------------------------------------------
struct BEUSSP_A24RegStruct
{
  /* 0x00000 */ volatile unsigned int boardID;
  /* 0x00004 */ volatile unsigned int FWRevision ;
  /* 0x00008 */ volatile unsigned int not_used_0;
  /* 0x0000C */ volatile unsigned int not_used_1;
	
  /* 0x00010 */ volatile unsigned int InterruptReg;			
  /* 0x00014 */ volatile unsigned int Interreupt_Ack;
  /* 0x00018 */ volatile unsigned int not_used_2;
  /* 0x0001C */ volatile unsigned int not_used_3;
  
  /* 0x00020 */ volatile unsigned int adr32m;
  /* 0x00024 */ volatile unsigned int adr32;			
  /* 0x00028 */ volatile unsigned int adrgeo;
  /* 0x0002C */ volatile unsigned int not_used_4;

  /* 0x00030 */ volatile unsigned int tokenin_cfg;
  /* 0x00034 */ volatile unsigned int tokenout_cfg;
  /* 0x00038 */ volatile unsigned int infobram_adr;
  /* 0x0003C */ volatile unsigned int infobram_rdata;

  /* 0x00040 */ volatile unsigned int regin_0;		//active_gtx	
  /* 0x00044 */ volatile unsigned int regin_1;		//emufeu_bypass
  /* 0x00048 */ volatile unsigned int regin_2;		//trig_parameter(31 downto 0) : internal trigger period in 8ns clock cycles
  /* 0x0004C */ volatile unsigned int regin_3;		//31-16 : nb_samples_per_trigger, 15 to 0 : nb_events_per_block

  /* 0x00050 */ volatile unsigned int regin_4;		//bram_info_bzrd
  /* 0x00054 */ volatile unsigned int regin_5;		//31-24 : busy_source, 23-16 :sync_source, 15-8 :trig_source, 7-0 : send command or trigger
  /* 0x00058 */ volatile unsigned int regin_6;		//0: sig_slwctrl_request_send,  15-8: slwctrl_target_feu
  /* 0x0005C */ volatile unsigned int regin_7;		//0:enable, 1: trigger enable, 16 : multigtx reset, 17: glclk pll reset, 18: soft reset / flush
 
  /* 0x00060 */ volatile unsigned int regin_8;		//slwctrldata to be sent
  /* 0x00064 */ volatile unsigned int regin_9;		//slwctrldata to be sent
  /* 0x00068 */ volatile unsigned int regin_A;		//clk chip control
  /* 0x0006C */ volatile unsigned int regin_B;		//number of words per sample from emulator	

  /* 0x00070 */ volatile unsigned int regin_C;		//links in nearendloopbacken
  /* 0x00074 */ volatile unsigned int regin_D;		//15-0 : feu_high_threshold, 31-16 : feu_low_threshold
  /* 0x00078 */ volatile unsigned int regin_E;		//15-0 : feu_full_threshold	
  /* 0x0007C */ volatile unsigned int regin_F;		//topbeussp_ctrl, timeout for data concentrator
 
  /* 0x00080 */ volatile unsigned int regout_0;		 //sig_error_flags(31 downto 0)
  /* 0x00084 */ volatile unsigned int regout_1;		 //sig_error_flags(
  /* 0x00088 */ volatile unsigned int regout_2;		 //sig_error_flags(
  /* 0x0008C */ volatile unsigned int regout_3;		 //sig_error_flags(
 
  /* 0x00090 */ volatile unsigned int regout_4;		 //sig_error_flags(
  /* 0x00094 */ volatile unsigned int regout_5;		 //sig_error_flags(
  /* 0x00098 */ volatile unsigned int regout_6;		 //sig_error_flags(
  /* 0x0009C */ volatile unsigned int regout_7;		 //sig_error_flags(255 downto 222)
 
  /* 0x000A0 */ volatile unsigned int regout_8;		//sig_trig_sent_counter(31 downto 0);
  /* 0x000A4 */ volatile unsigned int regout_9;		//sig_cmd_sent_counter(31 downto 0);
  /* 0x000A8 */ volatile unsigned int regout_A;		//sig_slwctrl_sent_counter(31 downto 0);
  /* 0x000AC */ volatile unsigned int regout_B;		//slowcontrol and clkstatus register	
 
  /* 0x000B0 */ volatile unsigned int regout_C;		//slow control packets received ( 31 downto 16) data packets received (15 downto 0);
  /* 0x000B4 */ volatile unsigned int regout_D;		//sig_ddr_status( 31 downto 0): counter_trailers_in_fifo ( 15 downto 0) -- counter_headers_in_fifo( 31 downto 16) 
  /* 0x000B8 */ volatile unsigned int regout_E;		//sig_slwctrl_valid_counter(31 downto 0);
  /* 0x000BC */ volatile unsigned int regout_F;		//bram_info readme (16) page number (6 downto 0) , VMEFIFOFULL ( 24) VMEFIFOEMPTY ( 28)		

  /* 0x000C0 */ volatile unsigned int regout_10;	//sig_multigtxstatus
  /* 0x000C4 */ volatile unsigned int regout_11;	//multigtxaligned
  /* 0x000C8 */ volatile unsigned int regout_12;	//sig_slwctrl_data_receive(63 downto 32);
  /* 0x000CC */ volatile unsigned int regout_13;	//sig_slwctrl_data_receive(31 downto  0);
 
  /* 0x000D0 */ volatile unsigned int regout_14;	//sig_topbeussp_error(31 downto  0);
  /* 0x000D4 */ volatile unsigned int regout_15;	//sig_topbeussp_status(31 downto  0);
  /* 0x000D8 */ volatile unsigned int regout_16;	//sig_trg_fifo_max_occupancy(15 downto  0) and MAX NB of HEADERS in VMEFIFO (31 downto 16)
  /* 0x000DC */ volatile unsigned int regout_17;	//sig_timedout_links;	
 
  /* 0x000E0 */ volatile unsigned int regout_18;	//sig_bram_error;	
  /* 0x000E4 */ volatile unsigned int regout_19;	//selftrig error
  /* 0x000E8 */ volatile unsigned int regout_1A;	//selftrig status
  /* 0x000EC */ volatile unsigned int regout_1B;	//selftrig counter
 
  /* 0x000F0 */ volatile unsigned int regout_1C;	// 0: SPI DO
  /* 0x000F4 */ volatile unsigned int regout_1D;
  /* 0x000F8 */ volatile unsigned int regout_1E;
  /* 0x000FC */ volatile unsigned int regout_1F;

  /* 0x00040 */ volatile unsigned int regin_10;		// self trig pusle width 	
  /* 0x00044 */ volatile unsigned int regin_11;		// 14 downto 0 : soft ID
  /* 0x00048 */ volatile unsigned int regin_12;		// 31 downto 16 : selftrig control ( bit 16 : selftrig enable) 13 downto 0 : selftrigger latency ( initial read adress given that initial write adress is 0)	
  /* 0x0004C */ volatile unsigned int regin_13;		// 31 downto 0 : selftrig link control : 1 enable for each link

  /* 0x00050 */ volatile unsigned int regin_14;		// 0: SPI CSB
  /* 0x00054 */ volatile unsigned int regin_15;		// 0: SPI DI
  /* 0x00058 */ volatile unsigned int regin_16;		// 0: SPI CLK
  /* 0x0005C */ volatile unsigned int regin_17;		// 
 
  /* 0x00060 */ volatile unsigned int regin_18;		//
  /* 0x00064 */ volatile unsigned int regin_19;		//
  /* 0x00068 */ volatile unsigned int regin_1A;		//
  /* 0x0006C */ volatile unsigned int regin_1B;		//	

  /* 0x00070 */ volatile unsigned int regin_1C;		//
  /* 0x00074 */ volatile unsigned int regin_1D;		//
  /* 0x00078 */ volatile unsigned int regin_1E;		//
  /* 0x0007C */ volatile unsigned int regin_1F;		//
 
 
 };



//constant VME_DNV_WORD				: std_logic_vector(31 downto 0) := x"F0000000";
//constant VME_FILLER_WORD			: std_logic_vector(31 downto 0) := x"F8000000";


// ------------------------------------------------------------------------------------------------
// declaration of utility functions and macros used in beusspLibTest.c
// ------------------------------------------------------------------------------------------------
#define PROMPT "BEUSSP>"
#define MAX_ARGC 20

void handle_help( void );
int handle_setlocalClk( void );
int my_hex_string_to_int(char   * hex_string, unsigned int * pval);
void my_get_line(char * line, unsigned int maxlen);	
void tokenize( char * line, int * pargc, char ** argv, unsigned int max_arguments);
// ------------------------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------------------------
int beusspInit(unsigned int vmebaseaddr, volatile struct BEUSSP_A24RegStruct  * * BEUSSPreg, volatile unsigned int * * BEUSSPfifo, BeuSspConf  * BEUSSPconf);
int beusspInitCtrlRegs(volatile struct BEUSSP_A24RegStruct * BEUSSPreg);
int beusspWriteConf(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg, volatile unsigned int * * BEUSSPfifo, BeuSspConf  * BEUSSPconf);
int beusspSetAdr32(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg,  volatile unsigned int * * BEUSSPfifo,  unsigned int a32base);
int beusspSetAdr32m(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int A32mMin, unsigned int A32mMax, unsigned int MblkRank);
int beusspSetSampleBlock(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int NbOfSamples, unsigned int NbofEventsPerBlock);
int beusspSetActiveLinks(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int rol_enb);
int beusspSetClkSrc(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int ClkSrc);
int beusspSetTrgFifoThresh(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int TrgFifo_Lwm,  unsigned int TrgFifo_Hwm, unsigned int TrgFifo_Full);
int beusspSetEmuDatasize(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int emu_data_size);
int beusspSetBypassEmulator(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int feu_emu_msk);
int beusspSetNearEndLoopBack(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int near_end_loop_enb);
int beusspSetConcentratorTimeout(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int timeout);
int beusspSetSBTSource(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int TrgSrc, unsigned int BsySrc, unsigned int SyncSrc );
int beusspSetSoftId(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int soft_id);
int beusspResetGClkPll(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg);
int beusspResetMultiGTX(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg);
int beusspGetMultiGTXStatus(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg, unsigned int *mgtx_status);
int beusspFlushPipeline(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg);
int beusspDisplayAllReg(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg);
int beusspSetTargetFeuAndDisplayAllReg(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg, int numFeu);
int beusspSetTargetFeuAndDumpAllReg(   volatile struct BEUSSP_A24RegStruct  * BEUSSPreg, int numFeu, FILE *fptr);

int beusspGetBlockSize(volatile struct BEUSSP_A24RegStruct  * BEUSSPreg, unsigned int * blocksize);
int beusspSendSlowControl(volatile struct BEUSSP_A24RegStruct * BEUSSPreg, unsigned int numFeu, unsigned int * datain1, unsigned int * datain2, unsigned int * dataout1, unsigned int * dataout2);
int beusspSetTargetFeu(volatile struct BEUSSP_A24RegStruct * BEUSSPreg, int numFeu);
int beusspVmeResyncRtsRec(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspEnableTriggerSource(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspDisableTriggerSource(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspEnSyncCom(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspDisSyncCom(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspReadBlock(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg,  volatile unsigned int *BEUSSPfifo, volatile unsigned int * data, unsigned int nwrds, int rflag);
int beusspDisableA32(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspEnableA32(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);

int beusspEnableA32m(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspDisableA32m(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspTakeToken(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspGetTokenStatus(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspEnBerrTerm(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspDisBerrTerm(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);

int beusspInitMblk(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg[], volatile unsigned int * * BEUSSPmblk);
int beusspInitMblkTESTMODE(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, volatile unsigned int * * BEUSSPmblk);
int beusspTokenReadBlock(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg,  volatile unsigned int *BEUSSPmblk, volatile unsigned int * data, unsigned int nwrds, int rflag);
int beusspBReady(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);

int beusspFlashSatusReg(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspFlashSatus(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspFlashID(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspFlashLoadbuffer(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, int NumBuf, unsigned char data[1056] );
int beusspFlashBufferToMemory(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, int NumBuf, int NumPage );
int beusspFWU( volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, char *filename ) ;
int beusspFlashREAD(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, int NumPage);
int beusspCompareFlashBufferToMemory(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, int NumBuf, int NumPage );
int beusspFWCompare( volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, char *filename ); 



int beusspCheckFeuLink( volatile struct BEUSSP_A24RegStruct *BEUSSPreg, int numFeu);
int beusspBZRDLow(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspBZRDHigh(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspGetVMEFIFOStatus(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);

int beusspSelfTrigSetLatency(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, int selftriglatency);
int beusspSelfTrigSetMultiplicity(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg,  int selftrigmultiplicity);
int beusspSelfTrigEnable(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspSelfTrigDisable(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg);
int beusspSelfTrigEnableLinks(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg, unsigned int selftrigenabledlinks );
int beusspSelfTrigSetWindow(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg,  int selftrigwindow );

//sergey: was commented out
int beusspSelfTrigSetPulseWidth(volatile struct BEUSSP_A24RegStruct  *BEUSSPreg,  int selftrigpulsewidth );



/* Supported firmware version */
//#define BEUSSP_SUPPORTED_FIRMWARE 0x20160810
//#define BEUSSP_SUPPORTED_FIRMWARE 0x20170210
//#define BEUSSP_SUPPORTED_FIRMWARE 0x20170228
//#define BEUSSP_SUPPORTED_FIRMWARE 0x20170310
//#define BEUSSP_SUPPORTED_FIRMWARE 0x20170320
//#define BEUSSP_SUPPORTED_FIRMWARE 0x20170509
//#define BEUSSP_SUPPORTED_FIRMWARE 0x20170607
#define BEUSSP_SUPPORTED_FIRMWARE 0x20191122


/* boardID bits and masks */
//#define BEUSSP_BOARDID_TYPE_MASK     0xFF000000
//#define BEUSSP_BOARDID_PROD_MASK     0x00FF0000
//#define BEUSSP_BOARDID_GEOADR_MASK   0x00001F00
//#define BEUSSP_BOARDID_CRATEID_MASK  0x000000FF
#define BEUSSP_BOARDID_TYPE_BEUSSP	0xBE55FCEA

/* adr32 bits and masks */
#define BEUSSP_ADR32_MBLK_ADDR_MAX_MASK           0x01FF0000
#define BEUSSP_ADR32_MBLK_ADDR_MIN_MASK           0x000001FF
#define BEUSSP_ADR32_MBLK_EN_MASK                 0x02000000
#define BEUSSP_ADR32_MBLK_EN_MASK_COMP            0xFDFFFFFF
#define BEUSSP_ADR32_MBLK_FIRST_MASK         0x04000000
#define BEUSSP_ADR32_MBLK_LAST_MASK          0x08000000
#define BEUSSP_ADR32_MBLK_TAKE_MASK          0x10000000
#define BEUSSP_ADR32_MBLK_STATUS_MASK        0x80000000

#define BEUSSP_ADR32_BASE_MASK       0x0000FF80
#define BEUSSP_EN_ADR32_MASK         0x00000001

/* vmeControl bits and masks */
#define BEUSSP_VMECONTROL_BERR           (1<<0)
#define BEUSSP_VMECONTROL_TOKEN_TESTMODE (1<<1)
#define BEUSSP_VMECONTROL_MBLK           (1<<2)
#define BEUSSP_VMECONTROL_A32M           (1<<3)
#define BEUSSP_VMECONTROL_A32            (1<<4)
#define BEUSSP_VMECONTROL_ERROR_INT      (1<<7)
#define BEUSSP_VMECONTROL_I2CDEV_HACK    (1<<8)
#define BEUSSP_VMECONTROL_TOKENOUT_HI    (1<<9)
#define BEUSSP_VMECONTROL_FIRST_BOARD    (1<<10)
#define BEUSSP_VMECONTROL_LAST_BOARD     (1<<11)
#define BEUSSP_VMECONTROL_BUFFER_DISABLE (1<<15)





#endif /* BEUSSPLIB_H */
