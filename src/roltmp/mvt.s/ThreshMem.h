/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    ThreshMem.h
-- Description:    Threshold memory declarations
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2012/09/07 IM
-- Revision:       
--
--------------------------------------------------------------------------------
*/

#ifndef H_TheshMem
#define H_TheshMem

	#include "bitmanip.h"

	// Address space
	#define D_ThreshMem_Adr_Ofs  0
	#define D_ThreshMem_Len     12

	// Address space is divided to registry and memory zones
	#define D_ThreshMem_AdrZone_Reg 0
	#define D_ThreshMem_AdrZone_Mem 1

	// Offset and Length of address zone field  
	#define D_ThreshMem_AdrZone_Ofs 11
	#define D_ThreshMem_AdrZone_Len  1

	// Offset and Length of memory address zone field  
	#define D_ThreshMem_MemAdr_Ofs   0
	#define D_ThreshMem_MemAdr_Len  10
	#define D_ThreshMem_Mem_Size    (1<<D_ThreshMem_MemAdr_Len)

	// The memory zone is up to 8 x 64 32-bit words
	#define C_ThreshMem_BramAdr_Ofs  0
	#define C_ThreshMem_BramAdr_Len    8
	#define C_ThreshMem_BramId_Ofs   8
	#define C_ThreshMem_BramId_Len     2

	// Currently There are no registry

	// Address manipulation user macros
	#define D_ThreshMem_Adr_Get( adr )            GetBits(adr, D_ThreshMem_Adr_Ofs, D_ThreshMem_Len )
	#define D_ThreshMem_Adr_Clr( adr )            ClrBits(adr, D_ThreshMem_Adr_Ofs, D_ThreshMem_Len )

	#define D_ThreshMem_AdrZone_Get( adr )        GetBits(adr, D_ThreshMem_AdrZone_Ofs, D_ThreshMem_AdrZone_Len )
	#define D_ThreshMem_AdrZone_Set( adr, zone )  PutBits(adr, D_ThreshMem_AdrZone_Ofs, D_ThreshMem_AdrZone_Len, zone )

	#define D_ThreshMem_AdrMem_Get( adr )         GetBits(adr, D_ThreshMem_MemAdr_Ofs, D_ThreshMem_MemAdr_Len )
	#define D_ThreshMem_AdrMem_Clr( adr )         ClrBits(adr, D_ThreshMem_MemAdr_Ofs, D_ThreshMem_MemAdr_Len )
	#define D_ThreshMem_AdrMem_Set( adr, subadr ) PutBits(adr, D_ThreshMem_MemAdr_Ofs, D_ThreshMem_MemAdr_Len, subadr )

	#define D_ThreshMem_BramAdr_Get( adr )         GetBits(adr, C_ThreshMem_BramAdr_Ofs, C_ThreshMem_BramAdr_Len )
	#define D_ThreshMem_BramAdr_Clr( adr )         ClrBits(adr, C_ThreshMem_BramAdr_Ofs, C_ThreshMem_BramAdr_Len )
	#define D_ThreshMem_BramAdr_Set( adr, subadr ) PutBits(adr, C_ThreshMem_BramAdr_Ofs, C_ThreshMem_BramAdr_Len, subadr )

	#define D_ThreshMem_BramId_Get( adr )         GetBits(adr, C_ThreshMem_BramId_Ofs, C_ThreshMem_BramId_Len )
	#define D_ThreshMem_BramId_Clr( adr )         ClrBits(adr, C_ThreshMem_BramId_Ofs, C_ThreshMem_BramId_Len )
	#define D_ThreshMem_BramId_Set( adr, subadr ) PutBits(adr, C_ThreshMem_BramId_Ofs, C_ThreshMem_BramId_Len, subadr )

#endif //#ifndef H_TheshMem
