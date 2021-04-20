/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    UdpChan.h
-- Description:    Udp communication channel library
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE 10.1
-- 
-- Create Date:    0.0 2011/09/02 IM
-- Revision:       1.0 2014/09/04 IM Add Phy Stat and reset signals
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_UdpChan
#define H_UdpChan

	#include "bitmanip.h"

	// Dream Emulator collection address space
	#define D_UdpChan_Adr_Ofs  0
	#define D_UdpChan_Adr_Len  3

	// Offset and Length of Dream emulators collection registers address zone field  
	#define D_UdpChan_AdrReg_Get( adr )         GetBits(adr, D_UdpChan_Adr_Ofs, D_UdpChan_Adr_Len )
	#define D_UdpChan_AdrReg_Set( adr, subadr ) PutBits(adr, D_UdpChan_Adr_Ofs, D_UdpChan_Adr_Len, subadr )

	// Register addresses
	#define C_UdpChan_AdrReg_Csr         0x00
	#define C_UdpChan_AdrReg_TxPacketLsb 0x04

	/*
	--   0-3   | 4 | 5 |  6   |  7   | 8-11| 12| 13-17  |  18-28  |    29   |   30    |     31
	--  Current|Phy|Phy|Ready |Enable|State|Phy|TxPacket|MultiPack|MultiPack|Interrupt| Interrupt
	--  Buffer |Abs|LoS|Notify|      |     |Rst|   Msb  |Threshold|  Enable |  Enable |(packet notify)
	*/
	#define D_UdpChan_Csr_BufPtr_Ofs       0
	#define D_UdpChan_Csr_BufPtr_Len           4
	#define D_UdpChan_Csr_PhyAbs_Ind       4
	#define D_UdpChan_Csr_PhyLoS_Ind       5
	#define D_UdpChan_Csr_RdyNot_Ind       6
	#define D_UdpChan_Csr_Enable_Ind       7
	#define D_UdpChan_Csr_State_Ofs        8
	#define D_UdpChan_Csr_State_Len            4
	#define D_UdpChan_Csr_PhyRst_Ind       12
	#define D_UdpChan_Csr_TxPacketMsb_Ofs  13
	#define D_UdpChan_Csr_TxPacketMsb_Len      5
	#define D_UdpChan_Csr_MultiPackThr_Ofs 18
	#define D_UdpChan_Csr_MultiPackThr_Len    11
	#define D_UdpChan_Csr_MultiPackEnb_Ind 29
	#define D_UdpChan_Csr_IntEnable_Ind    30
	#define D_UdpChan_Csr_IntPending_Ind   31

	// Field manipulation macros
	#define D_UdpChan_Csr_BufPtr_Get(       word )      GetBits(word, D_UdpChan_Csr_BufPtr_Ofs,       D_UdpChan_Csr_BufPtr_Len)
	#define D_UdpChan_Csr_PhyAbs_Get(       word )      GetBits(word, D_UdpChan_Csr_PhyAbs_Ind,       1)
	#define D_UdpChan_Csr_PhyLoS_Get(       word )      GetBits(word, D_UdpChan_Csr_PhyLoS_Ind,       1)
	#define D_UdpChan_Csr_RdyNot_Get(       word )      GetBits(word, D_UdpChan_Csr_RdyNot_Ind,       1)
	#define D_UdpChan_Csr_Enable_Get(       word )      GetBits(word, D_UdpChan_Csr_Enable_Ind,       1)
	#define D_UdpChan_Csr_Enable_Clr(       word )      ClrBits(word, D_UdpChan_Csr_Enable_Ind,       1)
	#define D_UdpChan_Csr_Enable_Set(       word )      SetBits(word, D_UdpChan_Csr_Enable_Ind,       1)
	#define D_UdpChan_Csr_Enable_Put(       word, val ) PutBits(word, D_UdpChan_Csr_Enable_Ind,       1, val)
	#define D_UdpChan_Csr_State_Get(        word )      GetBits(word, D_UdpChan_Csr_State_Ofs,        D_UdpChan_Csr_State_Len)
	#define D_UdpChan_Csr_TxPacketMsb_Get(  word )      GetBits(word, D_UdpChan_Csr_TxPacketMsb_Ofs,  D_UdpChan_Csr_TxPacketMsb_Len)
	#define D_UdpChan_Csr_IntEnable_Get(    word )      GetBits(word, D_UdpChan_Csr_IntEnable_Ind,    1)
	#define D_UdpChan_Csr_IntEnable_Clr(    word )      ClrBits(word, D_UdpChan_Csr_IntEnable_Ind,    1)
	#define D_UdpChan_Csr_IntEnable_Set(    word )      SetBits(word, D_UdpChan_Csr_IntEnable_Ind,    1)
	#define D_UdpChan_Csr_IntEnable_Put(    word, val ) PutBits(word, D_UdpChan_Csr_IntEnable_Ind,    1, val)
	#define D_UdpChan_Csr_IntPending_Get(   word )      GetBits(word, D_UdpChan_Csr_IntPending_Ind,   1)

	#define D_UdpChan_Csr_PhyRst_Get( word )            GetBits(word, D_UdpChan_Csr_PhyRst_Ind, 1)
	#define D_UdpChan_Csr_PhyRst_Clr( word )            ClrBits(word, D_UdpChan_Csr_PhyRst_Ind, 1)
	#define D_UdpChan_Csr_PhyRst_Set( word )            SetBits(word, D_UdpChan_Csr_PhyRst_Ind, 1)
	#define D_UdpChan_Csr_PhyRst_Put( word, val )       PutBits(word, D_UdpChan_Csr_PhyRst_Ind, 1, val)

	#define D_UdpChan_Csr_MultiPackThr_Get( word )      GetBits(word, D_UdpChan_Csr_MultiPackThr_Ofs, D_UdpChan_Csr_MultiPackThr_Len)
	#define D_UdpChan_Csr_MultiPackThr_Clr( word )      ClrBits(word, D_UdpChan_Csr_MultiPackThr_Ofs, D_UdpChan_Csr_MultiPackThr_Len)
	#define D_UdpChan_Csr_MultiPackThr_Set( word )      SetBits(word, D_UdpChan_Csr_MultiPackThr_Ofs, D_UdpChan_Csr_MultiPackThr_Len)
	#define D_UdpChan_Csr_MultiPackThr_Put( word, val ) PutBits(word, D_UdpChan_Csr_MultiPackThr_Ofs, D_UdpChan_Csr_MultiPackThr_Len, val)

	#define D_UdpChan_Csr_MultiPackEnb_Get( word )      GetBits(word, D_UdpChan_Csr_MultiPackEnb_Ind, 1)
	#define D_UdpChan_Csr_MultiPackEnb_Clr( word )      ClrBits(word, D_UdpChan_Csr_MultiPackEnb_Ind, 1)
	#define D_UdpChan_Csr_MultiPackEnb_Set( word )      SetBits(word, D_UdpChan_Csr_MultiPackEnb_Ind, 1)
	#define D_UdpChan_Csr_MultiPackEnb_Put( word, val ) PutBits(word, D_UdpChan_Csr_MultiPackEnb_Ind, 1, val)

	/*
	-- Tx paket LSB registers is just 32-bit wide
	*/

	// define Udp state machine states
	typedef enum _UdpFsm_State
	{
		WaitForUdpEnable     = 0x0,
		SetFreeUdpBufferAdr  = 0x1,
		WaitForFreeUdpBuffer = 0x2,
		WaitForData          = 0x3,
		WriteData            = 0x4,
		BufferData           = 0x5,
		ReadCsumWord         = 0x6,
		ReadUdpLenWord       = 0x7,
		WriteUdpLen          = 0x8,
		WriteBufLen          = 0x9,
		WriteIpLen           = 0xA,
		WriteIpCsum          = 0xB,
		UdpPacketReady       = 0xC,
		UdpState_Unknown     = 0xF
	} UdpFsm_State;
	char *UdpFsm_State2String( UdpFsm_State udp_fsm_state );

	// Communication channel CSR structure and functions
	typedef struct _UdpChan_Csr
	{
		//Stat
		unsigned long csr;
		unsigned long tx_packet;
	} UdpChan_Csr;
	int UdpChanCsr_Init(   UdpChan_Csr *csr );
	int UdpChanCsr_Sprintf( UdpChan_Csr *csr, char *buf );

	int  UdpChan_Enable( UdpChan_Csr *csr, int en, int int_en, int mlt_pack_en, int mlt_pack_thr);
	int  UdpChan_ClrRdy( UdpChan_Csr *csr );
	int  UdpChan_PhyRst( UdpChan_Csr *csr );
	int  UdpChan_GetCsr( UdpChan_Csr *csr );
	void UdpChan_CallBack();

	// Optional message container
	extern char *udp_chan_msg;

#endif // #ifndef H_UdpChan
