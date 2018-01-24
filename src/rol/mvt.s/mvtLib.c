//10 AOUT 2016
/******************************************************************************
*
*  mvtLib.h  - Library implementation file for readout of the Clas12 MVT & FTT
*                
*  Author: Yassir.Moudden@cea.fr 
*          Irakli.Mandjavidze@cea.fr
*          June 2015
*
*  Revision  1.0 - Initial Revision
*                  2015/08/27: mvtUploadAll added
*                  2015/08/28: mvtConfig added
*
*  SVN: $Rev$
*
******************************************************************************/

#if defined(Linux_vme)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "mvtLib.h"
#include "Parser.h"
#include "SysConfig.h"
#include "ReturnCodes.h"
#include "BeuConfig.h"

// For the moment an ugly declaration
extern SysParams *sys_params_ptr; // DEFINED IN SysConfig.c

int static num_of_beu;
int beu_id2slot[DEF_MAX_NB_OF_BEU];
static int first_beu_in_token = -1;

// configuration file pointer
static FILE *sys_conf_params_fptr = (FILE *)NULL;

/*
 * Log file management functions
 */
int mvtManageLogFile( FILE* *fptr, int roc_id )
{
	// Log file variables
	char    logfilename[128];
	char    logfilename_backup[128];
	char    log_file_perms[16];
	char    log_message[256];
	FILE   *mvt_fptr_err_1;
	struct  stat log_stat;   
	char   *env_home;
	char    tmp_dir[512];
	char    mkcmd[512];
	int     ret;
	// time variables
	time_t      cur_time;
	struct tm  *time_struct;
	
	// Get current time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	mvt_fptr_err_1 = *fptr;
	if( mvt_fptr_err_1 == (FILE *)NULL )
	{
		if( (env_home = getenv( "HOME" )) == (char *)NULL )
		{
			fprintf(stderr, "%s: Unable to HOME variable; log file in . directory\n", __FUNCTION__ );
			sprintf( tmp_dir, "./" );
		}
		else
		{
			//Check that tmp directory exists and if not create it
			sprintf( tmp_dir, "%s/mvt/tmp", env_home );
			if( stat( tmp_dir, &log_stat ) )
			{
				// create directory
				sprintf( mkcmd, "mkdir -p %s", tmp_dir );
				ret = system( mkcmd );
				if( (ret<0) || (ret==127) )
				{
					fprintf(stderr, "%s: failed to create dir %s with %d\n", __FUNCTION__, tmp_dir, ret );
					return(-1);
				}
			}
			else if( !(S_ISDIR(log_stat.st_mode)) )
			{
				fprintf(stderr, "%s: %s file exists but is not directory\n", __FUNCTION__, tmp_dir );
				return(-1);
			}
		}

		// Check if log file already exists	
		sprintf(logfilename, "%s/mvt_roc_%d_rol_1.log", tmp_dir, roc_id);
		sprintf(log_file_perms, "a+");
		if( stat( logfilename, &log_stat ) == 0 )
		{
			// file exists, check its size
			if( log_stat.st_size > 100000 )
			{
				// Too big, rename it
				// form backup log file name
				sprintf
				(
					logfilename_backup,
					"%s/mvt_roc_%d_rol_1_%02d%02d%02d_%02dH%02d.log",
					tmp_dir,
					roc_id,
					time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
					time_struct->tm_hour, time_struct->tm_min
				);
  				if( rename(logfilename, logfilename_backup) ) 
				{
					fprintf(stderr, "%s: rename failed from log file %s to %s with %d\n", __FUNCTION__, logfilename, logfilename_backup, errno);
				 	perror("rename failed");
					return( -1 );
				}
				sprintf(log_file_perms, "w");
			}
		}

		// Open file
		if( (mvt_fptr_err_1 = fopen(logfilename, log_file_perms)) == (FILE *)NULL )
		{
			fprintf(stderr, "%s: fopen failed to open log file %s in %s mode with %d\n", __FUNCTION__, logfilename, log_file_perms, errno);
		 	perror("fopen failed");
			return( -1 );
		}
		mvtSetLogFilePointer( mvt_fptr_err_1 );
	}
	*fptr = mvt_fptr_err_1;
	if( mvt_fptr_err_1 != (FILE *)NULL )
	{
		fprintf( mvt_fptr_err_1, "**************************************************\n" );
		fprintf( mvt_fptr_err_1, "%s at %02d%02d%02d %02dH%02d\n", __FUNCTION__,
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday, time_struct->tm_hour, time_struct->tm_min );
		fflush( mvt_fptr_err_1 );
		return (1);
	}
	return (0);
}

