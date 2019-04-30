/*
 * File: 1190WriteCompensationTable.c
 *
 *
 * Description:
 *    Test writing to the CAEN1290 FLASH from a file
 *
 *
 */

#if defined(Linux_vme)

#define DO_VME

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/signal.h>
#include <errno.h>

#include "jvme.h"

/*nclude "c1190Lib.h"*/
#include "tdc1190.h"

#define TABLE_SIZE (4*256*32)

const int NUM_V1190 = 1;
char *programName;
extern int Nc1190;

static void sig_handler(int signo);
static void Usage();

/*
   * Slot  3:   0x11180000 <-1290
   * Slot  4:   0x11200000
   * Slot  5:   0x11280000
   * Slot  6:   0x11300000 <-1290
   * Slot  7:   0x11380000
   * Slot  8:   0x11400000
   * Slot  9:   0x11480000 <-1290
   * Slot 10:   0x11500000
   * Slot 11:   0x11580000
   * Slot 12:   0x11600000
   * Slot 13:   0x11680000 <-1290
   * Slot 14:   0x11700000
   * Slot 15:   0x11780000
   * Slot 16:   0x11800000
   * Slot 17:   0x11880000
   * Slot 18:   0x11900000
   * Slot 19:   0x11980000
   * Slot 20:   0x11A00000

c1190WriteCompensationTable 0x11180000 tdcftof2_slot3.txt 1
c1190WriteCompensationTable 0x11300000 tdcftof2_slot6.txt 1
c1190WriteCompensationTable 0x11480000 tdcftof2_slot9.txt 1
c1190WriteCompensationTable 0x11680000 tdcftof2_slot13.txt 1

   */

int
main(int argc, char *argv[])
{
  int stat = 0;
  int idata = 0, ndata = 0;
  int ichan = 0;
  unsigned char *compram;
  float *file_data;
  int inputchar=10;
  char *filename;
  FILE *filep;
  unsigned int vme_addr=0;
  int file_is_hex = 0;

  printf("\nCAEN 1190/1290 INL Compensation Table Loader\n");
  printf("-----------------------------------------------------------------\n");

  /* Handle commandline arguments */
  programName = argv[0];

  if(argc<3)
    {
      printf(" ERROR: Must specify two arguments\n");
      Usage();
      return(-1);
    }
  else
    {
      vme_addr = (unsigned int) strtoll(argv[1],NULL,16)&0xffffffff;
      filename = argv[2];
      if(argc==4) file_is_hex = 1;
    }

#ifdef DO_VME
  vmeSetQuietFlag(1);
  /* Open the VME interface */
  stat = vmeOpenDefaultWindows();
  if(stat != OK)
    goto CLOSE;

  /* Check that the shared mutex is in good health */
  vmeCheckMutexHealth(1);
#endif /* do_VME */

  /* Initalize the signal handler */
  signal(SIGINT, sig_handler);

  /* Open the compensation table file */
  filep = fopen(filename,"r");
  if(filep==NULL)
    {
      perror("fopen");
      printf("%s: ERROR: Unable to open file %s\n",__FUNCTION__,filename);

      goto CLOSE;
    }

  printf("\n");
  printf("Reading in table from: %s\n",
	 filename);
  if(file_is_hex)
    printf(" (HEXADECIMAL)\n");
  
  /* Initialize the arrays */
  file_data = (float *)malloc(TABLE_SIZE*sizeof(float));
  compram   = (unsigned char *)malloc(TABLE_SIZE*sizeof(unsigned char));

  memset((void *) file_data, 0, TABLE_SIZE*sizeof(float));
  memset((void *) compram, 0x0, TABLE_SIZE*sizeof(unsigned char));
  
  /* Read in the file into file_data array */
  int line_no = 0;
  while(!feof(filep))
    {
      char *line = NULL;
      ssize_t read = 0;
      size_t len = 0;
      if(ndata > TABLE_SIZE)
	{
	  printf("%s: ERROR: Table too large (%d numbers)",
		 __FUNCTION__, ndata);
	}
      read = getline(&line, &len, filep);
      if(read > 0)
	{
	  line_no++;
	  if(strncmp((const char *)"#", (const char *)line, strlen("#")) != 0)
	    {
	      char *tok;
	      tok = strtok(line, " ");
	      while(tok != NULL)
		{
		  if(file_is_hex)
		    len = sscanf(tok, "0x%02hhx", (unsigned char *)&compram[ndata]);
		  else
		    len = sscanf(tok, "%f", &file_data[ndata]);
		    
		  if(len==1)
		    {
/* #define DEBUG_FILE_READ */
#ifdef DEBUG_FILE_READ
		      if((ndata % 1024) == 0)
			printf("%8d: Data Read: %5d\n", line_no, ndata);
#endif
		      ndata++;
		      tok = strtok(NULL, " ");
		    }
		  else
		    break;
		}
#ifdef DEBUG_FILE_READ
	      if(((ndata % 1024) == 0) && (ndata > 0))
		{
		  int idata=0;
		  for(idata = 0; idata < 256*4; idata++)
		    {
		      if((idata % 15) == 0)
			printf("\n");

		      if(file_is_hex)
			printf("0x%02x ", compram[idata + (ndata - 256*4)]);
		      else
			printf("%.2f ", file_data[idata + (ndata - 256*4)]);
		    }
		}
#endif
	    }
#ifdef DEBUG_FILE_READ
	  else
	    {
	      printf("%8d: Comment Line:\n %s\n", line_no, line);
	    }
#endif
	}
    }
  fclose(filep);

  printf("Table Size Read from file = %d\n", ndata);
  printf("\n");

  if(file_is_hex == 0)
  {
    /* Flip the sign */
    for(idata = 0; idata < ndata; idata++) file_data[idata] *= -1;
      
    /* Convert float data to two's complement 2byte integer */
    tdc1190ConvertTable_Float2TCInt(file_data, compram, ndata);
  }
  
#ifdef DO_VME
  vmeBusLock();
  tdc1190Init(vme_addr, 1, 1, 1);
  if (Nc1190 == 0)
    {
      vmeBusUnlock();
      goto CLOSE;
    }

  printf("Press y to load firmware (%s) to the TI via VME...\n",
	 filename);
  printf("\t or n to quit without update\n");

 REPEAT2:
  printf("(y/n): ");
  inputchar = getchar();
  
  if((inputchar == 'n') ||
     (inputchar == 'N'))
    {
      printf("--- Exiting without update ---\n");
      {
	vmeBusUnlock();
	goto CLOSE;
      }
    }
  else if((inputchar == 'y') ||
     (inputchar == 'Y'))
    {
    }
  else
    goto REPEAT2;

  printf("Writing compensation table...\n");
  printf("|------------------------------|\n");
  for(ichan = 0; ichan < 32; ichan++)
    {
      printf("."); fflush(stdout);
      tdc1190WriteChannelCompensation(0, ichan, &compram[ichan*(4*256)]);
    }

  vmeBusUnlock();
printf("\nDone\n");
#endif /* DO_VME */
  
CLOSE:

#ifdef DO_VME
  vmeCloseDefaultWindows();
#endif
  
  exit(0);
}

static void
closeup()
{
#ifdef DO_VME
  vmeBusUnlock();
  vmeCloseDefaultWindows();
#endif
  
  exit(0);
}


static void
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
  printf("OR... for input table in hexadecimal form:\n");
  printf("\n");
  printf("%s <VME Address (A24)> <comp_table file> 1\n",programName);
  printf("\n");

}

#else

int
main()
{
}

#endif
