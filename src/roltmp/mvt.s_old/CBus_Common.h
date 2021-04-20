/*
-------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    Cbus_Common.h
-- Description:    Control Bus registers common to all projects library
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE / XPS 10.1 
-- 
-- Create Date:    0.0 2011/01/27 IM
-- Revision:       1.0 2011/02/14 IM Raw data read error counter added
--                     2011/02/15 IM Last Event register added
--                     2011/02/18 IM Trigger drop counter added
--                     2011/02/21 IM Clear stat function added
--                     2012/02/23 IM Local trigger throttling added
--                     2014/04/16 IM Clear EvtCntr, Tstp & Resync commands added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_CBus_Common
#define H_CBus_Common

#include "bitmanip.h"

// Main module address space
#define D_Main_Adr_Ofs  0
#define D_Main_Adr_Len  6

// Register Addresses
#define D_MainAdr_RegCommand      0x00
#define D_MainAdr_RegConfig       0x04
#define D_MainAdr_RegTrigConfig   0x08
#define D_MainAdr_RegStatus       0x0C
#define D_MainAdr_RegFwRev        0x10
#define D_MainAdr_RegSwRev        0x14
#define D_MainAdr_RegTrigAcptCntr 0x18
#define D_MainAdr_RegTrigDropCntr 0x1C
#define D_MainAdr_RegError        0x20
#define D_MainAdr_RegLastEvent    0x24

#define D_MainAdr_Get( adr )         GetBits(adr, D_Main_Adr_Ofs, D_Main_Adr_Len )
#define D_MainAdr_Clr( adr )         ClrBits(adr, D_Main_Adr_Ofs, D_Main_Adr_Len )
#define D_MainAdr_Set( adr, val )    PutBits(adr, D_Main_Adr_Ofs, D_Main_Adr_Len, val )

/*
// Command register
	--    10     |   9    |   8    |     7      |     6      |     5     |    4      |   3   |  2  |     1     |   0   |
	-- DrmClkRst | EthRst | Resync | ClrTimeStp | ClrEvtCntr | ClearStat | LatchStat | Pause | Run | Configure | Reset |
	--     0     |   0    |   0    |      0     |      0     |       0   |      0    |    0  |   0 |      0    |    0  |
*/
#define D_Main_Cmd_Reset_Ind       0
#define D_Main_Cmd_Config_Ind      1
#define D_Main_Cmd_Run_Ind         2
#define D_Main_Cmd_Pause_Ind       3
#define D_Main_Cmd_Latch_Ind       4
#define D_Main_Cmd_Clear_Ind       5
#define D_Main_Cmd_ClrEvtCntr_Ind  6
#define D_Main_Cmd_ClrTstp_Ind     7
#define D_Main_Cmd_ReSync_Ind      8
#define D_Main_Cmd_EthRst_Ind      9
#define D_Main_Cmd_DrmClkRst_Ind  10
#define D_Main_Cmd_HwRst_Ind      11

// Command register field manipulation macros
#define D_Main_Cmd_Reset_Get( word )       GetBits(word, D_Main_Cmd_Reset_Ind, 1 )
#define D_Main_Cmd_Reset_Clr( word )       ClrBits(word, D_Main_Cmd_Reset_Ind, 1 )
#define D_Main_Cmd_Reset_Set( word )       SetBits(word, D_Main_Cmd_Reset_Ind, 1 )

#define D_Main_Cmd_Config_Get( word )      GetBits(word, D_Main_Cmd_Config_Ind, 1 )
#define D_Main_Cmd_Config_Clr( word )      ClrBits(word, D_Main_Cmd_Config_Ind, 1 )
#define D_Main_Cmd_Config_Set( word )      SetBits(word, D_Main_Cmd_Config_Ind, 1 )

#define D_Main_Cmd_Run_Get( word )         GetBits(word, D_Main_Cmd_Run_Ind, 1 )
#define D_Main_Cmd_Run_Clr( word )         ClrBits(word, D_Main_Cmd_Run_Ind, 1 )
#define D_Main_Cmd_Run_Set( word )         SetBits(word, D_Main_Cmd_Run_Ind, 1 )

#define D_Main_Cmd_Pause_Get( word )       GetBits(word, D_Main_Cmd_Pause_Ind, 1 )
#define D_Main_Cmd_Pause_Clr( word )       ClrBits(word, D_Main_Cmd_Pause_Ind, 1 )
#define D_Main_Cmd_Pause_Set( word )       SetBits(word, D_Main_Cmd_Pause_Ind, 1 )