int mvtSetLogFilePointer( FILE *fptr )
{
	int ret;
	if( (ret = SysConfig_SetLogFilePointer( fptr ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfig_SetLogFilePointer failed with %d\n", __FUNCTION__, ret );
		return( -1 );
	}
	return( 0 );
}

int mvtClrLogFilePointer()
{
	int ret;
	if( (ret = SysConfig_ClrLogFilePointer() ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfig_ClrLogFilePointer failed with %d\n", __FUNCTION__, ret );
		return( -1 );
	}
	return( 0 );
}

char *mvtRocId2SysName( int roc_id )
{
         if( roc_id == 0x45 ) return "MVT"; // 69 mvt1
    else if( roc_id == 0x44 ) return "MVT"; // 68 mvt2
    else if( roc_id == 0x4B ) return "FTT"; // 75 mmft1
    else if( roc_id == 0x3F ) return "JTB"; // 63 svt3
    else if( roc_id == 0x01 ) return "STB"; //  1 sedipcq156
    else                      return "UKN";
}

int mvtRocName2RocId( char *roc_str )
{
	     if( strcmp( roc_str, "mvt1"       ) == 0 ) return 0x45; // 69 mvt1
	else if( strcmp( roc_str, "mvt2"       ) == 0 ) return 0x44; // 68 mvt2
	else if( strcmp( roc_str, "mmft1"      ) == 0 ) return 0x4B; // 75 mmft1
	else if( strcmp( roc_str, "svt3"       ) == 0 ) return 0x3F; // 63 svt3
	else if( strcmp( roc_str, "sedipcq156" ) == 0 ) return 0x01; //  1 sedipcq156
	else                                            return 0x00; // Unknown
}

char *mvtRocId2RocName( int roc_id )
{
         if( roc_id == 0x45 ) return "mvt1";       // 69 mvt1
    else if( roc_id == 0x44 ) return "mvt2";       // 68 mvt2
    else if( roc_id == 0x4B ) return "mmft1";      // 75 mmft1
    else if( roc_id == 0x3F ) return "svt3";       // 63 svt3
    else if( roc_id == 0x01 ) return "sedipcq156"; //  1 sedipcq156
    else                      return "UKN";
}


/*
 * End of Log file management functions
 */

/*
 *
 */
int mvtInit(unsigned int addr, unsigned int addr_inc, int nfind, int iFlag) 
{
	int ret;
	char conf_file_name[128];
	int bec_id;
	int bec;
	int beu;

	bec_id = iFlag;
	num_of_beu = 0;

 	/*--------------------*/
	/* SysConfig (start)  */
	/*--------------------*/
	// Set up config file name
	sprintf(conf_file_name, "%s", "/home/daq/Software/SysConfig/Implementation/Projects/Software/CentOs/bin/Mvt.cfg");  
	if( (ret=SysConfigFromFile(conf_file_name)) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfigFromFile failed for file %s with %d\n", __FUNCTION__, conf_file_name, ret );
		SysConfig_CleanUp();
		fprintf( stdout, "%s: **** Failed ****\n", __FUNCTION__ );
//		daLogMsg("ERROR","%s: SysConfigFromFile failed for file %s with %d", __FUNCTION__, conf_file_name, ret);
		return( ret );
	}
	/*-------------------------------------------------*/
	/* SysConfig (end)       */
	/*-------------------------------------------------*/ 
	fprintf(stdout, "%s: **** end ****\n", __FUNCTION__);

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == bec_id )
		{ 
			for( beu=0; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
				{
					beu_id2slot[num_of_beu]=sys_params_ptr->Bec_Params[bec].Beu_Slot[beu];
					num_of_beu++;
 				}
			}
		} // if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

	return num_of_beu;
}

/*
 *
 */
int mvtSlot( int id ) 
{
	if( (id < 0) || (num_of_beu <= id) )
	{
		fprintf( stderr, "%s: requested ID=%d out of range [0;%d]; num_of_beu =%d\n", __FUNCTION__, id, num_of_beu-1, num_of_beu );
		return -1;
	}
	return beu_id2slot[id];
}

/*
 * 
 */
int mvtDownload()
{
	int ret;
	char conf_file_name[128];

 	/*--------------------*/
	/* SysConfig (start)  */
	/*--------------------*/
	// Set up config file name
        sprintf(conf_file_name, "%s", "/home/daq/Software/SysConfig/Implementation/Projects/Software/CentOs/bin/Mvt.cfg");
	if( (ret=SysConfigFromFile(conf_file_name)) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfigFromFile failed for file %s with %d\n", __FUNCTION__, conf_file_name, ret );
		SysConfig_CleanUp();
		fprintf( stdout, "%s: **** Failed ****\n", __FUNCTION__ );
		return( ret );
	}
	/*-------------------------------------------------*/
	/* SysConfig (end)       */
	/*-------------------------------------------------*/ 
	fprintf(stdout, "%s: **** end ****\n", __FUNCTION__);

	return(1);
}

/*
 * 
 */
int mvtPrestart()
{
	int ret;
	int bec;
	int beu;
	int beu_lnk;
	int feu;
	int num_of_feu;

	// time variables
	struct timeval t0;
	struct timeval t1;
	struct timeval dt;

	fprintf(stdout, "%s: **** starting ****\n", __FUNCTION__);
	if( sys_log_fptr != (FILE *)NULL )
		fprintf(sys_log_fptr, "%s: **** starting ****\n", __FUNCTION__);

	// Get start time for performance measurements
	gettimeofday(&t0,0);

	/**********************
	 * Configure the system
	 **********************/
	// Move this from download
	if( (ret=SysConfig( sys_params_ptr, 2 )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfig failed with %d at first pass\nSecond attempt...\n",
			__FUNCTION__, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr, "%s: SysConfig failed with %d at first pass\nSecond attempt...\n",
				__FUNCTION__, ret );
		// Do only MVT config staff here; no TI/SD
		if( (ret=SysConfig( sys_params_ptr, 2 )) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SysConfig failed with %d at second pass\nThird attempt...\n",
				__FUNCTION__, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr, "%s: SysConfig failed with %d at second pass\nThird attempt...\n",
					__FUNCTION__, ret );
			if( (ret=SysConfig( sys_params_ptr, 2 )) != D_RetCode_Sucsess )
		  	{
		 	   	fprintf( stderr, "%s: SysConfig failed with %d at third pass\nAbandon...\n",
					__FUNCTION__, ret );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr, "%s: SysConfig failed with %d at third pass\nAbandon...\n",
						__FUNCTION__, ret );
		   		return ret;
		  	}
		}
	}

	// Set system in Running state
	if( (ret=SysRun()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysRun failed with %d\n", __FUNCTION__, ret );
		SysConfig_CleanUp();
		return(ret);
	}

	num_of_feu = 0;
	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		/* Configure FEU-s */
		for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
		{
			beu        = ((feu>>5)&0x07)+1;
			beu_lnk    = ( feu    &0x1F)-1;
			if( (1<=sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
			{
				num_of_feu++;
			} // if( (1<=sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]
		} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

	// Get end time for performance measurements
	gettimeofday(&t1, 0);
	timersub(&t1,&t0,&dt);
	fprintf(stdout, "%s: The system with %d FEUs has been configured in %d.%d sec\n",
		__FUNCTION__, num_of_feu, dt.tv_sec, (int)( ((double)dt.tv_usec) / 100000. + 0.5) );
	if( sys_log_fptr != (FILE *)NULL )
		fprintf(sys_log_fptr, "%s: The system with %d FEUs has been configured in %d.%d sec\n",
			__FUNCTION__, num_of_feu, dt.tv_sec, (int)( ((double)dt.tv_usec) / 100000. + 0.5) );

	fprintf(stdout, "%s: **** end ****\n", __FUNCTION__);
	if( sys_log_fptr != (FILE *)NULL )
		fprintf(sys_log_fptr, "%s: **** end ****\n", __FUNCTION__);

	return(num_of_feu);
}

/*
 * 
 */
int mvtGo()
{
	int ret;

//	fprintf(stdout, "%s: **** starting ****\n", __FUNCTION__);

	// Synchronize and Enable trigger processing
	if( (ret=SysGo()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysGo failed with %d\n", __FUNCTION__, ret );
		SysConfig_CleanUp();
		return(ret);		
	}
//	fprintf(stdout, "%s: **** end ****\n", __FUNCTION__);

 	// If the multi board block transfer is requested
	if( sys_params_ptr->Bec_Params[1].BaseAdr_A32m_Com_Enb )
	{
		// Give the token to the first board in advance
		// the index of the control register must point to the board in smolest slot number
		// TBD: during configuration remember the smalles slot index and use it 
		if( (first_beu_in_token <= 0) || (DEF_MAX_NB_OF_BEU<= first_beu_in_token) )
		{
			fprintf(stderr, "%s: first_beu_in_token = %d non valid\n", __FUNCTION__, first_beu_in_token );
			return(first_beu_in_token);
		}
		beusspTakeToken(beu_reg_control[first_beu_in_token]); // to the board with the samllest slot number.
	}
	return(0);
}

/*
 * 
 */
int mvtEnd()
{
	int ret;

	fprintf(stdout, "%s: **** starting ****\n", __FUNCTION__);

	// Set system in Idle state
	if( (ret=SysStop()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysStop failed with %d\n", __FUNCTION__, ret );
		SysConfig_CleanUp();
		return(ret);
	}

	// Set system in Idle state
	if( (ret=SysDumpFeuOptLnk()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysDumpFeuOptLnk failed with %d\n", __FUNCTION__, ret );
		SysConfig_CleanUp();
		return(ret);
	}

	fprintf(stdout, "%s: **** end ****\n", __FUNCTION__);
	return(0);
}

/*
 * 
 */
int mvtGBReady(int id)
{
	unsigned int readme;
	unsigned int tokenstatus;
	int slot_flag = 0;
	int bec, beu;

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
			for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
				{
 					// If the multi board block transfer is requested
					if( sys_params_ptr->Bec_Params[bec].BaseAdr_A32m_Com_Enb )
					{
						if( sys_params_ptr->Bec_Params[bec].Beu_Mblk_Rank[beu] == MblkRank_First )
						{
							tokenstatus = beusspGetTokenStatus(beu_reg_control[beu]);
							if( (tokenstatus & 0x80000000) == 0 )
								return 0;
						}
					}
					// Check if beu-s are ready
					readme = beusspBReady(beu_reg_control[beu]);
						
					//if( readme & 0x00010000 )
					if( readme )
						slot_flag |= (1<<sys_params_ptr->Bec_Params[bec].Beu_Slot[beu]);
 				}
			}
		} // if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	return( slot_flag );
}

int mvtGetNbrOfBeu(int id)
{
	unsigned int NbrOfBeu=0;
	int bec, beu;

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
			for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
				{
 					NbrOfBeu++;
				}
			}
		} 
	} 
	return( NbrOfBeu);
}

int mvtGetNbrOfEventsPerBlock(int id)
{
	unsigned int NbrOfEventsPerBlock=0;
	int bec, beu;

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
			NbrOfEventsPerBlock = sys_params_ptr->NbOfEvtPerBlk;
		} 
	} 
	return( NbrOfEventsPerBlock);
}

