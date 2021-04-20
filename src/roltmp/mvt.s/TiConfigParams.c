/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    TiConfigParams.c
-- Description:    Trigger Interface (TI) configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/01 IM
-- Revision:       1.0 2015/03/23 IM Extended Eid & Tstp capability
--                     2015/03/27 IM Thrigger inhibit threshold
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Parser.h"
#include "ReturnCodes.h"
#include "TiConfigParams.h"
#include "BecConfigParams.h"

// Conversion function from TiClkSrc to string
char *TiClkSrc2Str( TiClkSrc src )
{
	if( src == TiClkSrc_Undefined )
		return ("Undefined");
	else if( src == TiClkSrc_Internal )
		return ("Internal ");
	else if( src == TiClkSrc_HFBR1 )
		return ("HFBR1    ");
	else if( src == TiClkSrc_HFBR5 )
		return ("HFBR5    ");
	else
		return ("Unknown  ");
}

// Conversion function from TiTrgSrc to string
char *TiTrgSrc2Str( TiTrgSrc src )
{
	if( src == TiTrgSrc_Undefined )
		return ("Undefined");
	else if( src == TiTrgSrc_None )
		return ("None     ");
	else if( src == TiTrgSrc_HFBR1 )
		return ("HFBR1    ");
	else if( src == TiTrgSrc_Soft )
		return ("Soft     ");
	else if( src == TiTrgSrc_FpInp1 )
		return ("FpInp1   ");
	else if( src == TiTrgSrc_FpInp2 )
		return ("FpInp2   ");
	else if( src == TiTrgSrc_IntCst )
		return ("IntCst    ");
	else if( src == TiTrgSrc_IntRnd )
		return ("IntRnd    ");
	else if( src == TiTrgSrc_HFBR5 )
		return ("HFBR1     ");
	else
		return ("Unknown   ");
}

// Conversion function from TiBsySrc to string
char *TiBsySrc2Str( TiBsySrc src )
{
	if( src == TiBsySrc_Undefined )
		return ("Undefined");
	else if( src == TiBsySrc_None )
		return ("None     ");
	else if( src == TiBsySrc_SwB )
		return ("SwB      ");
	else
		return ("Unknown  ");
}

// Conversion function from TiSynSrc to string
char *TiSynSrc2Str( TiSynSrc src )
{
	if( src == TiSynSrc_Undefined )
		return ("Undefined");
	else if( src == TiSynSrc_None )
		return ("None     ");
	else if( src == TiSynSrc_HFBR1 )
		return ("HFBR1    ");
	else if( src == TiSynSrc_HFBR5 )
		return ("HFBR5    ");
	else if( src == TiSynSrc_Soft )
		return ("Soft     ");
	else
		return ("Unknown  ");
}

