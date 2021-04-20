/*
----------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    CBus.h
-- Description:    Control bus library
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
--
-- Create Date:    0.0 2011/01/11 IM
-- Revision:       1.0 2014/04/14 IM Auxiliary Trigger Interface added
--                     2014/12/09 IM DrmClk module added
--
-- Comments: 
--
----------------------------------------------------------------------------------
*/
#ifndef CBUS_H
#define CBUS_H

	#include "bitmanip.h"

	// Module types
	#define D_CBus_Mod_None         0x0
	#define D_CBus_Mod_Main         0x1
	#define D_CBus_Mod_Feu          0x2
	#define D_CBus_Mod_ComChan      0x3
	#define D_CBus_Mod_Pedestal     0x4
	#define D_CBus_Mod_Threshold    0x5
	#define D_CBus_Mod_UdpChan      0x6
	#define D_CBus_Mod_TrigInt      0x9
	#define D_CBus_Mod_SelfTrig     0xA
	#define D_CBus_Mod_EmbededDream 0xB
	#define D_CBus_Mod_EmbededTbMgr 0xC
	#define D_CBus_Mod_DetSigGen    0xD
	#define D_CBus_Mod_DrmClk       D_CBus_Mod_DetSigGen
	#define D_CBus_Mod_TrigGen      0xE
	#define D_CBus_Mod_DreamEmulCol 0xF

	// Offset in local bus address for CSR module fields and mask
	#define D_CBus_ModType_Ofs 20
	#define D_CBus_ModType_Len  4

	// User macros
	#define D_CBus_GetModType( adr )       GetBits(adr, D_CBus_ModType_Ofs, D_CBus_ModType_Len )
	#define D_CBus_ClrModType( adr )       PutBits(adr, D_CBus_ModType_Ofs, D_CBus_ModType_Len, D_CBus_Mod_None )
	#define D_CBus_SetModType( adr, typ )  PutBits(adr, D_CBus_ModType_Ofs, D_CBus_ModType_Len, typ )

#endif // #ifndef CBUS_H