int mvtGetNbrOfSamplesPerEvent(int id)
{
  //	unsigned int NbrOfSamplesPerEvent=0;
  //	int bec, beu;

	// Go through back end crates
  /*
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
			NbrOfSamplesPerEvent = sys_params_ptr->NbOfSmpPerEvt;
		} 
	} 
	return( NbrOfSamplesPerEvent);
  */
  return (sys_params_ptr->NbOfSmpPerEvt);
}

int mvtGetPrescale(int id)
{
	unsigned int BlockPrescale=0;
	int bec, beu;

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
			BlockPrescale = sys_params_ptr->BlockPrescale;
		} 
	} 
	return( BlockPrescale);
}

int mvtGetZSMode(int id)
{
	int zsmode=-1;
	int bec, beu;

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
			zsmode = sys_params_ptr->FeuParams_Col.feu_params[0].Feu_RunCtrl_ZS;
		} 
	} 
	return( zsmode );
}


int mvtGetNbrOfFeu(int id, int BeuId)
{
	unsigned int activefeu = 0;
	int bec, ii;
	int nf=0;

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
				if( sys_params_ptr->Bec_Params[bec].Beu_Id[BeuId] > 0 )
				{
 					activefeu = sys_params_ptr->BeuSspConf_Col.beu_conf[BeuId].rol_enb;
 					nf = 0;
 					for (ii = 0; ii<32; ii++){
 						nf+= (activefeu>>ii)&0x00000001;
 					} 					
 				}
			}
		} 
	
	return(nf);
}

