/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Front End Unit FEU
--
-- Module Name:    TrigInt.h
-- Description:    Auxiliary Trigger Interface library
--
-- Target Devices: Virtex-6 / FEU
-- Tool versions:  ISE 14.3
-- 
-- Create Date:    0.0 2014/04/14 IM
-- Revision:       1.0 2014/09/03 IM BERT support added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_TrigInt
#define H_TrigInt

	#include "bitmanip.h"

	// Dream Emulator collection address space
	#define D_TrigInt_Adr_Ofs  0
	#define D_TrigInt_Adr_Len  1

	// Offset and Length of Dream emulators collection registers address zone field  
	#define D_TrigInt_AdrReg_Get( adr )         GetBits(adr, D_TrigInt_Adr_Ofs, D_TrigInt_Adr_Len )
	#define D_TrigInt_AdrReg_Set( adr, subadr ) PutBits(adr, D_TrigInt_Adr_Ofs, D_TrigInt_Adr_Len, subadr )

	// Register addresses
	#define C_TrigInt_AdrReg_Csr            0x00
	/*
		-- Configuration
		--   0  |  1  |   2  |  3 |4-6 | 7 |8-15|16-23|24-31
		-- DCBAL|DCBAL|  TCM |BERT|RSVD|TCM| Rx | Rx  | Tx
		--  ENC | DEC |Ignore|    |    |On |Cntr|PErr |Cntr
	*/
	#define D_TrigInt_Csr_DcBalEnc_Ind          0
	#define D_TrigInt_Csr_DcBalDec_Ind          1
	#define D_TrigInt_Csr_TcmIgn_Ind            2 
	#define D_TrigInt_Csr_Bert_Ind              3
	#define D_TrigInt_Csr_Conf_Len                4

	#define D_TrigInt_Csr_TcmOn_Ind             7

	#define D_TrigInt_Csr_RxCnt_Ind             8
	#define D_TrigInt_Csr_RxPer_Ind            16
	#define D_TrigInt_Csr_TxCnt_Ind            24
	#define D_TrigInt_Csr_Cnt_Len                8

	// Field manipulation macros
	#define D_TrigInt_Csr_DcBalEnc_Get(     word )       GetBits(word, D_TrigInt_Csr_DcBalEnc_Ind,     1)
	#define D_TrigInt_Csr_DcBalEnc_Clr(     word )       ClrBits(word, D_TrigInt_Csr_DcBalEnc_Ind,     1)
	#define D_TrigInt_Csr_DcBalEnc_Set(     word )       SetBits(word, D_TrigInt_Csr_DcBalEnc_Ind,     1)
	#define D_TrigInt_Csr_DcBalEnc_Put(     word, val )  PutBits(word, D_TrigInt_Csr_DcBalEnc_Ind,     1, val)

	#define D_TrigInt_Csr_DcBalDec_Get(     word )       GetBits(word, D_TrigInt_Csr_DcBalDec_Ind,     1)
	#define D_TrigInt_Csr_DcBalDec_Clr(     word )       ClrBits(word, D_TrigInt_Csr_DcBalDec_Ind,     1)
	#define D_TrigInt_Csr_DcBalDec_Set(     word )       SetBits(word, D_TrigInt_Csr_DcBalDec_Ind,     1)
	#define D_TrigInt_Csr_DcBalDec_Put(     word, val )  PutBits(word, D_TrigInt_Csr_DcBalDec_Ind,     1, val)

	#define D_TrigInt_Csr_TcmIgn_Get(     word )       GetBits(word, D_TrigInt_Csr_TcmIgn_Ind,     1)
	#define D_TrigInt_Csr_TcmIgn_Clr(     word )       ClrBits(word, D_TrigInt_Csr_TcmIgn_Ind,     1)
	#define D_TrigInt_Csr_TcmIgn_Set(     word )       SetBits(word, D_TrigInt_Csr_TcmIgn_Ind,     1)
	#define D_TrigInt_Csr_TcmIgn_Put(     word, val )  PutBits(word, D_TrigInt_Csr_TcmIgn_Ind,     1, val)

	#define D_TrigInt_Csr_Bert_Get(     word )       GetBits(word, D_TrigInt_Csr_Bert_Ind,     1)
	#define D_TrigInt_Csr_Bert_Clr(     word )       ClrBits(word, D_TrigInt_Csr_Bert_Ind,     1)
	#define D_TrigInt_Csr_Bert_Set(     word )       SetBits(word, D_TrigInt_Csr_Bert_Ind,     1)
	#define D_TrigInt_Csr_Bert_Put(     word, val )  PutBits(word, D_TrigInt_Csr_Bert_Ind,     1, val)

	#define D_TrigInt_Csr_Conf_Get(      word )        GetBits(word, D_TrigInt_Csr_DcBalEnc_Ind,  D_TrigInt_Csr_Conf_Len)
	#define D_TrigInt_Csr_Conf_Set( word, bert, ignore, dc_bal_dec, dc_bal_enc ) \
		PutBits \
		( \
			word, \
			D_TrigInt_Csr_DcBalEnc_Ind, \
			D_TrigInt_Csr_Conf_Len, \
			(bert<<D_TrigInt_Csr_Bert_Ind)+(ignore<<D_TrigInt_Csr_TcmIgn_Ind)+(dc_bal_dec<<D_TrigInt_Csr_DcBalDec_Ind)+ dc_bal_enc \
		)

	#define D_TrigInt_Csr_TcmOn_Get(      word )       GetBits(word, D_TrigInt_Csr_TcmOn_Ind,  1)

	#define D_TrigInt_Csr_RxCnt_Get(     word )       GetBits(word, D_TrigInt_Csr_RxCnt_Ind,     D_TrigInt_Csr_Cnt_Len)
	#define D_TrigInt_Csr_RxPer_Get(     word )       GetBits(word, D_TrigInt_Csr_RxPer_Ind,     D_TrigInt_Csr_Cnt_Len)
	#define D_TrigInt_Csr_TxCnt_Get(     word )       GetBits(word, D_TrigInt_Csr_TxCnt_Ind,     D_TrigInt_Csr_Cnt_Len)

// Communication channel CSR structure and functions
	typedef struct _TrigInt_Csr
	{
		//Stat
		unsigned long csr;
	} TrigInt_Csr;
	int TrigIntCsr_Init(   TrigInt_Csr *csr );
	int TrigIntCsr_Sprintf( TrigInt_Csr *csr, char *buf );

	int TrigInt_GetCsr( TrigInt_Csr *csr );
	int TrigInt_Config( int dc_bal_enc, int dc_bal_dec, int tcm_ignore, int bert );

	// Optional message container
	extern char *trig_int_msg;

#endif // #ifndef H_TrigInt
