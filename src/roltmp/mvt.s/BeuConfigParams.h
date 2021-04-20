/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    BeuConfigParams.h
-- Description:    Beu / Ssp configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/01 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_BeuSspConfigParams
#define H_BeuSspConfigParams

#include "BecConfigParams.h"

// Define Beu/Ssp clock sources
typedef enum _BeuClkSrc
{
	BeuClkSrc_Undefined      = -1,
	BeuClkSrc_OnBoard_250MHz =  0,
	BeuClkSrc_TiVxs          =  1,
	BeuClkSrc_Unsupported    =  2
} BeuClkSrc;
char *BeuClkSrc2Str( BeuClkSrc src );

// Define Beu/Ssp trigger sources
typedef enum _BeuTrgSrc
{
	BeuTrgSrc_Undefined = -1,
	BeuTrgSrc_Soft      =  0,
	BeuTrgSrc_Const     =  1,
	BeuTrgSrc_Ti1       =  2,
	BeuTrgSrc_Ti2       =  3
} BeuTrgSrc;
char *BeuTrgSrc2Str( BeuTrgSrc src );

// Define Beu/Ssp trigger sources
typedef enum _BeuBsySrc
{
	BeuBsySrc_Undefined = -1,
	BeuBsySrc_None      =  0,
	BeuBsySrc_Perm      =  1,
	BeuBsySrc_TrgFifo   =  2,
	BeuBsySrc_Feu       =  4,
	BeuBsySrc_InMem     =  8
} BeuBsySrc;
char *BeuBsySrc2Str( BeuBsySrc src );

// Define Beu/Ssp trigger sources
typedef enum _BeuSynSrc
{
	BeuSynSrc_Undefined = -1,
	BeuSynSrc_Soft      =  0,
	BeuSynSrc_Ti        =  1
} BeuSynSrc;
char *BeuSynSrc2Str( BeuSynSrc src );

#define Beu_MblkRank_First   1
#define Beu_MblkRank_Last    2
#define Beu_MblkRank_Inter   0

typedef struct _BeuSspConf
{
	// Device ID and VME parameters
	// usually derived from BEC
	int Id;
	int Slot;
	unsigned int base_adr_reg;
	unsigned int base_adr_fifo;
	unsigned int base_adr_com_min;
	unsigned int base_adr_com_max;
	unsigned int mblk_rank;

	// Global parameters
	// usually derived from System
	int NbOfSamples;
	int NbOfEventsPerBlock;

	// Optical link parameters
	unsigned int near_end_loop_enb;
	unsigned int rol_enb;
	unsigned int feu_emu_msk;
	int emu_data_size;

	// Self trigger parameters
	int SelfTrigMult;   // Multiplicity from 0 to 31 : trigger if active links > SelfTrigMult
	// Derived from system and BEC parameters
	int SelfTrigLnkEnb; // up to 32 FEUs - 1 bit per FEU : 0 - disable ; 1 - enable
	int SelfTrigLat;    // equalisation latency ns (8ns steps from 750 to 1024 * 8 ns 10 bits)
	int SelfTrigWin;    // coinsidence window in ns (8ns steps from 2 to 64 * 8 ns 6 bits)
	int SelfTrigWid;    // width of the resulting trigger signal

	// Synchronisation parameters
	int ClkSrc;  // usually derived from System
	int TrgSrc;
	int BsySrc; 
	int SyncSrc;

	// Trigger FIFO low and high water mark for Busy generation
	int TrgFifo_Lwm;
	// if nb of triggers is above TrgFifo_Hwm either
	// a) assert Busy to TI and keep going
	// b) assert Busy to TI, do not send triggers to FEU, send empty packets
	// c) do not assert Busy, do no send triggers to FEU, respond by empty packets, notify by software
	// d) keep sending triggers to FEU but notify by software
	int TrgFifo_Hwm; 
	// If despite of TrgFifo_Hwm triggers are sent to FEUs,
	// above the TrgFifo_Full thershold error flag is set
	int TrgFifo_Full;
	// Timestamp offset
	int TrgTstp_Offset;

	// if non 0, the value overrides the def thresholf
	// When input memory is above  
	int InMemBusyThr;

	// Feu data packet concentration timeout
	// in number of 8ns clock cycles
	int timeout;
	// If set, timedout input is masked and ignored
	int MaskToInput;

	// Internal Trigger parameters
	int TrigRate; // 8ns clock cycles
} BeuSspConf;
int BeuSspConf_Init(    BeuSspConf *params );
int BeuSspConf_Sprintf( BeuSspConf *params, char *buf  );
int BeuSspConf_Fprintf( BeuSspConf *params, FILE *fptr );
int BeuSspConf_Parse(   BeuSspConf *params, int line_num );
int BeuSspConf_Fread(   BeuSspConf *params, FILE *fptr   );

// A collection of the BeuSspConf structures
typedef struct _BeuSspConfCol
{
	BeuSspConf beu_conf[DEF_MAX_NB_OF_BEU];
} BeuSspConfCol;
int BeuSspConfCol_Init(    BeuSspConfCol *col );
int BeuSspConfCol_Sprintf( BeuSspConfCol *col, char *buf    );
int BeuSspConfCol_Fprintf( BeuSspConfCol *col, FILE *fptr   );
int BeuSspConfCol_Parse(   BeuSspConfCol *col, int line_num );
int BeuSspConfCol_Fread(   BeuSspConfCol *col, FILE *fptr   );
int BeuSspConfCol_Prop(    BeuSspConfCol *col );

#endif // #ifndef H_BeuSspConfigParams