int mvtGetRepRawData()
{
	unsigned int activefeu = 0;
	int bec, ii;
	int nf=0;
	
	if( (sys_params_ptr->RunMode == Standalone) || (sys_params_ptr->RunMode == Expert) )
		return(sys_params_ptr->RepRawData);
	return(0);
}


int mvtSetCurrentBlockLevel( int block_level )
{
  int bec;
  int beu;
  // Go through back end crates
  for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
    {
      if( sys_params_ptr->Bec_Params[bec].Crate_Id > 0 )
	{ 
	  for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	    {
	      if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
		{
		  beusspSetSampleBlock(beu_reg_control[beu], sys_params_ptr->NbOfSmpPerEvt, block_level );
		}
	    } 					
	}
    }
  sys_params_ptr->NbOfEvtPerBlk = block_level;
  return 0;
}

/*
int mvtGetNbrOfBeu(int id)
int mvtGetNbrOfEventsPerBlock(int id)
int mvtGetNbrOfSamplesPerEvent(int id)
int mvtGetNbrOfFeu(int id)
*/
/*int MVT_NBR_OF_BEU = 2;
int MVT_NBR_EVENTS_PER_BLOCK = 4;
int MVT_NBR_SAMPLES_PER_EVENT = 6;
int MVT_NBR_OF_FEU[2] = {4,4};
*/

