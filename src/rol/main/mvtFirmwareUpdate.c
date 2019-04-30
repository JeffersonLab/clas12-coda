/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
--
-- Il y a deux options pour l'update du firmware;
-- option maximale : on tente de configurer tout le systeme et on voit ce qui se passe
-- option minimale : on se concentre sur les beussp uniquement
-- la premiere option s'appuie sur simpletest
-- la deuxeime option a besoin des VME_BASE_ADDRESS des cartes et doit faire le VMeopenDefault etc, c'est plus long à coder.
--
-- Je retiens l'OPTION MAXIMALE
--
--
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
#include <libgen.h>
#include <unistd.h>

#ifdef Linux_vme

//#include "Platform.h"


//#include "jvme.h"
//#include "TiConfigParams.h"
//#include "sdLib.h"
//#include "BeuConfig.h"
//#include "tiLib.h"
//#include "SysConfigParams.h"

#include "SysConfig.h"
#include "ReturnCodes.h"
#include "beusspLib.h"
#include "BeuConfig.h"						//includes some global variables

// For the moment an ugly declaration
extern SysParams *sys_params_ptr;					// DEFINED IN SysConfig.c, should be defined in SysConfig.h ??





extern char *optarg;
extern int   optind;

// Default values
#define DEF_ConfFileName        "Sys.cfg"
#define DEF_BITFileName        "Default.bit"

// Global variables
int verbose = 0; // if >0 some debug output

/*
 * Usage function
 */
void usage( char *name )
{
	printf( "\nUsage: %s", name );
	printf( " [-c Conf_FileName]" );
	printf( " [-b BIT_Filename]" );
	printf( " [-v [-v]]" );
	printf( " [-h]" );
	printf( "\n" );
	
	printf( "\n" );
	printf( "-c Conf_FileName        - name for config file; default: %s \n", DEF_ConfFileName );
	printf( "-b BIT_Filename         - name for firmware.bit  file;  default: %s\n", DEF_BITFileName );
	printf( "-v [-v]                 - forces debug output\n" );
	printf( "-h                      - help\n" );

	printf( "  Get this text:\n"); 
	printf( "   %s -h\n\n", name);
}

/*
 * Cleanup function
 */
void cleanup(int param)
{
	if( param )
	{
		fprintf(stderr, "cleanup: Entering with %d\n", param);
	}
	if( verbose )
		printf( "cleanup: Entering with %d\n", param );

	// Close mamory configuration file if any open
	SysConfig_CleanUp();

	if( verbose )
		printf( "cleanup: Exiting\n" );
	exit( param );
}

/*
 * Signal hendler
 */
void sig_hndlr( int param )
{
	cleanup(param);
}

/*
 * Main
 */
