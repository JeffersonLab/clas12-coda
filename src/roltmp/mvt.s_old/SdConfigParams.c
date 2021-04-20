/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    SdConfigParams.c
-- Description:    Signal distribution (SD) configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/03 IM
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
#include "BecConfigParams.h"
#include "SdConfigParams.h"

// Initialize the MvtParams structure with default values
int SdParams_Init( SdParams *params )
{
	// Check for Null pointer
	if( params == (SdParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Device ID and VME parameters
	// usually derived from BEC
	params->Id   = -1;
	params->Slot = 0;

	// Global parameters
	// usually derived from BEC
	params->ActiveSlotFlags = 0;
	params->ActiveTrigFlags = 0;
	params->TrigMult        = 0;
	params->TrigWin         = 0;

	return D_RetCode_Sucsess;
}

// Dump in ASCII the SdParams structure to a buffer
int SdParams_Sprintf( SdParams *params, char *buf  )
{
	// Check for Null pointer
	if( params == (SdParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	if( buf == (char *)NULL )
	{
		fprintf( stderr, "%s: buf=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// If parameters were not set, just exit queitly
	if( params->Id < 0 )
		return D_RetCode_Sucsess;

	// SD VME info
	if( params->Id == 0 )
	{
		fprintf( stderr, "%s: SD ID=%d not in range [1;%d]\n",
			__FUNCTION__, params->Id, DEF_MAX_NB_OF_BEC );
		return D_RetCode_Err_Wrong_Param;
	}
	sprintf( buf, "%s# Sd %d Slot %d\n", buf, params->Id, params->Slot );

	// Global parameters: defived from global parameters
	sprintf( buf, "%s# Sd %d ActiveSlotFlags 0x%08x\n",  buf, params->Id, params->ActiveSlotFlags );
	sprintf( buf, "%s# Sd %d ActiveTrigFlags 0x%08x\n",  buf, params->Id, params->ActiveTrigFlags );
	sprintf( buf, "%s# Sd %d TrigMult        %d\n",      buf, params->Id, params->TrigMult );
	sprintf( buf, "%s# Sd %d TrigWin         %d # ns\n", buf, params->Id, params->TrigWin );

	// All went fine
	return D_RetCode_Sucsess;
}

// Dump in ASCII the SdParams structure to a file
int SdParams_Fprintf( SdParams *params, FILE *fptr )
{
	char buf[128*1024];
	int ret;

	// Check for Null pointer
	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "%s: fptr=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// If parameters were not set, just exit queitly
	if( params->Id < 0 )
		return D_RetCode_Sucsess;

	// Beu VME info
	sprintf( buf, "# Beu %d configuration\n", params->Id );

	// First produce the ASCII representation of the SdParams parameters structure 
	if( (ret = SdParams_Sprintf( params, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SdParams_Sprintf failed with  %d\n", __FUNCTION__, ret );
		return ret;
	}

	// Then save it in the file  
	if( (ret = fprintf( fptr, "%s", buf ) ) < 0 )
	{
		fprintf( stderr, "%s: fprintf failed with %d\n", __FUNCTION__, ret );
		fprintf( stderr, "%s: fprintf failed with %s\n", __FUNCTION__, strerror(errno) );
		return ret;
	}

	// All went fine
	return D_RetCode_Sucsess;
}