#define D_Main_Cmd_Latch_Get( word )       GetBits(word, D_Main_Cmd_Latch_Ind, 1 )
#define D_Main_Cmd_Latch_Clr( word )       ClrBits(word, D_Main_Cmd_Latch_Ind, 1 )
#define D_Main_Cmd_Latch_Set( word )       SetBits(word, D_Main_Cmd_Latch_Ind, 1 )

#define D_Main_Cmd_Clear_Get( word )       GetBits(word, D_Main_Cmd_Clear_Ind, 1 )
#define D_Main_Cmd_Clear_Clr( word )       ClrBits(word, D_Main_Cmd_Clear_Ind, 1 )
#define D_Main_Cmd_Clear_Set( word )       SetBits(word, D_Main_Cmd_Clear_Ind, 1 )

#define D_Main_Cmd_ClrEvtCntr_Get( word ) GetBits(word, D_Main_Cmd_ClrEvtCntr_Ind, 1 )
#define D_Main_Cmd_ClrEvtCntr_Clr( word ) ClrBits(word, D_Main_Cmd_ClrEvtCntr_Ind, 1 )
#define D_Main_Cmd_ClrEvtCntr_Set( word ) SetBits(word, D_Main_Cmd_ClrEvtCntr_Ind, 1 )

#define D_Main_Cmd_ClrTstp_Get( word ) GetBits(word, D_Main_Cmd_ClrTstp_Ind, 1 )
#define D_Main_Cmd_ClrTstp_Clr( word ) ClrBits(word, D_Main_Cmd_ClrTstp_Ind, 1 )
#define D_Main_Cmd_ClrTstp_Set( word ) SetBits(word, D_Main_Cmd_ClrTstp_Ind, 1 )

#define D_Main_Cmd_ReSync_Get( word ) GetBits(word, D_Main_Cmd_ReSync_Ind, 1 )
#define D_Main_Cmd_ReSync_Clr( word ) ClrBits(word, D_Main_Cmd_ReSync_Ind, 1 )
#define D_Main_Cmd_ReSync_Set( word ) SetBits(word, D_Main_Cmd_ReSync_Ind, 1 )

#define D_Main_Cmd_DrmClkRst_Get( word ) GetBits(word, D_Main_Cmd_DrmClkRst_Ind, 1 )
#define D_Main_Cmd_DrmClkRst_Clr( word ) ClrBits(word, D_Main_Cmd_DrmClkRst_Ind, 1 )
#define D_Main_Cmd_DrmClkRst_Set( word ) SetBits(word, D_Main_Cmd_DrmClkRst_Ind, 1 )

#define D_Main_Cmd_DrmClkRst_Get( word ) GetBits(word, D_Main_Cmd_DrmClkRst_Ind, 1 )
#define D_Main_Cmd_DrmClkRst_Clr( word ) ClrBits(word, D_Main_Cmd_DrmClkRst_Ind, 1 )
#define D_Main_Cmd_DrmClkRst_Set( word ) SetBits(word, D_Main_Cmd_DrmClkRst_Ind, 1 )

#define D_Main_Cmd_HwRst_Get( word ) GetBits(word, D_Main_Cmd_HwRst_Ind, 1 )
#define D_Main_Cmd_HwRst_Clr( word ) ClrBits(word, D_Main_Cmd_HwRst_Ind, 1 )
#define D_Main_Cmd_HwRst_Set( word ) SetBits(word, D_Main_Cmd_HwRst_Ind, 1 )


