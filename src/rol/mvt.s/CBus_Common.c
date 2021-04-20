/*
-------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    Cbus_Common.c
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
--                     2011/02/25 IM DreamTrig delay measured in Core Clock ticks
--                     2012/02/23 IM Local trigger throttling added
--                 3.0 2018/11/04 IM Configuration register: Number of samples extended to 512 from 256
--                                                           Sparse readout replaced data pipe length
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#include <sys/time.h>
#include <string.h>

#include "Platform.h"
#include "ReturnCodes.h"
#include "CBus.h"
#include "Clas12.h"
#include "CBus_Common.h"
#include "Feu.h"


// Static variables
unsigned int     previous_trig_acpt_cntr;
struct   timeval previous_time;
struct   timeval current_time;

//Conversion function from RunCtrlFsm_State to string
//extern char *RunCtrlFsm_State2String( int rc_fsm_state );

//Conversion function from software type to string
char *Sw_Typ2Str( SwType typ )
{
	if( typ == SwType_FeuClas12 )
		return ("C12");
	else if( typ == SwType_FeuAsacusa )
		return ("Asa");
	else if( typ == SwType_FeuLowLevelTests )
		return ("LLT");
	else if( typ == SwType_TbDream )
		return ("TbD");
	else
		return ("Err");
}

//Conversion function from firmware type to string
char *Fw_Typ2Str( FwType typ )
{
	if( typ == FwType_FeuClas12 )
		return ("C12");
	else if( typ == FwType_FeuAsacusa )
		return ("Asa");
	else if( typ == FwType_FeuProto )
		return ("Prt");
	else if( typ == FwType_TbDream )
		return ("TbD");
	else
		return ("Err");
}
//Conversion function from trigger clock source to string
char *TrigClk_Src2Str( TrigClk_Src src )
{
	if( src == OnBoardClk )
		return ("OnBrd  ");
	else if( src == TrgIfConClk )
		return ("TICon  ");
	else if( src == RecClk )
		return ("Rcvd   ");
	else
		return ("Unknown");
}

// Initialize Csr with default values
int CBus_CommonCsr_Init( CBus_CommonCsr *csr )
{
	if( csr == NULL )
		return D_RetCode_Err_Null_Pointer;

	csr->Command      = 0;
	csr->Config       = D_Def_Main_Conf;
	csr->TrigConfig   = D_Def_Main_Trig;
	csr->FwRev        = 0;
	csr->SwRev        = 0;
	csr->TrigActpCntr = 0;
	csr->TrigDropCntr = 0;
	csr->Status       = 0;
	csr->Error        = 0;
	csr->LastEvent    = 0;

	// Clear previous accepted trigger counter
	previous_trig_acpt_cntr = 0;
	previous_time.tv_sec    = 0;
	previous_time.tv_usec   = 0;
	current_time.tv_sec     = 0;
	current_time.tv_usec    = 0;

	return(D_RetCode_Sucsess);
};

// Decode CSR content
int CBus_CommonCsr_Sprintf( CBus_CommonCsr *csr, char *buf )
{
	unsigned int nb_of_triggers;
	struct timeval delta;
	double delta_sec;
	int rate;
	char append_str[256];

	if( (csr==NULL) || (buf==NULL) )
		return D_RetCode_Err_Null_Pointer;

	// Command register
	sprintf
	(
		append_str,
		" Cmd : Rst=%1d Cfg=%1d Run=%1d Pause=%1d ReSync=%1d HwRst=%1d\n\r",
		(int)D_Main_Cmd_Reset_Get( csr->Command ),
		(int)D_Main_Cmd_Config_Get(csr->Command),
		(int)D_Main_Cmd_Run_Get(   csr->Command),
		(int)D_Main_Cmd_Pause_Get( csr->Command),
		(int)D_Main_Cmd_ReSync_Get(csr->Command),
		(int)D_Main_Cmd_HwRst_Get( csr->Command)
	);
	strcat( buf, append_str);

	// Config
	sprintf
	(
		append_str,
		" Conf: Smp=%8d Msk=0x%02x Pol=0x%02x Sparse=%3d TrigClk=%s\n\r",
		(int)D_Main_Conf_Samples_Get( csr->Config),
		(int)D_Main_Conf_Mask_Get(    csr->Config),
		(int)D_Main_Conf_DrmPol_Get(  csr->Config),
		(int)D_Main_Conf_SparseRd_Get(csr->Config),
		TrigClk_Src2Str( D_Main_Conf_ClkSel_Get(csr->Config) )
	);
	strcat( buf, append_str);

	// Trigger Config
	sprintf
	(
		append_str,
		" Trig: TStamp=%4d OvrLwm=%2d OvrHwm=%2d OvrThesh=%2d LocThrot=%d\n\r",
		(int)D_Main_Trig_TimeStamp_Get(csr->TrigConfig),
		(int)D_Main_Trig_OvrWrnLwm_Get(csr->TrigConfig),
		(int)D_Main_Trig_OvrWrnHwm_Get(csr->TrigConfig),
		(int)D_Main_Trig_OvrThersh_Get(csr->TrigConfig),
		(int)D_Main_Trig_LocThrot_Get( csr->TrigConfig)
	);
	strcat( buf, append_str);

	// Status
	sprintf
	(
		append_str,
		" Stat: Dreams=%1d Status=%s AcFsm=%s RcFsm=%s ClkOk=%1d Cfg=%1d\n\r",
		(int)D_Main_Stat_NbOfDreams_Get(csr->Status),
		PartStatus2Str(          D_Main_Stat_Status_Get(csr->Status) ),
		AcqFsm_State2String(     D_Main_Stat_AcqFsm_Get(csr->Status) ),
		RunCtrlFsm_State2String( D_Main_Stat_RcFsm_Get( csr->Status) ),
		(int)D_Main_Stat_ClkValid_Get(  csr->Status),
		(int)D_Main_Stat_Configured_Get(csr->Status)
	);
	strcat( buf, append_str);

	// Trigger statistics
	sprintf
	(
		append_str,
		" Trig: Acpt=%4d M + %7d TrigDrop=%3d FifoDrop=%3d FifoMaxOcc=%2d",
		(int)((csr->TrigActpCntr & 0xFFF00000) >> 20),
		(int)(csr->TrigActpCntr & 0x000FFFFF),
		(int)D_Main_Drop_TrigCntr_Get(csr->TrigDropCntr),
		(int)D_Main_Drop_FifoCntr_Get(csr->TrigDropCntr),
		(int)D_Main_Drop_FifoOcup_Get(csr->TrigDropCntr)
	);
	strcat( buf, append_str);

	/*
	 * Estimate trigger rate
	 */
	// First determine how much time elapsed since last measurement
	timersub(&current_time,&previous_time,&delta);
	delta_sec = (double)(delta.tv_sec) + (((double)delta.tv_usec)/1000000.);

	// Then determine how many triggers received since last time
	nb_of_triggers = ((csr->TrigActpCntr & 0xFFFFFF) - (previous_trig_acpt_cntr & 0xFFFFFF)) & 0xFFFFFF;
	if( (delta_sec <= 0.0) || (nb_of_triggers < 0.0) )
		sprintf(append_str, " Trig Rate= Unknown\n\r" );
	else
	{
		rate = (int)((double)nb_of_triggers/delta_sec + 0.5);
		if( (0<= rate) && (rate <= 100000) )
			sprintf(append_str, " Trig Rate=%5d Hz\n\r", rate);
		else
			sprintf(append_str, " Trig Rate= Unknown\n\r");
	}
	strcat( buf, append_str);

	previous_trig_acpt_cntr = csr->TrigActpCntr;
	previous_time           = current_time;
