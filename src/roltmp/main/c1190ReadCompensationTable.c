/*
 * File: c1190ReadCompensationTable.c
 *
 *
 * Description:
 *    Test reading from CAEN1290 FLASH and SRAM Compensation Table
 *
 *
 */

#if defined(Linux_vme)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/signal.h>

#include "jvme.h"

/*#include "c1190Lib.h"*/
#include "tdc1190.h"

#define COMP_PAGE_SIZE (4*256)

const int NUM_V1190 = 1;
char *programName;
extern int Nc1190;

void sig_handler(int signo);
static void Usage();

/*
   * Slot  3:   0x11180000 <-1290
   * Slot  4:   0x11200000
   * Slot  5:   0x11280000
   * Slot  6:   0x11300000 <-1290, 1190
   * Slot  7:   0x11380000
   * Slot  8:   0x11400000
   * Slot  9:   0x11480000 <-1290
   * Slot 10:   0x11500000
   * Slot 11:   0x11580000
   * Slot 12:   0x11600000
   * Slot 13:   0x11680000 <-1290
   * Slot 14:   0x11700000
   * Slot 15:   0x11780000
   * Slot 16:   0x11800000 <-      1190
   * Slot 17:   0x11880000
   * Slot 18:   0x11900000
   * Slot 19:   0x11980000
   * Slot 20:   0x11A00000

c1190ReadCompensationTable 0x11180000 tdcftof2_slot3.txt
c1190ReadCompensationTable 0x11300000 tdcftof2_slot6.txt
c1190ReadCompensationTable 0x11480000 tdcftof2_slot9.txt
c1190ReadCompensationTable 0x11680000 tdcftof2_slot13.txt

   */

int
main(int argc, char *argv[])
{
  int status, i, tdctype, nchan;
  int ipage = 0;//, npage = 4;
  int ichar = 0;//, nchar = 256;
  unsigned char sram[256];
  int inputchar=10;
  char *filename;
  FILE *filep;
  unsigned int vme_addr;
  
  memset((void *) &sram, 0, sizeof(sram));
  
  printf("\nCAEN 1190/1290 INL Compensation Table Reader\n");
  printf("-----------------------------------------------------------------\n");

    /* Handle commandline arguments */
  programName = argv[0];

  if(argc<3)
    {
      printf(" ERROR: Must specify two arguments\n");
      Usage();
      exit(0);
    }
  else
    {
      vme_addr = (unsigned int) strtoll(argv[1],NULL,16)&0xffffffff;
      filename = argv[2];
    }

  vmeSetQuietFlag(1);
  /* Open the VME interface */
  status = vmeOpenDefaultWindows();
  if(status != OK)
    goto CLOSE;

  /* Check that the shared mutex is in good health */
  vmeCheckMutexHealth(1);

  /* Initalize the signal handler */
  signal(SIGINT, sig_handler);

  struct stat buffer;
  if(stat(filename, &buffer) == 0)
    {
      printf("WARNING: File (%s) exists!\n Overwrite?\n",
	     filename);

    REPEAT2:
      printf(" (y/n): ");
      inputchar = getchar();
      
      if((inputchar == 'n') ||
	 (inputchar == 'N'))
	{
	  printf("--- Exiting without reading ---\n");
	  goto CLOSE;
	}
      else if((inputchar == 'y') ||
	      (inputchar == 'Y'))
	{
	}
      else
	goto REPEAT2;
      
    }

  filep = fopen(filename,"w");
  if(filep==NULL)
    {
      perror("fopen");
      printf("%s: ERROR: Unable to open file for writing %s\n",
	     __FUNCTION__, filename);

      goto CLOSE;
    }

  vmeBusLock();
  tdc1190Init(vme_addr, 0, 1, 1);
  if (Nc1190 == 0)
    {
      vmeBusUnlock();
      goto CLOSE;
    }

  tdctype = tdc1190Type(0);
  printf("tdc type=%d\n",tdctype);

  /* the number of 16-channel groups in board */
  if(tdctype==1)
  {
    nchan = 128;
  }
  else
  {
    nchan = 32;
  }

  if(argc==4) /* read delays */
  {
    int adjust[4];
    for(i=0; i<4; i++)
	{
      adjust[i] = tdc1190GetRCadjust(0, i);
      printf("adjust[%d]=%d\n",i,adjust[i]);
	}
  }
  else /* read compensation table */
  {
    tdc1190ReadCompensation(0, 1);

    int ichan;
    for(ichan = 0; ichan < nchan; ichan++)
    {
      fprintf(filep,"# Channel %d\n", ichan);

      if(tdctype==1) /* 128-ch */
      {
	    tdc1190ReadCompensationSramPage(0, (unsigned char *)&sram, ichan);
	    for(ichar = 0; ichar < 256; ichar++)
	    {
	      if(((ichar % 8) == 0) && (ichar != 0)) fprintf(filep, "\n");
	      fprintf(filep, "0x%02x  ", sram[ichar]);
	    }
	    fprintf(filep, "\n");
      }
      else /* 32-ch and 16-ch */
      {
        for(ipage = 0; ipage < 4; ipage++)
	    {
	      tdc1190ReadCompensationSramPage(0, (unsigned char *)&sram, ichan*4 + ipage);
	      for(ichar = 0; ichar < 256; ichar++)
	      {
	        if(((ichar % 8) == 0) && (ichar != 0)) fprintf(filep, "\n");
	        fprintf(filep, "0x%02x  ", sram[ichar]);
	      }
	      fprintf(filep, "\n");
	    }
      }

    }
  }

  vmeBusUnlock();

  fclose(filep);
  
CLOSE:

  vmeCloseDefaultWindows();

  exit(0);
}

void
closeup()
{
  vmeBusUnlock();
  vmeCloseDefaultWindows();

  exit(0);
}


void
sig_handler(int signo)
{

  switch (signo)
    {
    case SIGINT:
      printf("\n\n");
      closeup();
      exit(1);			/* exit if CRTL/C is issued */
    }

  return;
}

static void
Usage()
{
  printf("\n");
  printf("%s <VME Address (A24)> <comp_table file>\n",programName);
  printf("\n");

}

#else

int
main()
{
}

#endif
