/*************************************************************************
 *
 *  vtp_mpdro_list.c - Library of routines for readout and buffering of
 *                events using a VTP as a readout path for the MPD
 *
 *     To be used with the vtp HW ROC readout list
 *
 */

#ifdef Linux_armv7l

#define VTP

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "codautil.h"

#include "libconfig.h"

#include "vtp.h"
#include "mpdLib.h"
#include "mpdConfig.h"
#include "vtpMpdConfig.h"

// These are now defined in the config file
//   e.g. ~/vtp/cfg/sbsvtp3.config


//#define ROCID rol->pid //sergey




void
vtpSetPedSubtractionMode(int enable) // routine prototype
{
  printf("%s: NO!  This routine is not supported! Use the config file!\n",
	 __func__);
}
/* vtp defs */

/*MPD Definitions*/
extern int I2C_SendStop(int id);

int fnMPD=0;

extern volatile struct mpd_struct *MPDp[(MPD_MAX_BOARDS+1)]; /* pointers to MPD memory map */

// End of MPD definition

/*
  Disable troubled APV if an error occurs during configuration
*/

/* Filenames obtained from the platform or other config files */
char APV_REJECT_FILENAME[250];

/* default config filenames, if they are not defined in COOL */
#define APV_REJECT_FILENAME_PROTO "/tttmp/%d_ROC%d.err"

int32_t apvRejectMode = 1; /* 1: Reject APV that error during config,
			      0: Keep them in (will usually cause trigger lock-up)
			   */

int32_t
vtpSetApvRejectMode(int32_t enable)
{
  apvRejectMode = (enable) ? 1 : 0;

  return apvRejectMode;
}

int32_t
vtpGetApvRejectMode(int32_t pflag)
{
  if(pflag)
    {
      printf("%s: apvRejectMode = %d\n",
	     __func__, apvRejectMode);
    }
  return apvRejectMode;
}

char *apvbuffer = NULL;
char *bufp;


