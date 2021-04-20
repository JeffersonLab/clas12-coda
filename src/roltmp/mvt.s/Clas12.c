/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    Clas12.c
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
#include "Clas12.h"

char *PartStatus2Str(PartStatus status)
{
	     if( status == PartStatus_Unknown0 )
		return( "Unknown0 " );
	else if( status == PartStatus_OverflowWarn )
		return( "OvrWarn  " );
	else if( status == PartStatus_OutOfSync )
		return( "OutOfSync" );
	else if( status == PartStatus_Forbidden3 )
		return( "Forbid3  " );
	else if( status == PartStatus_Busy )
		return( "Busy     " );
	else if( status == PartStatus_Forbidden5 )
		return( "Forbid5  " );
	else if( status == PartStatus_Forbidden6 )
		return( "Forbid6  " );
	else if( status == PartStatus_Forbidden7 )
		return( "Forbid7  " );
	else if( status == PartStatus_Ready )
		return( "Ready    " );
	else if( status == PartStatus_Forbidden9 )
		return( "Forbid9  " );
	else if( status == PartStatus_Forbidden10 )
		return( "Forbid10 " );
	else if( status == PartStatus_Forbidden11 )
		return( "Forbid11 " );
	else if( status == PartStatus_Error )
		return( "Error    " );
	else if( status == PartStatus_Forbidden13 )
		return( "Forbid13 " );
	else if( status == PartStatus_Forbidden14 )
		return( "Forbid14 " );
	else if( status == PartStatus_Unknown15 )
		return( "Unknown15" );
	else
		return( "Unknown  " );
}