int main( int argc, char* *argv )
{
	// Internal variables
	int  opt;
	char optformat[128];
	char progname[128];

	// Parameters
	char conf_file_name[128];
	char bit_file_name[128];

	int ret;
 	int bec;
  	int beu;
	
struct timeval t0;
	struct timeval t1;
	struct timeval dt;

	// Initialization
	verbose =  1;
	sprintf(conf_file_name, DEF_ConfFileName );
	sprintf(bit_file_name, DEF_BITFileName );
	sprintf(progname, "%s", basename(argv[0]));

	/******************************/
	/* Check for input parameters */
	/******************************/
	sprintf( optformat, "c:b:vh" );
	while( ( opt = getopt( argc, argv, optformat ) ) != -1 )
	{
		switch( opt )
		{
		break;
			case 'c':
				sprintf( conf_file_name, "%s", optarg );
			break;
			
			case 'b':
				sprintf( bit_file_name, "%s", optarg );
			break;
			
			case 'v':
				verbose++;
			break;

			case 'h':
			default:
				usage( progname );
				return 0;
		}
	}
	if( verbose )
	{
		printf( "conf_file_name          = %s\n",    conf_file_name );
		printf( "bit_file_name           = %s\n",    bit_file_name );
		printf( "verbose                 = %d\n",    verbose );
	}

	/***********************/
	/* Set signal handler  */
	/***********************/
	signal( SIGABRT, sig_hndlr);
	signal( SIGFPE,  sig_hndlr);
	signal( SIGILL,  sig_hndlr);
	signal( SIGINT,  sig_hndlr);
	signal( SIGSEGV, sig_hndlr);
	signal( SIGTERM, sig_hndlr);
	//if( verbose )
	//	printf( "%s: signal handler set\n", progname );

	// Get start time for performance measurements
	//gettimeofday(&t0,0);

	/*
	 * Configure system
	 */
	if( (ret=SysConfigFromFile( conf_file_name )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfigFromFile failed for file %s with %d\n", progname, conf_file_name, ret );
		//fprintf( checking that beussps are present in the setup, that they are adressable and that the firmware is loaded, checking firmware version on each board, updating firmware in flash
			// on all adressable SSPs)
		//cleanup(ret);
	}
	

  for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
    {
      if( sys_params_ptr->Bec_Params[bec].Crate_Id > 0 )
	{ 
	  for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	    {
	      if( sys_params_ptr->Bec_Params[bec].Beu_Id[beu] > 0 )
			{
				//beusspSetTargetFeuAndDumpAllReg(beu_reg_control[beu], numFeu, fptr);
				//check firmware version
				// fi board not initialized, break => then error : print power cycle needed to load firmware from flash or JTAG configuration only
				
				
				//beusspFlashREAD(beu_reg_control[beu], 0);
				//beusspFlashREAD(beu_reg_control[beu],1);

                                //if( (ret=beusspFWCompare(beu_reg_control[beu],  bit_file_name )) != D_RetCode_Sucsess )
				printf("----------------------------------------------------------\n\r");			
				printf("Updating firmware in flash on BEU id=%d \n\r",sys_params_ptr->Bec_Params[bec].Beu_Id[beu] );
				printf("----------------------------------------------------------\n\r");
				if( (ret=beusspFWU(beu_reg_control[beu],  bit_file_name )) != D_RetCode_Sucsess )
					{
						fprintf( stderr, "%s: beusspFWU failed for file %s with %d\n", progname, bit_file_name, ret );
						//fprintf( checking that beussps are present in the setup, that they are adressable and that the firmware is loaded, checking firmware version on each board, updating firmware in flash
							// on all adressable SSPs)
						//cleanup(ret);
					}
				//verifying
                                printf("Verifying firmware in flash on BEU id=%d \n\r",sys_params_ptr->Bec_Params[bec].Beu_Id[beu] );			
 				if( (ret=beusspFWCompare(beu_reg_control[beu],  bit_file_name )) != D_RetCode_Sucsess )
                                        {
                                                fprintf( stderr, "%s: beusspFWCompare failed for file %s with %d\n", progname, bit_file_name, ret );
                                                //fprintf( checking that beussps are present in the setup, that they are adressable and that the firmware is loaded, checking firmware version on each board, updating firmware in flash
                                                        // on all adressable SSPs)
                                                //cleanup(ret);
                                        }
				printf("----------------------------------------------------------\n\r");
                                printf("Firmware updated successfully on BEU id=%d \n\r", sys_params_ptr->Bec_Params[bec].Beu_Id[beu] );
                                printf("----------------------------------------------------------\n\r");

				//if success, power cycle needed to load new firmware from flash
				//if failed , print power cycle needed to load firmware from flash, jtag configuratio required
				
				// is it possible to check bit file format ???
				// get flash ID from bit file ?
				// get the value of a firmware register from the file ? 
				
			}
	    } 					
	}
    } 
	
	
	
	
	
	
	if( verbose )
		printf( "%s: SysConfig OK\n", progname );

	// Get end time for performance measurements
	//gettimeofday(&t1, 0);
	//timersub(&t1,&t0,&dt);
	//printf("%s: The system has been configured in %d sec and %d usec\n", progname, dt.tv_sec, dt.tv_usec );

	
	/********************/
	/* Cleanup and stop */
	/********************/
	
	
	
	

	/********************/
	/* Cleanup and stop */
	/********************/
	cleanup(0);
	return 0;
}

#else

int
main()
{
  exit(0);
}

#endif