/*
 * 
 */
#define MAX_EVENT_LENGTH MAX_BLOCK_LENGTH
int mvtReadBlock(int id, unsigned int *data, int nwrds, int rflag)
{
	int len = 0;
	int dCnt=0;
	unsigned int blocksize=0;
//	int first_beu = -1;
	int bec=0, beu=0;
	int beu_block_size;
	int beu_rd_iter;

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
		{ 
			//set bzrd high ( acknowledge )
			for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
				{
					beusspBZRDHigh(beu_reg_control[beu]);
//					if( sys_params_ptr->Bec_Params[bec].Beu_Mblk_Rank[beu] == MblkRank_First )
//						first_beu = beu;
				}
			} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )

 			// If the multi board block transfer is requested
			if( sys_params_ptr->Bec_Params[1].BaseAdr_A32m_Com_Enb )
			{
				if( first_beu_in_token <= 0 )
				{
					fprintf(stderr, "%s: first_beu_in_token = %d in bec %d non valid\n", __FUNCTION__, first_beu_in_token, bec );
					return(first_beu_in_token);
				}

				//SHOULD WE DO HERE SOMETHING SIMILAR TO WHAT WE DO BELOW WITH A MAXIMUM READ SIZE EQUAL TO THE FIFO DEPTH ?							
				dCnt = beusspTokenReadBlock(beu_reg_control[first_beu_in_token], BEUSSPmblk, data, (MAX_EVENT_LENGTH>>2)-1024, 1);

				// Forward: first board needs to take token 
				beusspTakeToken(beu_reg_control[first_beu_in_token]);

				if(dCnt<=0)
				{
					fprintf(stderr, "%s: beusspTokenReadBlock failed for in bec %d with first_beu_in_token %d with dCnt = %d\n", __FUNCTION__, bec, first_beu_in_token, dCnt);
					return(dCnt);
				}
				len = dCnt;
			}
			else
			{
				for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
				{
					if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
					{

						blocksize = beusspGetVMEFIFOStatus(beu_reg_control[beu]);
//fprintf(stderr, "%s: beusspGetVMEFIFOStatus for beu %d in bec %d  vmefifostatus %x\n", __FUNCTION__,beu, bec,blocksize);
						beusspGetBlockSize(beu_reg_control[beu], &blocksize);
						beu_block_size = blocksize;
						beu_rd_iter = 0;


//VME USER FIFO FULL THRESHOLD IS 0X7FF0 but thanks to the "if" below its OK to set DEF_BEU_READ_BUF_SIZE to 0X7FFF OR EVEN TO 0X8000
#define DEF_BEU_READ_BUF_SIZE 32768

						while( blocksize )
						{

							//SHOULD WE FIRST CHECK THAT THE FIFO IS FULL OR THAT A TRAILER IS PRESENT IN THE FIFO ??
							//SHOULD WE DO THAT HERE OR IN BEUSSPREADBLOCK ? 
							//SHOULD WE MOVE ALL THIS INTO BEUSSPLIB.C ?							

							if( blocksize > DEF_BEU_READ_BUF_SIZE )
								dCnt = beusspReadBlock(beu_reg_control[beu], beu_fifo[beu], data, DEF_BEU_READ_BUF_SIZE, 1);
							else
								dCnt = beusspReadBlock(beu_reg_control[beu], beu_fifo[beu], data, blocksize, 1);
							if(dCnt<=0)
							{
								fprintf(stderr, "%s: beusspReadBlock failed for beu %d in bec %d with dCnt = %d reading beu_block_size=%d in iter=%d\n",
									__FUNCTION__, beu, bec, dCnt, beu_block_size, beu_rd_iter);
								return(dCnt);
							}
							beu_rd_iter++;
							blocksize -= dCnt;
							len  += dCnt;
							data += dCnt;
						}
/*
						dCnt = beusspReadBlock(beu_reg_control[beu], beu_fifo[beu], data, blocksize, 1);
						if(dCnt<=0)
						{
							fprintf(stderr, "%s: beusspReadBlock failed for beu %d in bec %d with dCnt = %d\n", __FUNCTION__, beu, bec, dCnt);
							return(dCnt);
						}
						len += dCnt;
						data += dCnt;
*/
					}
				} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
 			}

			//set bzrd low ( acknowledge second step : fin relecture data bloc)
			for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
					beusspBZRDLow(beu_reg_control[beu]);
			} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
		} // if( sys_params_ptr->Bec_Params[1].Crate_Id == id )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	return(len);
}