/*
-- Configuration register
-- Trigger interface clock source
--   '00' - On-board clock
--   '01' - Clock from on-board trigger interface connector  
--   '1x' - Recovered Rocket IO clock
*/
typedef enum _TrigClk_Src
{
	OnBoardClk  = 0,
	TrgIfConClk = 1,
	RecClk      = 2
} TrigClk_Src;
char *TrigClk_Src2Str( TrigClk_Src src );
/*
-- AdcDtp
--    Forces ADC to output digital data pattern
-- DataPipeLen
--    Determins Dummu Dream data pipeline length in Wclk ticks
--    Example: for 16 µs trigger latency with 48ns sampling clock period
--    333 clock ticks are needed, hense the 9 bits
-- Mask
-- 	a bit per Dream, 1 - masked, 0 - active
--    Up to 8 Dreams, hense 8 bits
-- NbOfSamples
-- 	Current absolute maximum for Dreams is 256 cels / channel, hense 8 bits
--
-- |   27-26    |   25   |    24-16    | 15-8 |     7-0     |
-- |Clock Select| AdcDtp | DataPipeLen | Mask | NbOfSamples |
-- |    0       |   0    |     333     |   0  |      4      |
*/
#define D_Main_Conf_Samples_Ofs      0
#define D_Main_Conf_Samples_Len         8
#define D_Main_Conf_Mask_Ofs         8
#define D_Main_Conf_Mask_Len            8
#define D_Main_Conf_DataPipeLen_Ofs 16
#define D_Main_Conf_DataPipeLen_Len     9
#define D_Main_Conf_AdcDtp_Ind      25
#define D_Main_Conf_ClkSel_Ofs      26 
#define D_Main_Conf_ClkSel_Len          2
// Configuration register field manipulation macros
#define D_Main_Conf_Samples_Get( word )      GetBits(word, D_Main_Conf_Samples_Ofs, D_Main_Conf_Samples_Len )
#define D_Main_Conf_Samples_Set( word, val ) PutBits(word, D_Main_Conf_Samples_Ofs, D_Main_Conf_Samples_Len, val )

#define D_Main_Conf_Mask_Get( word )         GetBits(word, D_Main_Conf_Mask_Ofs, D_Main_Conf_Mask_Len )
#define D_Main_Conf_Mask_Set( word, val )    PutBits(word, D_Main_Conf_Mask_Ofs, D_Main_Conf_Mask_Len, val )

#define D_Main_Conf_PipeLen_Get( word )      GetBits(word, D_Main_Conf_DataPipeLen_Ofs, D_Main_Conf_DataPipeLen_Len )
#define D_Main_Conf_PipeLen_Set( word, val ) PutBits(word, D_Main_Conf_DataPipeLen_Ofs, D_Main_Conf_DataPipeLen_Len, val )

#define D_Main_Conf_AdcDtp_Get( word )       GetBits(word, D_Main_Conf_AdcDtp_Ind, 1 )
#define D_Main_Conf_AdcDtp_Clr( word )       ClrBits(word, D_Main_Conf_AdcDtp_Ind, 1 )
#define D_Main_Conf_AdcDtp_Set( word, val )  SetBits(word, D_Main_Conf_AdcDtp_Ind, 1, val )

#define D_Main_Conf_ClkSel_Get( word )       GetBits(word, D_Main_Conf_ClkSel_Ofs, D_Main_Conf_ClkSel_Len )
#define D_Main_Conf_ClkSel_Set( word, val )  PutBits(word, D_Main_Conf_ClkSel_Ofs, D_Main_Conf_ClkSel_Len, val )

#define D_Main_Conf_Set( reg, smp, msk, dpl, adcdtp, clksel ) \
	PutBits \
	( \
		reg, D_Main_Conf_Samples_Ofs, \
		(D_Main_Conf_Samples_Len+D_Main_Conf_Mask_Len + D_Main_Conf_DataPipeLen_Len + 1 + D_Main_Conf_ClkSel_Len), \
		(clksel << D_Main_Conf_ClkSel_Ofs ) + (adcdtp<< D_Main_Conf_AdcDtp_Ind) + \
		(dpl<<D_Main_Conf_DataPipeLen_Ofs) + (msk<<D_Main_Conf_Mask_Ofs) + (smp) \
	)

// Default value
#define D_Def_Main_Conf D_Main_Conf_Set( 0, 4, 0, 332, 0, 0 )


/*
-- Status register
-- Trigger interface clock source
--   '00' - On-board clock
--   '01' - Clock from on-board trigger interface connector  
--   '1x' - Recovered Rocket IO clock
-- Number of samples:
-- 	Current absolute maximum for Dreams is 256 cels / channel, hense 8 bits
--
--     21     |      20      | 19-16  |  15-12  |   11-8  |    7-0     |
-- Configured | Clocks Valid | Rc FSM | Acq FSM |  Status | NbOfDreams |
*/
#define D_Main_Stat_NbOfDreams_Ofs  0
#define D_Main_Stat_NbOfDreams_Len    8
#define D_Main_Stat_Status_Ofs      8
#define D_Main_Stat_Status_Len        4 
#define D_Main_Stat_AcqFsm_Ofs     12 
#define D_Main_Stat_AcqFsm_Len        4 
#define D_Main_Stat_RcFsm_Ofs      16 
#define D_Main_Stat_RcFsm_Len         4 
#define D_Main_Stat_ClkValid_Ind   20 
#define D_Main_Stat_Configured_Ind 21 

