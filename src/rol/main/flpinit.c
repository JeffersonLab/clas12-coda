
/* flpinit.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flpLib.h"



#ifdef Linux_vme

#include "jvme.h"
#include "flpLib.h"
#include "flpConfig.h"

int
main(int argc, char *argv[])
{
  int res;
  unsigned int addr, laddr;
  int slot = 0;
  float v1 = 0, v2 = 0, v3 = 0;
  float freq = 6000;
  int nflp;

  if(argc!=2)
  {
    printf("Usage: flpinit 0/1\n");
    exit(0);
  }

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
 
vmeBusLock();

  flpInit(0x900000,20); /* 0x900000 - FLP in slot 18 */
  nflp = flpGetNflp();
  printf ("nflp=%d, setting flp(s)\n",nflp);


  if(nflp>0)
  {
    if (atoi(argv[1]) == 1)
    {  flpEnableOutput(0);
	 flpEnableOutput(1);
	 flpEnableIntPulser(0);
	 flpEnableIntPulser(1);
	
	 flpConfig("");
	
	 flpStatus(0);
	
	 flpUploadAllPrint();
    }
    if (atoi(argv[1]) == 0)
    {
      flpDisableOutput(0);
      flpDisableOutput(1);
      flpDisableIntPulser(1);
      flpDisableIntPulser(0);
      flpStatus(0);

    }

  }



#if 0
  if(argc == 4)
  {
    v3 = atof(argv[2]);
    printf("set voltage, %4.3f\n", v3);
    freq = atof(argv[3]);
    printf ("set frequency, %4.3f", freq);
    if (atoi(argv[1]) == 1)
    {
      
      flpEnableOutput(0);
      flpEnableOutput(1);
      flpEnableIntPulser(0);
      flpEnableIntPulser(1);
      flpSetOutputVoltages(0, 3.5, 3.5, v3);
      flpSetOutputVoltages(1, 3.5, 3.5, v3);
      flpSetPulserPeriod(0, 50000000.0/freq);
      flpSetPulserPeriod(1, 50000000.0/freq);
    }
    else{
      if (atoi(argv[1]) == 0){
	flpDisableOutput(0);
	flpDisableOutput(1);
	flpDisableIntPulser(1);
	flpDisableIntPulser(0);
      }
      else {
	printf("Input 1 to tunr LED on, 0 to turn LED off.\n");
      }
    }
  }
  else printf ("Must be three arguments\n");
  flpStatus(0);
#endif

vmeBusUnlock();

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
