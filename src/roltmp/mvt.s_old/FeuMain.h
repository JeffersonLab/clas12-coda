/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    FeuMain.h
-- Description:    Front End Unit Main library
--
-- Target Devices: Virtex-6 / FEU
-- Tool versions:  ISE 14.3
-- 
-- Create Date:    0.0 2014/04/15 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_FeuMain
#define H_FeuMain

	#include "Feu.h"
	#include "CBus_Common.h"
	#include "ComChan.h"
	#include "UdpChan.h"
	#include "TrigInt.h"
	#include "TrigGen.h"
	#include "SelfTrig.h"

	// Main CSR structure and functions
	typedef struct _Main_Csr
	{
		CBus_CommonCsr   common_csr;
		ComChan_Csr    com_chan_csr;
		UdpChan_Csr    udp_chan_csr;
		Tg_Csr               tg_csr;
		Feu_Csr             feu_csr;
		TrigInt_Csr    trig_int_csr;
		SelfTrig_Csr  self_trig_csr;
	} Main_Csr;
	int Main_CsrInit(    Main_Csr *csr );
	int Main_CsrGet(     Main_Csr *csr );
	int Main_CsrSprintf( Main_Csr *csr, char *buf );

	// Optional message container
	extern char *main_msg;

	#define Application "FeuMain"

#endif // #ifndef H_FeuMain
