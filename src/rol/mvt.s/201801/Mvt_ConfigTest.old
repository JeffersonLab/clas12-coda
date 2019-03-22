/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    SysConfigTest.c
-- Description:    Test for System configuration
--
-- Target Devices: Windows or Linux PC
-- Tool versions:  Windows Visual C++ or Linux Make
-- 
-- Create Date:    0.0 2014/10/14 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifdef Linux_vme

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "Platform.h"
#include "ReturnCodes.h"
#include "SysConfig.h"

extern char *optarg;
extern int   optind;

// Default values
#define DEF_ConfFileName        "Sys.cfg"

// Global variables
int verbose = 0; // if >0 some debug output

/*
 * Usage function
 */
void usage( char *name )
{
	printf( "\nUsage: %s", name );
	printf( " [-c Conf_FileName]" );
	printf( " [-s]" );
	printf( " [-m]" );
	printf( " [-v [-v]]" );
	printf( " [-h]" );
	printf( "\n" );
	
	printf( "\n" );
	printf( "-c Conf_FileName        - name for config file; default: %s; \"None\" no file consulted\n", DEF_ConfFileName );
	printf( "-s                      - Scan system for self trigger thresholds\n" );
	printf( "-m file_upd_per_sec     - Scan system for monitoring information and store in file every file_upd_per_sec; 0 - no file\n" );
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
	int  scan_trg_thr;
	int  scan_monit;

	int ret;

	struct timeval t0;
	struct timeval t1;
	struct timeval dt;

	// Initialization
	verbose      =  0;
	scan_trg_thr =  0;
	scan_monit   = -1;
	sprintf(conf_file_name,            DEF_ConfFileName );
	sprintf(progname,            "%s", basename(argv[0]));

	/******************************/
	/* Check for input parameters */
	/******************************/
	sprintf( optformat, "c:m:svh" );
	while( ( opt = getopt( argc, argv, optformat ) ) != -1 )
	{
		switch( opt )
		{
		break;
			case 'c':
				sprintf( conf_file_name, "%s", optarg );
			break;

			case 's':
				scan_trg_thr = 1;
			break;

			case 'm':
				scan_monit = atoi(optarg);
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
		printf( "scan_trg_thr            = %d\n",    scan_trg_thr );
		printf( "scan_monit              = %d\n",    scan_monit );
		printf( "verbose                 = %d\n",    verbose );
	}

	/***********************/
	/* Set signal hendler  */
	/***********************/
	signal( SIGABRT, sig_hndlr);
	signal( SIGFPE,  sig_hndlr);
	signal( SIGILL,  sig_hndlr);
	signal( SIGINT,  sig_hndlr);
	signal( SIGSEGV, sig_hndlr);
	signal( SIGTERM, sig_hndlr);
	if( verbose )
		printf( "%s: signal handler set\n", progname );

	// Get start time for performance measurements
	gettimeofday(&t0,0);

	/*
	 * Configure system
	 */
	if( (ret=SysConfigFromFile( conf_file_name )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysConfigFromFile failed for file %s with %d\n", progname, conf_file_name, ret );
		cleanup(ret);
	}
	if( verbose )
		printf( "%s: SysConfig OK\n", progname );

	// Get end time for performance measurements
	gettimeofday(&t1, 0);
	timersub(&t1,&t0,&dt);
	printf("%s: The system has been configured in %d sec and %d usec\n", progname, dt.tv_sec, dt.tv_usec );

	/*
	 * The following does not belong to configuration
	 * This is an attempt to derive self trigger thresholds
	 */
	if( scan_trg_thr )
	{
		printf("%s: The system should be in Idle state; Press CR to go to Trigger Threshold Scan <-", progname );
		getchar();
		gettimeofday(&t0,0);
		if( (ret=SysScanSlfTrgThresh( conf_file_name )) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SysScanSlfTrgThresh failed for file %s with %d\n", progname, conf_file_name, ret );
			cleanup(ret);
		}
		gettimeofday(&t1, 0);
		timersub(&t1,&t0,&dt);
		printf("%s: The thresholds have been scanned in %d sec and %d usec\n", progname, dt.tv_sec, dt.tv_usec );
	}

	/*
	 * The following does not belong to configuration
	 * This is an attempt to get monitoring information
	 */
	if( scan_monit >= 0 )
	{
		printf("%s: The system should be in Idle state; Press CR to go to Monotoring Scan <-", progname );
		getchar();
		if( (ret=SysScanFeuMonit( conf_file_name, scan_monit )) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SysScanFeuMonit failed for file %s with %d\n", progname, conf_file_name, ret );
			cleanup(ret);
		}
		printf("%s: Monitoring scan finished\n", progname );
	}

	printf("%s: The system should be in Idle state; Press CR to go to Running state <-", progname );
	getchar();

	/*
	 * The following does not belong to configuration
	 * This is a test of bringing the system in running mode for data taking
	 * This has to be added to the CODA rocPrestart()
	 */

	// Set system in Running state
	if( (ret=SysRun()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysRun failed for file %s with %d\n", progname, conf_file_name, ret );
		cleanup(ret);
	}
	if( verbose )
		printf( "%s: SysRun OK\n", progname );

	/*
	 * This is a test of enabling trigger processing
	 * This has to be added to the CODA rocGo()
	 */
	printf("%s: The system should be in Running state; Press CR to enable trigger processing <-", progname );
	getchar();
	// Enable trigger processing
	if( (ret=SysGo()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysGo failed for file %s with %d\n", progname, conf_file_name, ret );
		cleanup(ret);
	}
	if( verbose )
		printf( "%s: SysGo OK\n", progname );

	/*
	 * This is a test of diabling trigger processing
	 * This has to be added to the CODA rocEnd()
	 */
	printf("%s: The system should be in Running state; Press CR to go to IDLE state <-", progname );
	getchar();
	// Set system in Idle state
	if( (ret=SysStop()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysStop failed for file %s with %d\n", progname, conf_file_name, ret );
		cleanup(ret);
	}
	if( verbose )
		printf( "%s: SysStop OK\n", progname );

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
