/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    TiConfigParams.h
-- Description:    Trigger Interface configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/01 IM
-- Revision:       1.0 2015/03/23 IM Extended Eid & Tstp capability
--                     2015/03/27 IM Thrigger inhibit threshold
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_TiConfigParams
#define H_TiConfigParams

#include "BecConfigParams.h"

// Define Ti clock sources
typedef enum _TiClkSrc
{
	TiClkSrc_Undefined   = -1,
	TiClkSrc_Internal    =  0,
	TiClkSrc_HFBR1       =  2,
	TiClkSrc_HFBR5       =  3
} TiClkSrc;
char *TiClkSrc2Str( TiClkSrc src );

// Define Ti busy sources
typedef enum _TiBsySrc
{
	TiBsySrc_Undefined   = -1,
	TiBsySrc_None        =  0,
	TiBsySrc_SwB         =  (1<<1)
} TiBsySrc;
char *TiBsySrc2Str( TiBsySrc src );

// Define Ti sync sources
typedef enum _TiSynSrc
{
	TiSynSrc_Undefined   = -1,
	TiSynSrc_None        =  0,
	TiSynSrc_HFBR1       =  (1<<1),
	TiSynSrc_HFBR5       =  (1<<2),
	TiSynSrc_Soft        =  (1<<7)
} TiSynSrc;
char *TiSynSrc2Str( TiSynSrc src );

// Define Ti trigger sources
typedef enum _TiTrgSrc
{
	TiTrgSrc_Undefined   = -1,
	TiTrgSrc_None        =  0,
	TiTrgSrc_HFBR1       =  1,
	TiTrgSrc_Soft        =  2,
	TiTrgSrc_FpInp1      =  3,
	TiTrgSrc_FpInp2      =  4,
	TiTrgSrc_IntCst      =  5,
	TiTrgSrc_IntRnd      =  6,
	TiTrgSrc_HFBR5       =  7,
} TiTrgSrc;
char *TiTrgSrc2Str( TiTrgSrc src );

typedef struct _TiParams
{
	// Device ID and VME parameters
	// usually derived from BEC
	int Id;
	int Slot;
	unsigned int BaseAdr_A24_Reg;
	unsigned int BaseAdr_A32_Fifo;
	unsigned int BaseAdr_A32m_Com_Min;
	unsigned int BaseAdr_A32m_Com_Max;
	MblkRank Mblk_Rank;

	// Global parameters
	// Derived from System
	int NbOfEvtPerBlk;
	int BlockLimit;

	// Synchronisation parameters
	TiClkSrc ClkSrc;
	TiTrgSrc TrgSrc;
	TiBsySrc BsySrc;
	TiSynSrc SynSrc;

	// Trigger parameters
#define Def_Ti_TrgRules_MaxNum 4
// For some reasons the max number of units supported by software is 63
// The hardware supports 127
#define Def_Ti_TrgRules_MaxNumOfUnitCnt     63
#define Def_Ti_TrgRules_TimeUnit_016ns      16
#define Def_Ti_TrgRules_TimeUnit_480ns     480
	int TrgRules_TimeUnit[Def_Ti_TrgRules_MaxNum];
	int TrgRules_UnitCount[Def_Ti_TrgRules_MaxNum];
	int TrgRate;
	int TrgPrescale;
	int TrgDel_4ns;
	int TrgWid_4ns;

	// Sync parameters
	int SynDel_4ns;
	
	// Format: if set enables extended EventId and Timestamp
	int ExtEidTstp;

	// Trigger inhibit threshold in event blocks
	int TrgInhThr;
} TiParams;
int TiParams_Init(    TiParams  *params );
int TiParams_Sprintf( TiParams  *params, char *buf  );
int TiParams_Fprintf( TiParams  *params, FILE *fptr );
int TiParams_Parse(   TiParams  *params, int line_num );
int TiParams_Fread(   TiParams  *params, FILE *fptr );
int TiParams_Prop(    TiParams  *params );

#endif // #ifndef H_TiConfigParams
