/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    SysConfig.c
-- Description:    System configuration
--
-- Target Devices: Windows or Linux PC
-- Tool versions:  Windows Visual C++ or Linux Make
-- 
-- Create Date:    0.0 2014/10/15 IM
-- Revision:       1.0 2019/01/15 IM Add verbosity
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#include "Platform.h"
#include "ReturnCodes.h"
#include "BecConfigParams.h"
#include "FeuConfigParams.h"
#include "SysConfigParams.h"
#include "FeuConfig.h"
#include "BeuConfig.h"
#include "SysConfig.h"

// System configuration parameters structure
SysParams sys_params;
SysParams *sys_params_ptr = &sys_params;

// Log file pointer to be set from outside
FILE *sys_log_fptr = (FILE *)NULL;

// configuration file pointer
static FILE *sys_conf_params_fptr = (FILE *)NULL;
// monitoring file pointer
static FILE *monit_fptr[DEF_MAX_NB_OF_FEU];
static int   monit_fptr_init = 0;

// verbosity level
static int sys_verbose = 0;

// Set verbosity level
void SysConfig_SetVerbosity( int ver_level )
{
  sys_verbose = ver_level;
}

int SysConfig_SetLogFilePointer( FILE *fptr )
{
	// time variables
	time_t     cur_time;
	struct tm *time_struct;

	if( sys_log_fptr != (FILE *)NULL )
	{
		fprintf( sys_log_fptr, "%s: Error sys_log_fptr is non NULL\n", __FUNCTION__ );
		return D_RetCode_Wrn_Null_Pointer;
	}

	sys_log_fptr = fptr;
	if( sys_log_fptr != (FILE *)NULL )
	{
		// Get execution time
		cur_time = time(NULL);
		time_struct = localtime(&cur_time);
		fprintf
		(
			sys_log_fptr,
			"\n%s: was called on %02d%02d%02d at %02dH%02d\n",
			__FUNCTION__, 
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
			time_struct->tm_hour,     time_struct->tm_min
		);
		return D_RetCode_Sucsess;
	}
	else
	{
		fprintf( stderr, "%s: Warning new sys_log_fptr is NULL\n", __FUNCTION__ );
		return D_RetCode_Wrn_Null_Pointer;
	}
}

int SysConfig_ClrLogFilePointer()
{
	sys_log_fptr = (FILE *)NULL;
	return D_RetCode_Sucsess;
}


// Cleanup function
void SysConfig_CleanUp()
{
	int index;

	FeuConfig_CleanUp();
	if(sys_conf_params_fptr != (FILE *)NULL)
	{
		fclose(sys_conf_params_fptr);
		sys_conf_params_fptr = (FILE *)NULL;
	}
	if( monit_fptr_init == 1 )
	{
		for( index=1; index<DEF_MAX_NB_OF_FEU; index++ )
		{
			if(monit_fptr[index] != (FILE *)NULL)
			{
				fflush(monit_fptr[index]);
				fclose(monit_fptr[index]);
				monit_fptr[index] = (FILE *)NULL;
			}
		}
	}
}

/*
 * Configuration function reading parameters file
 */