// Status register field manipulation macros
#define D_Main_Stat_NbOfDreams_Get( word ) GetBits(word, D_Main_Stat_NbOfDreams_Ofs, D_Main_Stat_NbOfDreams_Len )
#define D_Main_Stat_Status_Get( word )     GetBits(word, D_Main_Stat_Status_Ofs, D_Main_Stat_Status_Len )
#define D_Main_Stat_AcqFsm_Get( word )     GetBits(word, D_Main_Stat_AcqFsm_Ofs, D_Main_Stat_AcqFsm_Len )
#define D_Main_Stat_RcFsm_Get( word )      GetBits(word, D_Main_Stat_RcFsm_Ofs, D_Main_Stat_RcFsm_Len )
#define D_Main_Stat_ClkValid_Get( word )   GetBits(word, D_Main_Stat_ClkValid_Ind, 1 )
#define D_Main_Stat_Configured_Get( word ) GetBits(word, D_Main_Stat_Configured_Ind, 1 )

/*
-- Configuration register for Trigger logic
-- |     30     |   29-24  |   23-18  |   17-12  |    11-0   |
-- |LocTrigThrot|OvrThersh |OvrWrnHwm |OvrWrnLwm | TimeStamp |
-- |      0     |    60    |    48    |    24    |     0     |
*/
#define D_Main_Trig_TimeStamp_Ofs 0
#define D_Main_Trig_TimeStamp_Len   12
#define D_Main_Trig_OvrWrnLwm_Ofs 12
#define D_Main_Trig_OvrWrnHwm_Ofs 18
#define D_Main_Trig_OvrThersh_Ofs 24 
#define D_Main_Trig_OvrFields_Len    6
#define D_Main_Trig_LocThrot_Ind  30 

// Trigger configuration register field manipulation macros
#define D_Main_Trig_TimeStamp_Get( word )      GetBits(word, D_Main_Trig_TimeStamp_Ofs, D_Main_Trig_TimeStamp_Len )
#define D_Main_Trig_TimeStamp_Set( word, val ) PutBits(word, D_Main_Trig_TimeStamp_Ofs, D_Main_Trig_TimeStamp_Len, val )

#define D_Main_Trig_OvrWrnLwm_Get( word )      GetBits(word, D_Main_Trig_OvrWrnLwm_Ofs, D_Main_Trig_OvrFields_Len )
#define D_Main_Trig_OvrWrnLwm_Set( word, val ) PutBits(word, D_Main_Trig_OvrWrnLwm_Ofs, D_Main_Trig_OvrFields_Len, val )

#define D_Main_Trig_OvrWrnHwm_Get( word )      GetBits(word, D_Main_Trig_OvrWrnHwm_Ofs, D_Main_Trig_OvrFields_Len )
#define D_Main_Trig_OvrWrnHwm_Set( word, val ) PutBits(word, D_Main_Trig_OvrWrnHwm_Ofs, D_Main_Trig_OvrFields_Len, val )

#define D_Main_Trig_OvrThersh_Get( word )      GetBits(word, D_Main_Trig_OvrThersh_Ofs, D_Main_Trig_OvrFields_Len )
#define D_Main_Trig_OvrThersh_Set( word, val ) PutBits(word, D_Main_Trig_OvrThersh_Ofs, D_Main_Trig_OvrFields_Len, val )

#define D_Main_Trig_LocThrot_Get( word )      GetBits(word, D_Main_Trig_LocThrot_Ind, 1 )
#define D_Main_Trig_LocThrot_Clr( word )      ClrBits(word, D_Main_Trig_LocThrot_Ind, 1 )
#define D_Main_Trig_LocThrot_Set( word )      SetBits(word, D_Main_Trig_LocThrot_Ind, 1 )
#define D_Main_Trig_LocThrot_Put( word, val ) PutBits(word, D_Main_Trig_LocThrot_Ind, 1, val )

