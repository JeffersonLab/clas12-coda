/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    ComChan.h
-- Description:    Serial communication channel library
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE 10.1
-- 
-- Create Date:    0.0 2011/06/20 IM
-- Revision:       1.0 2012/10/12 IM: D_ComChan_Csr_Enable_Ind moved from 10 to 15
--                     2014/07/09 IM: Phy status added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_ComChan
#define H_ComChan

	#include "bitmanip.h"

	// Dream Emulator collection address space
	#define D_ComChan_Adr_Ofs  0
	#define D_ComChan_Adr_Len  5

	// Offset and Length of Dream emulators collection registers address zone field  
	#define D_ComChan_AdrReg_Get( adr )         GetBits(adr, D_ComChan_Adr_Ofs, D_ComChan_Adr_Len )
	#define D_ComChan_AdrReg_Set( adr, subadr ) PutBits(adr, D_ComChan_Adr_Ofs, D_ComChan_Adr_Len, subadr )

	// Register addresses
	#define C_ComChan_AdrReg_Csr            0x00
	#define C_ComChan_AdrReg_TxPacketLsb    0x04
	#define C_ComChan_AdrReg_TxError        0x08
	#define C_ComChan_AdrReg_RxPacketLsb    0x0C
	#define C_ComChan_AdrReg_RxSyncDataLsb  0x10
	#define C_ComChan_AdrReg_RxError        0x14

	/*
	-- Csr register
	--      Common   |                 Rx                |     Tx    |PhyStat|        Command      |Config
	--     0   |  1  |  2 |   3   | 4  |  5  |  6  |  7  |  8  |  9  |10 | 11|  12   |  13   | 14  | 15
	--  PllKDet|Reset| Re | Loss  |Sync|Byte |Align|Ready| Buf |Ready|Phy|Phy|TxReset|RxReset|Reset|Enable
	--         |Done |Sync|of Sync|Done|Align|Done |     |Error|     |Abs|LoS|       |
	*/
	#define D_ComChan_Csr_PllKDet_Ind          0
	#define D_ComChan_Csr_ResetDone_Ind        1
	#define D_ComChan_Csr_RxReSync_Ind         2
	#define D_ComChan_Csr_RxLoS_Ind            3
	#define D_ComChan_Csr_RxSyncDone_Ind       4
	#define D_ComChan_Csr_RxByteIsAligned_Ind  5
	#define D_ComChan_Csr_RxAlignDone_Ind      6
	#define D_ComChan_Csr_RxReady_Ind          7
	#define D_ComChan_Csr_TxBufErr_Ind         8
	#define D_ComChan_Csr_TxReady_Ind          9
	#define D_ComChan_Csr_PhyAbs_Ind          10
	#define D_ComChan_Csr_PhyLoS_Ind          11

	#define D_ComChan_Csr_Enable_Ind          15

	// Field manipulation macros
	#define D_ComChan_Csr_PllKDet_Get(    word )  GetBits(word, D_ComChan_Csr_PllKDet_Ind,         1)
	#define D_ComChan_Csr_RstDone_Get(    word )  GetBits(word, D_ComChan_Csr_ResetDone_Ind,       1)
	#define D_ComChan_Csr_RxReSync_Get(   word )  GetBits(word, D_ComChan_Csr_RxReSync_Ind,        1)
	#define D_ComChan_Csr_RxLoS_Get(      word )  GetBits(word, D_ComChan_Csr_RxLoS_Ind,           1)
	#define D_ComChan_Csr_RxSyncDone_Get( word )  GetBits(word, D_ComChan_Csr_RxSyncDone_Ind,      1)
	#define D_ComChan_Csr_RxBIsAlgnd_Get( word )  GetBits(word, D_ComChan_Csr_RxByteIsAligned_Ind, 1)
	#define D_ComChan_Csr_RxAlgnDone_Get( word )  GetBits(word, D_ComChan_Csr_RxAlignDone_Ind,     1)
	#define D_ComChan_Csr_RxReady_Get(    word )  GetBits(word, D_ComChan_Csr_RxReady_Ind,         1)
	#define D_ComChan_Csr_TxBufErr_Get(   word )  GetBits(word, D_ComChan_Csr_TxBufErr_Ind,        1)
	#define D_ComChan_Csr_TxReady_Get(    word )  GetBits(word, D_ComChan_Csr_TxReady_Ind,         1)
	#define D_ComChan_Csr_PhyAbs_Get(     word )  GetBits(word, D_ComChan_Csr_PhyAbs_Ind,          1)
	#define D_ComChan_Csr_PhyLoS_Get(     word )  GetBits(word, D_ComChan_Csr_PhyLoS_Ind,          1)

	#define D_ComChan_Csr_Enable_Get(     word )       GetBits(word, D_ComChan_Csr_Enable_Ind,     1)
	#define D_ComChan_Csr_Enable_Clr(     word )       ClrBits(word, D_ComChan_Csr_Enable_Ind,     1)
	#define D_ComChan_Csr_Enable_Set(     word )       SetBits(word, D_ComChan_Csr_Enable_Ind,     1)
	#define D_ComChan_Csr_Enable_Put(     word, val )  PutBits(word, D_ComChan_Csr_Enable_Ind,     1, val)

	/*
	-- Tx and Rx paket LSB registers are just 32-bit wide
	*/

	/*
	-- Tx Error register
	--   0-15   | 16-23|
	--  TxPacket|  Tx  |
	--    MSB   |Error |
	*/
	#define D_ComChan_TxErr_PacketMsb_Ofs   0
	#define D_ComChan_TxErr_PacketMsb_Len     16
	#define D_ComChan_TxErr_Error_Ofs      16
	#define D_ComChan_TxErr_Error_Len          8
	// Field manipulation macros
	#define D_ComChan_TxErr_PacketMsb_Get(  word )  GetBits(word, D_ComChan_TxErr_PacketMsb_Ofs,  D_ComChan_TxErr_PacketMsb_Len)
	#define D_ComChan_TxErr_Error_Get(      word )  GetBits(word, D_ComChan_TxErr_Error_Ofs,      D_ComChan_TxErr_Error_Len)

	/*
	-- Rx Error register
	--   0-7  | 8-15|16-23|24-31
	--  RxSync|RxPac| Rx  | Rx
	--   MSB  |Error|Error|ParErr
	*/
	#define D_ComChan_RxErr_SyncMsb_Ofs     0
	#define D_ComChan_RxErr_RxPacErr_Ofs    8
	#define D_ComChan_RxErr_Error_Ofs      16
	#define D_ComChan_RxErr_Parity_Ofs     24

	#define D_ComChan_RxErr_Field_Len  8
	// Field manipulation macros
	#define D_ComChan_RxErr_SyncMsb_Get(   word )  GetBits(word, D_ComChan_RxErr_SyncMsb_Ofs,   D_ComChan_RxErr_Field_Len)
	#define D_ComChan_RxErr_RxPacErr_Get(  word )  GetBits(word, D_ComChan_RxErr_RxPacErr_Ofs,  D_ComChan_RxErr_Field_Len)
	#define D_ComChan_RxErr_Error_Get(     word )  GetBits(word, D_ComChan_RxErr_Error_Ofs,     D_ComChan_RxErr_Field_Len)
	#define D_ComChan_RxErr_Parity_Get(    word )  GetBits(word, D_ComChan_RxErr_Parity_Ofs,    D_ComChan_RxErr_Field_Len)

	// Communication channel CSR structure and functions
	typedef struct _ComChan_Csr
	{
		//Stat
		unsigned long csr;
		unsigned long tx_packet;
		unsigned long tx_error;
		unsigned long rx_packet;
		unsigned long rx_sync_data;
		unsigned long rx_error;
	} ComChan_Csr;
	int ComChanCsr_Init(   ComChan_Csr *csr );
	int ComChanCsr_Sprintf( ComChan_Csr *csr, char *buf );

	int ComChan_GetCsr( ComChan_Csr *csr );
	int ComChan_Ctrl(   ComChan_Csr *csr, int enable );
	
	#define ComChan_Enable(  csr ) ComChan_Ctrl( csr, 1 );
	#define ComChan_Disable( csr ) ComChan_Ctrl( csr, 0 );

	// Optional message container
	extern char *com_chan_msg;

#endif // #ifndef H_ComChan
