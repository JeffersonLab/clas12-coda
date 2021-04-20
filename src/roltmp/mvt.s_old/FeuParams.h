/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    Feu.h
-- Description:    Front End Unit library
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE 10.1
-- 
-- Create Date:    0.0 2011/01/27 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_Feu
#define H_Feu

	#include "bitmanip.h"

	// Dream Emulator collection address space
	#define D_Feu_Adr_Ofs  0
	#define D_Feu_Adr_Len  5

	// Offset and Length of Dream emulators collection registers address zone field  
	#define D_Feu_AdrReg_Get( adr )         GetBits(adr, D_Feu_Adr_Ofs, D_Feu_Adr_Len )
	#define D_Feu_AdrReg_Set( adr, subadr ) PutBits(adr, D_Feu_Adr_Ofs, D_Feu_Adr_Len, subadr )

	// Register addresses
	#define C_Feu_AdrReg_PwrUp     0
	#define C_Feu_AdrReg_SlowCtrl  4
	#define C_Feu_AdrReg_RunCtrl   8
	#define C_Feu_AdrReg_RunStat  12
	#define C_Feu_AdrReg_TrgStat  16
	#define C_Feu_AdrReg_Pulser   20

	/*
	-- PowerUp Register
	--|ADC|ProtFlt|     Dream     |
	--|   |       |8-7|6-5|4-3|2-1|
	--| 20|  19-4 | 3 | 2 | 1 | 0 |
	*/
	#define D_Feu_RegPwr_Dream_Ofs    0
	#define D_Feu_RegPwr_Dream_Len       4
	#define D_Feu_RegPwr_PrtFlt_Ofs   4 
	#define D_Feu_RegPwr_PrtFlt_Len     16 
	#define D_Feu_RegPwr_Adc_Ind     20

	#define D_Feu_RegPwr_Ofs     0
	#define D_Feu_RegPwr_Len       24

	// Field manipulation macros
	#define D_Feu_RegPwr_Dream_Get( word )       GetBits(word, D_Feu_RegPwr_Dream_Ofs, D_Feu_RegPwr_Dream_Len )
	#define D_Feu_RegPwr_Dream_Clr( word )       ClrBits(word, D_Feu_RegPwr_Dream_Ofs, D_Feu_RegPwr_Dream_Len )
	#define D_Feu_RegPwr_Dream_Set( word, val )  PutBits(word, D_Feu_RegPwr_Dream_Ofs, D_Feu_RegPwr_Dream_Len, val )

	#define D_Feu_RegPwr_PrtFlt_Get( word )      GetBits(word, D_Feu_RegPwr_PrtFlt_Ofs, D_Feu_RegPwr_PrtFlt_Len )
	#define D_Feu_RegPwr_PrtFlt_Clr( word )      ClrBits(word, D_Feu_RegPwr_PrtFlt_Ofs, D_Feu_RegPwr_PrtFlt_Len )
	#define D_Feu_RegPwr_PrtFlt_Set( word, val ) PutBits(word, D_Feu_RegPwr_PrtFlt_Ofs, D_Feu_RegPwr_PrtFlt_Len, val )

	#define D_Feu_RegPwr_Adc_Get( word )         GetBits(word, D_Feu_RegPwr_Adc_Ind,    1 )
	#define D_Feu_RegPwr_Adc_Clr( word )         ClrBits(word, D_Feu_RegPwr_Adc_Ind,    1 )
	#define D_Feu_RegPwr_Adc_Set( word )         SetBits(word, D_Feu_RegPwr_Adc_Ind,    1 )

	#define D_Feu_RegPwr_Get( word )             GetBits(word, D_Feu_RegPwr_Ofs, D_Feu_RegPwr_Len )

	/*
	-- Feu status register
	--|Embedded |ADC data|
	--|Dream Rdy| Valid  |
	--|    25   |   24   |
	*/
	#define D_Feu_RegStat_AdcDataValid_Ind  24
	#define D_Feu_RegStat_EmbDreamRdy_Ind   25

	#define D_Feu_RegStat_AdcDataValid_Get( word )      GetBits(word, D_Feu_RegStat_AdcDataValid_Ind, 1 )
	#define D_Feu_RegStat_EmbDreamRdy_Get(  word )      GetBits(word, D_Feu_RegStat_EmbDreamRdy_Ind,  1 )

	/*
	-- Slow Control Register
	--Sema-| OneWire|   Pulser    |OneWire|     Dream     |      ADC     |
	--phore| Action |En|WrD|Clk|Cs| LdAct |En|WrD|Clk| CS |TsD|WrD|Clk|CS|
	--23-22|  21-20 |19| 18| 17|16|  15   |14| 13| 12|11-4| 3 | 2 | 1 |0 |
	*/
	#define D_Feu_RegSC_Adc_Cs_Ind       0
	#define D_Feu_RegSC_Adc_Clk_Ind      1
	#define D_Feu_RegSC_Adc_WrD_Ind      2
	#define D_Feu_RegSC_Adc_TsD_Ind      3
	#define D_Feu_RegSC_Dream_Cs_Ofs     4
	#define D_Feu_RegSC_Dream_Cs_Len       8
	#define D_Feu_RegSC_Dream_Clk_Ind   12
	#define D_Feu_RegSC_Dream_WrD_Ind   13
	#define D_Feu_RegSC_Dream_En_Ind    14
	#define D_Feu_RegSC_1Wire_LdAct_Ind 15
	#define D_Feu_RegSC_Pulse_Cs_Ind    16
	#define D_Feu_RegSC_Pulse_Clk_Ind   17
	#define D_Feu_RegSC_Pulse_WrD_Ind   18
	#define D_Feu_RegSC_Pulse_En_Ind    19
	#define D_Feu_RegSC_1Wire_Act_Ofs   20
	#define D_Feu_RegSC_1Wire_Act_Len      2
	#define D_Feu_RegSC_Semaphores_Ofs  22
	#define D_Feu_RegSC_Semaphores_Len     2

	// Field manipulation macros
	#define D_Feu_RegSC_Adc_Cs_Get(  word )        GetBits(word, D_Feu_RegSC_Adc_Cs_Ind,  1 )
	#define D_Feu_RegSC_Adc_Cs_Clr(  word )        ClrBits(word, D_Feu_RegSC_Adc_Cs_Ind,  1 )
	#define D_Feu_RegSC_Adc_Cs_Set(  word )        SetBits(word, D_Feu_RegSC_Adc_Cs_Ind,  1 )
	#define D_Feu_RegSC_Adc_Clk_Get( word )        GetBits(word, D_Feu_RegSC_Adc_Clk_Ind, 1 )
	#define D_Feu_RegSC_Adc_Clk_Clr( word )        ClrBits(word, D_Feu_RegSC_Adc_Clk_Ind, 1 )
	#define D_Feu_RegSC_Adc_Clk_Set( word )        SetBits(word, D_Feu_RegSC_Adc_Clk_Ind, 1 )
	#define D_Feu_RegSC_Adc_WrD_Get( word )        GetBits(word, D_Feu_RegSC_Adc_WrD_Ind, 1 )
	#define D_Feu_RegSC_Adc_WrD_Clr( word )        ClrBits(word, D_Feu_RegSC_Adc_WrD_Ind, 1 )
	#define D_Feu_RegSC_Adc_WrD_Set( word )        SetBits(word, D_Feu_RegSC_Adc_WrD_Ind, 1 )
	#define D_Feu_RegSC_Adc_WrD_Put( word, val )   PutBits(word, D_Feu_RegSC_Adc_WrD_Ind, 1, val )
	#define D_Feu_RegSC_Adc_TsD_Get( word )        GetBits(word, D_Feu_RegSC_Adc_TsD_Ind, 1 )
	#define D_Feu_RegSC_Adc_TsD_Clr( word )        ClrBits(word, D_Feu_RegSC_Adc_TsD_Ind, 1 )
	#define D_Feu_RegSC_Adc_TsD_Set( word )        SetBits(word, D_Feu_RegSC_Adc_TsD_Ind, 1 )

	#define D_Feu_RegSC_Dream_Cs_Get(  word )      GetBits(word, D_Feu_RegSC_Dream_Cs_Ofs,  D_Feu_RegSC_Dream_Cs_Len )
	#define D_Feu_RegSC_Dream_Cs_Clr(  word )      ClrBits(word, D_Feu_RegSC_Dream_Cs_Ofs,  D_Feu_RegSC_Dream_Cs_Len )
	#define D_Feu_RegSC_Dream_Cs_Set(  word, val ) PutBits(word, D_Feu_RegSC_Dream_Cs_Ofs,  D_Feu_RegSC_Dream_Cs_Len, val )
	#define D_Feu_RegSC_Dream_Clk_Get( word )      GetBits(word, D_Feu_RegSC_Dream_Clk_Ind, 1 )
	#define D_Feu_RegSC_Dream_Clk_Clr( word )      ClrBits(word, D_Feu_RegSC_Dream_Clk_Ind, 1 )
	#define D_Feu_RegSC_Dream_Clk_Set( word )      SetBits(word, D_Feu_RegSC_Dream_Clk_Ind, 1 )
	#define D_Feu_RegSC_Dream_WrD_Get( word )      GetBits(word, D_Feu_RegSC_Dream_WrD_Ind, 1 )
	#define D_Feu_RegSC_Dream_WrD_Clr( word )      ClrBits(word, D_Feu_RegSC_Dream_WrD_Ind, 1 )
	#define D_Feu_RegSC_Dream_WrD_Set( word )      SetBits(word, D_Feu_RegSC_Dream_WrD_Ind, 1 )
	#define D_Feu_RegSC_Dream_WrD_Put( word, val ) PutBits(word, D_Feu_RegSC_Dream_WrD_Ind, 1, val )
	#define D_Feu_RegSC_Dream_En_Get(  word )      GetBits(word, D_Feu_RegSC_Dream_En_Ind,  1 )
	#define D_Feu_RegSC_Dream_En_Clr(  word )      ClrBits(word, D_Feu_RegSC_Dream_En_Ind,  1 )
	#define D_Feu_RegSC_Dream_En_Set(  word )      SetBits(word, D_Feu_RegSC_Dream_En_Ind,  1 )

	#define D_Feu_RegSC_Pulse_Cs_Get(  word )      GetBits(word, D_Feu_RegSC_Pulse_Cs_Ind,  1 )
	#define D_Feu_RegSC_Pulse_Cs_Clr(  word )      ClrBits(word, D_Feu_RegSC_Pulse_Cs_Ind,  1 )
	#define D_Feu_RegSC_Pulse_Cs_Set(  word )      SetBits(word, D_Feu_RegSC_Pulse_Cs_Ind,  1 )
	#define D_Feu_RegSC_Pulse_Clk_Get( word )      GetBits(word, D_Feu_RegSC_Pulse_Clk_Ind, 1 )
	#define D_Feu_RegSC_Pulse_Clk_Clr( word )      ClrBits(word, D_Feu_RegSC_Pulse_Clk_Ind, 1 )
	#define D_Feu_RegSC_Pulse_Clk_Set( word )      SetBits(word, D_Feu_RegSC_Pulse_Clk_Ind, 1 )
	#define D_Feu_RegSC_Pulse_WrD_Get( word )      GetBits(word, D_Feu_RegSC_Pulse_WrD_Ind, 1 )
	#define D_Feu_RegSC_Pulse_WrD_Clr( word )      ClrBits(word, D_Feu_RegSC_Pulse_WrD_Ind, 1 )
	#define D_Feu_RegSC_Pulse_WrD_Set( word, val ) PutBits(word, D_Feu_RegSC_Pulse_WrD_Ind, 1, (val) )
	#define D_Feu_RegSC_Pulse_En_Get(  word )      GetBits(word, D_Feu_RegSC_Pulse_En_Ind,  1 )
	#define D_Feu_RegSC_Pulse_En_Clr(  word )      ClrBits(word, D_Feu_RegSC_Pulse_En_Ind,  1 )
	#define D_Feu_RegSC_Pulse_En_Set(  word )      SetBits(word, D_Feu_RegSC_Pulse_En_Ind,  1 )
	#define D_Feu_RegSC_Pulse_En_Put( word, val )  PutBits(word, D_Feu_RegSC_Pulse_En_Ind,  1, (val) )

	#define D_Feu_RegSC_1Wire_LdAct_Get( word )    GetBits(word, D_Feu_RegSC_1Wire_LdAct_Ind, 1 )
	#define D_Feu_RegSC_1Wire_LdAct_Clr( word )    ClrBits(word, D_Feu_RegSC_1Wire_LdAct_Ind, 1 )
	#define D_Feu_RegSC_1Wire_LdAct_Set( word )    SetBits(word, D_Feu_RegSC_1Wire_LdAct_Ind, 1 )
	#define D_Feu_RegSC_1Wire_Act_Get( word )      GetBits(word, D_Feu_RegSC_1Wire_Act_Ofs, D_Feu_RegSC_1Wire_Act_Len )
	#define D_Feu_RegSC_1Wire_Act_Clr( word )      ClrBits(word, D_Feu_RegSC_1Wire_Act_Ofs, D_Feu_RegSC_1Wire_Act_Len )
	#define D_Feu_RegSC_1Wire_Act_Put( word, val ) PutBits(word, D_Feu_RegSC_1Wire_Act_Ofs, D_Feu_RegSC_1Wire_Act_Len, val )

	/*
	-- Slow Control Register
	--| OneWire|Pulser|Dream|ADC|
	--|Busy|RdD|  RdD | RdD |RdD|
	--| 28 | 27|   26 |  25 | 24|
	*/
	#define D_Feu_RegSC_Adc_RdD_Ind     24
	#define D_Feu_RegSC_Dream_RdD_Ind   25
	#define D_Feu_RegSC_Pulse_RdD_Ind   26
	#define D_Feu_RegSC_1Wire_RdD_Ind   27
	#define D_Feu_RegSC_1Wire_Busy_Ind  28

	#define D_Feu_RegSC_Adc_RdD_Get(   word )      GetBits(word, D_Feu_RegSC_Adc_RdD_Ind,    1 )
	#define D_Feu_RegSC_Dream_RdD_Get( word )      GetBits(word, D_Feu_RegSC_Dream_RdD_Ind,  1 )
	#define D_Feu_RegSC_Pulse_RdD_Get( word )      GetBits(word, D_Feu_RegSC_Pulse_RdD_Ind,  1 )
	#define D_Feu_RegSC_1Wire_RdD_Get( word )      GetBits(word, D_Feu_RegSC_1Wire_RdD_Ind,  1 )
	#define D_Feu_RegSC_1Wire_Busy_Get( word )     GetBits(word, D_Feu_RegSC_1Wire_Busy_Ind, 1 )

	/*
	-- Run Control Register
		--|   ADC |     |              Algo                       |
		--|DataRdy|FeuId|Rsvd|ZsChkSmp|DrOvh| ZS |ComModSub|PedSub|
		--| 21-16 |15-8 |  7 |  6-4   |  3  |  2 |    1    |  0   |
	*/

	#define D_Feu_RegRC_Pd_Ind            0
	#define D_Feu_RegRC_CM_Ind            1
	#define D_Feu_RegRC_ZS_Ind            2
	#define D_Feu_RegRC_DrOvr_Ind         3
	#define D_Feu_RegRC_ZsChkSmp_Ofs      4
	#define D_Feu_RegRC_ZsChkSmp_Len        3
	#define D_Feu_RegRC_Id_Ofs            8
	#define D_Feu_RegRC_Id_Len              8
	#define D_Feu_RegRC_AdcDatRdyDel_Ofs  16
	#define D_Feu_RegRC_AdcDatRdyDel_Len    6

	#define D_Feu_RegRC_Pd_Get( word )            GetBits(word, D_Feu_RegRC_Pd_Ind, 1 )
	#define D_Feu_RegRC_Pd_Clr( word )            ClrBits(word, D_Feu_RegRC_Pd_Ind, 1 )
	#define D_Feu_RegRC_Pd_Set( word )            SetBits(word, D_Feu_RegRC_Pd_Ind, 1 )

	#define D_Feu_RegRC_CM_Get( word )            GetBits(word, D_Feu_RegRC_CM_Ind, 1 )
	#define D_Feu_RegRC_CM_Clr( word )            ClrBits(word, D_Feu_RegRC_CM_Ind, 1 )
	#define D_Feu_RegRC_CM_Set( word )            SetBits(word, D_Feu_RegRC_CM_Ind, 1 )

	#define D_Feu_RegRC_ZS_Get( word )            GetBits(word, D_Feu_RegRC_ZS_Ind, 1 )
	#define D_Feu_RegRC_ZS_Clr( word )            ClrBits(word, D_Feu_RegRC_ZS_Ind, 1 )
	#define D_Feu_RegRC_ZS_Set( word )            SetBits(word, D_Feu_RegRC_ZS_Ind, 1 )

	#define D_Feu_RegRC_DrOvr_Get( word )         GetBits(word, D_Feu_RegRC_DrOvr_Ind, 1 )
	#define D_Feu_RegRC_DrOvr_Clr( word )         ClrBits(word, D_Feu_RegRC_DrOvr_Ind, 1 )
	#define D_Feu_RegRC_DrOvr_Set( word )         SetBits(word, D_Feu_RegRC_DrOvr_Ind, 1 )

	#define D_Feu_RegRC_ZsChkSmp_Get( word )      GetBits(word, D_Feu_RegRC_ZsChkSmp_Ofs,  D_Feu_RegRC_ZsChkSmp_Len )
	#define D_Feu_RegRC_ZsChkSmp_Clr( word )      ClrBits(word, D_Feu_RegRC_ZsChkSmp_Ofs,  D_Feu_RegRC_ZsChkSmp_Len )
	#define D_Feu_RegRC_ZsChkSmp_Set( word, val ) PutBits(word, D_Feu_RegRC_ZsChkSmp_Ofs,  D_Feu_RegRC_ZsChkSmp_Len, val )

	#define D_Feu_RegRC_Id_Get( word )            GetBits(word, D_Feu_RegRC_Id_Ofs,  D_Feu_RegRC_Id_Len )
	#define D_Feu_RegRC_Id_Clr( word )            ClrBits(word, D_Feu_RegRC_Id_Ofs,  D_Feu_RegRC_Id_Len )
	#define D_Feu_RegRC_Id_Set( word, val )       PutBits(word, D_Feu_RegRC_Id_Ofs,  D_Feu_RegRC_Id_Len, val )

	#define D_Feu_RegRC_AdcDatRdyDel_Get( word )      GetBits(word, D_Feu_RegRC_AdcDatRdyDel_Ofs,  D_Feu_RegRC_AdcDatRdyDel_Len )
	#define D_Feu_RegRC_AdcDatRdyDel_Clr( word )      ClrBits(word, D_Feu_RegRC_AdcDatRdyDel_Ofs,  D_Feu_RegRC_AdcDatRdyDel_Len )
	#define D_Feu_RegRC_AdcDatRdyDel_Set( word, val ) PutBits(word, D_Feu_RegRC_AdcDatRdyDel_Ofs,  D_Feu_RegRC_AdcDatRdyDel_Len, val )

	#define D_Feu_RunCtrl_Set( reg, pd, cms, zs, zs_smp_id, feu_id, adc_dat_rdy_del, dr_ovr ) \
	PutBits \
	( \
		reg, D_Feu_RegRC_Pd_Ind, \
		(D_Feu_RegRC_AdcDatRdyDel_Ofs+D_Feu_RegRC_AdcDatRdyDel_Len), \
		(pd              << D_Feu_RegRC_Pd_Ind)       + \
		(cms             << D_Feu_RegRC_CM_Ind)       + \
		(zs              << D_Feu_RegRC_ZS_Ind)       + \
		(dr_ovr          << D_Feu_RegRC_DrOvr_Ind)    + \
		(zs_smp_id       << D_Feu_RegRC_ZsChkSmp_Ofs) + \
		(feu_id          << D_Feu_RegRC_Id_Ofs)       + \
		(adc_dat_rdy_del << D_Feu_RegRC_AdcDatRdyDel_Ofs ) \
	)

	/*
	-- Run Control Statistics Register
	--|         FastCmd         |
	--|ReSync|RstEvtCntr|RstTstp|
	--| 0-7  |   8-15   | 16-23 |
	*/
	#define D_Feu_RegRS_FstCmd_ReSync_Ofs       0
	#define D_Feu_RegRS_FstCmd_ReSync_Len          8
	#define D_Feu_RegRS_FstCmd_RstEvtCntr_Ofs   8
	#define D_Feu_RegRS_FstCmd_RstEvtCntr_Len      8
	#define D_Feu_RegRS_FstCmd_RstTstp_Ofs     16
	#define D_Feu_RegRS_FstCmd_RstTstp_Len         8

	#define D_Feu_RegRS_FstCmd_ReSync_Get(     word )  GetBits(word, D_Feu_RegRS_FstCmd_ReSync_Ofs,     D_Feu_RegRS_FstCmd_ReSync_Len     )
	#define D_Feu_RegRS_FstCmd_RstEvtCntr_Get( word )  GetBits(word, D_Feu_RegRS_FstCmd_RstEvtCntr_Ofs, D_Feu_RegRS_FstCmd_RstEvtCntr_Len )
	#define D_Feu_RegRS_FstCmd_RstTstp_Get(    word )  GetBits(word, D_Feu_RegRS_FstCmd_RstTstp_Ofs,    D_Feu_RegRS_FstCmd_RstTstp_Len    )

	/*
	-- Pulser Register
	--
	--|TestFunc|Rsvd|DacTrigVal| Rsvd|DacBaseVal|
	--|   31   | 30 |  29-16   |15-14|   13-0   |
	*/
	#define D_Feu_RegPulser_DacBaseVal_Ofs      0
	#define D_Feu_RegPulser_DacTrigVal_Ofs     16
	#define D_Feu_RegPulser_DacVal_Len           14
	#define D_Feu_RegPulser_TestFunc_Ind       31

	#define D_Feu_RegPulser_DacBaseVal_Get( word )          GetBits(word, D_Feu_RegPulser_DacBaseVal_Ofs,  D_Feu_RegPulser_DacVal_Len )
	#define D_Feu_RegPulser_DacBaseVal_Clr( word )          ClrBits(word, D_Feu_RegPulser_DacBaseVal_Ofs,  D_Feu_RegPulser_DacVal_Len )
	#define D_Feu_RegPulser_DacBaseVal_Set( word, val )     PutBits(word, D_Feu_RegPulser_DacBaseVal_Ofs,  D_Feu_RegPulser_DacVal_Len, val )

	#define D_Feu_RegPulser_DacTrigVal_Get( word )          GetBits(word, D_Feu_RegPulser_DacTrigVal_Ofs,  D_Feu_RegPulser_DacVal_Len )
	#define D_Feu_RegPulser_DacTrigVal_Clr( word )          ClrBits(word, D_Feu_RegPulser_DacTrigVal_Ofs,  D_Feu_RegPulser_DacVal_Len )
	#define D_Feu_RegPulser_DacTrigVal_Set( word, val )     PutBits(word, D_Feu_RegPulser_DacTrigVal_Ofs,  D_Feu_RegPulser_DacVal_Len, val )

	#define D_Feu_RegPulser_TestFunc_Get( word )            GetBits(word, D_Feu_RegPulser_TestFunc_Ind,  1 )
	#define D_Feu_RegPulser_TestFunc_Clr( word )            ClrBits(word, D_Feu_RegPulser_TestFunc_Ind,  1 )
	#define D_Feu_RegPulser_TestFunc_Set( word )            SetBits(word, D_Feu_RegPulser_TestFunc_Ind,  1 )
	#define D_Feu_RegPulser_TestFunc_Put( word, val )       PutBits(word, D_Feu_RegPulser_TestFunc_Ind,  1, val )

	#define D_Feu_Pulser_Set( reg, base, trig, tstfun ) \
	PutBits \
	( \
		reg, D_Feu_RegRS_FstCmd_ReSync_Ofs, \
		32, \
		(base << D_Feu_RegPulser_DacBaseVal_Ofs) + \
		(trig << D_Feu_RegPulser_DacTrigVal_Ofs) + \
		(tstfun << D_Feu_RegPulser_TestFunc_Ind) \
	)

	// Define clock periods
	// Core clock 125 MHz period 8 ns
	#define D_Feu_CoreClkPeriod_ns 8

	// define Acquisition state machine states
	typedef enum _AcqFsm_State
	{
		AcqFsmState_WaitForAcqEnable = 0x0,
		AcqFsmState_WaitForTrig      = 0x1,
		AcqFsmState_ReadDream        = 0x2,
		AcqFsmState_WaitEoE          = 0xD,
		AcqFsmState_Error            = 0xE,
		AcqFsmState_Unknown          = 0xF
	} AcqFsm_State;
	char *AcqFsm_State2String( AcqFsm_State acq_fsm_state );

	// Trigger generator CSR structure and functions
	typedef struct _Feu_Csr
	{
		// Config
		unsigned long power;
		unsigned long slow_ctrl;
		unsigned long run_ctrl;
		unsigned long pulser;

		//Stat
		unsigned long run_stat;
		unsigned long trg_stat;
	} Feu_Csr;
	int FeuCsr_Init(    Feu_Csr *csr );
	int FeuCsr_Sprintf( Feu_Csr *csr, unsigned char *buf );

	int Feu_GetCsr( Feu_Csr *csr );
	int Feu_Config( unsigned long power, unsigned long run_ctrl, unsigned long pulser );

	// Optional message container
	extern char *feu_msg;

#endif // #ifndef H_Feu
