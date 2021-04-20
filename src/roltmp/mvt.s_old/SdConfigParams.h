/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    SdConfigParams.h
-- Description:    SD (signal distribution) configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/02 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_SdConfigParams
#define H_SdConfigParams

typedef struct _SdParams
{
	// Device ID and VME parameters
	// usually derived from BEC
	int Id;
	int Slot;

	// Global parameters
	// Derived from BEC
	int ActiveSlotFlags;
	int ActiveTrigFlags;
	int TrigMult;
	int TrigWin;
} SdParams;
int SdParams_Init(    SdParams *params );
int SdParams_Sprintf( SdParams *params, char *buf  );
int SdParams_Fprintf( SdParams *params, FILE *fptr );

#endif // #ifndef H_SdConfigParams