#define D_Main_Trig_Set( reg, ts, lwm, hwm, thersh, throt ) \
	PutBits \
	( \
		reg, \
		D_Main_Trig_TimeStamp_Ofs, \
		(D_Main_Trig_TimeStamp_Len+3*D_Main_Trig_OvrFields_Len +1), \
		(((throt)&0x01)  << D_Main_Trig_LocThrot_Ind ) + \
		(((thersh)&0x3F) << D_Main_Trig_OvrThersh_Ofs) + \
		(((hwm)&0x3F)    << D_Main_Trig_OvrWrnHwm_Ofs) + \
		(((lwm)&0x3F)    << D_Main_Trig_OvrWrnLwm_Ofs) + \
		( (ts)&0xFFF)                                    \
	)

// Default value
#define D_Def_Main_Trig (D_Main_Trig_Set( 0, 0, 23, 47, 56, 0 ) & 0xFFFFFF)


/*
-- Firmware Revision REG definitions
-- | 31-28| 27-26|      25-8     | 7-0 |
-- | SN3:0| Type |      Date     | Rev |
*/
/*
-- Software Revision REG definitions
-- | 31-28| 27-26|      25-8     | 7-0 |
-- | SN7:4| Type |      Date     | Rev |
*/
#define D_Main_Rev_Min_Ofs  0
#define D_Main_Rev_Min_Len    4
#define D_Main_Rev_Maj_Ofs  4
#define D_Main_Rev_Maj_Len    4
#define D_Main_Rev_Dat_Ofs  8
#define D_Main_Rev_Dat_Len   18
#define D_Main_Rev_Typ_Ofs 26
#define D_Main_Rev_Typ_Len    2
#define D_Main_Rev_SrN_Ofs 28
#define D_Main_Rev_SrN_Len    4

// Software and firmware revision register field manipulation macros
#define D_Main_Rev_GetMin( word )      GetBits(word, D_Main_Rev_Min_Ofs, D_Main_Rev_Min_Len )
#define D_Main_Rev_SetMin( word, val ) PutBits(word, D_Main_Rev_Min_Ofs, D_Main_Rev_Min_Len, val )

#define D_Main_Rev_GetMaj( word )      GetBits(word, D_Main_Rev_Maj_Ofs, D_Main_Rev_Maj_Len )
#define D_Main_Rev_SetMaj( word, val ) PutBits(word, D_Main_Rev_Maj_Ofs, D_Main_Rev_Maj_Len, val )

#define D_Main_Rev_GetDat( word )      GetBits(word, D_Main_Rev_Dat_Ofs, D_Main_Rev_Dat_Len )
#define D_Main_Rev_SetDat( word, val ) PutBits(word, D_Main_Rev_Dat_Ofs, D_Main_Rev_Dat_Len, val )

#define D_Main_Rev_GetTyp( word )      GetBits(word, D_Main_Rev_Typ_Ofs, D_Main_Rev_Typ_Len )
#define D_Main_Rev_SetTyp( word, val ) PutBits(word, D_Main_Rev_Typ_Ofs, D_Main_Rev_Typ_Len, val )

#define D_Main_Rev_GetSrN( word )      GetBits(word, D_Main_Rev_SrN_Ofs, D_Main_Rev_SrN_Len )

#define D_Main_Rev_GetRev( word ) \
	GetBits\
	( \
			word, D_Main_Rev_Min_Ofs, \
			(D_Main_Rev_Min_Len+D_Main_Rev_Maj_Len+D_Main_Rev_Dat_Len+D_Main_Rev_Typ_Len) \
	)

#define D_Main_Rev_Set( word, typ, dat, maj, min ) \
	PutBits \
	( \
		word, D_Main_Rev_Min_Ofs, \
		(D_Main_Rev_Min_Len+D_Main_Rev_Maj_Len+D_Main_Rev_Dat_Len+D_Main_Rev_Typ_Len), \
		(typ<<D_Main_Rev_Typ_Ofs) + (dat << D_Main_Rev_Dat_Ofs) + (maj<< D_Main_Rev_Maj_Ofs) + (min) \
	)
// Software type definitions
typedef enum _SwType
{
	SwType_FeuClas12 = 0,
	SwType_FeuAsacusa = 1,
	SwType_FeuLowLevelTests = 2,
	SwType_TbDream = 3
} SwType;
// firmware type definitions
typedef enum _FwType
{
	FwType_FeuClas12 = 0,
	FwType_FeuAsacusa = 1,
	FwType_FeuProto = 2,
	FwType_TbDream = 3
} FwType;
/*
-- Trigger Accept counter definitions
-- |      31-0      |
-- | TrigAcceptCntr |
-- This is 32 bit counter, no special definitions or macros needed
*/

