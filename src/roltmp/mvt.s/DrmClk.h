/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    DrmClk.h
-- Description:    FEU Dream clock dynamic reconfiguration library
--
-- Target Devices: Virtex-6 / FEU
-- Tool versions:  ISE 14.1
-- 
-- Create Date:    0.0 2014/12/09 IM
-- Revision:
--
-- Comments: See Xilinx app note 878 on MMCM dynamic reconfiguration
--           Only subset of registers are implemented
--
--------------------------------------------------------------------------------
*/

#ifndef H_TrgClk
#define H_TrgClk

	#include "bitmanip.h"

	// Trigger clock module address space
	#define D_TrgClk_Adr_Ofs  0
	#define D_TrgClk_Adr_Len  9

	// Offset and Length of trigger clock module registers address zone field
	#define D_TrgClk_AdrReg_Get( adr )         GetBits(adr, D_TrgClk_Adr_Ofs, D_TrgClk_Adr_Len )
	#define D_TrgClk_AdrReg_Set( adr, subadr ) PutBits(adr, D_TrgClk_Adr_Ofs, D_TrgClk_Adr_Len, subadr )

	// Register addresses
	// CLK OUT 1 Divide
	#define C_TrgClk_RdClk_Freq       (0x08 << 2)
	// CLK OUT 2 Divide & Phase
	#define C_TrgClk_WrClk_Freq       (0x0A << 2)
	#define C_TrgClk_WrClk_Phase      (0x0B << 2)
	// CLK OUT 3 Divide & Phase
	#define C_TrgClk_AdcClk_Freq      (0x0C << 2)
	#define C_TrgClk_AdcClk_Phase     (0x0D << 2)
	// Power bits
	#define C_TrgClk_PwrBits          (0x28 << 2)

	/*
	-- Frequency Register
	--|PHASE MUX | RSVD | HIGH TIME | LOW TIME
	--|   15-13  |  12  |   11-6    |   5-0
	*/
	#define D_TrgClk_RegFreq_PhaseMux_Ofs  13
	#define D_TrgClk_RegFreq_PhaseMux_Len      3
	#define D_TrgClk_RegFreq_HighTime_Ofs   6
	#define D_TrgClk_RegFreq_LowTime_Ofs    0
	#define D_TrgClk_RegFreq_Time_Len          6

	// Field manipulation macros
	#define D_TrgClk_RegFreq_HighTime_Get( word )       GetBits(word, D_TrgClk_RegFreq_HighTime_Ofs, D_TrgClk_RegFreq_Time_Len )
	#define D_TrgClk_RegFreq_HighTime_Clr( word )       ClrBits(word, D_TrgClk_RegFreq_HighTime_Ofs, D_TrgClk_RegFreq_Time_Len )
	#define D_TrgClk_RegFreq_HighTime_Set( word, val )  PutBits(word, D_TrgClk_RegFreq_HighTime_Ofs, D_TrgClk_RegFreq_Time_Len, val )

	// Field manipulation macros
	#define D_TrgClk_RegFreq_LowTime_Get( word )       GetBits(word, D_TrgClk_RegFreq_LowTime_Ofs, D_TrgClk_RegFreq_Time_Len )
	#define D_TrgClk_RegFreq_LowTime_Clr( word )       ClrBits(word, D_TrgClk_RegFreq_LowTime_Ofs, D_TrgClk_RegFreq_Time_Len )
	#define D_TrgClk_RegFreq_LowTime_Set( word, val )  PutBits(word, D_TrgClk_RegFreq_LowTime_Ofs, D_TrgClk_RegFreq_Time_Len, val )

	/*
	-- Phase Register
	--| RSVD| MX|EDGE|NO COUNT|DELEY TIME
	--|15-10|9-8|  7 |  6     |    5-0
	*/
	#define D_TrgClk_RegPhase_Mx_Ofs     8
	#define D_TrgClk_RegPhase_Mx_Len       2
	#define D_TrgClk_RegFreq_Edge_Ind    7
	#define D_TrgClk_RegFreq_NoCnt_Ind   6
	#define D_TrgClk_RegFreq_Delay_Ofs   0
	#define D_TrgClk_RegFreq_Dealy_Len     6

	// Field manipulation macros
	#define D_TrgClk_RegPhase_Delay_Get( word )       GetBits(word, D_TrgClk_RegFreq_Delay_Ofs, D_TrgClk_RegFreq_Dealy_Len )
	#define D_TrgClk_RegPhase_Delay_Clr( word )       ClrBits(word, D_TrgClk_RegFreq_Delay_Ofs, D_TrgClk_RegFreq_Dealy_Len )
	#define D_TrgClk_RegPhase_Delay_Set( word, val )  PutBits(word, D_TrgClk_RegFreq_Delay_Ofs, D_TrgClk_RegFreq_Dealy_Len, val )

	#define D_TrgClk_RegPhase_Mx_Get( word )          GetBits(word, D_TrgClk_RegPhase_Mx_Ofs, D_TrgClk_RegPhase_Mx_Len )
	#define D_TrgClk_RegPhase_Mx_Clr( word )          ClrBits(word, D_TrgClk_RegPhase_Mx_Ofs, D_TrgClk_RegPhase_Mx_Len )
	#define D_TrgClk_RegPhase_Mx_Set( word, val )     PutBits(word, D_TrgClk_RegPhase_Mx_Ofs, D_TrgClk_RegPhase_Mx_Len, val )

	/*
	-- Power register
	--|POWER|
	--| 15-0|
	*/
	#define D_TrgClk_RegPower_Ofs   0
	#define D_TrgClk_RegPower_Len     16

	#define D_TrgClk_RegPower_Get( word )          GetBits(word, D_TrgClk_RegPower_Ofs, D_TrgClk_RegPower_Len )
	#define D_TrgClk_RegPower_Clr( word )          ClrBits(word, D_TrgClk_RegPower_Ofs, D_TrgClk_RegPower_Len )
	#define D_TrgClk_RegPower_Set( word )          SetBits(word, D_TrgClk_RegPower_Ofs, D_TrgClk_RegPower_Len )

	// Optional message container
	extern char *trg_clk_msg;

#endif // #ifndef H_TrgClk
