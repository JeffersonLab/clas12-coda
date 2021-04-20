/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    BeuConfig.c
-- Description:    BEU configuration
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/16 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ReturnCodes.h"
#include "BeuConfig.h"

// BEU Control structures
volatile struct BEUSSP_A24RegStruct *beu_reg_control[DEF_MAX_NB_OF_BEU];
volatile unsigned int               *beu_fifo[DEF_MAX_NB_OF_BEU];
/* pointer to BEUSSP multi block adress window */
extern volatile unsigned int        *BEUSSPmblk = 0;


 #ifndef OK
 #define OK 0
 #endif 

/*
 * Set system in Running or Idle state
 */
int BeuRun( volatile struct BEUSSP_A24RegStruct *beu_regs, int run )
{
	int ret;

	if( beu_regs == (struct BEUSSP_A24RegStruct *)NULL )
	{
		fprintf( stderr, "%s: beu_regs=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}

	if( run )
	{
		//enable synchronous commands
		if( (ret = beusspEnSyncCom( beu_regs )) != OK )
		{
			fprintf( stderr, "%s: beusspEnSyncCom failed with %d\n", __FUNCTION__, ret ); 
			return D_RetCode_Err_NetIO;
		}
		usleep(50);

		// Flush hardware
		if( (ret = beusspFlushPipeline( beu_regs )) != OK )
		{
			fprintf( stderr, "%s: beusspFlushPipeline failed with %d\n", __FUNCTION__, ret ); 
			return D_RetCode_Err_NetIO;
		}
		// Enable triggers on beussp
		if( (ret = beusspEnableTriggerSource( beu_regs )) != OK )
		{
			fprintf( stderr, "%s: beusspEnableTriggerSource failed with %d\n", __FUNCTION__, ret ); 
			return D_RetCode_Err_NetIO;
		}
	}
	else
	{
		// Enable triggers on beussp
		if( (ret = beusspDisableTriggerSource( beu_regs )) != OK )
		{
			fprintf( stderr, "%s: beusspDisableTriggerSource failed with %d\n", __FUNCTION__, ret ); 
			return D_RetCode_Err_NetIO;
		}
	}
	return D_RetCode_Sucsess;
}

/*
 * Enable trigger processing 
 */
int BeuGo( volatile struct BEUSSP_A24RegStruct *beu_regs, BeuSspConf *params )
{
	int ret;

	// Check for valid parameters
	if( beu_regs == (struct BEUSSP_A24RegStruct *)NULL )
	{
		fprintf( stderr, "%s: beu_regs=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}
	if( params == (BeuSspConf *)NULL )
	{
		fprintf( stderr, "%s: params=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}

	// Flush hardware
/*
	if( (ret = beusspFlushPipeline( beu_regs )) != OK )
	{
		fprintf( stderr, "%s: beusspFlushPipeline failed with %d for beu %d in slot %d\n", __FUNCTION__, ret, params->Id, params->Slot ); 
		return D_RetCode_Err_NetIO;
	}
*/
/*
	// Enable triggers on beussp
	if( (ret = beusspEnableTriggerSource( beu_regs )) != OK )
	{
		fprintf( stderr, "%s: beusspEnableTriggerSource failed with %d\n", __FUNCTION__, ret ); 
		return D_RetCode_Err_NetIO;
	}
*/
	// Resync frontends if the sync source is set to siftware
	if( params->SyncSrc == BeuSynSrc_Soft )
	{
		if( (ret = beusspVmeResyncRtsRec( beu_regs )) != OK )
		{
			fprintf( stderr, "%s: beusspVmeResyncRtsRec failed with %d for beu %d in slot %d\n", __FUNCTION__, ret, params->Id, params->Slot ); 
			return D_RetCode_Err_NetIO;
		}
//fprintf( stdout, "%s: beusspVmeResyncRtsRec ok for beu %d in slot %d\n", __FUNCTION__, params->Id, params->Slot ); 
	}
//beusspDisplayAllReg(beu_regs);
	return D_RetCode_Sucsess;
}
