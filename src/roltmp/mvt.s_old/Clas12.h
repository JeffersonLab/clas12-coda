/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    Clas12.h
-- Description:    Clas12 library with common definitions
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/01/31 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_Clas12
#define H_Clas12

// Define clock periods
// System clock is 250 MHz - 4 ns
#define D_SystemClkPeriod_ns 4
// Trigger clock is 62.5 MHz - 16 ns
#define D_TrigClkPeriod_ns   D_SystemClkPeriod_ns * 4

// Partition status codes
typedef enum _PartStatus
{
	PartStatus_Unknown0     = 0x0,
	PartStatus_OverflowWarn = 0x1,
	PartStatus_OutOfSync    = 0x2,
	PartStatus_Forbidden3   = 0x3,
	PartStatus_Busy         = 0x4,
	PartStatus_Forbidden5   = 0x5,
	PartStatus_Forbidden6   = 0x6,
	PartStatus_Forbidden7   = 0x7,
	PartStatus_Ready        = 0x8,
	PartStatus_Forbidden9   = 0x9,
	PartStatus_Forbidden10  = 0xA,
	PartStatus_Forbidden11  = 0xB,
	PartStatus_Error        = 0xC,
	PartStatus_Forbidden13  = 0xD,
	PartStatus_Forbidden14  = 0xE,
	PartStatus_Unknown15    = 0xF
} PartStatus;
char *PartStatus2Str(PartStatus status);

#endif // #ifndef H_Clas12