int
vtp_mpd_setup(char *conffile)
{
  /*****************
   *   VTP SETUP
   *****************/
  int ret = 0;
  int iFlag = 0;
  uint64_t vtpFiberBit = 0;
  uint64_t vtpFiberMaskToInit;

  printf("vtp_mpd_setup() reached\n");fflush(stdout);

  //sergey
  char myhostname[100];
  get_hostname(myhostname,99);
  char *mysql_database = getenv("EXPID");
  int ROCID;
  get_roc_id(mysql_database, myhostname, &ROCID);
  printf("myhostname=%s -> ROCID=%d\n",myhostname,ROCID);

  char *clonparms = getenv("CLON_PARMS");
  char dirname[256], conffilename[256];
  // for nested config files, we can set top directory
  sprintf(dirname, "%s/mpd", clonparms);
  vtpMpdSetConfigDirectory(dirname);

  // for the main config file, we must specify full path
  if(conffile==NULL)
  {
    sprintf(conffilename, "%s/gem_config_apv_%s.txt",dirname,myhostname);
  }
  else if( (conffile[0]==' ') || (strlen(conffile)==0) )
  {
    sprintf(conffilename, "%s/gem_config_apv_%s.txt",dirname,myhostname);
  }
  else
  {
    sprintf(conffilename, "%s/%s",dirname,conffile);
  }
  printf("vtp_mpd_setup: using MPD config file >%s<\n",conffilename);

  //sergey

  if(vtpMpdConfigInit(conffilename /*sergey: was rol->usrConfig*/) == ERROR)
  {
    printf("ERROR: APV Configuration load error for %s (CHECK THIS FIRST)\n",/*rol->usrConfig*/conffilename);
    return(-1);
  }

  vtpMpdConfigLoad();

  vtpMpdFiberReset();
  vtpMpdFiberLinkReset(0xffffffffffffffffULL);

  /* ... the VTP holds all the MPD event build stuff in reset... */
  vtpMpdDisable(0xffffffffffffffffULL);

  /* setups up the MPD (so they clear their buffers) ... */

  /*****************
   *   MPD SETUP
   *****************/
  int rval = OK;
  uint64_t errSlotMask = 0;
  /* Index is the mpd / fiber... value mask if ADCs with APV config errors */
  uint32_t apvConfigErrorMask[VTP_MPD_MAX]; /* ADC channel that failed configuration */
  uint32_t apvErrorTypeMask[VTP_MPD_MAX]; /* Error Type bits: 0 = mpd init, 1 = apv not found , 2 = config */
  uint32_t configAdcMask[VTP_MPD_MAX]; /* ADC channels that are configured in cfg file */
  memset(apvConfigErrorMask, 0 , sizeof(apvConfigErrorMask));
  memset(apvErrorTypeMask, 0 , sizeof(apvErrorTypeMask));
  memset(configAdcMask, 0 , sizeof(configAdcMask));

  mpdSetPrintDebug(0);

  // discover MPDs and initialize memory mapping

  // In VTP mode, par1(fiber mask) and par3(number of mpds) are not used in mpdInit(par1, par2, par3, par4)
  // Instead, they come from the configuration file
  uint64_t chanmask = vtpMpdGetChanUpMask();

  chanmask = mpdGetVTPFiberMask();
  mpdInitVTP(chanmask, MPD_INIT_FIBER_MODE | MPD_INIT_NO_CONFIG_FILE_CHECK);
  fnMPD = mpdGetNumberMPD();

  if (fnMPD<=0)
  {
    printf("WARN: no MPD enabled in config file\n");
  }

  printf(" MPD discovered = %d\n",fnMPD);

  // APV configuration on all active MPDs
  int error_status = OK;
  int k, i;
  for (k=0;k<fnMPD;k++) // only active mpd set
  {
    i = mpdSlot(k);

    int try_cnt = 0;

  retry:
    error_status = OK;

    printf(" Try initialize I2C mpd in slot %d\n",i);
    if (mpdI2C_Init(i) != OK)
    {
      printf("WRN: I2C fails on MPD %d\n",i);
    }

    printf("Try APV discovery and init on MPD slot %d\n",i);
    if (mpdAPV_Scan(i)<=0 && try_cnt < 3 ) // no apd found, skip next
    {
      try_cnt++;
      printf("failing retrying\n");
      goto retry;
    }



    if( try_cnt == 3 )
    {
      printf("APV blind scan failed for %d TIMES !!!!\n\n", try_cnt);
      errSlotMask |= (1ull << (uint64_t)i);
      continue;
    }

    printf(" - APV Reset\n");
    fflush(stdout);
    if (mpdI2C_ApvReset(i) != OK)
    {
      printf(" * * FAILED\n");
      error_status = ERROR;
      errSlotMask |= (1ull << (uint64_t)i);
    }

    usleep(10);
    I2C_SendStop(i);


    // board configuration (APV-ADC clocks phase)
    printf("Do DELAY setting on MPD slot %d\n",i);
    mpdDELAY25_Set(i, mpdGetAdcClockPhase(i,0), mpdGetAdcClockPhase(i,1));



    // apv configuration
    printf("Configure %d APVs on MPD slot %d\n",mpdGetNumberAPV(i),i);


    // apv configuration
    mpdSetPrintDebug(0);
    printf(" - Configure Individual APVs\n");
    printf(" - - ");
    fflush(stdout);
    int itry, badTry = 0, iapv, saveError = error_status;
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
      for (iapv = 0; iapv < mpdGetNumberAPV(i); iapv++)
      {
	printf("%2d ", iapv);
	fflush(stdout);

	if (mpdAPV_Config(i, iapv) != OK)
	{
	  printf(" * * FAILED for APV %2d\n", iapv);
	  if(iapv < (mpdGetNumberAPV(i) - 1)) printf(" - - ");
	  fflush(stdout);
	  error_status = ERROR;
	  apvConfigErrorMask[i] |= (1 << mpdApvGetAdc(i,iapv));
	  apvErrorTypeMask[i] |= (1 << 2);
	  badTry = 1;
	}
	else
	{
	  apvConfigErrorMask[i] &= ~(1 << mpdApvGetAdc(i,iapv));
	}
      }
      printf("\n");
      fflush(stdout);
      if(badTry)
      {
	printf(" ***** APV RESET *****\n");
	fflush(stdout);
	mpdI2C_ApvReset(i);
      }
      else
      {
	if(itry > 0)
	{
	  printf(" ****** SUCCESS!!!! ******\n");
	  error_status = OK;
	  apvConfigErrorMask[i] = 0;
	  apvErrorTypeMask[i] &= ~(1 << 2);
	  fflush(stdout);
	}
	break;
      }

    }

    error_status |= saveError;

    if(error_status == ERROR)
      errSlotMask |= (1ull << (uint64_t)i);

    // configure adc on MPD
    printf("Configure ADC on MPD slot %d\n",i);
    mpdADS5281_Config(i);

    // configure fir
    // not implemented yet

    // 101 reset on the APV
    printf("Do 101 Reset on MPD slot %d  ...\n",i);fflush(stdout);
    mpdAPV_Reset101(i);
    printf("... done\n");fflush(stdout);

    // <- MPD+APV initialization ends here
    //sleep(1);

  } // end loop on mpds
  //END of MPD configure





  // summary report
  bufp = (char *) &(apvbuffer[0]);

  rval = sprintf(bufp, "\n");
  if(rval > 0) bufp += rval;
  rval = sprintf(bufp, "Configured APVs (ADC 15 ... 0) --------------- ERRORS ----\n");
  if(rval > 0) bufp += rval;

  int ibit;
  int ifiber, iapv;
  uint64_t id;
  for (ifiber = 0; ifiber < VTP_MPD_MAX; ifiber++)
  {
    id = ifiber;

    if( ((1ull << id) & mpdGetVTPFiberMask()) == 0) continue;

    /* Build the ADCmask of those in the config file */
    for (iapv = 0; iapv < mpdGetNumberAPV(id); iapv++)
    {
      if(mpdApvGetAdc(id,iapv) > -1)
      {
	configAdcMask[id] |= (1 << mpdApvGetAdc(id,iapv));
	apvErrorTypeMask[ifiber] = 0;
      }
    }

    if(mpdGetFpgaRevision(ifiber) == 0) apvErrorTypeMask[ifiber] = (1 << 0);

    /* if (mpdGetApvEnableMask(id) != 0) */
    {
      rval = sprintf(bufp, "  MPD %2d : ", (int)id);
      if(rval > 0) bufp += rval;
      iapv = 0;
      for (ibit = 15; ibit >= 0; ibit--)
      {
	if (((ibit + 1) % 4) == 0)
	{
	  rval = sprintf(bufp, " ");
	  if(rval > 0) bufp += rval;
	}
	if (mpdGetApvEnableMask(id) & (1 << ibit))
	{
	  if(apvConfigErrorMask[id] & (1 << ibit))
	  {
	    rval = sprintf(bufp, "C");
	  }
	  else
	  {
	    rval = sprintf(bufp, "1");
	  }
	  if(rval > 0) bufp += rval;
	  iapv++;
	}
	else if(configAdcMask[id] & (1 << ibit))
	{
	  apvErrorTypeMask[id] |= (1 << 1);
	  rval = sprintf(bufp, "E");
	  errSlotMask |= (1ull << id);
	  if(rval > 0) bufp += rval;
	}
	else
	{
	  rval = sprintf(bufp, ".");
	  if(rval > 0) bufp += rval;
	}
      }
      rval = sprintf(bufp, " (#APV %2d)", iapv);
      if(rval > 0) bufp += rval;
      if(errSlotMask & (1ull << id))
      {
	ret = -1;
	rval = sprintf(bufp, " %s  %s  %s\n",
			     (apvErrorTypeMask[id] & 0x1) ? "*MPD NotFound*" :
			     "              ",
			     (apvErrorTypeMask[id] & 0x2) ? "*APV NotFound*" :
			     "              ",
			     (apvErrorTypeMask[id] & 0x4) ? "*APV Config*" :
			     "");
	if(rval > 0) bufp += rval;
      }
      else
      {
        rval = sprintf(bufp, "\n");
	if(rval > 0) bufp += rval;
      }
    }
  }
  rval = sprintf(bufp, "\n");
  if(rval > 0) bufp += rval;



  //DALMAGO;
  printf("111111111111\n");fflush(stdout);
  printf("%s",apvbuffer);
  printf("222222222222: errSlotMask=0x%lx\n",errSlotMask);fflush(stdout);
  //DALMASTOP;

  if(ret<0)
  {
    printf("!!!!!!!!!!!!!!!!!! ret=%d\n",ret);fflush(stdout);
  }
  
  if ((errSlotMask != 0) || (error_status != OK))
  {
    printf("ERROR: MPD initialization errors");fflush(stdout);
  }

  if(apvRejectMode == 1)
  {
    /* For each MPD, disable those APV that returned error during configuration */

    printf("Reject Mode enabled\n");fflush(stdout);
    FILE *rejectFile = NULL;

    printf("\n\nFor each MPD, disable those APV that returned error during configuration\n");fflush(stdout);
    for(id = 0; id < VTP_MPD_MAX; id++)
    {
      printf("Checking id=%lld, apvErrorTypeMask[%lld] = 0x%08x\n",id,id,apvErrorTypeMask[id]);fflush(stdout);

      /* Skip MPD / Fiber without errors */
      if(apvErrorTypeMask[id] == 0) continue;

#if 0
      /* Have not opened the reject File yet */
      if(rejectFile == NULL)
      {
	/* Construct the filename from the runnumber and roc number */
	sprintf(APV_REJECT_FILENAME,
		APV_REJECT_FILENAME_PROTO,
		rol->runNumber, ROCID);
	rejectFile = fopen(APV_REJECT_FILENAME,"w+");
	if(rejectFile == NULL)
	{
	  //perror("fopen");
	  printf("ERROR in fopen: file name >%s< ---> will output to STANDARD OUT\n",APV_REJECT_FILENAME);fflush(stdout);
	  rejectFile = stdout;
	}
	else
	{
	  printf("Output rejectFile: %s\n", APV_REJECT_FILENAME);
	}

	fprintf(rejectFile, "Runnumber %d\n", rol->runNumber);
	fprintf(rejectFile, "ROCID %2d\n", ROCID);
	fprintf(rejectFile, "DISABLED APVs (ADC 15 ... 0)\n");
      }
#else
      rejectFile = stdout;
#endif

      /*Get the current APV Enabled mask */
      uint32_t current_mask = ((uint32_t) mpdGetApvEnableMask(id)) & 0xFFFF;

      /* Remove the broken bits */
      uint32_t updated_mask = current_mask & ~apvConfigErrorMask[id];

      fprintf(rejectFile, "  MPD %2d : ", (int)id);

      int ibit, enabledBits = 0, disabledBits = 0, missingBits = 0;
      for (ibit = 15; ibit >= 0; ibit--)
      {
	if (((ibit + 1) % 4) == 0)
	{
	  fprintf(rejectFile, " ");
	}
	if (mpdGetApvEnableMask(id) & (1 << ibit))
	{
	  enabledBits++;
	  if(apvConfigErrorMask[id] & (1 << ibit))
	  {
	    fprintf(rejectFile, "X");
	    disabledBits++;
	  }
	  else
	  {
	    fprintf(rejectFile, "1");
	  }
	  iapv++;
	}
	else if(configAdcMask[id] & (1 << ibit))
	{
	  fprintf(rejectFile, "E");
	  missingBits++;
	}
	else
	{
	  fprintf(rejectFile, ".");
	}
      }
      fprintf(rejectFile, " (Total: %2d   Disabled: %2d   NotFound: %2d)\n",
	      enabledBits+missingBits+disabledBits, disabledBits, missingBits);

      printf("ERROR: MPD %lld: %d APVs Disabled\n",id, disabledBits+missingBits);

      printf("Clearing the apv enabled mask\n");fflush(stdout);
      mpdResetApvEnableMask(id); /* Clear the apv enabled mask */

      /* Write the updated mask */
      printf("Writing the updated mask\n");fflush(stdout);
      mpdSetApvEnableMask(id, updated_mask);

      /* Remove the fiber, if MPD error or All APVs rejected */
      if((updated_mask == 0) || (apvErrorTypeMask[id] & (1 << 0)))
      {
        printf("Removing fiber %lld\n",id);fflush(stdout);
	chanmask &= ~(1ull << id);
      }

      printf("Finished processing id=%lld\n",id);fflush(stdout);
    }



#if 0
    /* Have not opened the reject File yet */
    if(rejectFile != NULL)
    {
      fclose(rejectFile);
    }
#endif

    printf("Finished Reject Mode process\n");fflush(stdout);
  }


  /* ...then VTP can enable the MPD event building,  */
  printf("Getting VTP fiber mask\n");fflush(stdout);
  vtpFiberMaskToInit = mpdGetVTPFiberMask();
  printf("VTP fiber mask = 0x%08x\n",vtpFiberMaskToInit);fflush(stdout);
  if(chanmask != vtpFiberMaskToInit)
  {
    printf("VTP Fiber Mask (Corrected): 0x%16llx\n", chanmask);fflush(stdout);
    vtpFiberMaskToInit = chanmask;
  }
  else
  {
    printf("VTP fiber mask: 0x%16llx\n", vtpFiberMaskToInit);fflush(stdout);
  }