/*
	// Error
	sprintf
	(
		buf,
		"%s Err : DreamRdErr=0x%02x RdErrCntr=%3d CmbErr=%1d\n\r",
		buf,
		(int)D_Main_Error_DreamRdErr_Get(csr->Error),
		(int)D_Main_Error_RdErrCntr_Get(csr->Error),
		(int)D_Main_Error_CombErr_Get(csr->Error)
	);
*/
	// Last Event
	sprintf
	(
		append_str,
		" Evt : Id=%4d Tstp=%4d (%5d ns) Del=%1d (%2d ns)",
		(int)D_Main_LastEvent_Id_Get(  csr->LastEvent),
		(int)D_Main_LastEvent_Tstp_Get(csr->LastEvent),
		(int)D_Main_LastEvent_Tstp_Get(csr->LastEvent) * D_CoreClkPeriod_ns,
		(int)D_Main_LastEvent_Del_Get( csr->LastEvent),
		(int)D_Main_LastEvent_Del_Get( csr->LastEvent) * D_CoreClkPeriod_ns
	);
	strcat( buf, append_str);

	return(D_RetCode_Sucsess);
}

/*
 * Convert firmware & software revisions to string
 */
int CBus_CommonCsr_Rev2Str( CBus_CommonCsr *csr, char *buf)
{
	if( (csr==NULL) || (buf==NULL) )
		return D_RetCode_Err_Null_Pointer;

	// Status
	sprintf
	(
		buf,
		"Sn=%3d Fw: Typ=%s Dat=%06d Rev=%02d.%02d Sw: Typ=%s Dat=%06d Rev=%02d.%02d",
		(int)((D_Main_Rev_GetSrN(csr->SwRev)<<4)+D_Main_Rev_GetSrN(csr->FwRev)),
		Fw_Typ2Str((int)D_Main_Rev_GetTyp(csr->FwRev)),
		(int)D_Main_Rev_GetDat(csr->FwRev),
		(int)D_Main_Rev_GetMaj(csr->FwRev),
		(int)D_Main_Rev_GetMin(csr->FwRev),
		Sw_Typ2Str((int)D_Main_Rev_GetTyp(csr->SwRev)),
		(int)D_Main_Rev_GetDat(csr->SwRev),
		(int)D_Main_Rev_GetMaj(csr->SwRev),
		(int)D_Main_Rev_GetMin(csr->SwRev)
	);

	return(D_RetCode_Sucsess);
}

