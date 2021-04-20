/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    SysConfigParams.h
-- Description:    System Configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/09/29 IM
-- Revision:       1.0 2015/08/28 IM: SysParams_Parse added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_SysConfigParams
#define H_SysConfigParams

#include "BecConfigParams.h"
#include "SdConfigParams.h"
#include "TiConfigParams.h"
#include "BeuConfigParams.h"
#include "FeuConfigParams.h"

// Define MVT run modes
typedef enum _SysRunMode
{
	SysModUdef = -1,
	Clas12     =  0,
	Standalone =  1,
	Expert     =  2,
	Spy        =  3
} SysRunMode;
char *SysRunMode2Str( SysRunMode mode );

// Define MVT clock modes
typedef enum _SysClkMode
{
	SysClkUdef = -1,
	Smp48_Rd48 = 0,
	Smp44_Rd44 = 1,
	Smp40_Rd40 = 2,
	Smp36_Rd36 = 3,
	Smp32_Rd32 = 4,
	Smp24_Rd48 = 5,
	SysClkLast = 6
} SysClkMode;
char *SysClkMode2Str( SysClkMode mode );

// System parameters structure
typedef struct _SysParams
{
	// Common parameters
	char Name[128];
	int  RunMode;
	int  NbOfSmpPerEvt;
	int  NbOfEvtPerBlk;
	int  ClkMode;
	int  SparseSmp;
	int  BlockPrescale;
	int  EventLimit;
	int  RepRawData;
	int  SelfTrigLat;
	int  SelfTrigWin;

	// Parameters for backend crates
	BecParams Bec_Params[DEF_MAX_NB_OF_BEC];

	// Parameters for TI & SD
	SdParams Sd_Params[DEF_MAX_NB_OF_BEC];
	TiParams Ti_Params[DEF_MAX_NB_OF_BEC];

	// Parameters for beu-s
	BeuSspConfCol BeuSspConf_Col;
	// Parameters for feu-s
	FeuParamsCol FeuParams_Col;
} SysParams;
int SysParams_Init(    SysParams *sys_params );
int SysParams_Sprintf( SysParams *sys_params, char *buf  );
int SysParams_Fprintf( SysParams *sys_params, FILE *fptr );
int SysParams_Fread(   SysParams *sys_params, FILE *fptr );
int SysParams_Prop(    SysParams *sys_params );
int SysParams_Parse(   SysParams *sys_params, int line_num );


#endif // #ifndef H_SysConfigParams