/*
 *  CLEANUP
 */
int mvtCleanup()
{
	fprintf(stdout, "%s: **** starting ****\n", __FUNCTION__);
	if(sys_conf_params_fptr != (FILE *)NULL)
	{
		fclose(sys_conf_params_fptr);
		sys_conf_params_fptr = (FILE *)NULL;
	}
	SysConfig_CleanUp();
	fprintf(stdout, "%s: **** end ****\n", __FUNCTION__);
	return(0);
}

/*
 * 
 */
int mvtConfig( char *sys_conf_params_filename, int run_number, int bec_id )
{
	// time variables
	time_t     cur_time;
	struct tm *time_struct;
	struct timeval t0;
	struct timeval t1;
	struct timeval dt;

	int ret;
	char filename[256];
	char copy_filename[256];

	char host_name[128];
	char roc_name[128];
	char roc_type[16];
	int active;
	int roc_detected;

	char *clonparms;
	char *expid;

	char line[LINE_SIZE];
	int line_num;

	int bec;
	int beu;

	// Get execution time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	// Get start time for performance measurements
	gettimeofday(&t0,0);

	// Check for Null pointer
	if( sys_conf_params_filename == (char *)NULL )
	{
		fprintf( stderr, "%s: sys_conf_params_filename=NULL\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Initialize parameters
	if( (ret = SysParams_Init( sys_params_ptr ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_Init failed with %d\n", __FUNCTION__, ret );
		return ret;
	}

	/**********************
	 * Read configuration *
	 **********************/
	gethostname( host_name, 128);
	clonparms = getenv( "CLON_PARMS" );
	expid = getenv( "EXPID" );
	if( strlen(sys_conf_params_filename) !=0 ) /* filename specified */
	{
		if( (sys_conf_params_filename[0]=='/') || ( (sys_conf_params_filename[0]=='.') && (sys_conf_params_filename[1]=='/') ) )
		{
			sprintf(filename, "%s", sys_conf_params_filename);
		}
		else
		{
			sprintf(filename, "%s/mvt/%s", clonparms, sys_conf_params_filename);
		}

		// Open config file
		if( (sys_conf_params_fptr=fopen(filename, "r")) == NULL )
		{
			fprintf( stderr, "%s: fopen failed for config file %s in read mode\n", __FUNCTION__, filename );
			fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
			return D_RetCode_Err_FileIO;
		}
	}
	else /* filename does not specified */
	{
		sprintf(filename, "%s/mvt/%s.cnf", clonparms, host_name);
		fprintf( stderr, "%s: attempt to open config file %s in read mode\n", __FUNCTION__, filename );
		if( (sys_conf_params_fptr=fopen(filename, "r")) == NULL )
		{
			fprintf( stderr, "%s: fopen failed for config file %s in read mode\n", __FUNCTION__, filename );
			sprintf(filename, "%s/mvt/%s.cnf", clonparms, expid);
			fprintf( stderr, "%s: attempt to open config file %s in read mode\n", __FUNCTION__, filename );
			if( (sys_conf_params_fptr=fopen(filename, "r")) == NULL )
			{
				fprintf( stderr, "%s: fopen failed for config file %s in read mode\n", __FUNCTION__, filename );
				fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
				return D_RetCode_Err_FileIO;
	  		}
		}
  	}
	fprintf(stdout, "%s: Using configuration file %s\n", __FUNCTION__, filename );

	// Process parameter file
	line_num = 0;
	roc_detected = -1;
	while( fgets( line, LINE_SIZE, sys_conf_params_fptr ) != NULL )
	{
		/* parse the line */
		parse_line(line);
		line_num++;
		if( argc > 0 )
		{
			if( ( strcmp( argv[0], "MVT_CRATE" )  == 0 ) || ( strcmp( argv[0], "FTT_CRATE" )  == 0 ) )
			{
				sprintf( roc_type, "%s", argv[0] );
				if( strcmp( argv[1], "all" ) == 0 )
				{
					fprintf(stdout, "%s: %s crate on host %s - activated implicitly\n", __FUNCTION__, argv[0], host_name);
					active = 1;
				}
				else if( strcmp( argv[1], host_name ) == 0 )
				{
					sprintf( roc_name, "%s", argv[1] );
					fprintf(stdout, "%s: %s crate on host %s - activated explicitly\n", __FUNCTION__, argv[0], host_name);
					active = 1;
					roc_detected = 0;
				}
				else if( strcmp( argv[1], "end" ) == 0 )
				{
					if( active )
					{
						fprintf(stdout, "%s: %s crate on host %s - disactivated\n", __FUNCTION__, argv[0], host_name);
						active = 0;
						if( roc_detected == 0 )
							roc_detected = 1;
					}
				}
				else
				{
					fprintf(stdout, "%s: %s crate on host %s - parameters will be ignored\n", __FUNCTION__, argv[0], host_name);
					active = 0;
				}
 			}
			else if( active )
			{
//fprintf( stderr, "%s: line_num=%d argc=%d argv[0]=%s\n", __FUNCTION__, line_num, argc, argv[0] );
				// Parse parameters
				if( (ret = SysParams_Parse( sys_params_ptr, line_num )) != D_RetCode_Sucsess )
				{
					fprintf( stderr, "%s: SysParams_Parse failed with %d\n", __FUNCTION__, ret );
					return ret;
				}
			}
		} // if( argc > 0 )
	} // while( fgets( line, LINE_SIZE, sys_conf_params_fptr ) != NULL )

	// Close config file
	fclose( sys_conf_params_fptr );
	sys_conf_params_fptr = (FILE *)NULL;
	
	if( roc_detected <= 0 )
	{
		fprintf( stderr, "%s: Could not detect roc parameters on host %s in file %s\n", __FUNCTION__, host_name, filename );
		return D_RetCode_Err_Wrong_Param;
	}

	// Propagate Global parameters
	if( (ret = SysParams_Prop( sys_params_ptr )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_Prop failed with %d\n", __FUNCTION__, ret );
		return ret;
	}

	/**********************
	 * Copy configuration *
	 **********************/
fprintf(stdout, "%s: run_number=%d\n", __FUNCTION__, run_number );
	 if( run_number >= 0 )
	 {
		// Prepare filename for configuration copy 
		sprintf
		(
			copy_filename,
			"%s/%s_%02d%02d%02d_%02dH%02d",
			dirname(filename),
			rootfilename(filename),
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
			time_struct->tm_hour, time_struct->tm_min
		);
		if( run_number > 0 )
			sprintf( filename, "%s_run%d.cnf", copy_filename, run_number );
		else
			sprintf( filename, "%s.cnf", copy_filename );
fprintf(stdout, "%s: Using configuration file copy %s\n", __FUNCTION__, filename );

		// Open config file to copy configuration
		if( (sys_conf_params_fptr=fopen(filename, "w")) == NULL )
		{
			fprintf( stderr, "%s: fopen failed for config file %s in write mode\n", __FUNCTION__, filename );
			fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
			return D_RetCode_Err_FileIO;
		}

		// Copy configuration to the file
		fprintf(sys_conf_params_fptr, "%s %s\n", roc_type, roc_name);
		if( (ret = SysParams_Fprintf( sys_params_ptr, sys_conf_params_fptr )) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SysParams_Fprintf failed for config file %s with %d\n", __FUNCTION__, filename, ret );
			mvtCleanup();
			return ret;
		}
		fprintf(sys_conf_params_fptr, "%s end\n", roc_type);

		// Close config file
		fclose( sys_conf_params_fptr );
		sys_conf_params_fptr = (FILE *)NULL;
	}

	/**********************
	 * Configure the system
	 **********************/
	if( (ret=SysConfig( sys_params_ptr, 1 )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfig failed for parameters from conf file %s with %d at first pass\nSecond attempt...\n",
			__FUNCTION__, sys_conf_params_filename, ret );
		// Do only TI/SD config here; no MVT staff; it will be done during prestart
		if( (ret=SysConfig( sys_params_ptr, 1 )) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SysConfig failed for parameters from conf file %s with %d at second pass\nThird attempt...\n",
				__FUNCTION__, sys_conf_params_filename, ret );
			if( (ret=SysConfig( sys_params_ptr, 1 )) != D_RetCode_Sucsess )
		  	{
		 	   	fprintf( stderr, "%s: SysConfig failed for parameters from conf file %s with %d at third pass\nAbandon...\n",
					__FUNCTION__, sys_conf_params_filename, ret );
		   		return ret;
		  	}
		}
	}

	// Get end time for performance measurements
	gettimeofday(&t1, 0);
	timersub(&t1,&t0,&dt);
	fprintf(stdout, "%s: The system has been configured in %d.%d sec\n", __FUNCTION__, dt.tv_sec, (int)( ((double)dt.tv_usec) / 100000. + 0.5) );

	// Go through back end crates
	num_of_beu = 0;
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( sys_params_ptr->Bec_Params[bec].Crate_Id == bec_id )
		{ 
			for( beu=0; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
				{
					beu_id2slot[num_of_beu]=sys_params_ptr->Bec_Params[bec].Beu_Slot[beu];
					num_of_beu++;
 					if( sys_params_ptr->Bec_Params[bec].Beu_Mblk_Rank[beu] == MblkRank_First )
						first_beu_in_token = beu;
 				}
			}
		} // if( sys_params_ptr->Bec_Params[bec].Crate_Id == id )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
fprintf(stderr, "%s: first_beu_in_token = %d\n", __FUNCTION__, first_beu_in_token );
	return num_of_beu;
}

/*
 *  Upload MVT settings
 */
int mvtUploadAll( char *string, int length )
{
	char buf[256*1024];
	int ret;

	// Make sure a buffer was provided
	if( length == 0 )
		return 0;

	// Check for non negative buffer length
	if( length < 0 )
		return D_RetCode_Err_Wrong_Param;

 	// Check for Null pointer
	if( string == (char *)NULL )
	{
		fprintf( stderr, "%s: string=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	
	// First produce the ASCII representation of the MVT parameters structure 
	if( (ret = SysParams_Sprintf( sys_params_ptr, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_Sprintf failed with  %d\n", __FUNCTION__, ret );
		return ret;
	}
	
	// Check that enough buffer has been provided
	if( length < strlen(buf) )
	{
		fprintf( stderr, "%s: not enough deep buffer of %d bytes; %d bytes needed\n", __FUNCTION__, length, strlen(buf) );
		return ret;
	}
	sprintf( string, "%s", buf );
	return( strlen(string) );
}


int mvtStatus(int numFeu)
{
 return mvtStatusDump(numFeu, stdout);  
}

int mvtStatusDump(int numFeu, FILE *fptr)
{
  int bec;
  int beu;
  // Go through back end crates
  for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
    {
      if( sys_params_ptr->Bec_Params[bec].Crate_Id > 0 )
	{ 
	  for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	    {
	      if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
			{
				//beusspSetTargetFeu(beu_reg_control[beu], numFeu); 				
				//beusspDisplayAllReg(beu_reg_control[beu]);
				beusspSetTargetFeuAndDumpAllReg(beu_reg_control[beu], numFeu, fptr);
			}
	    } 					
	}
    } 
 return 0;  
}


#endif // #if defined(Linux_vme)