//sergey: problem here
  while(vtpFiberMaskToInit != 0)
  {
    if((vtpFiberMaskToInit & 0x1) == 1)
    {
      vtpMpdEnable( 1ULL << vtpFiberBit );
    }
    vtpFiberMaskToInit = vtpFiberMaskToInit >> 1ULL;
    ++vtpFiberBit;
  }
//sergey: problem here



  /* and clear out it's buffers */
  vtpRocMigReset(1);
  vtpRocMigReset(0);

  vtpV7SetResetSoft(1);
  vtpV7SetResetSoft(0);





#if 0 //sergey: it is done in Download ...

  /* Read Config file and Intialize VTP */
  extern int vtpReadConfigFile(char *filename);
  extern int vtpDownloadAll();

  vtpInitGlobals();

  //vtpMpdConfigWrite(APV_TEMP_CONFIG); //sergey: ???

  if(vtpReadConfigFile(rol->usrString) == ERROR)
  {
    printf("%s: Error using rol->usrString %s.\n",
	     __func__, rol->usrString);

    printf("  trying %s\n", DEFAULT_VTP_CONFIG);

    if(vtpConfig(DEFAULT_VTP_CONFIG) == ERROR)
    {
      printf("ERROR: Error loading VTP configuration file");
      return(-2);
    }
    else
    {
      strncpy(VTP_CONFIG_FILENAME, DEFAULT_VTP_CONFIG, 250);
    }
  }
  else
  {
    strncpy(VTP_CONFIG_FILENAME, rol->usrString, 250);
  }
  vtpDownloadAll();

