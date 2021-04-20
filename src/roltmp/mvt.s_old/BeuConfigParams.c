/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    BeuConfigParams.c
-- Description:    BEU / SSP configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/01 IM
-- Revision:       
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
#include "BeuConfigParams.h"

// Conversion function from BeuClkSrc to string
char *BeuClkSrc2Str( BeuClkSrc src )
{
	if( src == BeuClkSrc_Undefined )
		return ("Undefined     ");
	else if( src == BeuClkSrc_OnBoard_250MHz )
		return ("OnBoard_250MHz");
	else if( src == BeuClkSrc_TiVxs )
		return ("TiVxs         ");
	else if( src == BeuClkSrc_Unsupported )
		return ("Unsupported   ");
	else
		return ("Unknown       ");
}

// Conversion function from BeuTrgSrc to string
char *BeuTrgSrc2Str( BeuTrgSrc src )
{
	if( src == BeuTrgSrc_Undefined )
		return ("Undefined");
	else if( src == BeuTrgSrc_Soft )
		return ("Soft     ");
	else if( src == BeuTrgSrc_Const )
		return ("Const    ");
	else if( src == BeuTrgSrc_Ti1 )
		return ("Ti1      ");
	else if( src == BeuTrgSrc_Ti2 )
		return ("Ti2      ");
	else
		return ("Unknown  ");
}

// Conversion function from BeuBsySrc to string
static char busy_str[128];
char *BeuBsySrc2Str( BeuBsySrc src )
{

	if( src == BeuBsySrc_Undefined )
		return ("Undefined");
	else if( src == BeuBsySrc_None )
		return ("None     ");
	else if( src == BeuBsySrc_Perm )
		return ("Perm     ");
	else if( src )
	{
		busy_str[0]='\0'; 
		if( src & BeuBsySrc_TrgFifo )
			sprintf( busy_str, "%sTrgFifo ", busy_str);
		if( src & BeuBsySrc_Feu )
			sprintf( busy_str, "%sFeu ", busy_str);
		if( src & BeuBsySrc_InMem )
			sprintf( busy_str, "%sInMem ", busy_str);
		return busy_str;
	}
	return ("Unknown  ");
}

// Conversion function from BeuSynSrc to string
char *BeuSynSrc2Str( BeuSynSrc src )
{
	if( src == BeuSynSrc_Undefined )
		return ("Undefined");
	else if( src == BeuSynSrc_Soft )
		return ("Soft     ");
	else if( src == BeuSynSrc_Ti )
		return ("Ti       ");
	else
		return ("Unknown  ");
}