/*
 * Fill CSR with data
 */
int CBus_CommonCsr_Get( CBus_CommonCsr *csr )
{
	unsigned long adr;

	if( csr == NULL )
		return D_RetCode_Err_Null_Pointer;

	// Set address 
	adr = D_CBus_SetModType(  0, D_CBus_Mod_Main );

	// Get Command
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegCommand );
	csr->Command = Peek(adr);
	// Get Config
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegConfig );
	csr->Config = Peek(adr);
	// Get Trigger config
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegTrigConfig );
	csr->TrigConfig = Peek(adr);
	// Get Status
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegStatus );
	csr->Status = Peek(adr);
	// Get FwRev
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegFwRev );
	csr->FwRev = Peek(adr);
	// Get SwRev
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegSwRev );
	csr->SwRev = Peek(adr);
	// Get TrigAccept
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegTrigAcptCntr );
	csr->TrigActpCntr = Peek(adr);
	// Get TrigDrop
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegTrigDropCntr );
	csr->TrigDropCntr = Peek(adr);
	// Get Error
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegError );
	csr->Error = Peek(adr);
	// Get Last Event
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegLastEvent );
	csr->LastEvent = Peek(adr);

	return(D_RetCode_Sucsess);
}

/*
 * Set software revision
 */
int CBus_Common_SetSwRev( unsigned int SwRev )
{
	unsigned int adr;
	unsigned long rd_data;

	// Set address 
	adr = D_CBus_SetModType(  0, D_CBus_Mod_Main );
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegSwRev );
	// Write data
	Poke( adr, SwRev );
	// Get data
	rd_data = Peek( adr );
	if( rd_data != SwRev )
	{
		return D_RetCode_Err_WrRd_Missmatch;
	}
	return(D_RetCode_Sucsess);
}

/*
 * Configure TbDreamMemMgr
 */
