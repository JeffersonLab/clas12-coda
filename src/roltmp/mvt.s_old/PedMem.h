/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    PedMem.h
-- Description:    Pedestal memory declarations
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2012/09/07 IM
-- Revision:       
--
--------------------------------------------------------------------------------
*/

#ifndef H_PedMem
#define H_PedMem

	#include "bitmanip.h"

	// Address space
	#define D_PedMem_Adr_Ofs  0
	#define D_PedMem_Len     15

	// Address space is divided to registry and memory zones
	#define D_PedMem_AdrZone_Reg 0
	#define D_PedMem_AdrZone_Mem 1

	// Offset and Length of address zone field  
	#define D_PedMem_AdrZone_Ofs 14
	#define D_PedMem_AdrZone_Len  1

	// Offset and Length of memory address zone field  
	#define D_PedMem_MemAdr_Ofs   0
	#define D_PedMem_MemAdr_Len  14
	#define D_PedMem_Mem_Size    (1<<D_PedMem_MemAdr_Len)

	// The memory zone is up to 4 x 1k 32-bit words
	#define C_PedMem_BramAdr_Ofs   0
	#define C_PedMem_BramAdr_Len    12
	#define C_PedMem_BramId_Ofs   12
	#define C_PedMem_BramId_Len      2

	// Currently There are no registry

	// Address manipulation user macros
	#define D_PedMem_Adr_Get( adr )            GetBits(adr, D_PedMem_Adr_Ofs, D_PedMem_Len )
	#define D_PedMem_Adr_Clr( adr )            ClrBits(adr, D_PedMem_Adr_Ofs, D_PedMem_Len )

	#define D_PedMem_AdrZone_Get( adr )        GetBits(adr, D_PedMem_AdrZone_Ofs, D_PedMem_AdrZone_Len )
	#define D_PedMem_AdrZone_Set( adr, zone )  PutBits(adr, D_PedMem_AdrZone_Ofs, D_PedMem_AdrZone_Len, zone )

	#define D_PedMem_AdrMem_Get( adr )         GetBits(adr, D_PedMem_MemAdr_Ofs, D_PedMem_MemAdr_Len )
	#define D_PedMem_AdrMem_Clr( adr )         ClrBits(adr, D_PedMem_MemAdr_Ofs, D_PedMem_MemAdr_Len )
	#define D_PedMem_AdrMem_Set( adr, subadr ) PutBits(adr, D_PedMem_MemAdr_Ofs, D_PedMem_MemAdr_Len, subadr )

	#define D_PedMem_BramAdr_Get( adr )         GetBits(adr, C_PedMem_BramAdr_Ofs, C_PedMem_BramAdr_Len )
	#define D_PedMem_BramAdr_Clr( adr )         ClrBits(adr, C_PedMem_BramAdr_Ofs, C_PedMem_BramAdr_Len )
	#define D_PedMem_BramAdr_Set( adr, subadr ) PutBits(adr, C_PedMem_BramAdr_Ofs, C_PedMem_BramAdr_Len, subadr )

	#define D_PedMem_BramId_Get( adr )         GetBits(adr, C_PedMem_BramId_Ofs, C_PedMem_BramId_Len )
	#define D_PedMem_BramId_Clr( adr )         ClrBits(adr, C_PedMem_BramId_Ofs, C_PedMem_BramId_Len )
	#define D_PedMem_BramId_Set( adr, subadr ) PutBits(adr, C_PedMem_BramId_Ofs, C_PedMem_BramId_Len, subadr )

#endif //#ifndef H_PedMem
