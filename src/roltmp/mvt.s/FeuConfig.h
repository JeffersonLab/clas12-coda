/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    FeuConfig.h
-- Description:    Feu specific configuration
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2017/09/04 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_SysConfiguration
#define H_SysConfiguration

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "FeuConfigParams.h"
#include "FeuConfig.h"

/*
 * Toggle FEU commands
 */
 typedef enum _FeuTogCmd
 {
 	Reset        =    1,
	LatchStat    =   16,
	ClearStat    =   32,
	ReSync       =  256,
	HwReset      = 2048
 } FeuTogCmd;
int  FeuToggleCommand( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id, FeuTogCmd tog_cmd );

int  FeuReset(  FeuParams *params, int feu_id, int beu_id, int beu_lnk_id );
int  FeuRun(    FeuParams *params, int feu_id, int beu_id, int beu_lnk_id, int run );
int  FeuConfig( FeuParams *params, int feu_id, int beu_id, int beu_lnk_id );
int  FeuComChanRxErr_Get( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id, int *rx_err_cntr, int *par_err_cntr, int *pac_err_cntr );
int  FeuConfig_Lock();
int  FeuConfig_UnLock();
void FeuConfig_CleanUp();


/***************************************************************************
 ***************************************************************************
            Trigger Scan Functions
 ***************************************************************************
 ***************************************************************************/
#define DEF_DRM_TRG_MIN_THR 0
#define DEF_DRM_TRG_MAX_THR 127
#define DEF_DRM_TRG_CHK_NUM 256
#define DEF_DRM_TRG_CHK_PER 10

typedef struct _FeuTrgScan
{
	int drm_msk;
	int running_drm;
	int running_thr;
	int drm_thr[D_FeuPar_NumOfDreams-1];
	int nb_of_checks;
	unsigned int prev_cntr;
	struct timeval tstart;
} FeuTrgScan;
extern FeuTrgScan feu_trg_scan[DEF_MAX_NB_OF_FEU];
int FeuTrgScan_Init(           FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id );
int FeuTrgScan_SetThr(         FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id );
int FeuTrgScan_MskDrm(         FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id );
int FeuTrgScan_ChkCoin(        FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id, int chk_cnt );
int FeuTrgScan_Sprintf(        FeuTrgScan *scan, FeuParams *feu_params, int feu_id, char *buf );
int FeuTrgScan_Fprintf(        FeuTrgScan *scan, FeuParams *feu_params, int feu_id, FILE *fptr );
int FeuTrgScan_DumpConfigInfo( FeuTrgScan *scan, FeuParams *feu_params, int feu_id, FILE *fptr );
int FeuTrgScan_UpdateConfig(   FeuTrgScan *scan, FeuParams *feu_params, int feu_id );

typedef struct _FeuMonit
{
	double Temp_MaxIntSensor;
	double Temp_XilSideSensor;
	double Temp_AdcSideSensor;
	double Curent;
} FeuMonit;
int  FeuMonit_Sprintf( FeuMonit *feu_monit, char *buf  );
int  FeuMonit_Fprintf( FeuMonit *feu_monit, FILE *fptr );
int  FeuMonit_Get(     FeuMonit *feu_monit, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id );

#endif // #ifndef H_SysConfiguration