int SysConfigFromFile( char *sys_conf_params_filename )
{
	// time variables
	time_t     cur_time;
	struct tm *time_struct;
	struct timeval t0;
	struct timeval t1;
	struct timeval dt;

	int ret;
	char filename[128];

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

	/**********************
	 * Read configuration *
	 **********************/
	// Open config file
	if( (sys_conf_params_fptr=fopen(sys_conf_params_filename, "r")) == NULL )
	{
		fprintf( stderr, "%s: fopen failed for config file %s in read mode\n", __FUNCTION__, sys_conf_params_fptr );
		fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
		return D_RetCode_Err_FileIO;
	}

	// Initialize parameters
	if( (ret = SysParams_Init( &sys_params ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_Init failed with %d\n", __FUNCTION__, ret );
		return ret;
	}
	if( (ret = SysParams_MinSet( sys_params_ptr ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_MinSet failed with %d\n", __FUNCTION__, ret );
		return ret;
	}

	// Read system parameters from file
	if( (ret = SysParams_Fread( &sys_params, sys_conf_params_fptr ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_Fread failed for config file %s with %d\n", __FUNCTION__, sys_conf_params_filename, ret );
		return ret;
	}

	// Propagate Global parameters
	if( (ret = SysParams_Prop( &sys_params )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_Prop failed with %d\n", __FUNCTION__, ret );
		return ret;
	}

	// Close config file
	fclose( sys_conf_params_fptr );
	sys_conf_params_fptr = (FILE *)NULL;

	/**********************
	 * Copy configuration *
	 **********************/
  if( sys_verbose )
  {
    if( sys_log_fptr != (FILE *)NULL )
    {
      sys_conf_params_fptr = sys_log_fptr;
      fprintf
      (
        sys_conf_params_fptr,
        "%s_%02d%02d%02d_%02dH%02d.cfg.cpy\n",
		    rootfilename(sys_conf_params_filename),
		    time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
		    time_struct->tm_hour, time_struct->tm_min
      );
	    sprintf( filename, "LOGFILE" );
    }
    else
    {
	    // Prepare filename for configuration copy 
	    sprintf
	    (
		    filename,
		    "%s_%02d%02d%02d_%02dH%02d.cfg.cpy",
		    rootfilename(sys_conf_params_filename),
		    time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
		    time_struct->tm_hour, time_struct->tm_min
	    );

	    // Open config file to copy configuration
	    if( (sys_conf_params_fptr=fopen(filename, "w")) == NULL )
	    {
		    fprintf( stderr, "%s: fopen failed for config file %s in write mode\n", __FUNCTION__, filename );
		    fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
		    return D_RetCode_Err_FileIO;
	    }
    }

    // Copy configuration to the file
    if( (ret = SysParams_Fprintf( &sys_params, sys_conf_params_fptr )) != D_RetCode_Sucsess )
    {
	    fprintf( stderr, "%s: SysParams_Fprintf failed for config copy file %s with %d\n", __FUNCTION__, filename, ret );
      return ret;
    }

	  // Close config file
    if( sys_log_fptr == (FILE *)NULL )
	    fclose( sys_conf_params_fptr );
	  sys_conf_params_fptr = (FILE *)NULL;
  }

	/*
	 * Configure the system
	 */
	if( (ret=SysConfig( &sys_params, 1 )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfig failed for parameters from conf file %s with %d\n", __FUNCTION__, sys_conf_params_filename, ret );
		return ret;
	}
	usleep(1000000);
	/*
	 * Configure the system
	 */
	if( (ret=SysConfig( &sys_params, 2 )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfig failed for parameters from conf file %s with %d\n", __FUNCTION__, sys_conf_params_filename, ret );
		return ret;
	}

	// Get end time for performance measurements
	gettimeofday(&t1, 0);
	timersub(&t1,&t0,&dt);
	fprintf(stdout, "%s: The system has been configured in %d.%d sec\n", __FUNCTION__, dt.tv_sec, (int)(dt.tv_usec / 100000. + 0.5) );
	return D_RetCode_Sucsess;
}

/*
 * Configuration function
 * 	if configs_to_do == 1 do TI/SD config
 *	if configs_to_do == 2 do MVT config
 *	if configs_to_do == 3 do TI/SD & MVT config
 */
int SysConfig( SysParams *params, int configs_to_do )
{
	int ret;
	int bec;
	int feu;
	int beu;
	int beu_lnk;
//	FeuParams *feu_def_params;
	FeuParams *feu_cur_params;
	int index;

	int feu_rx_err_cntr;
	int feu_par_err_cntr;
	int feu_pac_err_cntr;
	int reset_links;
	int reset_links_cntr;

	// Check for Null pointer
	if( params == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	/* First clear tables with pointers to the BEU control structures and data fifo-s */
	for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		beu_reg_control[beu]=NULL;
		beu_fifo[beu]=NULL;
	}

	// Default parameters for  FEU-s
//	feu_def_params = &(params->FeuParams_Col.feu_params[0]);

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( params->Bec_Params[bec].Config_Id < 0 )
			continue;
		/* First configure TI */
		/* But do it only in Clas12 running mode */
		if( configs_to_do & 0x1 )
		{
			if( params->RunMode != Clas12 )
			{
				if( params->Ti_Params[bec].Id > 0 )
				{
					if( (ret=TiConfig( &(params->Ti_Params[bec]))) != D_RetCode_Sucsess )
					{
						fprintf(stderr, "%s: TiConfig failed with %d for ti %d\n", __FUNCTION__, ret, params->Ti_Params[bec].Id );
						return ret;
					}
				}
			}

			// To be understood during the rol merging
			// For now do it in this library
			/* Next configure SD */
			if( params->Sd_Params[bec].Id > 0 )
			{
				if( (ret=SdConfig( &(params->Sd_Params[bec]))) != D_RetCode_Sucsess )
				{
					fprintf(stderr, "%s: SdConfig failed with %d\n", __FUNCTION__, ret );
					return ret;
				}
			}
		}

		// Configure MVT staff if requested
		if( configs_to_do & 0x2 )
		{
			/* Configure BEU-s */
			for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				if( params->Bec_Params[bec].Beu_Id[beu] > 0 )
				{
					if( (params->Bec_Params[bec].Beu_Slot[beu]<DEF_MIN_BEU_SLOT) || (DEF_MAX_BEU_SLOT<=params->Bec_Params[bec].Beu_Slot[beu]) )
					{
						fprintf( stderr, "%s: usupported slot=%d for beu %d in bec %d, must be in [%d;%d] range\n",
							__FUNCTION__, params->Bec_Params[bec].Beu_Slot[beu],
							params->Bec_Params[bec].Beu_Id[beu], bec, DEF_MIN_BEU_SLOT, DEF_MAX_BEU_SLOT );
						return D_RetCode_Err_Wrong_Param;
					}
					if( (ret=beusspInit(params->BeuSspConf_Col.beu_conf[beu].base_adr_reg, &(beu_reg_control[beu]), &(beu_fifo[beu]), &(params->BeuSspConf_Col.beu_conf[beu]))) < 0 )
					{
						fprintf( stderr, "%s: beusspInit failed for beu %d in bec %d with %d\n",
							__FUNCTION__, beu, bec, ret );
						return D_RetCode_Err_Wrong_Param;
					}
					if( (ret=beusspResetGClkPll( beu_reg_control[beu] ) ) < 0 )
					{
						fprintf( stderr, "%s: beusspResetGClkPll failed for beu %d in bec %d with %d\n",
							__FUNCTION__, beu, bec, ret );
						if( sys_log_fptr != (FILE *)NULL )
							fprintf( sys_log_fptr, "%s: beusspResetGClkPll failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
						return D_RetCode_Err_Wrong_Param;
					}
					usleep( 200000 );
					if( (ret=beusspResetMultiGTX( beu_reg_control[beu] ) ) < 0 )
					{
						fprintf( stderr, "%s: beusspResetMultiGTX failed for beu %d in bec %d with %d\n",
							__FUNCTION__, beu, bec, ret );
						beusspSetTargetFeuAndDumpAllReg(beu_reg_control[beu], 0, stdout);
						if( sys_log_fptr != (FILE *)NULL )
						{
							fprintf( sys_log_fptr, "%s: beusspResetMultiGTX failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							beusspSetTargetFeuAndDumpAllReg(beu_reg_control[beu], 0, sys_log_fptr);
						}
						return D_RetCode_Err_Wrong_Param;
					}
					usleep( 200000 );
					// Display BEU registers
					beusspDisplayAllReg(beu_reg_control[beu]);
				}
			} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; feu++ )

			// If needed Multi module block transfer requested
			if( params->Bec_Params[bec].BaseAdr_A32m_Com_Enb )
			{
				if( (ret=beusspInitMblk(beu_reg_control, &BEUSSPmblk)) < 0 )
				{
					fprintf( stderr, "%s: beusspInitMblk failed for bec %d with %d\n",
						__FUNCTION__, bec, ret );
					return D_RetCode_Err_Wrong_Param;
				}

				/* Configure BEU-s */
				for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
				{
					if( params->Bec_Params[bec].Beu_Id[beu] > 0 )
					{
						if( (ret=beusspEnableA32m(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspEnableA32m failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}
						if( (ret=beusspEnableA32(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspEnableA32 failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}

/*						if( (ret=beusspDisableA32(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspDisableA32 failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}
*/						
						if( (ret=beusspEnBerrTerm(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspEnBerrTerm failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}
						// Display BEU registers
						beusspDisplayAllReg(beu_reg_control[beu]);
					}
				} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; feu++ )
			}
			else
			{
				/* Configure BEU-s */
				for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
				{
					if( params->Bec_Params[bec].Beu_Id[beu] > 0 )
					{
						if( (ret=beusspDisableA32m(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspDisableA32m failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}
						if( (ret=beusspEnableA32(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspEnableA32 failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}
/*	
						if( (ret=beusspEnBerrTerm(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspEnBerrTerm failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}	
*/					
						//The readout list does not use bus error termination when reading BEUs separately, but it can be done !		
						if( (ret=beusspDisBerrTerm(beu_reg_control[beu])) < 0 )
						{
							fprintf( stderr, "%s: beusspDisBerrTerm failed for beu %d in bec %d with %d\n",
								__FUNCTION__, beu, bec, ret );
							return D_RetCode_Err_Wrong_Param;
						}

						// Display BEU registers
						beusspDisplayAllReg(beu_reg_control[beu]);
					}
				} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; feu++ )
			} // if( params->Bec_Params[bec].BaseAdr_A32m_Com_Enb )

			/*
			 * Take care of FEUs
			 */
			/* First reset all FEU-s */
			for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
			{
				beu        = ((feu>>5)&0x07)+1;
				beu_lnk    = ( feu    &0x1F)-1;
				if( (1<=params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
				{
					feu_cur_params = &(params->FeuParams_Col.feu_params[feu]);
					// Reset the FEU
//					if( (ret=FeuReset( feu_cur_params, feu_cur_params->Feu_RunCtrl_Id, beu, beu_lnk )) != D_RetCode_Sucsess )
					if( (ret=FeuReset( feu_cur_params, feu, beu, beu_lnk )) != D_RetCode_Sucsess )
					{
						fprintf( stderr, "%s: FeuReset failed for feu %d beu %d link %d in bec %d with %d\n",
							__FUNCTION__, feu, beu, beu_lnk, bec, ret );
				    if( sys_log_fptr != (FILE *)NULL )
						  fprintf( sys_log_fptr, "%s: FeuReset failed for feu %d beu %d link %d in bec %d with %d\n",
							  __FUNCTION__, feu, beu, beu_lnk, bec, ret );
						return ret;
					}
					usleep(100000);
				}
			} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
			// Wait before the reset is done and links are ready
			usleep( 200000 );
//fprintf(stdout, "%s: Reset all FEUs in bec=%d\n", __FUNCTION__, bec);
//fprintf(stdout, "Enter CR to synchronise...");
//getchar();
			/* Configure FEU-s */
/*			for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
			{
				beu        = ((feu>>5)&0x07)+1;
				beu_lnk    = ( feu    &0x1F)-1;
				if( (1<=params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
				{
					feu_cur_params = &(params->FeuParams_Col.feu_params[feu]);
					// Configure the FEU
					if( (ret=FeuConfig( feu_cur_params, feu, beu, beu_lnk )) != D_RetCode_Sucsess )
					{
						fprintf( stderr, "%s: FeuConfig failed for feu %d beu %d link %d in bec %d with %d\n",
							__FUNCTION__, feu, beu, beu_lnk, bec, ret );
						return ret;
					}
				}
			} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
			usleep( 500000 );
*/
			// Synchronise links
			reset_links      = 0;
#define DEF_NUM_RX_LINK_RST 20
#define DEF_NUM_RX_LINK_TOLL_ERR 2
#define DEF_NUM_RX_LINK_CHK_SEC 4
			reset_links_cntr = DEF_NUM_RX_LINK_RST;
			while( reset_links_cntr )
			{
				fprintf(stdout, "%s: sync pass reset_links_cntr=%d reset_links=%d in bec=%d\n", __FUNCTION__, reset_links_cntr, reset_links, bec);
//fprintf(stdout, "Enter CR to synchronise...");
//getchar();
				/* Synchronize BEU - FEU optical links */
				if( reset_links )
				{
					for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
					{
						if( params->Bec_Params[bec].Beu_Id[beu] > 0 )
						{
							if( (ret=beusspResetGClkPll( beu_reg_control[beu] ) ) < 0 )
							{
								fprintf( stderr, "%s: beusspResetGClkPll failed for beu %d in bec %d with %d\n",
									__FUNCTION__, beu, bec, ret );
								return D_RetCode_Err_Wrong_Param;
							}
							usleep( 100000 );
							if( (ret=beusspResetMultiGTX( beu_reg_control[beu] ) ) < 0 )
							{
								fprintf( stderr, "%s: beusspResetMultiGTX failed for beu %d in bec %d with %d\n",
									__FUNCTION__, beu, bec, ret );
								return D_RetCode_Err_Wrong_Param;
							}
							// Display BEU registers
//							beusspDisplayAllReg(beu_reg_control[beu]);
						}
					} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; feu++ )
					usleep( 300000 );
				}
//fprintf(stdout, "Enter CR to Clear stat...");
//getchar();
				/* Check FEU links */
				/* First clear stat */
				for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
				{
					beu        = ((feu>>5)&0x07)+1;
					beu_lnk    = ( feu    &0x1F)-1;
					if( (1<=params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
					{
						feu_cur_params = &(params->FeuParams_Col.feu_params[feu]);
						// Clear stat
						if( (ret = FeuToggleCommand( feu_cur_params, feu, beu, beu_lnk, ClearStat ) ) != D_RetCode_Sucsess )
						{
								fprintf( stderr,       "%s: FeuToggleCommand ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
										__FUNCTION__, feu, beu, beu_lnk, ret );
							if( sys_log_fptr != (FILE *)NULL )
								fprintf( sys_log_fptr, "%s: FeuToggleCommand ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
										__FUNCTION__, feu, beu, beu_lnk, ret );
							return D_RetCode_Err_NetIO;
						}
					}
				} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
//fprintf(stdout, "Enter CR to Latch stats...");
//getchar();
				fprintf( stdout, "%s: checking during %d secs\n", __FUNCTION__, DEF_NUM_RX_LINK_CHK_SEC);
				for( index=0; index<DEF_NUM_RX_LINK_CHK_SEC; index++ )
				{
					usleep( 1000000 );
					/* Latch stat */
					for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
					{
						beu        = ((feu>>5)&0x07)+1;
						beu_lnk    = ( feu    &0x1F)-1;
						if( (1<=params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
						{
							feu_cur_params = &(params->FeuParams_Col.feu_params[feu]);
							// Latch stat
							if( (ret = FeuToggleCommand( feu_cur_params, feu, beu, beu_lnk, LatchStat ) ) != D_RetCode_Sucsess )
							{
									fprintf( stderr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
										__FUNCTION__, feu, beu, beu_lnk, ret );
								if( sys_log_fptr != (FILE *)NULL )
									fprintf( sys_log_fptr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
										__FUNCTION__, feu, beu, beu_lnk, ret );
								return D_RetCode_Err_NetIO;
							}
						}
					} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
//fprintf(stdout, "Enter CR to get errors...");
//getchar();
          reset_links = 0;
					/* Next get error registers */
					for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
					{
						beu        = ((feu>>5)&0x07)+1;
						beu_lnk    = ( feu    &0x1F)-1;
						if( (1<=params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
						{
							feu_cur_params = &(params->FeuParams_Col.feu_params[feu]);
							// Get error registers
							if( (ret = FeuComChanRxErr_Get( feu_cur_params, feu, beu, beu_lnk, &feu_rx_err_cntr, &feu_par_err_cntr, &feu_pac_err_cntr ) ) != D_RetCode_Sucsess )
							{
									fprintf( stderr,  "%s: FeuComChanRxErr_Get ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
										__FUNCTION__, feu, beu, beu_lnk, ret );
								if( sys_log_fptr != (FILE *)NULL )
									fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
										__FUNCTION__, feu, beu, beu_lnk, ret );
								return D_RetCode_Err_NetIO;
							}
							if( (feu_rx_err_cntr > 0) || (feu_par_err_cntr != 0) )
							{
									fprintf( stderr,  "%s: feu_rx_err_cntr=%d feu_par_err_cntr=%d feu_pac_err_cntr=%d for feu=%d beu=%d lnk=%d in pass %d\n",
										__FUNCTION__, feu_rx_err_cntr, feu_par_err_cntr, feu_pac_err_cntr, feu, beu, beu_lnk, reset_links_cntr );
//fprintf(stdout, "Enter CR to before reset the FEU...");
//getchar();
								if( sys_log_fptr != (FILE *)NULL )
									fprintf( sys_log_fptr,  "%s: feu_rx_err_cntr=%d feu_par_err_cntr=%d feu_pac_err_cntr=%d for feu=%d beu=%d lnk=%d in pass %d\n",
										__FUNCTION__, feu_rx_err_cntr, feu_par_err_cntr, feu_pac_err_cntr, feu, beu, beu_lnk, reset_links_cntr );
								if
								(
									( reset_links_cntr > (DEF_NUM_RX_LINK_RST/2) )
									||
									( feu_rx_err_cntr  > DEF_NUM_RX_LINK_TOLL_ERR )
									||
									( feu_par_err_cntr != 0 )
								)
								{
									reset_links++;
									if( (ret = FeuReset( feu_cur_params, feu, beu, beu_lnk ) ) != D_RetCode_Sucsess )
									{	
										fprintf( stderr,  "%s: FeuReset failed for feu=%d beu=%d lnk=%d with %d\n",
										  __FUNCTION__, feu, beu, beu_lnk, ret );
										if( sys_log_fptr != (FILE *)NULL )
										  fprintf( sys_log_fptr,  "%s: FeuReset failed for feu=%d beu=%d lnk=%d with %d\n",
                      __FUNCTION__, feu, beu, beu_lnk, ret );
                    return D_RetCode_Err_NetIO;
                  }
									usleep( 100000 );
								}
							}
						}
					} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
					if( reset_links )
						break;
				} // for( index=0; index<DEF_NUM_RX_LINK_CHK_SEC; index++ )
				if( reset_links )
				{
					reset_links_cntr--;
						fprintf( stderr,  "%s: failed to synchronise %d feu links; %d attemps left\n", __FUNCTION__, reset_links, reset_links_cntr );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr,  "%s: failed to synchronise %d feu links; %d attemps left\n", __FUNCTION__, reset_links, reset_links_cntr );
				}
				else
					reset_links_cntr = 0;
			} // while( reset_links )
			if( reset_links )
			{
				fprintf( stderr, "%s: failed to synchronise %d feu links\n", __FUNCTION__, reset_links );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: failed to synchronise %d feu links\n", __FUNCTION__, reset_links );
				return D_RetCode_Err_NetIO;
			}

			/* Configure FEU-s */
			for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
			{
				beu        = ((feu>>5)&0x07)+1;
				beu_lnk    = ( feu    &0x1F)-1;
				if( (1<=params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (params->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
				{
					feu_cur_params = &(params->FeuParams_Col.feu_params[feu]);
					// Configure the FEU
					if( (ret=FeuConfig( feu_cur_params, feu, beu, beu_lnk )) != D_RetCode_Sucsess )
					{
							fprintf( stderr, "%s: FeuConfig failed for feu %d beu %d link %d in bec %d with %d\n",
								__FUNCTION__, feu, beu, beu_lnk, bec, ret );
						if( sys_log_fptr != (FILE *)NULL )
							fprintf( sys_log_fptr, "%s: FeuConfig failed for feu %d beu %d link %d in bec %d with %d\n",
								__FUNCTION__, feu, beu, beu_lnk, bec, ret );
						return ret;
					}
				}
			} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
			usleep( 500000 );
		} // if( configs_to_do & 0x2 )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

	return D_RetCode_Sucsess;
}

/*
 * Set system to running state
 */
int SysRun()
{
	int ret;
	int bec;
	int feu;
	int beu;
	int beu_lnk;
//	FeuParams *feu_def_params;
	FeuParams *feu_cur_params;
	
	// Check for Null pointer
	if( sys_params_ptr == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

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
				feu_cur_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
				// Then set specific parameters
				if( (ret=FeuRun( feu_cur_params, feu, beu, beu_lnk, 1 )) != D_RetCode_Sucsess )
				{
					fprintf( stderr, "%s: FeuRun(1) failed for feu %d beu %d link %d in bec %d with %d\n",
						__FUNCTION__, feu, beu, beu_lnk, bec, ret );
					return ret;
				}
			}
		} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )

		/* Configure BEU-s */
		for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
		{
			if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
			{
				if( (sys_params_ptr->Bec_Params[bec].Beu_Slot[beu]<DEF_MIN_BEU_SLOT) || (DEF_MAX_BEU_SLOT<=sys_params_ptr->Bec_Params[bec].Beu_Slot[beu]) )
				{
					fprintf( stderr, "%s: usupported slot=%d for beu %d in bec %d, must be in [%d;%d] range\n",
						__FUNCTION__, sys_params_ptr->Bec_Params[bec].Beu_Slot[beu], sys_params_ptr->Bec_Params[bec].Beu_Id[beu], bec, DEF_MIN_BEU_SLOT, DEF_MAX_BEU_SLOT );
					return D_RetCode_Err_Wrong_Param;
				}
				if( (ret=BeuRun(beu_reg_control[beu], 1)) !=  D_RetCode_Sucsess )
				{
					fprintf( stderr, "%s: BeuRun(1) failed for beu %d in bec %d with %d\n",
						__FUNCTION__, beu, bec, ret );
					return ret;
				}
			}
		} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; feu++ )

		/* Configure TI */
		if( sys_params_ptr->Ti_Params[bec].Id > 0 )
		{
			if( (ret=TiRun( &(sys_params_ptr->Ti_Params[1]), 1)) != D_RetCode_Sucsess )
			{
				fprintf(stderr, "%s: TiRun(1) failed with %d for ti %d\n", __FUNCTION__, ret, sys_params_ptr->Ti_Params[1].Id );
				return ret;
			}
		}
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

	return D_RetCode_Sucsess;
}

/*
 * Bring system to idle state
 */
int SysStop()
{
	int ret;
	int bec;
	int feu;
	int beu;
	int beu_lnk;
//	FeuParams *feu_def_params;
	FeuParams *feu_cur_params;
	
	// Check for Null pointer
	if( sys_params_ptr == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		/* Configure TI */
		if( sys_params_ptr->Ti_Params[bec].Id > 0 )
		{
			if( (ret=TiRun( &(sys_params_ptr->Ti_Params[1]), 0)) != D_RetCode_Sucsess )
			{
				fprintf(stderr, "%s: TiRun(0) failed with %d for ti %d\n", __FUNCTION__, ret, sys_params_ptr->Ti_Params[1].Id );
				return ret;
			}
		}

		/* Configure BEU-s */
		for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
		{
			if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
			{
				if( (sys_params_ptr->Bec_Params[bec].Beu_Slot[beu]<DEF_MIN_BEU_SLOT) || (DEF_MAX_BEU_SLOT<=sys_params_ptr->Bec_Params[bec].Beu_Slot[beu]) )
				{
					fprintf( stderr, "%s: usupported slot=%d for beu %d in bec %d, must be in [%d;%d] range\n",
						__FUNCTION__, sys_params_ptr->Bec_Params[bec].Beu_Slot[beu], sys_params_ptr->Bec_Params[bec].Beu_Id[beu], bec, DEF_MIN_BEU_SLOT, DEF_MAX_BEU_SLOT );
					return D_RetCode_Err_Wrong_Param;
				}
				if( (ret=BeuRun(beu_reg_control[beu], 0)) !=  D_RetCode_Sucsess )
				{
					fprintf( stderr, "%s: BeuRun(0) failed for beu %d in bec %d with %d\n",
						__FUNCTION__, beu, bec, ret );
					return ret;
				}
				// Display BEU registers
				fprintf(stdout, "%s: Beu %d Registers in bec %d slot %d\n", __FUNCTION__, sys_params_ptr->Bec_Params[bec].Beu_Id[beu], bec, sys_params_ptr->Bec_Params[bec].Beu_Slot[beu] );
				beusspDisplayAllReg(beu_reg_control[beu]);
			}
		} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; feu++ )

		/* Configure FEU-s */
		for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
		{
			beu        = ((feu>>5)&0x07)+1;
			beu_lnk    = ( feu    &0x1F)-1;
			if( (1<=sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]) && (sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]<=DEF_MAX_FEU_SN) )
			{
				feu_cur_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
				// Then set specific parameters
				if( (ret=FeuRun( feu_cur_params, feu, beu, beu_lnk, 0 )) != D_RetCode_Sucsess )
				{
					fprintf( stderr, "%s: FeuRun(0) failed for feu %d beu %d link %d in bec %d with %d\n",
						__FUNCTION__, feu, beu, beu_lnk, bec, ret );
					return ret;
				}
			}
		} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

	return D_RetCode_Sucsess;
}

/*
 * Enable trigger processing
 */
int SysGo()
{
	int ret;
	int bec;
	int feu;
	int beu;
	int beu_lnk;
	
	// Check for Null pointer
	if( sys_params_ptr == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Go through back end crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		/* Configure BEU-s */
		for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
		{
			if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
			{
				if( (sys_params_ptr->Bec_Params[bec].Beu_Slot[beu]<DEF_MIN_BEU_SLOT) || (DEF_MAX_BEU_SLOT<=sys_params_ptr->Bec_Params[bec].Beu_Slot[beu]) )
				{
					fprintf( stderr, "%s: usupported slot=%d for beu %d in bec %d, must be in [%d;%d] range\n",
						__FUNCTION__, sys_params_ptr->Bec_Params[bec].Beu_Slot[beu], sys_params_ptr->Bec_Params[bec].Beu_Id[beu], bec, DEF_MIN_BEU_SLOT, DEF_MAX_BEU_SLOT );
					return D_RetCode_Err_Wrong_Param;
				}
				if( (ret=BeuGo(beu_reg_control[beu], &(sys_params_ptr->BeuSspConf_Col.beu_conf[beu]))) !=  D_RetCode_Sucsess )
				{
					fprintf( stderr, "%s: BeuGo failed for beu %d in bec %d with %d\n",
						__FUNCTION__, beu, bec, ret );
					return ret;
				}
			}
		} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; feu++ )

		/* Configure TI */
		if( sys_params_ptr->Ti_Params[bec].Id > 0 )
		{
			if( (ret=TiGo( &(sys_params_ptr->Ti_Params[1]))) != D_RetCode_Sucsess )
			{
				fprintf(stderr, "%s: TiGo failed with %d for ti %d\n", __FUNCTION__, ret, sys_params_ptr->Ti_Params[1].Id );
				return ret;
			}
		}
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

	return D_RetCode_Sucsess;
}

/*
 * Scan selftrigger thresholds
 */
// Timin loop definitions
#define DEF_KBD_READ_DELAY_US 1000  //1ms
#define DEF_REFRESH_TIME_SEC 0.1

void changemode(int dir)
{
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}
 
int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;
 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
 
}


int SysScanSlfTrgThresh( char *sys_conf_params_filename )
{
	int ret;
	int bec;
	int feu;
	int beu;
	int beu_lnk;
	FeuParams *feu_cur_params;

	int do_scan_trg_thr = 1;
	unsigned int  active_slf_trg_feu_lo = 0;
	unsigned int  active_slf_trg_feu_hi = 0;
	unsigned int  cp_active_slf_trg_feu_lo = 0;
	unsigned int  cp_active_slf_trg_feu_hi = 0;
	unsigned int *active_slf_trg_feu;
	int nb_of_active_slf_trg_feu = 0;
	int min_slf_trg_feu = -1;
	int max_slf_trg_feu = -1;
	unsigned int scanned_feu_lo;
	unsigned int scanned_feu_hi;
	unsigned int *scanned_feu;
	int running_feu;
	int slf_trg_chk_cnt;
	int nb_of_scanned_feu;
	int run;

	FILE *conf_fptr;
	char filename[256];

	// time variables
	time_t     cur_time;
	struct tm *time_struct;
	struct timeval t0;
	struct timeval t1;
	struct timeval dt;
	struct timeval refresh_time;
	float  f_refresh;
	char character;

	// Get execution time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	f_refresh = DEF_REFRESH_TIME_SEC;
	refresh_time.tv_sec = (int)f_refresh;
	refresh_time.tv_usec = (int)(1.e6*(f_refresh - refresh_time.tv_sec));
	if(f_refresh<0 || (refresh_time.tv_sec==0 && refresh_time.tv_usec<1)) {
		refresh_time.tv_usec = 1;
	}
	gettimeofday(&t0,0);
	gettimeofday(&t1,0);

	// Check for Null pointer
	if( sys_params_ptr == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// First prevent all Dreams of all FEUs to generate triggers
	if( do_scan_trg_thr == 1 )
	{
		printf("*** Scan trigger thresholds started\n\r");
		active_slf_trg_feu = 0;
		nb_of_active_slf_trg_feu = 0;
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
					feu_cur_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
					if( feu_cur_params->SelfTrig_DreamMask != 0xFF )
					{
						if( FeuTrgScan_Init( &feu_trg_scan[feu], feu_cur_params, feu, beu, beu_lnk ) != D_RetCode_Sucsess )
						{
							fprintf( stderr,  "%s: FeuTrgScan_Init failed for feu=%d beu=%d beu_lnk=%d\n\r", __FUNCTION__, feu, beu, beu_lnk );
							do_scan_trg_thr = 6;
							break;
						}
						if( feu < 32 )
							active_slf_trg_feu_lo |= (1ULL<< feu    );
						else
							active_slf_trg_feu_hi |= (1ULL<<(feu-32));
						nb_of_active_slf_trg_feu++;
						if( min_slf_trg_feu < 0 )
							min_slf_trg_feu = feu;
						max_slf_trg_feu = feu;
					}
				}
			} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
		} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
//		fprintf(stderr, "%s: nb_of_active_slf_trg_feu=%d active_slf_trg_feu lo=0x%08x hi=0x%08x min_slf_trg_feu=%d max_slf_trg_feu=%d\n\r",
//			__FUNCTION__, nb_of_active_slf_trg_feu, active_slf_trg_feu_lo, active_slf_trg_feu_hi, min_slf_trg_feu, max_slf_trg_feu);
//getchar();
		if( (do_scan_trg_thr==1) && (nb_of_active_slf_trg_feu) )
		{
			cp_active_slf_trg_feu_lo = active_slf_trg_feu_lo;
			cp_active_slf_trg_feu_hi = active_slf_trg_feu_hi;
			scanned_feu_lo = 0;
			scanned_feu_hi = 0;
			running_feu = min_slf_trg_feu;
			slf_trg_chk_cnt = (int)((double)DEF_DRM_TRG_CHK_NUM / (double)nb_of_active_slf_trg_feu + 0.5);
			do_scan_trg_thr = 2;
			nb_of_scanned_feu = 0;
			run = 1;
//fprintf(stderr, "slf_trg_chk_cnt=%d\n\r", slf_trg_chk_cnt);
//getchar();
		}
		else
			do_scan_trg_thr = 5;
	}

	if( do_scan_trg_thr == 2 )
	{
		changemode(1);
		// Main loop
		while(run)
		{
			if
			(
				( (running_feu  < 32) && (cp_active_slf_trg_feu_lo & (1ULL<< running_feu    ))  )
				||
				( (running_feu >= 32) && (cp_active_slf_trg_feu_hi & (1ULL<<(running_feu-32)))  )
			)
			{
				beu        = ((running_feu>>5)&0x07)+1;
				beu_lnk    = ( running_feu    &0x1F)-1;
				feu_cur_params = &(sys_params_ptr->FeuParams_Col.feu_params[running_feu]);
				if( feu_trg_scan[running_feu].nb_of_checks < 0 )
				{
					if( FeuTrgScan_SetThr( &feu_trg_scan[running_feu], feu_cur_params, running_feu, beu, beu_lnk ) != D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: FeuTrgScan_SetThr failed for feu=%d beu=%d beu_lnk=%d\n\r", __FUNCTION__, running_feu, beu, beu_lnk  );
						do_scan_trg_thr = 6;
						run = 0;
						changemode(0);
						break;
					}
//printf( "%s: FeuTrgScan_SetThr sucseeded for feu=%d beu=%d beu_lnk=%d\n\r", __FUNCTION__, running_feu, beu, beu_lnk  );
				}
				else if( (ret=FeuTrgScan_ChkCoin( &feu_trg_scan[running_feu], feu_cur_params, running_feu, beu, beu_lnk, slf_trg_chk_cnt )) < 0 )
				{
					fprintf( stderr,  "%s: FeuTrgScan_ChkCoin failed for feu=%d beu=%d beu_lnk=%d\n\r", __FUNCTION__, running_feu, beu, beu_lnk  );
					do_scan_trg_thr = 6;
					run = 0;
					changemode(0);
					break;
				}
				else if( ret > 0 )
				{
					// Dream check finished
					if( (ret = FeuTrgScan_MskDrm( &feu_trg_scan[running_feu], feu_cur_params, running_feu, beu, beu_lnk ) ) < 0 )
					{
						fprintf( stderr,  "%s: FeuTrgScan_MskDrm failed for feu=%d beu=%d beu_lnk=%d\n\r", __FUNCTION__, running_feu, beu, beu_lnk );
						do_scan_trg_thr = 6;
						run = 0;
						changemode(0);

						nb_of_scanned_feu++;
						if( running_feu < 32 )
							scanned_feu_lo |= (1ULL<< running_feu    );
						else
							scanned_feu_hi |= (1ULL<<(running_feu-32));
						
						if( running_feu < 32 )
							cp_active_slf_trg_feu_lo &= (~( (1ULL<< running_feu    ) ));
						else
							cp_active_slf_trg_feu_hi &= (~( (1ULL<<(running_feu-32)) ));

						break;
					}
					else if( ret == 1 )
					{
						// Feu Check Finished
						nb_of_scanned_feu++;
						if( running_feu < 32 )
							scanned_feu_lo |= (1ULL<< running_feu    );
						else
							scanned_feu_hi |= (1ULL<<(running_feu-32));
						
						if( running_feu < 32 )
							cp_active_slf_trg_feu_lo &= (~( (1ULL<< running_feu    ) ));
						else
							cp_active_slf_trg_feu_hi &= (~( (1ULL<<(running_feu-32)) ));
						
//						if( (scanned_feu_lo == active_slf_trg_feu_lo ) && (scanned_feu_hi == active_slf_trg_feu_hi ) )
						if( nb_of_scanned_feu == nb_of_active_slf_trg_feu )
						{
							do_scan_trg_thr = 3;
							run = 0;
							changemode(0);

						}
						else
							slf_trg_chk_cnt = (int)((double)DEF_DRM_TRG_CHK_NUM / (double)((nb_of_active_slf_trg_feu-nb_of_scanned_feu)) + 0.5);
					}
				}
			}

			printf("*** Scan trigger thresholds in process ... running feu=%d\n\r", running_feu);
			for( feu=min_slf_trg_feu; feu<=max_slf_trg_feu; feu++ )
			{
				if
				(
					( (feu <  32) && (active_slf_trg_feu_lo & (1ULL<< feu    )) )
					||
					( (feu >= 32) && (active_slf_trg_feu_hi & (1ULL<<(feu-32))) )
				)
				{
					printf
					(
						"feu=%3d drm=%1d thr=%3d chk=%5d coin=%d \n\r",
						feu,
						feu_trg_scan[feu].running_drm,  feu_trg_scan[feu].running_thr,
						feu_trg_scan[feu].nb_of_checks, feu_trg_scan[feu].prev_cntr
					);
				}
			}

			if( nb_of_scanned_feu != nb_of_active_slf_trg_feu )
			{
				while(1)
				{
					running_feu++;
					if( running_feu>max_slf_trg_feu )
						running_feu  = min_slf_trg_feu;
					if
					(
						( (running_feu <  32) && (cp_active_slf_trg_feu_lo & (1ULL<< running_feu    )) )
						||
						( (running_feu >= 32) && (cp_active_slf_trg_feu_hi & (1ULL<<(running_feu-32))) )
					)
						break;
				}
			}

			// Check for commands
			printf("*** Press Q (shift-q) to quit\n\n\r");
			character = 0;
			do
			{
				// handle keyboard input
				if( kbhit() )
					character=getchar();

				if( character == 0 )
				{
					usleep(DEF_KBD_READ_DELAY_US);
					gettimeofday(&t1, 0);
				}
				timersub(&t1,&t0,&dt);
			} while( timercmp(&dt,&refresh_time,<) && (character==0) );
			t0 = t1;

			// Decode keyboard input
			if( character )
			{
				switch( character )
				{
					case 'Q' :
						// Stop the scan
						if( (ret = FeuTrgScan_MskDrm( &feu_trg_scan[running_feu], feu_cur_params, feu, beu, beu_lnk ) ) < 0 )
						{
							fprintf( stderr,  "%s: FeuTrgScan_MskDrm failed for feu=%d beu=%d beu_lnk=%d\n\r", running_feu, beu, beu_lnk );
						}
						do_scan_trg_thr = 4;
						run = 0;
						changemode(0);
					break;
					default : ;
				}
			}
		}
	}

	if( (do_scan_trg_thr == 3) || (do_scan_trg_thr == 4) || (do_scan_trg_thr == 6) )
	{
		if( do_scan_trg_thr == 4 ) 
			printf( "%s: stopped by user\n", __FUNCTION__ );
		else if( do_scan_trg_thr == 6 ) 
			printf( "%s: stopped as something went wrong with hardware\n", __FUNCTION__ );
		else
			printf( "%s: *** Scan trigger thresholds done\n\r", __FUNCTION__ );
		if( nb_of_active_slf_trg_feu )
		{
			for( feu=min_slf_trg_feu; feu<=max_slf_trg_feu; feu++ )
			{
				if
				(
					( (feu <  32) && (scanned_feu_lo & (1ULL<< feu    )) )
					||
					( (feu >= 32) && (scanned_feu_hi & (1ULL<<(feu-32))) )
				)
				{
					feu_cur_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
					FeuTrgScan_Fprintf( &feu_trg_scan[feu], feu_cur_params, feu, stdout );
				}
			}
			// Prepare config copy filename
			// Prepare filename for self trig thersholds
			if( do_scan_trg_thr == 4 )
				sprintf
				(
					filename,
					"%s_%02d%02d%02d_%02dH%02d_stopped.slf",
					rootfilename(sys_conf_params_filename),
					time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
					time_struct->tm_hour, time_struct->tm_min
				);
			else if( do_scan_trg_thr == 6 )
				sprintf
				(
					filename,
					"%s_%02d%02d%02d_%02dH%02d_error.slf",
					rootfilename(sys_conf_params_filename),
					time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
					time_struct->tm_hour, time_struct->tm_min
				);
			else
				sprintf
				(
					filename,
					"%s_%02d%02d%02d_%02dH%02d.slf",
					rootfilename(sys_conf_params_filename),
					time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
					time_struct->tm_hour, time_struct->tm_min
				);
			// Open config file to write
			if( (conf_fptr=fopen(filename, "w")) == NULL )
			{
				fprintf( stderr, "%s: fopen failed for config file %s in write mode\n", __FUNCTION__, filename );
				fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
				return D_RetCode_Err_FileIO;
			}
			printf( "%s: config file %s opend in write mode\n", __FUNCTION__, filename );
			// Open config file to write
			for( feu=min_slf_trg_feu; feu<=max_slf_trg_feu; feu++ )
			{
				if
				(
					( (feu <  32) && (scanned_feu_lo & (1ULL<< feu    )) )
					||
					( (feu >= 32) && (scanned_feu_hi & (1ULL<<(feu-32))) )
				)
				{
					feu_cur_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
					FeuTrgScan_DumpConfigInfo( &feu_trg_scan[feu], feu_cur_params, feu, conf_fptr );
				}
			}
			// Close config file
			fclose( conf_fptr );
			conf_fptr = (FILE *)NULL;
			printf( "%s: config file %s closed\n", __FUNCTION__, filename );
		} // if( nb_of_active_slf_trg_feu )
	}

	if( do_scan_trg_thr == 5 )
	{
		printf( "%s: stopped as no active self trigger FEUs present\n", __FUNCTION__ );
	}

	return D_RetCode_Sucsess;
}

int SysScanFeuMonit( char *sys_conf_params_filename, int store_period_sec )
{
	int ret;
	int bec;
	int feu;
	int beu;
	int beu_lnk;
	FeuParams *feu_params;
	FeuMonit   feu_monit;
	char       feu_monit_str[256];
	int run;
	int iter;

	char filename[256];

	// time variables
	time_t     cur_time;
	struct tm *time_struct;
	struct timeval t0;
	struct timeval t1;
	struct timeval dt;
	struct timeval refresh_time;
	struct timeval tstart;
	struct timeval tsince;
	struct timeval store_last_time;
	struct timeval store_period_time;
	float  f_refresh;
	char   character;
	int    store;

# define DEF_MONIT_REFRESH_TIME_SEC 1
	// Get execution time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	f_refresh = DEF_MONIT_REFRESH_TIME_SEC;
	refresh_time.tv_sec = (int)f_refresh;
	refresh_time.tv_usec = (int)(1.e6*(f_refresh - refresh_time.tv_sec));
	if(f_refresh<0 || (refresh_time.tv_sec==0 && refresh_time.tv_usec<1)) {
		refresh_time.tv_usec = 1;
	}
	gettimeofday(&t0,0);
	gettimeofday(&t1,0);

	// Check for Null pointer
	if( sys_params_ptr == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Open monit files to write
	if( store_period_sec > 0 )
	{
		/* Go through all FEU-s and open monitoring files for configured FEUs */
		for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
		{
			feu_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
			if( feu_params->Feu_RunCtrl_Id > 0 )
			{
				sprintf
				(
					filename,
					"%s_%02d%02d%02d_%02dH%02d_f%02d.mnt",
					rootfilename(sys_conf_params_filename),
					time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
					time_struct->tm_hour, time_struct->tm_min, feu
				);
				if( (monit_fptr[feu]=fopen(filename, "w")) == NULL )
				{
					fprintf( stderr, "%s: fopen failed for monit file %s in write mode\n", __FUNCTION__, filename );
					fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
					return D_RetCode_Err_FileIO;
				}
			}
			else
				monit_fptr[feu] = (FILE *)NULL;
		} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
		monit_fptr_init = 1;
		store_period_time.tv_sec  = store_period_sec;
		store_period_time.tv_usec = 0;
		store_last_time = t0;
	}

	changemode(1);
	iter = 0;
	gettimeofday(&tstart,0);
	// Main loop
	while(run)
	{
		iter++;
		store = 0;
		if( monit_fptr_init == 1 )
		{
			timersub(&t1,&store_last_time,&tsince);
			if( (timercmp(&tsince,&store_period_time,>)) || (iter==1) )
			{
				store_last_time = t1;
				store = 1;
			}
		}
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
					feu_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
					gettimeofday(&t1, 0);
					timersub(&t1,&tstart,&tsince);
					if( (ret=FeuMonit_Get( &feu_monit, feu_params, feu, beu, beu_lnk )) != D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: FeuMonit failed for feu=%d beu=%d beu_lnk=%d with %d\n\r", __FUNCTION__, feu, beu, beu_lnk, ret  );
						run = 0;
						changemode(0);
						return ret;
					}
					FeuMonit_Sprintf( &feu_monit, feu_monit_str );
					fprintf
					(
						stdout,
						"%6d %6d sec Feu %2d SN %3d (beu=%1d lnk=%2d): %s\n",
						iter, tsince.tv_sec,
						feu, sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk], beu, beu_lnk,
						feu_monit_str
					);
					if( store )
					{
						fprintf
						(
							monit_fptr[feu],
							"%6d %6d sec Feu %2d SN %3d (beu=%1d lnk=%2d): %s\n",
							iter, tsince.tv_sec,
							feu, sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk], beu, beu_lnk,
							feu_monit_str
						);
						fflush( monit_fptr[feu] );
					}
				} // if( (1<=sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]
			} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
		} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

		// Check for commands
		printf("*** Press Q (shift-q) to quit\n\n\r");
		character = 0;
		do
		{
			// handle keyboard input
			if( kbhit() )
				character=getchar();

			if( character == 0 )
			{
				usleep(DEF_KBD_READ_DELAY_US);
				gettimeofday(&t1, 0);
			}
			timersub(&t1,&t0,&dt);
		} while( timercmp(&dt,&refresh_time,<) && (character==0) );
		t0 = t1;

		// Decode keyboard input
		if( character )
		{
			switch( character )
			{
				case 'Q' :
					// Stop the scan
					run = 0;
					changemode(0);
				break;
				default : ;
			}
		}
	} // while( run )

	return D_RetCode_Sucsess;
}



int SysDumpFeuOptLnk()
{
	int ret;
	int bec;
	int feu;
	int beu;
	int beu_lnk;
	FeuParams *feu_params;
	FeuMonit   feu_monit;
	char       feu_monit_str[256];
	int run;
	int iter;

	int feu_rx_err_cntr;
	int feu_par_err_cntr;
	int feu_pac_err_cntr;

	// Check for Null pointer
	if( sys_params_ptr == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}


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
				feu_params = &(sys_params_ptr->FeuParams_Col.feu_params[feu]);
				// Latch stat
				if( (ret = FeuToggleCommand( feu_params, feu, beu, beu_lnk, LatchStat ) ) != D_RetCode_Sucsess )
				{
						fprintf( stderr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
							__FUNCTION__, feu, beu, beu_lnk, ret );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
							__FUNCTION__, feu, beu, beu_lnk, ret );
					return D_RetCode_Err_NetIO;
				}
				// Get error registers
				if( (ret = FeuComChanRxErr_Get( feu_params, feu, beu, beu_lnk, &feu_rx_err_cntr, &feu_par_err_cntr, &feu_pac_err_cntr ) ) != D_RetCode_Sucsess )
				{
						fprintf( stderr,  "%s: FeuComChanRxErr_Get ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
							__FUNCTION__, feu, beu, beu_lnk, ret );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
							__FUNCTION__, feu, beu, beu_lnk, ret );
					return D_RetCode_Err_NetIO;
				}
				if( (feu_rx_err_cntr > 0) || (feu_par_err_cntr != 0) || (feu_pac_err_cntr != 0) )
				{
						fprintf
						(
							stdout,
							"%s: Feu=%2d Sn=%3d (beu=%d lnk=%2d) rx_err_cntr=%d par_err_cntr=%d pac_err_cntr=%d\n",
							__FUNCTION__,
							feu, sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk], beu, beu_lnk,
							feu_rx_err_cntr, feu_par_err_cntr, feu_pac_err_cntr
						);
					if( sys_log_fptr != (FILE *)NULL )
						fprintf
						(
							sys_log_fptr,
							"%s: Feu=%2d Sn=%3d (beu=%d lnk=%2d) rx_err_cntr=%d par_err_cntr=%d pac_err_cntr=%d\n",
							__FUNCTION__,
							feu, sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk], beu, beu_lnk,
							feu_rx_err_cntr, feu_par_err_cntr, feu_pac_err_cntr
						);
				}
			} // if( (1<=sys_params_ptr->Bec_Params[bec].BeuFeuConnectivity[beu][beu_lnk]
		} // for( feu=1; feu<DEF_MAX_NB_OF_FEU; feu++ )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )


	return D_RetCode_Sucsess;
}