// Initialize the MvtParams structure with default values
int BeuSspConf_Init( BeuSspConf *params )
{
	// Check for Null pointer
	if( params == (BeuSspConf *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Device ID and VME parameters
	// usually derived from BEC
	params->Id   = -1;
	params->Slot = 0;
	params->base_adr_reg     = 0;
	params->base_adr_fifo    = 0;
	params->base_adr_com_min = 0;
	params->base_adr_com_max = 0;
	params->mblk_rank        = Beu_MblkRank_Inter;

	// Global parameters
	// usually derived from BEC
	params->NbOfSamples        = 0;
	params->NbOfEventsPerBlock = 0;

	// Self trigger parameters
	params->SelfTrigMult   = -1;
	params->SelfTrigWid    = -1;
	// Derived from system and BEC parameters
	params->SelfTrigLnkEnb = 0;
	params->SelfTrigLat    = 0;
	params->SelfTrigWin    = 0;

	// Optical link parameters
	params->near_end_loop_enb = 0;
	params->rol_enb           = 0;
	params->feu_emu_msk       = 0;
	params->emu_data_size     = 0;

	// Synchronisation parameters
	params->ClkSrc  = BeuClkSrc_Undefined;
	params->TrgSrc  = BeuTrgSrc_Undefined;
	params->BsySrc  = BeuBsySrc_Undefined;
	params->SyncSrc = BeuSynSrc_Undefined;

	// Trigger FIFO Parameters
	params->TrgFifo_Lwm    = 0;
	params->TrgFifo_Hwm    = 0; 
	params->TrgFifo_Full   = 0;
	params->TrgTstp_Offset = 0;

	// Input memory Parameters
	params->InMemBusyThr = 0;

	// Feu data packet concentration timeout
	params->timeout = 0; 
	params->MaskToInput = 0;

	// Internal Trigger parameters
	params->TrigRate = 0;

	return D_RetCode_Sucsess;
}

// Dump in ASCII the BeuSspConf structure to a buffer
int BeuSspConf_Sprintf( BeuSspConf *params, char *buf  )
{
	char id_str[4];
	char mblk_rank_str[32];
	// Check for Null pointer
	if( params == (BeuSspConf *)NULL )
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

	// Beu VME info
	if( params->Id == 0 )
		sprintf(id_str, "*");
	else
		sprintf(id_str, "%d", params->Id );
	sprintf( buf, "%s# Beu %s configuration\n", buf, id_str );
	if( params->Id > 0 )
	{
		if( params->mblk_rank == Beu_MblkRank_First )
			sprintf(mblk_rank_str, "First");
		else if( params->mblk_rank == Beu_MblkRank_Last )
			sprintf(mblk_rank_str, "Last");
		else if( params->mblk_rank == Beu_MblkRank_Inter )
			sprintf(mblk_rank_str, "Inter");
		else
			sprintf(mblk_rank_str, "Undef");
		sprintf( buf, "%s# Beu %s Slot %d BaseAdr_A24_Reg=0x%08x BaseAdr_A32_Fifo=0x%08x\n",
			buf, id_str, params->Slot, params->base_adr_reg, params->base_adr_fifo );
		sprintf( buf, "%s# Beu %s Slot %d Mblk Min=0x%08x Max=0x%08x Rank=%s\n",
			buf, id_str,
			params->Slot, params->base_adr_com_min, params->base_adr_com_max, mblk_rank_str );
	}

	// Global parameters: defived from global parameters
	sprintf( buf, "%s# Beu %s NbOfSmpPerEvt  %d\n", buf, id_str, params->NbOfSamples );
	sprintf( buf, "%s# Beu %s NbOfEvtPerBlk  %d\n", buf, id_str, params->NbOfEventsPerBlock );

	// Optical link parameters: defived from global parameters
	sprintf( buf, "%s# Beu %s NearLoopEnb    0x%08x\n",   buf, id_str, params->near_end_loop_enb );
	sprintf( buf, "%s# Beu %s RolEnb         0x%08x\n",   buf, id_str, params->rol_enb );
	sprintf( buf, "%s# Beu %s FeuEmuMsk      0x%08x\n",   buf, id_str, params->feu_emu_msk );

	// Self trigger parameters: Derived from system and BEC parameters
	sprintf( buf, "%s# Beu %s SelfTrigLnkEnb 0x%08x\n",  buf, id_str, params->SelfTrigLnkEnb );
	sprintf( buf, "%s# Beu %s SelfTrigLat    %d # ns\n", buf, id_str, params->SelfTrigLat );
	sprintf( buf, "%s# Beu %s SelfTrigWin    %d # ns\n", buf, id_str, params->SelfTrigWin );
	sprintf( buf, "%sBeu %s SelfTrigMult     %d\n",      buf, id_str, params->SelfTrigMult );
	sprintf( buf, "%sBeu %s SelfTrigWid      %d # ns\n", buf, id_str, params->SelfTrigWid );

	// Clk source: defived from global parameters
	sprintf( buf, "%sBeu %s ClkSrc           %s\n", buf, id_str, BeuClkSrc2Str(params->ClkSrc) );
	// Trg source
	sprintf( buf, "%sBeu %s TrgSrc           %s\n", buf, id_str, BeuTrgSrc2Str(params->TrgSrc) );
	// Syn source
	sprintf( buf, "%sBeu %s SynSrc           %s\n", buf, id_str, BeuSynSrc2Str(params->SyncSrc) );
	// Busy source
	sprintf( buf, "%sBeu %s BsySrc           %s\n", buf, id_str, BeuBsySrc2Str(params->BsySrc) );

	// Trigger FIFO Parameters
	sprintf( buf, "%sBeu %s TrgFifo_Lwm      %d\n", buf, id_str, params->TrgFifo_Lwm    );
	sprintf( buf, "%sBeu %s TrgFifo_Hwm      %d\n", buf, id_str, params->TrgFifo_Hwm    );
	sprintf( buf, "%sBeu %s TrgFifo_FullThr  %d\n", buf, id_str, params->TrgFifo_Full   );
	sprintf( buf, "%sBeu %s TrgTstp_Offset   %d\n", buf, id_str, params->TrgTstp_Offset );

	// Input memory Parameters
	sprintf( buf, "%sBeu %s InMemBusyThr     %d\n", buf, id_str, params->InMemBusyThr );

	// Feu data packet concentration timeout in 8ns cycles
	sprintf( buf, "%sBeu %s TimeOut          %d\n", buf, id_str, params->timeout );
	sprintf( buf, "%sBeu %s MaskToInput      %d\n", buf, id_str, params->MaskToInput );

	// Emulation parameters
	sprintf( buf, "%sBeu %s EmuDataSize      %d\n", buf, id_str, params->emu_data_size );
	sprintf( buf, "%sBeu %s TrigRate         %d\n", buf, id_str, params->TrigRate );

	// All went fine
	return D_RetCode_Sucsess;
}

// Dump in ASCII the BeuSspConf structure to a file
int BeuSspConf_Fprintf( BeuSspConf *params, FILE *fptr )
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

	// Beu VME info
	sprintf( buf, "# Beu %d configuration\n", params->Id );

	// First produce the ASCII representation of the BeuSspConf parameters structure 
	if( (ret = BeuSspConf_Sprintf( params, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: BeuSspConf_Sprintf failed with  %d\n", __FUNCTION__, ret );
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
int BeuSspConf_Parse( BeuSspConf *params, int line_num )
{
	int beu;
	int index;

	// Check for Null pointer
	if( params == (BeuSspConf *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	if( argc > 0 )
	{
		if( ( strcmp( argv[0], "Beu" ) == 0 ) || ( strcmp( argv[0], "MVT_Beu" ) == 0 ) || ( strcmp( argv[0], "FTT_Beu" ) == 0 ) )
		{
			// Get beu ID
			if( strcmp( argv[1], "*" ) == 0 )
				beu = 0;
			else
				beu = atoi( argv[1] );
			if( (beu < 0) || (DEF_MAX_NB_OF_BEU <= beu) )
			{
				fprintf( stderr, "%s: line %d: Beu ID %d not in range [0,%d]\n", __FUNCTION__, line_num, beu, DEF_MAX_NB_OF_BEU ); 
				return D_RetCode_Err_Wrong_Param;
			}
			params->Id = beu;

			// Check for keywords and assign parameters
			if( strcmp( argv[2], "SelfTrigMult" ) == 0 )
			{
				params->SelfTrigMult = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "SelfTrigWid" ) == 0 )
			{
				params->SelfTrigWid = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgFifo_Lwm" ) == 0 )
			{
				params->TrgFifo_Lwm = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgFifo_Hwm" ) == 0 )
			{
				params->TrgFifo_Hwm = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgFifo_FullThr" ) == 0 )
			{
				params->TrgFifo_Full = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgTstp_Offset" ) == 0 )
			{
				params->TrgTstp_Offset = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "InMemBusyThr" ) == 0 )
			{
				params->InMemBusyThr = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrgSrc" ) == 0 )
			{
				if( strcmp( argv[3], "Soft" ) == 0 )
					params->TrgSrc = BeuTrgSrc_Soft;
				else if( strcmp( argv[3], "Const" ) == 0 )
					params->TrgSrc = BeuTrgSrc_Const;
				else if( strcmp( argv[3], "Ti1" ) == 0 )
					params->TrgSrc = BeuTrgSrc_Ti1;
				else if( strcmp( argv[3], "Ti2" ) == 0 )
					params->TrgSrc = BeuTrgSrc_Ti2;
				else
				{
					params->BsySrc = BeuTrgSrc_Undefined;
					fprintf( stderr, "%s: line %d: Beu ID %d usupporet value %s for TrgSrc\n", __FUNCTION__, line_num, beu, argv[3] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[2], "BsySrc" ) == 0 )
			{
//printf("%s: BsySrc argc=%d argv[3]=%s\n", __FUNCTION__, argc, argv[3]);
				if( strcmp( argv[3], "None" ) == 0 )
					params->BsySrc = BeuBsySrc_None;
				else if( strcmp( argv[3], "Perm" ) == 0 )
					params->BsySrc = BeuBsySrc_Perm;
				else
				{
					params->BsySrc = BeuBsySrc_None;
					for( index=3; index<argc; index++ )
					{
//printf("%s: BsySrc argv[%d]=%s\n", __FUNCTION__, index, argv[index]);
						if( strcmp( argv[index], "TrgFifo" ) == 0 )
							params->BsySrc |= BeuBsySrc_TrgFifo;
						else if( strcmp( argv[index], "Feu" ) == 0 )
							params->BsySrc |= BeuBsySrc_Feu;
						else if( strcmp( argv[index], "InMem" ) == 0 )
							params->BsySrc |= BeuBsySrc_InMem;
						else
						{
							params->BsySrc = BeuBsySrc_Undefined;
							fprintf( stderr, "%s: line %d: Beu ID %d usupporet value %s for BsySrc\n", __FUNCTION__, line_num, beu, argv[index] ); 
							return D_RetCode_Err_Wrong_Param;
						}
//printf("%s: BsySrc=%d\n", __FUNCTION__, params->BsySrc);
					}
				}
//printf("%s: Final BsySrc=%d\n", __FUNCTION__, params->BsySrc);
//getchar();
			}
			else if( strcmp( argv[2], "SynSrc" ) == 0 )
			{
				if( strcmp( argv[3], "Soft" ) == 0 )
					params->SyncSrc = BeuSynSrc_Soft;
				else if( strcmp( argv[3], "Ti" ) == 0 )
					params->SyncSrc = BeuSynSrc_Ti;
				else
				{
					params->SyncSrc = BeuSynSrc_Undefined;
					fprintf( stderr, "%s: line %d: Beu ID %d usupporet value %s for SyngSrc\n", __FUNCTION__, line_num, beu, argv[3] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[2], "TimeOut" ) == 0 )
			{
				params->timeout = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "MaskToInput" ) == 0 )
			{
				params->MaskToInput = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "EmuDataSize" ) == 0 )
			{
				params->emu_data_size = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "TrigRate" ) == 0 )
			{
				params->TrigRate = atoi( argv[3] );
			}
		} // if( strcmp( argv[0], "Beu" ) == 0 )
	} // if( argc > 0 )

	// All went fine
	return D_RetCode_Sucsess;
}

// Fill the BeuSspConf structure from an ASCII file
int BeuSspConf_Fread( BeuSspConf *params, FILE *fptr )
{
	char line[LINE_SIZE];
	int line_num;
	int ret;

	// Check for Null pointer
	if( params == (BeuSspConf *)NULL )
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
		ret = BeuSspConf_Parse( params, line_num );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: BeuSspConf_Parse failed with %d\n", __FUNCTION__, ret );
			return ret;
		}
	} // while( fgets( line, LINE_SIZE, fptr ) != NULL )

	// All went fine
	return D_RetCode_Sucsess;
}

// Initialize the collection
int BeuSspConfCol_Init( BeuSspConfCol *col )
{
	int beu;
	int ret;

	// Check for Null pointer
	if( col == (BeuSspConfCol *)NULL )
	{
		fprintf( stderr, "%s: col=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Initialize individual structures
	for( beu=0; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		if( ( ret = BeuSspConf_Init( &(col->beu_conf[beu]) ) ) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: BeuSspConf_Init failed for beu %d with %d\n", __FUNCTION__, beu, ret );
			return ret;
		}
	}

//BeuSspConfCol_Fprintf( col, stdout );
	// All went fine
	return D_RetCode_Sucsess;
}

// Initialize the collection
int BeuSspConfCol_Sprintf( BeuSspConfCol *col, char *buf )
{
	int beu;
	int ret;

	// Check for Null pointer
	if( col == (BeuSspConfCol *)NULL )
	{
		fprintf( stderr, "%s: col=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Initialize individual structures
	for( beu=0; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		if( ( ret = BeuSspConf_Sprintf( &(col->beu_conf[beu]), buf ) ) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: BeuSspConf_Sprintf failed for beu %d with %d\n", __FUNCTION__, beu, ret );
			return ret;
		}
	}

	// All went fine
	return D_RetCode_Sucsess;
}

// Dump in ASCII the BeuSspConf structure to a file
int BeuSspConfCol_Fprintf( BeuSspConfCol *col, FILE *fptr )
{
	char buf[128*1024];
	int ret;

	// Check for Null pointer
	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "%s: fptr=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// First produce the ASCII representation of the BeuSspConf parameters structure 
	sprintf( buf, "###############################\n" );
	sprintf( buf, "%s# Backend unit configurations #\n", buf );
	sprintf( buf, "%s###############################\n", buf );
	if( (ret = BeuSspConfCol_Sprintf( col, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: BeuSspConfCol_Sprintf failed with  %d\n", __FUNCTION__, ret );
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

// Fill the BeuSspConfCol structure entries from externally parsed line
int BeuSspConfCol_Parse( BeuSspConfCol *col, int line_num )
{
	int beu;
	int ret;

	// Check for Null pointer
	if( col == (BeuSspConfCol *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	if( argc > 0 )
	{
		if( ( strcmp( argv[0], "Beu" ) == 0 ) || ( strcmp( argv[0], "MVT_Beu" ) == 0 ) || ( strcmp( argv[0], "FTT_Beu" ) == 0 ) )
		{
			// Get beu ID
			if( strcmp( argv[1], "*" ) == 0 )
				beu = 0;
			else
				beu = atoi( argv[1] );
			if( (beu < 0) || (DEF_MAX_NB_OF_BEU <= beu) )
			{
				fprintf( stderr, "%s: line %d: Beu ID %d not in range [0,%d]\n", __FUNCTION__, line_num, beu, DEF_MAX_NB_OF_BEU ); 
				return D_RetCode_Err_Wrong_Param;
			}

			// First produce the ASCII representation of the BeuSspConf parameters structure 
			if( (ret = BeuSspConf_Parse(  &(col->beu_conf[beu]), line_num )) != D_RetCode_Sucsess )
			{
				fprintf( stderr, "%s: BeuSspConf_Parse failed for beu %d with  %d\n", __FUNCTION__, beu, ret );
				return ret;
			}
		} // if( strcmp( argv[0], "Beu" ) == 0 )
	} // if( argc > 0 )

	// All went fine
	return D_RetCode_Sucsess;
}

// Fill the BeuSspConf structure from an ASCII file
int BeuSspConfCol_Fread( BeuSspConfCol *col, FILE *fptr )
{
	char line[LINE_SIZE];
	int line_num;
	int ret;

	// Check for Null pointer
	if( col == (BeuSspConfCol *)NULL )
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
		ret = BeuSspConfCol_Parse( col, line_num );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: BeuSspConfConf_Parse failed with %d\n", __FUNCTION__, ret );
			return ret;
		}
	} // while( fgets( line, LINE_SIZE, fptr ) != NULL )

	// All went fine
	return D_RetCode_Sucsess;
}

// Propagate common Beu parameters
int BeuSspConfCol_Prop( BeuSspConfCol *col )
{
	int beu;
	BeuSspConf *beu_conf_com;
	BeuSspConf *beu_conf_cur;

	// Check for Null pointer
	if( col == (BeuSspConfCol *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Common beu configuration
	beu_conf_com = &(col->beu_conf[0]);
	// Initialize individual structures
	for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		// Running beu configuration
		beu_conf_cur = &(col->beu_conf[beu]);
		// Set only active BEU-s
		if( beu_conf_cur->Id > 0 )
		{
			if( beu_conf_cur->SelfTrigMult == -1 )
				beu_conf_cur->SelfTrigMult = beu_conf_com->SelfTrigMult;
			if( beu_conf_cur->SelfTrigWid == -1 )
				beu_conf_cur->SelfTrigWid = beu_conf_com->SelfTrigWid;
			if( beu_conf_cur->TrgFifo_Lwm == 0 )
				beu_conf_cur->TrgFifo_Lwm = beu_conf_com->TrgFifo_Lwm;
			if( beu_conf_cur->TrgFifo_Hwm == 0 )
				beu_conf_cur->TrgFifo_Hwm = beu_conf_com->TrgFifo_Hwm;
			if( beu_conf_cur->TrgFifo_Full == 0 )
				beu_conf_cur->TrgFifo_Full = beu_conf_com->TrgFifo_Full;
			if( beu_conf_cur->TrgTstp_Offset == 0 )
				beu_conf_cur->TrgTstp_Offset = beu_conf_com->TrgTstp_Offset;
			if( beu_conf_cur->timeout == 0 )
				beu_conf_cur->timeout = beu_conf_com->timeout;
			if( beu_conf_cur->MaskToInput == 0 )
				beu_conf_cur->MaskToInput = beu_conf_com->MaskToInput;
			if( beu_conf_cur->InMemBusyThr == 0 )
				beu_conf_cur->InMemBusyThr = beu_conf_com->InMemBusyThr;
			if( beu_conf_cur->TrigRate == 0 )
				beu_conf_cur->TrigRate = beu_conf_com->TrigRate;
			if( beu_conf_cur->emu_data_size == 0 )
				beu_conf_cur->emu_data_size = beu_conf_com->emu_data_size;
			if( beu_conf_cur->TrgSrc == BeuTrgSrc_Undefined )
				beu_conf_cur->TrgSrc = beu_conf_com->TrgSrc;
			if( beu_conf_cur->BsySrc == BeuBsySrc_Undefined )
				beu_conf_cur->BsySrc = beu_conf_com->BsySrc;
			if( beu_conf_cur->SyncSrc == BeuSynSrc_Undefined )
				beu_conf_cur->SyncSrc = beu_conf_com->SyncSrc;
			if( beu_conf_cur->ClkSrc == BeuClkSrc_Undefined )
				beu_conf_cur->ClkSrc = beu_conf_com->ClkSrc;
		}
	}
	// All went fine
	return D_RetCode_Sucsess;
}
