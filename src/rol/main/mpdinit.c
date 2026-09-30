
/* mpdinit.c */

#if defined(VXWORKS) || defined(Linux_vme)


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "libconfig.h"

#ifdef VXWORKS
/*sergey#include "vxCompat.h"*/
#else
#include "jvme.h"
#endif

#include "tiLib.h"
#include "tsLib.h"
#include "mpdLib.h"
#include "mpdConfig.h"

void Usage();
char bin_name[50];


int fnMPD = 0;
extern int mpdOutputBufferBaseAddr;	/* output buffer base address */
#define MPD_DMA_BUFSIZE 80000













/*sergey: just to resolve reference(s)*/
/*extern*/ int sspID[MPD_SSP_MAX_BOARDS + 1];
/*extern*/ int nSSP;
/*extern*/ //uint32_t sspMpdReadReg(int id, int impd, unsigned int reg);
/*extern*/ //int sspMpdWriteReg(int id, int impd, unsigned int reg, unsigned int value);


int
resetMPDs(unsigned int *broken_list, int nbroken)
{
  int impd = 0,  id = 0, rval = OK;
  static int ncalls = 0;

  printf("%s: Number of calls = %d\n",
	 __func__, ncalls);

  for (impd = 0; impd < nbroken; impd++)
  {
    id = broken_list[impd];
    mpdDAQ_Disable(id);
  }

  for (impd = 0; impd < nbroken; impd++)
  {				// only active mpd set
    id = broken_list[impd];

    // mpd latest configuration before trigger is enabled
    mpdSetAcqMode(id, "process");

    // load pedestal and thr default values
    mpdPEDTHR_Write(id);

    // enable acq
    mpdDAQ_Enable(id);

    if (mpdAPV_Reset101(id) != OK)
    {
      printf("MPD Slot %2d: Reset101 FAILED\n", id);
      rval = ERROR;
    }
  }

  /* Check MPDs for data */
  int sd_init, sd_overrun, sd_rdaddr, sd_wraddr, sd_nwords;
  int obuf_nblock = 0, empty = 0, full = 0, nwords = 0;
  for (impd = 0; impd < nbroken; impd++)
  {				// only active mpd set
    id = broken_list[impd];
    mpdSDRAM_GetParam(id, &sd_init, &sd_overrun, &sd_rdaddr, &sd_wraddr, &sd_nwords);

    if ((sd_nwords != 0) || (sd_overrun == 1) || (sd_init == 0))
    {
      printf("ERROR: Slot %2d SDRAM status: \n"
	     "init=%d, overrun=%d, rdaddr=0x%x, wraddr=0x%x, nwords=%d\n",
	     id, sd_init, sd_overrun, sd_rdaddr, sd_wraddr, sd_nwords);
      rval = ERROR;
    }

    obuf_nblock = mpdOBUF_GetBlockCount(id);
    mpdOBUF_GetFlags(id, &empty, &full, &nwords);

    if ((obuf_nblock != 0) || (empty == 0) || (full == 1) || (nwords != 0))
    {
      printf("ERROR: Slot %2d OBUF status: \n"
	     "nblock = %d  empty=%d  full=%d  nwords=%d\n",
	     id, obuf_nblock, empty, full, nwords);
      rval = ERROR;
    }
  }

  return rval;
}

















int I2C_SendStop(int id);