int CBus_Common_Config( unsigned int config, unsigned int trig_config )
{
	unsigned int adr;
	unsigned long rd_data;

//xil_printf("CBus_Common_Config: started\n\r");
	// Set module address 
	adr = D_CBus_SetModType(  0, D_CBus_Mod_Main );

	// Config register 
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegConfig );
	// Write data
	Poke( adr, config );
	// Get data
	rd_data = Peek( adr );
	if( rd_data != config )
	{
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Trigger Config register 
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegTrigConfig );
	// Write data
	Poke( adr, trig_config );
	// Get data
	rd_data = Peek( adr );
	if( rd_data != trig_config )
	{
		return D_RetCode_Err_WrRd_Missmatch;
	}
//xil_printf("CBus_Common_Config: end\n\r");
	return(D_RetCode_Sucsess);
}

/*
 * Configure TbDreamMemMgr
 */
int CBus_Common_SetCommand( unsigned int command )
{
	unsigned int adr;
	unsigned long rd_data;

	// Set module address 
	adr = D_CBus_SetModType(  0, D_CBus_Mod_Main );

	// Config register 
	adr = D_MainAdr_Set(  adr, D_MainAdr_RegCommand );
	// Write data
	Poke( adr, command );
	// Get data
	rd_data = Peek( adr );
	if( rd_data != command )
	{
		return D_RetCode_Err_WrRd_Missmatch;
	}
	return(D_RetCode_Sucsess);
}

/*
 * Get register value
 */
int CBus_Common_GetReg( unsigned int subadr, unsigned int *val)
{
	unsigned int adr;

	// Set module address 
	adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	adr = D_MainAdr_Set( adr, subadr );

	// Config register 
	// Write data
	*val = Peek(adr);
	return(D_RetCode_Sucsess);
}
/*
 * Set register value
 */
int CBus_Common_SetReg( unsigned int subadr, unsigned int  val)
{
	unsigned int adr;
	unsigned int rd_data;

	// Set module address 
	adr = D_CBus_SetModType(  0, D_CBus_Mod_Main );
	adr = D_MainAdr_Set(    adr, subadr );

	// Write data
	Poke( adr, val );
	// Get data
	rd_data = Peek( adr );
	if( rd_data != val )
	{
		return D_RetCode_Err_WrRd_Missmatch;
	}
	return(D_RetCode_Sucsess);
}

/*
 * Latch statistics
 */
int CBus_Common_LatchStat()
{
	unsigned int adr;
	unsigned long cmd;

	// Set module address 
	adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	adr = D_MainAdr_Set(   adr, D_MainAdr_RegCommand );

	// Get data
//printf("CBus_Common_LatchStat: Before Peek at adr=0x%08x\n\r", adr);
	cmd = Peek( adr );
//printf("CBus_Common_LatchStat: after Peek at adr=0x%08x cmd=0x%08x\n\r", adr, cmd);

	// Issue Latch command
	cmd = D_Main_Cmd_Latch_Set(cmd);
//printf("CBus_Common_LatchStat: Before Poke 1...\n\r");
	Poke( adr, cmd );
	// Get current time
	gettimeofday(&current_time,0);

	// Remove Latch command
	cmd = D_Main_Cmd_Latch_Clr(cmd);
//printf("CBus_Common_LatchStat: Before Poke 2...\n\r");
	Poke( adr, cmd );

//printf("CBus_Common_LatchStat: Before Retourn...\n\r");
	return(D_RetCode_Sucsess);
}

/*
 * Clear statistics
 */
int CBus_Common_ClearStat()
{
	unsigned int adr;
	unsigned long cmd;

	// Set module address 
	adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	adr = D_MainAdr_Set(   adr, D_MainAdr_RegCommand );

	// Get data
	cmd = Peek( adr );

	// Issue Clear command
	cmd = D_Main_Cmd_Clear_Set(cmd);
	Poke( adr, cmd );

	// Get current time
//	gettimeofday(&current_time,0);

	// Remove Clear command
	cmd = D_Main_Cmd_Clear_Clr(cmd);
	Poke( adr, cmd );
	
	// Clear previous accepted trigger counter
	previous_trig_acpt_cntr = 0;
	previous_time.tv_sec = 0;
	previous_time.tv_usec = 0;
	current_time.tv_sec = 0;
	current_time.tv_usec = 0;

	return(D_RetCode_Sucsess);
}