#endif







  vtpMpdGetCommonModeFilename(conffilename);
  sprintf(COMMON_MODE_FILENAME,"%s/%s",dirname,conffilename);

  vtpMpdGetPedestalFilename(conffilename);
  sprintf(PEDESTAL_FILENAME,"%s/%s",dirname,conffilename);

  float pedestal_factor = 1;
  vtpMpdGetPedestalFactor(&pedestal_factor);

  //char* mpdSlot[10],apvId[10],cModeMin[10],cModeMax[10];
  int apvId, cModeMin, cModeMax, cModeRocId, cModeSlot;
  int fiberID = -1, last_mpdSlot = -1;
  FILE *fcommon   = fopen(COMMON_MODE_FILENAME,"r");

  if(fcommon == NULL)
  {
    //perror("fopen");
    printf("ERROR in fopen: Failed to open GEM CommonModeRange file >%s\n<",COMMON_MODE_FILENAME);fflush(stdout);
    exit(0);
  }
  else
  {
    printf("Opened GEM CommonModeRange file >%s<\n",COMMON_MODE_FILENAME);fflush(stdout);
  }

  //valid pedestal file => will load APV offset file and subtract from APV samples
  FILE *fpedestal = fopen(PEDESTAL_FILENAME,"r");//all GEMs

  if(fpedestal == NULL)
  {
    //perror("fopen");
    printf("ERROR in fopen: Failed to open GEM Pedestal file >%s<\n",PEDESTAL_FILENAME);fflush(stdout);
  }
  else
  {
    printf("Opened GEM Pedestal file >%s<\n",PEDESTAL_FILENAME);fflush(stdout);
  }

  // Load pedestal & threshold file settings
  int stripNo;
  float ped_offset, ped_rms;
  char buf[10];
  int i1, i2, i3, i4, n;
  char *line_ptr = NULL;
  size_t line_len;
  fiberID = -1, last_mpdSlot = -1;

  // We will force a pedestal mode run, if any of these conditions are met
  // - enable_cm = 0
  // - PEDESTAL_FILENAME undefined or missing
  int build_all_samples, build_debug_headers,
    enable_cm, noprocessing_prescale,
    allow_peak_any_time, min_avg_samples;

  vtpMpdEbGetFlags(&build_all_samples, &build_debug_headers,
		   &enable_cm, &noprocessing_prescale,
		   &allow_peak_any_time, &min_avg_samples);

  printf("GOTFLAGS: %d %d %d %d %d %d\n",build_all_samples,build_debug_headers,enable_cm,noprocessing_prescale,allow_peak_any_time,min_avg_samples);

  if((fpedestal==NULL) || (enable_cm == 0))
  {
    printf("INFO: Pedestal Run Engaged");

    if (fpedestal==NULL)
	printf("INFO: Undefined or Missing Pedestal File\n");

    if (enable_cm == 0)
	printf("INFO: enable_cm = 0\n");

    /* Reset the enable_cm, and build_all_samples
       assuming missing pedestal file means its a pedestal run */
    enable_cm = 0;
    build_all_samples = 1;

    vtpMpdEbSetFlags(build_all_samples, build_debug_headers,
		       enable_cm, noprocessing_prescale,
		       allow_peak_any_time, min_avg_samples);

    for(fiberID=0; fiberID<VTP_MPD_MAX; fiberID++)
    {
      for(apvId=0; apvId<15; apvId++)
      {
	for(stripNo=0; stripNo<128; stripNo++)
	{
	  vtpMpdSetApvOffset(fiberID, apvId, stripNo, 0);
	  vtpMpdSetApvThreshold(fiberID, apvId, stripNo, 0);
	}
      }
    }
  }
  else
  {
    printf("trying to read pedestal, file %s\n",PEDESTAL_FILENAME);

    while(!feof(fpedestal))
    {
      getline(&line_ptr, &line_len, fpedestal);

      n = sscanf(line_ptr, "%10s %d %d %d %d", buf, &i1, &i2, &i3, &i4);
      if( (n == 5) && !strcmp("APV", buf))
      {
	cModeRocId = i1;
	fiberID = i3;
	apvId = i4;
	continue;
      }

      n = sscanf(line_ptr, "%d %f %f", &stripNo, &ped_offset, &ped_rms);

      // if settings are for us
      if(cModeRocId != ROCID)
      {
	/* printf("Skipping pedestal settings not for us (us=%d, file=%d)\n", ROCID, cModeRocId); */
	continue;
      }

      if(n == 3)
      {
	// if settings are for us
   	if(cModeRocId == ROCID)
	{
	  //printf(" fiberID: %2d, apvId %2d, stripNo: %3d, ped_offset: %4.0f ped_rms: %4.0f \n", fiberID, apvId, stripNo, ped_offset, ped_rms);
	  vtpMpdSetApvOffset(fiberID, apvId, stripNo, (int)ped_offset);
	  vtpMpdSetApvThreshold(fiberID, apvId, stripNo, (int)(pedestal_factor*ped_rms));
	}
        else
        {
          printf("Skipping pedestal settings not for us (us=%d, file=%d)\n", ROCID, cModeRocId);
          continue;
        }
      }
      //else
      //  printf("VTP PED: skipping line: %s\n", line_ptr);
    }
    fclose(fpedestal);
  }

  // Load common-mode file settings
  if(fcommon==NULL)
  {
    printf("no commonMode file\n");
  }
  else
  {
    printf("trying to read commonMode, file %s\n",COMMON_MODE_FILENAME);
    while(!feof(fcommon))
    {
      getline(&line_ptr, &line_len, fcommon);
      if(sscanf(line_ptr, "%d %d %d %d %d %d", &cModeRocId, &cModeSlot, &fiberID, &apvId, &cModeMin, &cModeMax)==6)
      {
        /* printf("fiberID %d %d %d %d %d %d \n", cModeRocId, cModeSlot, fiberID, apvId, cModeMin, cModeMax); */

  	//	  cModeMin = 200;
	//	  cModeMax = 800;
	//	  cModeMin = 0;
	//	  cModeMax = 4095;

	// if settings are for us
	if(cModeRocId != ROCID)
	{
	  /* printf("Skipping common-mode settings not for us (us=%d, file=%d)\n", ROCID, cModeRocId); */
	  continue;
        }

	vtpMpdSetAvg(fiberID, apvId, cModeMin, cModeMax);
      }
//    else
//      printf("VTP CM: skipping line: %s\n", line_ptr);
    }
    fclose(fcommon);
  }

  return(ret);
}

