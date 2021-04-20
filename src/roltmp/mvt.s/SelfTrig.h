/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Front End Unit FEU
--
-- Module Name:    SelfTrig.h
-- Description:    Library for self trigger based on Dream Hit
--
-- Target Devices: Virtex-6 / FEU
-- Tool versions:  ISE 14.3
-- 
-- Create Date:    0.0 2014/04/14 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_SelfTrig
#define H_SelfTrig

	#include "bitmanip.h"

	// Self Trigger address space
	#define D_SelfTrig_Adr_Ofs  0
	#define D_SelfTrig_Adr_Len  8

	// Trigger Generator address space is divided to registry and memory zones
	#define D_SelfTrig_AdrZone_Reg 0
	#define D_SelfTrig_AdrZone_Mem 1

	// Offset and Length of Trigger Generator address zone field
	#define D_SelfTrig_AdrZone_Ofs  7
	#define D_SelfTrig_AdrZone_Len  1

	// Offset and Length of Trigger Generator memory address zone field
	#define D_SelfTrig_MemAdr_Ofs   0
	#define D_SelfTrig_MemAdr_Len   7
	#define D_SelfTrig_Mem_Size    (1<<D_SelfTrig_MemAdr_Len)

	// Offset and Length of Self-trigger registers address zone field
	#define D_SelfTrig_RegAdr_Ofs  0
	#define D_SelfTrig_RegAdr_Len  4

	// Address manipulation user macros
	#define D_SelfTrig_Adr_Get( adr )            GetBits(adr, D_SelfTrig_Adr_Ofs, D_Tg_Adr_Len )
	#define D_SelfTrig_Adr_Clr( adr )            ClrBits(adr, D_SelfTrig_Adr_Ofs, D_Tg_Adr_Len )

	#define D_SelfTrig_AdrZone_Get( adr )        GetBits(adr, D_SelfTrig_AdrZone_Ofs, D_SelfTrig_AdrZone_Len )
	#define D_SelfTrig_AdrZone_Set( adr, zone )  PutBits(adr, D_SelfTrig_AdrZone_Ofs, D_SelfTrig_AdrZone_Len, zone )

	#define D_SelfTrig_AdrReg_Get( adr )         GetBits(adr, D_SelfTrig_RegAdr_Ofs, D_SelfTrig_RegAdr_Len )
	#define D_SelfTrig_AdrReg_Set( adr, subadr ) PutBits(adr, D_SelfTrig_RegAdr_Ofs, D_SelfTrig_RegAdr_Len, subadr )

	#define D_SelfTrig_AdrMem_Get( adr )         GetBits(adr, D_SelfTrig_MemAdr_Ofs, D_SelfTrig_MemAdr_Len )
	#define D_SelfTrig_AdrMem_Clr( adr )         ClrBits(adr, D_SelfTrig_MemAdr_Ofs, D_SelfTrig_MemAdr_Len )
	#define D_SelfTrig_AdrMem_Set( adr, subadr ) PutBits(adr, D_SelfTrig_MemAdr_Ofs, D_SelfTrig_MemAdr_Len, subadr )

	// Register addresses
	#define C_SelfTrig_AdrReg_Csr            0x0
	#define C_SelfTrig_AdrReg_CoinCntrLsb    0x4
	#define C_SelfTrig_AdrReg_Veto           0x8
	/*
		-- Configuration
		--  0-7 |  8-10 |   11    |12-17 |18-23 | 24 |  25     | 26-31 |
		-- Dream| Multi | CmbHitFb|DrmHit|CmbHit|Topo| CmbHitOl| Coin  |
		-- Masks|plicity|Propagate|Width |Width |Trig|Propagate|CntrMsb|
	*/
	#define D_SelfTrig_Csr_DrmMsk_Ind            0
	#define D_SelfTrig_Csr_DrmMsk_Len               8
	#define D_SelfTrig_Csr_Mult_Ind              8
	#define D_SelfTrig_Csr_Mult_Len                 3
	#define D_SelfTrig_Csr_CmbHitFbProp_Ind       11
	#define D_SelfTrig_Csr_DrmHitWid_Ind        12
	#define D_SelfTrig_Csr_CmbHitWid_Ind        18
	#define D_SelfTrig_Csr_HitWid_Len               6
	#define D_SelfTrig_Csr_TopoTrig_Ind         24
	#define D_SelfTrig_Csr_CmbHitOlProp_Ind     25
	#define D_SelfTrig_Csr_CoinCntrMsb_Ind      26
	#define D_SelfTrig_Csr_CoinCntrMsb_Len          6

	#define D_SelfTrig_Csr_Conf_Len             (D_SelfTrig_Csr_CmbHitOlProp_Ind + 1)

	// Field manipulation macros
	#define D_SelfTrig_Csr_DrmMsk_Get(      word )       GetBits(word, D_SelfTrig_Csr_DrmMsk_Ind,     D_SelfTrig_Csr_DrmMsk_Len)
	#define D_SelfTrig_Csr_DrmMsk_Clr(      word )       ClrBits(word, D_SelfTrig_Csr_DrmMsk_Ind,     D_SelfTrig_Csr_DrmMsk_Len)
	#define D_SelfTrig_Csr_DrmMsk_Set(      word )       SetBits(word, D_SelfTrig_Csr_DrmMsk_Ind,     D_SelfTrig_Csr_DrmMsk_Len)
	#define D_SelfTrig_Csr_DrmMsk_Put(      word, val )  PutBits(word, D_SelfTrig_Csr_DrmMsk_Ind,     D_SelfTrig_Csr_DrmMsk_Len, val)

	#define D_SelfTrig_Csr_Mult_Get(        word )       GetBits(word, D_SelfTrig_Csr_Mult_Ind,       D_SelfTrig_Csr_Mult_Len)
	#define D_SelfTrig_Csr_Mult_Clr(        word )       ClrBits(word, D_SelfTrig_Csr_Mult_Ind,       D_SelfTrig_Csr_Mult_Len)
	#define D_SelfTrig_Csr_Mult_Set(        word )       SetBits(word, D_SelfTrig_Csr_Mult_Ind,       D_SelfTrig_Csr_Mult_Len)
	#define D_SelfTrig_Csr_Mult_Put(        word, val )  PutBits(word, D_SelfTrig_Csr_Mult_Ind,       D_SelfTrig_Csr_Mult_Len, val)

	#define D_SelfTrig_Csr_CmbHitFbProp_Get(  word )       GetBits(word, D_SelfTrig_Csr_CmbHitFbProp_Ind, 1)
	#define D_SelfTrig_Csr_CmbHitFbProp_Clr(  word )       ClrBits(word, D_SelfTrig_Csr_CmbHitFbProp_Ind, 1)
	#define D_SelfTrig_Csr_CmbHitFbProp_Set(  word )       SetBits(word, D_SelfTrig_Csr_CmbHitFbProp_Ind, 1)
	#define D_SelfTrig_Csr_CmbHitFbProp_Put(  word, val )  PutBits(word, D_SelfTrig_Csr_CmbHitFbProp_Ind, 1, val)

	#define D_SelfTrig_Csr_DrmHitWid_Get(   word )       GetBits(word, D_SelfTrig_Csr_DrmHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len)
	#define D_SelfTrig_Csr_DrmHitWid_Clr(   word )       ClrBits(word, D_SelfTrig_Csr_DrmHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len)
	#define D_SelfTrig_Csr_DrmHitWid_Set(   word )       SetBits(word, D_SelfTrig_Csr_DrmHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len)
	#define D_SelfTrig_Csr_DrmHitWid_Put(   word, val )  PutBits(word, D_SelfTrig_Csr_DrmHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len, val)

	#define D_SelfTrig_Csr_CmbHitWid_Get(   word )       GetBits(word, D_SelfTrig_Csr_CmbHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len)
	#define D_SelfTrig_Csr_CmbHitWid_Clr(   word )       ClrBits(word, D_SelfTrig_Csr_CmbHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len)
	#define D_SelfTrig_Csr_CmbHitWid_Set(   word )       SetBits(word, D_SelfTrig_Csr_CmbHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len)
	#define D_SelfTrig_Csr_CmbHitWid_Put(   word, val )  PutBits(word, D_SelfTrig_Csr_CmbHitWid_Ind,     D_SelfTrig_Csr_HitWid_Len, val)

	#define D_SelfTrig_Csr_TopoTrig_Get(  word )         GetBits(word, D_SelfTrig_Csr_TopoTrig_Ind, 1)
	#define D_SelfTrig_Csr_TopoTrig_Clr(  word )         ClrBits(word, D_SelfTrig_Csr_TopoTrig_Ind, 1)
	#define D_SelfTrig_Csr_TopoTrig_Set(  word )         SetBits(word, D_SelfTrig_Csr_TopoTrig_Ind, 1)
	#define D_SelfTrig_Csr_TopoTrig_Put(  word, val )    PutBits(word, D_SelfTrig_Csr_TopoTrig_Ind, 1, val)

	#define D_SelfTrig_Csr_CmbHitOlProp_Get(  word )       GetBits(word, D_SelfTrig_Csr_CmbHitOlProp_Ind, 1)
	#define D_SelfTrig_Csr_CmbHitOlProp_Clr(  word )       ClrBits(word, D_SelfTrig_Csr_CmbHitOlProp_Ind, 1)
	#define D_SelfTrig_Csr_CmbHitOlProp_Set(  word )       SetBits(word, D_SelfTrig_Csr_CmbHitOlProp_Ind, 1)
	#define D_SelfTrig_Csr_CmbHitOlProp_Put(  word, val )  PutBits(word, D_SelfTrig_Csr_CmbHitOlProp_Ind, 1, val)

	#define D_SelfTrig_Csr_Conf_Get(        word )     GetBits(word, D_SelfTrig_Csr_DrmMsk_Ind,      D_SelfTrig_Csr_Conf_Len)
	#define D_SelfTrig_Csr_Conf_Set( word, drm_msk, mult, prop_fb, drm_hit_wid, cmb_hit_wid, topo_trig, prop_ol ) \
		PutBits \
		( \
			word, \
			D_SelfTrig_Csr_DrmMsk_Ind, \
			D_SelfTrig_Csr_Conf_Len, \
			(prop_ol<<D_SelfTrig_Csr_CmbHitOlProp_Ind)+(topo_trig<<D_SelfTrig_Csr_TopoTrig_Ind)+ \
			(cmb_hit_wid<<D_SelfTrig_Csr_CmbHitWid_Ind)+(drm_hit_wid<<D_SelfTrig_Csr_DrmHitWid_Ind)+ \
			(prop_fb<<D_SelfTrig_Csr_CmbHitFbProp_Ind)+(mult<<D_SelfTrig_Csr_Mult_Ind)+ drm_msk \
		)

	#define D_SelfTrig_Csr_CoinCntrMsb_Get( word )     GetBits(word, D_SelfTrig_Csr_CoinCntrMsb_Ind, D_SelfTrig_Csr_CoinCntrMsb_Len)

	// The coincidence counter LSB is a 32-bit register

	/* The Veto register
 	-- | Veto  |
 	-- |Counter|
	-- | 23-0  |
	*/
	#define D_SelfTrig_RegVeto_Veto_Ind            0
	#define D_SelfTrig_RegVeto_Veto_Len               24

	#define D_SelfTrig_RegVeto_Veto_Get(        word )       GetBits(word, D_SelfTrig_RegVeto_Veto_Ind,       D_SelfTrig_RegVeto_Veto_Len)
	#define D_SelfTrig_RegVeto_Veto_Clr(        word )       ClrBits(word, D_SelfTrig_RegVeto_Veto_Ind,       D_SelfTrig_RegVeto_Veto_Len)
	#define D_SelfTrig_RegVeto_Veto_Set(        word )       SetBits(word, D_SelfTrig_RegVeto_Veto_Ind,       D_SelfTrig_RegVeto_Veto_Len)
	#define D_SelfTrig_RegVeto_Veto_Put(        word, val )  PutBits(word, D_SelfTrig_RegVeto_Veto_Ind,       D_SelfTrig_RegVeto_Veto_Len, val)

	// Communication channel CSR structure and functions
	#define D_SelfTrig_NbOfMemWords    (D_SelfTrig_Mem_Size/sizeof(int))
	typedef struct _SelfTrig_Csr
	{
		//Registers
		unsigned int csr;
		unsigned int coin_cntr_lsb;
		unsigned int veto;
		//Memory
		unsigned int mem_entry[D_SelfTrig_NbOfMemWords];
	} SelfTrig_Csr;
	int SelfTrigCsr_Init(    SelfTrig_Csr *csr );
	int SelfTrigCsr_Sprintf( SelfTrig_Csr *csr, char *buf );

	int SelfTrig_GetCsr( SelfTrig_Csr *csr );
int SelfTrig_Config( int drm_msk, int mult, int prop_fb, int drm_hit_wid, int cmb_hit_wid, int topo, int prop_ol, int veto );
	// Optional message container
	extern char *self_trig_msg;

#endif // #ifndef H_SelfTrig
