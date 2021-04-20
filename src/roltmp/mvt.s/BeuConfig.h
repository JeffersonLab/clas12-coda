/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    BeuConfig.h
-- Description:    Beu configuration
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/16 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_BeuConfiguration
#define H_BeuConfiguration

#include "BecConfigParams.h"
#include "beusspLib.h"

// Tables with pointers to the BEU control structures and data fifo-s
extern volatile struct BEUSSP_A24RegStruct *beu_reg_control[DEF_MAX_NB_OF_BEU];
extern volatile unsigned int               *beu_fifo[DEF_MAX_NB_OF_BEU];
/* pointer to BEUSSP multi block adress window */
extern volatile unsigned int               *BEUSSPmblk;

int BeuRun( volatile struct BEUSSP_A24RegStruct *beu_regs, int run );
int BeuGo(  volatile struct BEUSSP_A24RegStruct *beu_regs, BeuSspConf *params );

#endif // #ifndef H_BeuConfiguration
