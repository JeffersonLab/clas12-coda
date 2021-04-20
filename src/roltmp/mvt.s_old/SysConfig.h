/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    SysConfig.h
-- Description:    System configuration
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/13 IM
-- Revision:       1.0 2019/01/15 IM Add verbosity
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_SysConfiguration
#define H_SysConfiguration

#include "BecConfigParams.h"
#include "TiConfigParams.h"
#include "SdConfigParams.h"
#include "FeuConfigParams.h"
#include "SysConfigParams.h"


// Log file pointer to be set from outside
extern FILE *sys_log_fptr;

// System configuration parameters structure
extern SysParams *sys_params_ptr;

/* Max allowed 4194304 */
/* 1 event 48 FEU-s, 64 samples, 604 2-byte words per sample */
#define MAX_BLOCK_LENGTH 3710976

int  TiConfig(   TiParams *params );
int  TiRun(      TiParams *params, int run );
int  TiGo(       TiParams *params );
int  SdConfig(   SdParams *params );

int  SysConfig(SysParams *params, int configs_to_do);
int  SysRun();
int  SysStop();
int  SysGo();
int  SysConfigFromFile( char *params );
int  SysScanSlfTrgThresh( char *sys_conf_params_filename );
int  SysScanFeuMonit( char *sys_conf_params_filename, int store_period_sec );
void SysConfig_CleanUp();
int  SysConfig_SetLogFilePointer( FILE *fptr );
void SysConfig_SetVerbosity( int ver_level );
int  SysDumpFeuOptLnk();

int  SysConfig_ClrLogFilePointer();

#endif // #ifndef H_SysConfiguration
