#ifdef Linux_vme

/*
 * File:
 *    faV3peds.c
 *
 * Description:
 *    JLab V3 Flash ADC pedestal measurement
 *
 *

 cd $CLON_PARMS/fadc250/peds/
 faV3peds <rocname>_ped.cnf
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef Linux_vme
#include "jvme.h"
#include "usrvme.h"
#endif

#include "codautil.h"
#include "faV3Lib.h"
#include "faV3Config.h"

#define FADC_ADDR (3<<19)
#define NFADC     18
#define DIST_ADDR 0x0

DMA_MEM_ID vmeIN, vmeIN2, vmeOUT;
#define MAX_NUM_EVENTS    400
#define MAX_SIZE_EVENTS   1024*10      /* Size in Bytes */

extern int nfaV3;
char *progName;

void Usage();

int 
main(int argc, char *argv[]) 
{
  int status, iflag;
  char *filename;
  int inputchar=10;
  int ch, ifa=0;
  unsigned int cfw=0;
  FILE *f;
  faV3Ped ped;

  char myhostname[128];
  get_hostname(myhostname, 128);

  printf("\nJLAB fadc pedestal measurement on host %s\n",myhostname);
  printf("----------------------------\n");

  progName = argv[0];

  if(argc != 2)
  {
    printf(" ERROR: Must specify one arguments\n");
    Usage();
    exit(-1);
  }
  else
    filename = argv[1];

  status = vmeOpenDefaultWindows();
  if(status != OK)
  {
    printf("ERROR in vmeOpenDefaultWindows(), status=%d returned\n",status);
    goto CLOSE;
  }

  vmeCheckMutexHealth(1);
  vmeBusLock();

  iflag = 0;
  faV3Init((unsigned int)(FADC_ADDR), 1<<19, NFADC, iflag);
  if(nfaV3 <= 0)
  {
    printf(" Unable to initialize any FADCs.\n");
    goto CLOSE;
  }
  faV3Config("");



  faV3GSetClockSource(0); //internal clock
  //faV3GSetClockSource(2); //external clock
  for(ifa=0; ifa < nfaV3; ifa++)
  {
    faV3SoftReset(faV3Slot(ifa),0);
  }

  f = fopen(filename, "wt");
 
  if(f) fprintf(f, "FADC250_CRATE %s\n", myhostname);
  for(ifa=0; ifa<nfaV3; ifa++)
  {
    if(f) fprintf(f, "FADC250_SLOT %d\nFADC250_ALLCH_PED", faV3Slot(ifa));

    for(ch=0; ch<16; ch++)
    {
      if(faV3MeasureChannelPedestal(faV3Slot(ifa), ch, &ped) != OK)
      {
        printf(" Unabled to measure pedestal on slot %d, ch %d...\n", faV3Slot(ifa), ch);
        fclose(f);
        goto CLOSE;
      }
	  if(f) fprintf(f, " %8.3f", ped.avg);
    }
    if(f) fprintf(f, "\n");
  }
  if(f) fprintf(f, "FADC250_CRATE end\n");

  if(f)
    fclose(f);
  else
    printf(" Unable to open pedestal file %s\n", filename);

CLOSE:

  vmeBusUnlock();
  status = vmeCloseDefaultWindows();
  if (status != 0)
  {
    printf("vmeCloseDefaultWindows failed: code 0x%08x\n",status);
  }

  exit(0);
}


void
Usage()
{
  printf("\n");
  printf("%s <pedestal filename>\n",progName);
  printf("\n");
}


#else

int
main()
{
  return(0);
}

#endif