// Initialize the MvtParams structure with default values
int TiParams_Init( TiParams *params )
{
	int rule;

	// Check for Null pointer
	if( params == (TiParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Device ID and VME parameters
	// usually derived from BEC
	params->Id   = -1;
	params->Slot = 0;
	params->BaseAdr_A24_Reg  = 0;
	params->BaseAdr_A32_Fifo = 0;
	params->BaseAdr_A32m_Com_Min = 0;
	params->BaseAdr_A32m_Com_Max = 0;
	params->Mblk_Rank = MblkRank_Disable;

	// Global parameters
	// usually derived from BEC
	params->NbOfEvtPerBlk = 0;
	params->BlockLimit = 0;

	// Synchronisation parameters
	params->ClkSrc   = TiClkSrc_Undefined;
	params->TrgSrc   = TiTrgSrc_Undefined;
	params->BsySrc   = TiBsySrc_Undefined;
	params->SynSrc   = TiSynSrc_Undefined;

	// Trigger parameters
	params->TrgRate       = -1;
	params->TrgPrescale   = -1;
	params->TrgDel_4ns    = -1;
	params->TrgWid_4ns    = -1;
	for( rule=0; rule<Def_Ti_TrgRules_MaxNum; rule++ )
	{
		params->TrgRules_TimeUnit[rule]  = -1;
		params->TrgRules_UnitCount[rule] = -1;
	}

	// Sync parameters
	params->SynDel_4ns = -1; 

	// Format: if set enables extended EventId and Timestamp
	params->ExtEidTstp = -1;

	// Trigger inhibit threshold
	params->TrgInhThr = -1;

	return D_RetCode_Sucsess;
}

// Dump in ASCII the TiParams structure to a buffer
int TiParams_Sprintf( TiParams *params, char *buf  )
{
	char id_str[4];
	int rule;

	// Check for Null pointer
	if( params == (TiParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	if( buf == (char *)NULL )
	{
		fprintf( stderr, "%s: buf=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	// If parameters were not set, just exit queitly
	if( params->Id < 0 )
		return D_RetCode_Sucsess;

	// VME info
	if( params->Id == 0 )
		sprintf(id_str, "*");
	else
		sprintf(id_str, "%d", params->Id );
	sprintf( buf, "%s# Ti %s configuration\n", buf, id_str );
	if( params->Id > 0 )
	{
		sprintf( buf, "%s# Ti %s Slot %d BaseAdr_A24_Reg=0x%08x BaseAdr_A32_Fifo=0x%08x\n",
			buf, id_str, params->Slot, params->BaseAdr_A24_Reg, params->BaseAdr_A32_Fifo );
		sprintf( buf, "%s# Ti %s Slot %d Mblk Min=0x%08x Max=0x%08x Rank=%s\n",
			buf, id_str, params->Slot,
			params->BaseAdr_A32m_Com_Min, params->BaseAdr_A32m_Com_Max, MblkRank2Str( params->Mblk_Rank ) );
	}

	// Global parameters: derived from global parameters
	sprintf( buf, "%s# Ti %s NbOfEvtPerBlk  %d\n",    buf, id_str, params->NbOfEvtPerBlk );
	sprintf( buf, "%s# Ti %s BlockLilit     %d\n",    buf, id_str, params->BlockLimit );

	// Clk source: derived from global parameters
	sprintf( buf, "%s# Ti %s ClkSrc         %s\n",    buf, id_str, TiClkSrc2Str(params->ClkSrc) );
	// Trg source
	sprintf( buf, "%sTi %s TrgSrc           %s\n",    buf, id_str, TiTrgSrc2Str(params->TrgSrc) );
	// Syn source
	sprintf( buf, "%sTi %s SynSrc           %s\n",    buf, id_str, TiSynSrc2Str(params->SynSrc) );
	// Busy source
	sprintf( buf, "%sTi %s BsySrc           %s\n",    buf, id_str, TiBsySrc2Str(params->BsySrc) );

	// Trigger parameters
	sprintf( buf, "%sTi %s TrgRate          %d\n",    buf, id_str, params->TrgRate );
	sprintf( buf, "%sTi %s TrgPrescale      %d\n",    buf, id_str, params->TrgPrescale );
	sprintf( buf, "%sTi %s TrgTimPar        %d %d\n", buf, id_str, params->TrgDel_4ns, params->TrgWid_4ns    );
	for( rule=0; rule<Def_Ti_TrgRules_MaxNum; rule++ )
		sprintf( buf, "%sTi %s TrgRule %d      %3d %d \n", buf, id_str, rule+1, params->TrgRules_UnitCount[rule], params->TrgRules_TimeUnit[rule] );

	// Sync parameters
	sprintf( buf, "%sTi %s SynDel           %d\n",    buf, id_str, params->SynDel_4ns );

	// Format: if set enables extended EventId and Timestamp
	sprintf( buf, "%sTi %s ExtEidTstp       %d\n",    buf, id_str, params->ExtEidTstp );

	// Trigger inhibit threshold in event blocks
	sprintf( buf, "%sTi %s TrgInhThr        %d\n",    buf, id_str, params->TrgInhThr );

	// All went fine
	return D_RetCode_Sucsess;
}

// Dump in ASCII the TiParams structure to a file
int TiParams_Fprintf( TiParams *params, FILE *fptr )
{
	char buf[128*1024];
	int ret;

	// Check for Null pointer
	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "%s: fptr=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// If parameters were not set, just exit queitly
	if( params->Id < 0 )
		return D_RetCode_Sucsess;

	// Ti VME info
	sprintf( buf, "# Ti %d configuration\n", params->Id );

	// First produce the ASCII representation of the TiParams parameters structure 
	if( (ret = TiParams_Sprintf( params, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: TiParams_Sprintf failed with  %d\n", __FUNCTION__, ret );
		return ret;
	}

	// Then save it in the file  
	if( (ret = fprintf( fptr, "%s", buf ) ) < 0 )
	{
		fprintf( stderr, "%s: fprintf failed with %d\n", __FUNCTION__, ret );
		fprintf( stderr, "%s: fprintf failed with %s\n", __FUNCTION__, strerror(errno) );
		return ret;
	}

	// All went fine
	return D_RetCode_Sucsess;
}

// Fill the BecParams structure entries from externally parsed line
int TiParams_Parse( TiParams *params, int line_num )
{
	int ti;
	int rule;
	int unit_cnt;
	int time_unit;

	// Check for Null pointer
	if( params == (TiParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	if( argc > 0 )
	{
		if( ( strcmp( argv[0], "Ti" ) == 0 ) || ( strcmp( argv[0], "MVT_Ti" ) == 0 ) || ( strcmp( argv[0], "FTT_Ti" ) == 0 ) )
		{
			// Get TI ID
			if( strcmp( argv[1], "*" ) == 0 )
				ti = 0;
			else
				ti = atoi( argv[1] );
			if( (ti < 0) || (DEF_MAX_NB_OF_BEC <= ti) )
			{
				fprintf( stderr, "%s: line %d: Ti ID %d not in range [0,%d]\n", __FUNCTION__, line_num, ti, DEF_MAX_NB_OF_BEC ); 
				return D_RetCode_Err_Wrong_Param;
			}
			params->Id = ti;

			// Check for keywords and assign parameters
			if( strcmp( argv[2], "TrgTimPar" ) == 0 )
			{
				params->TrgDel_4ns = atoi( argv[3] );
				params->TrgWid_4ns = atoi( argv[4] );
			}
			else if( strcmp( argv[2], "TrgRate" ) == 0 )
			{
				params->TrgRate = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgPrescale" ) == 0 )
			{
				params->TrgPrescale = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "SynDel" ) == 0 )
			{
				params->SynDel_4ns = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "ExtEidTstp" ) == 0 )
			{
				params->ExtEidTstp = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgInhThr" ) == 0 )
			{
				params->TrgInhThr = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgSrc" ) == 0 )
			{
				if( strcmp( argv[3], "None" ) == 0 )
					params->TrgSrc = TiTrgSrc_None;
				else if( strcmp( argv[3], "HFBR1" ) == 0 )
					params->TrgSrc = TiTrgSrc_HFBR1;
				else if( strcmp( argv[3], "HFBR5" ) == 0 )
					params->TrgSrc = TiTrgSrc_HFBR5;
				else if( strcmp( argv[3], "Soft" ) == 0 )
					params->TrgSrc = TiTrgSrc_Soft;
				else if( strcmp( argv[3], "FpInp1" ) == 0 )
					params->TrgSrc = TiTrgSrc_FpInp1;
				else if( strcmp( argv[3], "FpInp2" ) == 0 )
					params->TrgSrc = TiTrgSrc_FpInp2;
				else if( strcmp( argv[3], "IntCst" ) == 0 )
					params->TrgSrc = TiTrgSrc_IntCst;
				else if( strcmp( argv[3], "IntRnd" ) == 0 )
					params->TrgSrc = TiTrgSrc_IntRnd;
				else
				{
					params->BsySrc = TiTrgSrc_Undefined;
					fprintf( stderr, "%s: line %d: Ti ID %d usupporet value %s for TrgSrc\n", __FUNCTION__, line_num, ti, argv[3] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[2], "BsySrc" ) == 0 )
			{
				if( strcmp( argv[3], "None" ) == 0 )
					params->BsySrc = TiBsySrc_None;
				else if( strcmp( argv[3], "SwB" ) == 0 )
					params->BsySrc = TiBsySrc_SwB;
				else
				{
					params->BsySrc = TiBsySrc_Undefined;
					fprintf( stderr, "%s: line %d: Ti ID %d usupporet value %s for BsySrc\n", __FUNCTION__, line_num, ti, argv[3] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[2], "SynSrc" ) == 0 )
			{
				if( strcmp( argv[3], "None" ) == 0 )
					params->SynSrc = TiSynSrc_None;
				else if( strcmp( argv[3], "Soft" ) == 0 )
					params->SynSrc = TiSynSrc_Soft;
				else if( strcmp( argv[3], "HFBR1" ) == 0 )
					params->SynSrc = TiSynSrc_HFBR1;
				else if( strcmp( argv[3], "HFBR5" ) == 0 )
					params->SynSrc = TiSynSrc_HFBR5;
				else
				{
					params->SynSrc = TiSynSrc_Undefined;
					fprintf( stderr, "%s: line %d: Ti ID %d unsupported value %s for SyngSrc\n", __FUNCTION__, line_num, ti, argv[3] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[2], "TrgRule" ) == 0 )
			{
				rule = atoi( argv[3] );
				unit_cnt  = atoi( argv[4] );
				time_unit = atoi( argv[5] );
				if( (rule<1) || (Def_Ti_TrgRules_MaxNum<rule) )
				{
					fprintf( stderr, "%s: line %d: Ti ID %d unsupported trig rule number %d, must bi in [1,%d]\n",
						__FUNCTION__, line_num, ti, rule, Def_Ti_TrgRules_MaxNum ); 
					return D_RetCode_Err_Wrong_Param;
				}
				if( (unit_cnt<0) || (Def_Ti_TrgRules_MaxNumOfUnitCnt<unit_cnt) )
				{
					fprintf( stderr, "%s: line %d: Ti ID %d unsupported unit count %d for trig rule %d, must bi in [0,%d]\n",
						__FUNCTION__, line_num, ti, unit_cnt, rule, Def_Ti_TrgRules_MaxNumOfUnitCnt ); 
					return D_RetCode_Err_Wrong_Param;
				}
				if( (time_unit<0) || (1<time_unit) )
				{
					fprintf( stderr, "%s: line %d: Ti ID %d unsupported time unit %d for trig rule %d, must bi in 0 or 1\n",
						__FUNCTION__, line_num, ti, time_unit, rule ); 
					return D_RetCode_Err_Wrong_Param;
				}
				params->TrgRules_UnitCount[rule-1] = unit_cnt;
				params->TrgRules_TimeUnit[rule-1]  = time_unit;
			}
		} // if( strcmp( argv[0], "Ti" ) == 0 )
	} // if( argc > 0 )

	// All went fine
	return D_RetCode_Sucsess;
}

// Fill the TiParams structure from an ASCII file
int TiParams_Fread( TiParams *params, FILE *fptr )
{
	char line[LINE_SIZE];
	int line_num;
	int ret;

	// Check for Null pointer
	if( params == (TiParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "%s: fptr=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Process parameter file
	line_num = 0;
	while( fgets( line, LINE_SIZE, fptr ) != NULL )
	{
		/* parse the line */
		parse_line(line);
		line_num++;
		ret = TiParams_Parse( params, line_num );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: TiParams_Parse failed with %d\n", __FUNCTION__, ret );
			return ret;
		}
	} // while( fgets( line, LINE_SIZE, fptr ) != NULL )

	// All went fine
	return D_RetCode_Sucsess;
}

// Propagate common Ti parameters
int TiParams_Prop( TiParams *params )
{
	int ti;
	TiParams *ti_conf_com;
	TiParams *ti_conf_cur;
	int rule;

	// Check for Null pointer
	if( params == (TiParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Common ti configuration
	ti_conf_com = &(params[0]);
	// Initialize individual structures
	for( ti=1; ti<DEF_MAX_NB_OF_BEC; ti++ )
	{
		// Running ti configuration
		ti_conf_cur = &(params[ti]);
		// Set only active TI-s
		if( ti_conf_cur->Id > 0 )
		{
			if( ti_conf_cur->TrgDel_4ns < 0 )
				ti_conf_cur->TrgDel_4ns = ti_conf_com->TrgDel_4ns;
			if( ti_conf_cur->TrgWid_4ns < 0 )
				ti_conf_cur->TrgWid_4ns = ti_conf_com->TrgWid_4ns;
			if( ti_conf_cur->TrgRate < 0 )
				ti_conf_cur->TrgRate = ti_conf_com->TrgRate;
			if( ti_conf_cur->TrgPrescale < 0 )
				ti_conf_cur->TrgPrescale = ti_conf_com->TrgPrescale;
			if( ti_conf_cur->SynDel_4ns < 0 )
				ti_conf_cur->SynDel_4ns = ti_conf_com->SynDel_4ns;
			if( ti_conf_cur->ExtEidTstp < 0 )
				ti_conf_cur->ExtEidTstp = ti_conf_com->ExtEidTstp;
			if( ti_conf_cur->TrgInhThr < 0 )
				ti_conf_cur->TrgInhThr = ti_conf_com->TrgInhThr;
			if( ti_conf_cur->TrgSrc == TiTrgSrc_Undefined )
				ti_conf_cur->TrgSrc = ti_conf_com->TrgSrc;
			if( ti_conf_cur->BsySrc == TiBsySrc_Undefined )
				ti_conf_cur->BsySrc = ti_conf_com->BsySrc;
			if( ti_conf_cur->SynSrc == TiSynSrc_Undefined )
				ti_conf_cur->SynSrc = ti_conf_com->SynSrc;
			if( ti_conf_cur->ClkSrc == TiClkSrc_Undefined )
				ti_conf_cur->ClkSrc = ti_conf_com->ClkSrc;
			for( rule=0; rule<Def_Ti_TrgRules_MaxNum; rule++ )
			{
				if( (ti_conf_cur->TrgRules_TimeUnit[rule] < 0) || (ti_conf_cur->TrgRules_UnitCount[rule] < 0) )
				{
					ti_conf_cur->TrgRules_TimeUnit[rule]  = ti_conf_com->TrgRules_TimeUnit[rule];
					ti_conf_cur->TrgRules_UnitCount[rule] = ti_conf_com->TrgRules_UnitCount[rule];
				}
			}
		}
	}
	// All went fine
	return D_RetCode_Sucsess;
}
