/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    BecConfigParams.c
-- Description:    Backend crate configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/09/29 IM
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
#include "BecConfigParams.h"

// Conversion function from TiClkSrc to string
char *MblkRank2Str( MblkRank rank )
{
	if( rank == MblkRank_Undefined )
		return ("Undefined");
	else if( rank == MblkRank_Disable )
		return ("Disable  ");
	else if( rank == MblkRank_First )
		return ("First    ");
	else if( rank == MblkRank_Inter )
		return ("Inter    ");
	else if( rank == MblkRank_Last )
		return ("Last     ");
	else
		return ("Unknown  ");
}
MblkRank MblkStr2Rank( char *str )
{
	if( strcmp(str, "Disable") == 0 )
		return( MblkRank_Disable );
	else if( strcmp(str, "First") == 0 )
		return( MblkRank_First );
	else if( strcmp(str, "Inter") == 0 )
		return( MblkRank_Inter );
	else if( strcmp(str, "Last") == 0 )
		return( MblkRank_Last );
	else
		return( MblkRank_Undefined );
}


// Initialize the MvtParams structure with default values
int BecParams_Init( BecParams *params )
{
	int beu;
	int feu;

	// Check for Null pointer
	if( params == (BecParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Backend crate configurateion and crate Id
	params->Config_Id = -1;
	params->Crate_Id  = 0;

	/*
	 * Devices
	 */
	// TI
	params->Ti_Id   = -1;
	params->Ti_Slot = 0;
	params->Ti_BaseAdr_A24_Reg  = 0;
	params->Ti_BaseAdr_A32_Fifo = 0;
	params->Ti_Mblk_Rank        = MblkRank_Disable;

	// SD
	params->Sd_Id   = -1;
	params->Ti_Slot = 0;

	// Beu
	for( beu=0; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		params->Beu_Id[beu]   = -1;
		params->Beu_Slot[beu] = 0;
		params->Beu_BaseAdr_A24_Reg[beu]  = 0;
		params->Beu_BaseAdr_A32_Fifo[beu] = 0;
		params->Beu_Mblk_Rank[beu]        = MblkRank_Disable;
	}

	// Common A32 memory space for chained block transfers
	params->BaseAdr_A32m_Com_Enb = 0;
	params->BaseAdr_A32m_Com_Min = 0;
	params->BaseAdr_A32m_Com_Max = 0;

	// System and self trigger parameters and topology
	params->SelfTrigMult = -1;
	params->SelfTrigWin  = -1;
	for( beu=0; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		for( feu=0; feu<DEF_MAX_NB_OF_FEU_PER_BEU; feu++ )
			params->BeuFeuConnectivity[beu][feu] = -1;
		params->BeuFeuSlfTrigEn[beu] = 0;
	}
	for( feu=0; feu<DEF_MAX_NB_OF_FEU; feu++ )
	{
		params->FeuSn2BeuLnkId[feu] = 0;
	}

	return D_RetCode_Sucsess;
}

// Dump in ASCII the BecParams structure to a buffer
int BecParams_Sprintf( BecParams *params, char *buf  )
{
	int beu;
	int feu;
	int beu_lnk;

	// Check for Null pointer
	if( params == (BecParams *)NULL )
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
	if( params->Config_Id < 0 )
		return D_RetCode_Sucsess;

	// Create info
	sprintf( buf, "%sBec %d CrateId %d\n", buf, params->Config_Id, params->Crate_Id );
	// SD
	sprintf( buf, "%sBec %d Slot %d Sd  %d\n",
		buf, params->Config_Id, params->Sd_Slot, params->Sd_Id );
	// TI
	sprintf
	(
		buf,
		"%sBec %d Slot %d Ti  %d 0x%08x 0x%08x Mblk %s\n",
		buf,
		params->Config_Id, params->Ti_Slot, params->Ti_Id, params->Ti_BaseAdr_A24_Reg, params->Ti_BaseAdr_A32_Fifo,
		MblkRank2Str(params->Ti_Mblk_Rank)
	);
	// Beu
	for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		if( (DEF_MIN_BEU_SLOT <= params->Beu_Slot[beu]) && (params->Beu_Slot[beu] <= DEF_MAX_BEU_SLOT) )
		{
			sprintf
			(
				buf,
				"%sBec %d Slot %d Beu %d 0x%08x 0x%08x Mblk %s\n",
				buf,
				params->Config_Id, params->Beu_Slot[beu], params->Beu_Id[beu], params->Beu_BaseAdr_A24_Reg[beu], params->Beu_BaseAdr_A32_Fifo[beu],
				MblkRank2Str(params->Beu_Mblk_Rank[beu])
			);
		}
	}

	// Common A32 memory address space
	sprintf
	(
		buf, "%sBec %d BaseAdr_A32m_Com %1d 0x%08x 0x%08x\n", buf,
		params->Config_Id, params->BaseAdr_A32m_Com_Enb,
		params->BaseAdr_A32m_Com_Min, params->BaseAdr_A32m_Com_Max
	);

	// System topology
	sprintf( buf, "%s# BE Crate %d topology\n", buf, params->Config_Id );
	for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		if( (DEF_MIN_BEU_SLOT <= params->Beu_Slot[beu]) && (params->Beu_Slot[beu] <= DEF_MAX_BEU_SLOT) )
		{
			sprintf( buf, "%sBec %d Beu %d Feu", buf, params->Config_Id, beu );
			for( feu=0; feu<DEF_MAX_NB_OF_FEU_PER_BEU; feu++ )
			{
				if( params->BeuFeuConnectivity[beu][feu] < 0 )
					sprintf( buf, "%s  M", buf );
				else if( params->BeuFeuConnectivity[beu][feu] == 0 )
					sprintf( buf, "%s  E", buf );
				else if( params->BeuFeuConnectivity[beu][feu] < DEF_MAX_FEU_SN )
				{
					if( params->BeuFeuSlfTrigEn[beu] & (1<<feu) )
						sprintf( buf, "%s T%2d", buf, params->BeuFeuConnectivity[beu][feu] );
					else
						sprintf( buf, "%s  %2d", buf, params->BeuFeuConnectivity[beu][feu] );
				}
				else
				{
					fprintf( stderr, "%s: Unsupported FeuId=%d (>%d) for beu %d connection %d\n",
						__FUNCTION__, params->BeuFeuConnectivity[beu][feu], DEF_MAX_NB_OF_FEU-1, beu, feu );
					return D_RetCode_Err_Wrong_Param;
				}
			}
		}
		else if( params->Beu_Slot[beu] != 0 )
		{
			fprintf( stderr, "%s: BeuSlot=%d not in range [%d;%d] for beu %d\n",
				__FUNCTION__, params->Beu_Slot[beu], DEF_MIN_BEU_SLOT, DEF_MAX_BEU_SLOT, beu );
			return D_RetCode_Err_Wrong_Param;
		}
		sprintf( buf, "%s\n", buf );
	}
	for( feu=1; feu<=DEF_MAX_NB_OF_FEU; feu++ )
	{
		if( ((feu-1) % 4) == 0 )
			sprintf( buf, "%s#", buf );
		sprintf( buf, "%s FeuId %3d <-> ", buf, feu );
		beu        = ((feu>>5)&0x07)+1;
		beu_lnk    = ( feu    &0x1F)-1;
		if( (1<=params->BeuFeuConnectivity[beu][beu_lnk]) && (params->BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
		{
			if( params->BeuFeuSlfTrigEn[beu] & (1<<beu_lnk) )
				sprintf( buf, "%s Beu=%1d Lnk=%2d Sn=%2d T", buf, beu, beu_lnk, params->BeuFeuConnectivity[beu][beu_lnk] );
			else
				sprintf( buf, "%s Beu=%1d Lnk=%2d Sn=%2d  ", buf, beu, beu_lnk, params->BeuFeuConnectivity[beu][beu_lnk] );
		}
		else
			sprintf( buf, "%s      Inactive       ", buf );
		if( feu % 4 )
			sprintf( buf, "%s     ", buf );
		else
			sprintf( buf, "%s\n", buf );
	}
	sprintf( buf, "%s\n", buf );

	// Self trigger parameters
	sprintf( buf, "%sBec %d SelfTrigMult %d\n",      buf, params->Config_Id, params->SelfTrigMult );
	sprintf( buf, "%sBec %d SelfTrigWin  %d # ns\n", buf, params->Config_Id, params->SelfTrigWin );

	// All went fine
	return D_RetCode_Sucsess;
}

// Dump in ASCII the BecParams structure to a file
int BecParams_Fprintf( BecParams *params, FILE *fptr )
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
	if( params->Config_Id < 0 )
		return D_RetCode_Sucsess;

	// Create info
	sprintf( buf, "Bec %d CrateId %d\n", params->Config_Id, params->Crate_Id );
	// First produce the ASCII representation of the MVT parameters structure 
	sprintf( buf, "%s# BE Crate %d configuration\n", buf, params->Config_Id );
	if( (ret = BecParams_Sprintf( params, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: BecParams_Sprintf failed with  %d\n", __FUNCTION__, ret );
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
int BecParams_Parse( BecParams *params, int line_num )
{
	char *end_ptr;
	char *id_ptr;
	int bec;
	int slot;
	int dev_id;
	int base_adr_a24_reg;
	int base_adr_a32_fifo;
	int beu;
	int feu;
	int feu_min;
	int feu_max;
	int beu_feu_con;
	int index;
	MblkRank mblk_rank;

	// Check for Null pointer
	if( params == (BecParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	if( argc > 0 )
	{
		if( ( strcmp( argv[0], "Bec" ) == 0 ) || ( strcmp( argv[0], "MVT_Bec" ) == 0 ) || ( strcmp( argv[0], "FTT_Bec" ) == 0 ) )
		{
			// Get Bec ID
			if( strcmp( argv[1], "*" ) == 0 )
				bec = 0;
			else
				bec = atoi( argv[1] );
			if( (bec < 0) || (DEF_MAX_NB_OF_BEC <= bec) )
			{
				fprintf( stderr, "%s: line %d: Bec Config_ID %d not in range [1,%d]\n", __FUNCTION__, line_num, bec, DEF_MAX_NB_OF_BEC ); 
				return D_RetCode_Err_Wrong_Param;
			}
			params->Config_Id = bec;
			// Crate ID
			if( strcmp( argv[2], "CrateId" ) == 0 )
			{
				params->Crate_Id = atoi( argv[3] );
			}
			// Slots
			else if( strcmp( argv[2], "Slot" ) == 0 )
			{
				slot = atoi( argv[3] );
				dev_id = atoi( argv[5] );
				base_adr_a24_reg  = strtoul( argv[6], &end_ptr, 16 ); 
				base_adr_a32_fifo = strtoul( argv[7], &end_ptr, 16 );
				if( strcmp( argv[8], "Mblk" ) == 0 )
					mblk_rank = MblkStr2Rank( argv[9] );
				else
					mblk_rank = MblkRank_Undefined;
				// TI parameters
				if( strcmp( argv[4], "Ti" ) == 0 )
				{
					if( (dev_id < 0) || (DEF_MAX_NB_OF_BEC <= dev_id) )
					{
						fprintf( stderr, "%s: line %d: TI ID %d not in range [1,%d]\n", __FUNCTION__, line_num, dev_id, DEF_MAX_NB_OF_BEC ); 
						return D_RetCode_Err_Wrong_Param;
					}
					params->Ti_Id = dev_id;
					if( slot != DEF_TI_SLOT )
					{
						fprintf( stderr, "%s: line %d: Wrong TI %d slot id %d in bec %d, must be %d\n",
							__FUNCTION__, line_num, dev_id, slot, bec, DEF_TI_SLOT ); 
						return D_RetCode_Err_Wrong_Param;
					}
					params->Ti_Slot = slot;
					params->Ti_BaseAdr_A24_Reg  = base_adr_a24_reg;
					params->Ti_BaseAdr_A32_Fifo = base_adr_a32_fifo;
					params->Ti_Mblk_Rank        = mblk_rank;
				}
				// SD parameters
				else if( strcmp( argv[4], "Sd" ) == 0 )
				{
					if( (dev_id < 1) || (DEF_MAX_NB_OF_BEC <= dev_id) )
					{
						fprintf( stderr, "%s: line %d: SD ID %d not in range [1,%d]\n", __FUNCTION__, line_num, dev_id, DEF_MAX_NB_OF_BEC ); 
						return D_RetCode_Err_Wrong_Param;
					}
					params->Sd_Id = dev_id;
					if( slot != DEF_SD_SLOT )
					{
						fprintf( stderr, "%s: line %d: Wrong SD %d slot id %d in bec %d, must be %d\n",
							__FUNCTION__, line_num, dev_id, slot, bec, DEF_SD_SLOT ); 
						return D_RetCode_Err_Wrong_Param;
					}
					params->Sd_Slot = slot;
				}
				// Beu parameters
				else if( strcmp( argv[4], "Beu" ) == 0 )
				{
					if( (dev_id < 1) || (DEF_MAX_NB_OF_BEU <= dev_id) )
					{
						fprintf( stderr, "%s: line %d: Beu ID %d not in range [1,%d]\n", __FUNCTION__, line_num, dev_id, DEF_MAX_NB_OF_BEU ); 
						return D_RetCode_Err_Wrong_Param;
					}
					params->Beu_Id[dev_id] = dev_id;
					if( (slot < DEF_MIN_BEU_SLOT) || (DEF_MAX_BEU_SLOT < slot) )
					{
						fprintf( stderr, "%s: line %d: Wrong BEU %d slot id %d in bec %d, must be in range [%d,%d]\n",
							__FUNCTION__, line_num, dev_id, slot, bec, DEF_MIN_BEU_SLOT, DEF_MAX_BEU_SLOT ); 
						return D_RetCode_Err_Wrong_Param;
					}
					params->Beu_Slot[dev_id] = slot;
					params->Beu_BaseAdr_A24_Reg[dev_id]  = base_adr_a24_reg;
					params->Beu_BaseAdr_A32_Fifo[dev_id] = base_adr_a32_fifo;
					params->Beu_Mblk_Rank[dev_id]        = mblk_rank;
				}
			}
			// Crate ID
			else if( strcmp( argv[2], "BaseAdr_A32m_Com" ) == 0 )
			{
				params->BaseAdr_A32m_Com_Enb = atoi( argv[3] );
				params->BaseAdr_A32m_Com_Min = strtoul( argv[4], &end_ptr, 16 );
				params->BaseAdr_A32m_Com_Max = strtoul( argv[5], &end_ptr, 16 );
			}
			else if( strcmp( argv[2], "SelfTrigMult" ) == 0 )
			{
				params->SelfTrigMult = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "SelfTrigWin" ) == 0 )
			{
				params->SelfTrigWin = atoi( argv[3] );
			}
			else if( strcmp( argv[2], "Beu" ) == 0 )
			{
				beu = atoi( argv[3] );
				if( (beu < 1) || (DEF_MAX_NB_OF_BEU <= beu) )
				{
					fprintf( stderr, "%s: line %d: beu id %d out of range [1,%d]\n", __FUNCTION__, line_num, beu, DEF_MAX_NB_OF_BEU ); 
					return D_RetCode_Err_Wrong_Param;
				}
				if( strcmp( argv[4], "Feu" ) != 0 )
				{
					fprintf( stderr, "%s: line %d: wrong topology keyword %s, must be Feu\n", __FUNCTION__, line_num, argv[4] ); 
					return D_RetCode_Err_Wrong_Param;
				}
				beu_feu_con = 0;
				for( index=5; index<argc; index++ )
				{
					// Emulated
					if( ((char)argv[index][0] == 'E') || ((char)argv[index][0] == 'e') )
					{
						params->BeuFeuConnectivity[beu][beu_feu_con] = 0;
						beu_feu_con++;
					}
					// Masked
					else if( ((char)argv[index][0] == 'M') || ((char)argv[index][0] == 'm') )
					{
						params->BeuFeuConnectivity[beu][beu_feu_con] = -1;
						beu_feu_con++;
					}
					// Individual triggeringd
					else if( ((char)argv[index][0] == 'T') || ((char)argv[index][0] == 't') )
					{
						id_ptr = (char *)&(argv[index][1]);
						feu = atoi(id_ptr);
						params->BeuFeuConnectivity[beu][beu_feu_con] = feu;
						if( params->BeuFeuConnectivity[beu][beu_feu_con] >= DEF_MAX_FEU_SN )
						{
							fprintf( stderr, "%s: Unsupported feu %d (>%d) for beu %d connection %d\n",
								__FUNCTION__, params->BeuFeuConnectivity[beu][beu_feu_con], DEF_MAX_NB_OF_FEU, beu, beu_feu_con );
							params->BeuFeuConnectivity[beu][beu_feu_con] = -1;
							return D_RetCode_Err_Wrong_Param;
						}
						params->FeuSn2BeuLnkId[feu] = ((beu-1)<<5) + beu_feu_con + 1;
						params->BeuFeuSlfTrigEn[beu] |= (1<<beu_feu_con);
						beu_feu_con++;
					}
					else 
					{
						if( ((char)argv[index][1] == '-') || ((char)argv[index][2] == '-') )
						{
							// Range
							if( (char)argv[index][1] == '-' )
							{
								feu_min = argv[index][0] - '0';
								if( argv[index][3] == '\0' )
									feu_max = argv[index][2] - '0';
								else
									feu_max = (argv[index][2] - '0')*10 + (argv[index][3] - '0');
							}
							else
							{
								feu_min = (argv[index][0] - '0')*10 + (argv[index][1] - '0');
								if( argv[index][4] == '\0' )
									feu_max = argv[index][3] - '0';
								else
									feu_max = (argv[index][3] - '0')*10 + (argv[index][4] - '0');
							}
						}
						else
						{
							// Individual
							feu_min = atoi(argv[index]);
							feu_max = feu_min;
						}
						for( feu=feu_min; feu<=feu_max; feu++ )
						{
							params->BeuFeuConnectivity[beu][beu_feu_con] = feu;
							if( params->BeuFeuConnectivity[beu][beu_feu_con] >= DEF_MAX_FEU_SN )
							{
								fprintf( stderr, "%s: Unsupported feu %d (>%d) for beu %d connection %d\n",
									__FUNCTION__, params->BeuFeuConnectivity[beu][beu_feu_con], DEF_MAX_NB_OF_FEU, beu, beu_feu_con );
								params->BeuFeuConnectivity[beu][beu_feu_con] = -1;
								return D_RetCode_Err_Wrong_Param;
							}
							params->FeuSn2BeuLnkId[feu] = ((beu-1)<<5) + beu_feu_con + 1;
							beu_feu_con++;
						}
					}
				} // for( index=5; index<argc; index++ )
			} // else if( strcmp( argv[1], "Beu" ) == 0 )
		} // if( strcmp( argv[0], "Bec" ) == 0 )
	} // if( argc > 0 )

	// All went fine
	return D_RetCode_Sucsess;
}

// Fill the BecParams structure from an ASCII file
int BecParams_Fread( BecParams *params, FILE *fptr )
{
	char line[LINE_SIZE];
	int line_num;
	int ret;

	// Check for Null pointer
	if( params == (BecParams *)NULL )
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
		ret = BecParams_Parse( params, line_num );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: BecParams_Parse failed with %d\n", __FUNCTION__, ret );
			return ret;
		}
	} // while( fgets( line, LINE_SIZE, fptr ) != NULL )

	// All went fine
	return D_RetCode_Sucsess;
}

// Propagate common Bec parameters
int BecParams_Prop( BecParams *params )
{
	int bec;
	BecParams *bec_conf_com;
	BecParams *bec_conf_cur;

	// Check for Null pointer
	if( params == (BecParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Common ti configuration
	bec_conf_com = &(params[0]);
	// Initialize individual structures
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		// Running ti configuration
		bec_conf_cur = &(params[bec]);
		// Set only active BEC-s
		if( bec_conf_cur->Config_Id > 0 )
		{
			if( bec_conf_cur->SelfTrigMult < 0 )
				bec_conf_cur->SelfTrigMult = bec_conf_com->SelfTrigMult;
			if( bec_conf_cur->SelfTrigWin < 0 )
				bec_conf_cur->SelfTrigWin = bec_conf_com->SelfTrigWin;
		}
	}
	// All went fine
	return D_RetCode_Sucsess;
}