void
vtpMpdApvConfigStatus()
{
  printf("%s",apvbuffer);
}

/****************************************
 *  DOWNLOAD
 ****************************************/
void
vtpMpdDownload()
{
  apvbuffer = (char *)malloc(1024*50*sizeof(char));

  printf("rocDownload: User Download Executed\n");
}

/****************************************
 *  PRESTART
 ****************************************/
int
vtpMpdPrestart(char *conffile)
{
  int ret = 0;
  int UseSdram, FastReadout;

  // Setup in Prestart since TI clock glitches at end of Download()
  ret = vtp_mpd_setup(conffile);
  if(ret<0)
  {
    printf("vtpMpdPrestart: ERROR: vtp_mpd_setup returned ret=%d\n",ret);
    return(ret);
  }
  else
  {
    printf("vtpMpdPrestart: INFO: vtp_mpd_setup returned ret=%d\n",ret);
  }
    
  /*Enable MPD*/
  UseSdram = mpdGetUseSdram(mpdSlot(0)); // assume sdram and fastreadout are the same for all MPDs
  FastReadout = mpdGetFastReadout(mpdSlot(0));
  printf(" UseSDRAM= %d , FastReadout= %d\n",UseSdram, FastReadout);
  int k, i;
  for (k=0;k<fnMPD;k++) // only active mpd set
  {
    i = mpdSlot(k);

    /* mpd latest configuration before trigger is enabled */
    mpdSetAcqMode(i, "process");

    /* load pedestal and thr default values */
    mpdPEDTHR_Write(i);

    /* enable acq */
    mpdDAQ_Enable(i);

    mpdTRIG_Enable(i);
  }

  mpdTRIG_GSetFrontInput(0); // SyncReset input to FP

  return(ret);
}

/****************************************
 *  PAUSE
 ****************************************/
void
vtpMpdPause()
{
}

/****************************************
 *  GO
 ****************************************/
void
vtpMpdGo()
{
  mpdTRIG_GSetFrontInput(1); // Trigger input to FP

  //DALMAGO;
  printf("MPD PEDESTAL FILENAME: %s\n", PEDESTAL_FILENAME);
  printf("COMMON_MODE_FILENAME: %s\n", COMMON_MODE_FILENAME);
  //DALMASTOP;
}

/****************************************
 *  END
 ****************************************/
void
vtpMpdEnd()
{
  int k;
  for (k=0;k<fnMPD;k++) { // only active mpd set
    mpdTRIG_Disable(mpdSlot(k));
  }

}

void
vtpMpdReset()
{
  int k;
  for (k=0;k<fnMPD;k++) { // only active mpd set
    mpdTRIG_Disable(mpdSlot(k));
  }
};



void
vtpMpdCleanup()
{
  if(apvbuffer)
  {
    free(apvbuffer);
    apvbuffer = NULL;
  }
}

/*
  Local Variables:
  compile-command: "make -k vtp_roc_mpdro.so"
  End:
 */




#else

int
vtp_mpdro_dummy()
{
}

#endif