int
main(int argc, char *argv[])
{
  int iflag, id, res=0, rval=0;
  char firmware_filename[50];
  int current_fw_version=0;
  int inputchar=10;
  int error_status = 0;
  char dirname[256];
  char conffilename[256];
  char *clonparms = getenv("CLON_PARMS");

  printf("\n------------------------------------------------\n\n");

  vmeSetQuietFlag(1);
  res = vmeOpenDefaultWindows();
  if(res!=OK)
  {
    printf("vmeOpenDefaultWindows problem - exit\n");
    exit(0);
  }


#if 0
  
  printf("tiInit called ...%d\n");fflush(stdout);

  res = tiInit(21<<19,0,1);
  //res = tiInit(0,2,1);
  printf("tiInit returns %d\n",res);
  if(res!=OK)
  {
    /* try tsInit, instead */
    printf("trying TS instead of TI ..\n");
    res = tsInit(21<<19,0,1);
    printf("tsInit returns %d\n",res);
    if(res!=OK)
    {
      printf("TI problem - exit\n");
      exit(0);
    }
  }
  
#endif

  
  printf("\n======================================\n\n");
  
  sprintf(dirname, "%s/mpd", clonparms);
  mpdSetConfigDirectory(dirname);

  sprintf(conffilename, "%s/%s",dirname,"gem_config_apv.txt");
  rval = mpdConfigInit(conffilename); 

  if(rval != OK)
  {
    printf("\n===> ERROR in configuration file %s\n\n",conffilename);
    error_status = ERROR;
  }

  mpdConfigLoad();

  /* Init and config MPD+APV */

  iflag = 0;
  iflag |= MPD_INIT_NO_CONFIG_FILE_CHECK;
  iflag |= MPD_INIT_SKIP_FIRMWARE_CHECK;

  // discover MPDs and initialize memory mapping

  mpdInit((2<<19), 0x80000, 21, iflag);
  //mpdInit((2<<19), 0x80000, MPD_SSP_MAX_BOARDS, iflag);

  fnMPD = mpdGetNumberMPD();

#if 0
  
  if (fnMPD > 0)
  {
    printf("MPD discovered = %d\n", fnMPD);

    printf("\n");

    // APV configuration on all active MPDs
    int impd, iapv;

    for (impd = 0; impd < fnMPD; impd++)
    {				// only active mpd set
      id = mpdSlot(impd);
      printf("MPD slot %2d config:\n", id);


      rval = mpdHISTO_MemTest(id);

      printf(" - Initialize I2C\n");
      fflush(stdout);
      if (mpdI2C_Init(id) != OK)
	{
	  printf(" * * FAILED\n");
	  error_status = ERROR;
	}

      printf(" - APV discovery and init\n");

      fflush(stdout);
      mpdSetPrintDebug(0x0);
      if (mpdAPV_Scan(id) <= 0)
	{			// no apd found, skip next
	  printf(" * * None Found\n");
	  error_status = ERROR;
	  continue;
	}
      mpdSetPrintDebug(0);

      // apv reset
      printf(" - APV Reset\n");
      fflush(stdout);
      if (mpdI2C_ApvReset(id) != OK)
	{
	  printf(" * * FAILED\n");
	  error_status = ERROR;
	}

      usleep(10);
      I2C_SendStop(id);
      // board configuration (APV-ADC clocks phase)
      // (do this while APVs are resetting)
      printf(" - DELAY setting\n");
      fflush(stdout);
      if (mpdDELAY25_Set
	  (id, mpdGetAdcClockPhase(id, 0), mpdGetAdcClockPhase(id, 1)) != OK)
	{
	  printf(" * * FAILED\n");
	  error_status = ERROR;
	}

      // apv configuration
      mpdSetPrintDebug(0);
      printf(" - Configure Individual APVs\n");
      printf(" - - ");
      fflush(stdout);
      int itry, badTry = 0, saveError = error_status;
      error_status = OK;
      for (itry = 0; itry < 3; itry++)
	{
	  if(badTry)
	    {
	      printf(" ******** RETRY ********\n");
	      printf(" - - ");
	      fflush(stdout);
	      error_status = OK;
	    }
	  badTry = 0;
	  for (iapv = 0; iapv < mpdGetNumberAPV(id); iapv++)
	    {
	      printf("%2d ", iapv);
	      fflush(stdout);

	      if (mpdAPV_Config(id, iapv) != OK)
		{
		  printf(" * * FAILED for APV %2d\n", iapv);
		  if(iapv < (mpdGetNumberAPV(id) - 1))
		    printf(" - - ");
		  fflush(stdout);
		  error_status = ERROR;
		  badTry = 1;
		}
	    }
	  printf("\n");
	  fflush(stdout);
	  if(badTry)
	    {
	      printf(" ***** APV RESET *****\n");
	      fflush(stdout);
	      mpdI2C_ApvReset(id);
	    }
	  else
	    {
	      if(itry > 0)
		{
		  printf(" ****** SUCCESS!!!! ******\n");
		  fflush(stdout);
		}
	      break;
	    }

	}

      error_status |= saveError;
      mpdSetPrintDebug(0);

      // configure adc on MPD
      printf(" - Configure ADC\n");
      fflush(stdout);
      if (mpdADS5281_Config(id) != OK)
	{
	  printf(" * * FAILED\n");
	  error_status = ERROR;
	}

      // configure fir
      // not implemented yet

      // RESET101 on the APV
      printf(" - Do APV RESET101\n");
      fflush(stdout);
      if (mpdAPV_Reset101(id) != OK)
	{
	  printf(" * * FAILED\n");
	  error_status = ERROR;
	}

      // <- MPD+APV initialization ends here
      printf("\n");
      fflush(stdout);
    }				// end loop on mpds
    //END of MPD configure


    mpdGStatus(1);

    // summary report
    printf("\n");
    printf("Configured APVs (ADC 15 ... 0)\n");
    int ibit;
    for (impd = 0; impd < fnMPD; impd++)
    {
      id = mpdSlot(impd);

      if (mpdGetApvEnableMask(id) != 0)
      {
        printf("  MPD %2d : ", id);
	iapv = 0;
	for (ibit = 15; ibit >= 0; ibit--)
	{
	  if (((ibit + 1) % 4) == 0) printf(" ");
	  if (mpdGetApvEnableMask(id) & (1 << ibit))
	  {
	    printf("1");
	    iapv++;
	  }
	  else
	  {
		  printf(".");
	  }
	}
	printf(" (#APV %d)\n", iapv);
      }
    }
    printf("\n");

    if (error_status != OK)
    {
      printf("\nERROR: MPD initialization has errors\n\n");
      fnMPD = 0;
    }

  }
  else
  {				// test all possible vme slot ?
    printf("ERR: no MPD discovered, cannot continue\n");
    //return;
    fnMPD = 0;
  }

#endif




  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