/*
-- Trigger Drop counter definitions
-- |     21-16      |     15-8     |      7-0     |
-- | TrigFifoMaxOcc | FifoDropCntr | TrigDropCntr |
*/
#define D_Main_Drop_TrigCntr_Ofs   0
#define D_Main_Drop_TrigCntr_Len      8
#define D_Main_Drop_FifoCntr_Ofs   8
#define D_Main_Drop_FifoCntr_Len      8
#define D_Main_Drop_FifoOcup_Ofs   16
#define D_Main_Drop_FifoOcup_Len      6

// Field register field manipulation macros
#define D_Main_Drop_TrigCntr_Get( word ) GetBits(word, D_Main_Drop_TrigCntr_Ofs, D_Main_Drop_TrigCntr_Len )
#define D_Main_Drop_FifoCntr_Get( word ) GetBits(word, D_Main_Drop_FifoCntr_Ofs, D_Main_Drop_FifoCntr_Len )
#define D_Main_Drop_FifoOcup_Get( word ) GetBits(word, D_Main_Drop_FifoOcup_Ofs, D_Main_Drop_FifoOcup_Len )

/*
// Error register
	--    24   |   23-16   |   15-0     |
	-- CombErr | RdErrCntr | DreamRdErr |
*/
#define D_Main_RegError_DreamRdErr_Ofs    0	
#define D_Main_RegError_DreamRdErr_Len      16 
#define D_Main_RegError_RdErrCntr_Ofs    16	
#define D_Main_RegError_RdErrCntr_Len       8 
#define D_Main_RegError_CombErr_Ind      24 

// Field manipulation macros
#define D_Main_Error_DreamRdErr_Get( word ) GetBits(word, D_Main_RegError_DreamRdErr_Ofs, D_Main_RegError_DreamRdErr_Len )
#define D_Main_Error_RdErrCntr_Get(  word ) GetBits(word, D_Main_RegError_RdErrCntr_Ofs,  D_Main_RegError_RdErrCntr_Len )
#define D_Main_Error_CombErr_Get(    word ) GetBits(word, D_Main_RegError_CombErr_Ind,    1 )

/*
// Last Event register
	-- 26-24 |   23-12   |  11-0   |
	-- Delay | TimeStamp | EventId |
*/
#define D_Main_RegLastEvent_Id_Ofs      0	
#define D_Main_RegLastEvent_Id_Len        12 
#define D_Main_RegLastEvent_Tstp_Ofs   12	
#define D_Main_RegLastEvent_Tstp_Len      12 
#define D_Main_RegLastEvent_Del_Ofs    24	
#define D_Main_RegLastEvent_Del_Len        3 

// Field manipulation macros
#define D_Main_LastEvent_Id_Get(  word )  GetBits(word, D_Main_RegLastEvent_Id_Ofs,   D_Main_RegLastEvent_Id_Len )
#define D_Main_LastEvent_Tstp_Get( word ) GetBits(word, D_Main_RegLastEvent_Tstp_Ofs, D_Main_RegLastEvent_Tstp_Len )
#define D_Main_LastEvent_Del_Get(  word ) GetBits(word, D_Main_RegLastEvent_Del_Ofs,  D_Main_RegLastEvent_Del_Len )

// Control and status register structure
typedef struct _CBus_CommonCsr
{
	unsigned int Command;
	unsigned int Config;
	unsigned int TrigConfig;
	unsigned int Status;
	unsigned int Error;
	unsigned int LastEvent;
	unsigned int FwRev;
	unsigned int SwRev;
	unsigned int TrigActpCntr;
	unsigned int TrigDropCntr;
} CBus_CommonCsr;

int CBus_CommonCsr_Init(   CBus_CommonCsr *csr);
int CBus_CommonCsr_Sprintf(CBus_CommonCsr *csr, char *buf);
int CBus_CommonCsr_Rev2Str(CBus_CommonCsr *csr, char *buf);
int CBus_CommonCsr_Get(    CBus_CommonCsr *csr);

int CBus_Common_Config( unsigned int config, unsigned int TrigConfig );
int CBus_Common_GetReg( unsigned int subadr, unsigned int *val);
int CBus_Common_SetReg( unsigned int subadr, unsigned int  val);

int CBus_Common_LatchStat();
int CBus_Common_ClearStat();

// Define clock periods
// Core clock 125 MHz period 8 ns
#define D_CoreClkPeriod_ns 8

#endif // #ifndef H_CBus_Common
