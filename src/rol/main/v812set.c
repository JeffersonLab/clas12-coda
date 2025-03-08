
/* v812set.cc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <iostream>
//using namespace std;


#ifdef Linux_vme

#include "jvme.h"
#include "v812.h"

static unsigned short threshold=8; /* mV */
static unsigned short width=50; /* ns */
static unsigned short deadtime=0;
static unsigned short majority=1;
static unsigned short mask=65535;
static int pulse=0;

void
decode_command_line(int argc, char**argv)
{
  char help[] = 
    "\nUsage:\n\n  v812set [-t threshold(mV)] [-w width(ns)] [-d deadtime]\n"
    "           [-mjr majority] [-msk mask] [-pulse]\n";

  if(argc<1) {
    printf("%s\n",help);
    exit(0);
  } 

  /* loop over arguments */
  int i=1;
  while (i<argc)
  {
    if (strncasecmp(argv[i],"-h",2)==0) {
      printf("%s\n",help);

      printf("  Default settings (if corresponding parameter is not specified):\n");
      printf("    Threshold=%d (mV)\n",threshold);
      printf("    Width=%d (ns)\n",width);
      printf("    Deadtime=%d\n",deadtime);
      printf("    Majority=%d\n",majority);
      printf("    Mask=%d\n",mask);
      printf("    Pulse=%d\n\n",pulse);

      exit(0);

    } else if (strncasecmp(argv[i],"-t",2)==0) {
      threshold=atoi(argv[i+1]);
      i=i+2;
      if(threshold<1) threshold=1;
      else if(threshold>255) threshold=255;

    } else if (strncasecmp(argv[i],"-w",2)==0) {
      width=atoi(argv[i+1]);
      i=i+2;
      if(width<12) width=12;
      else if(width>203) width=203;

    } else if (strncasecmp(argv[i],"-d",2)==0) {
      deadtime=atoi(argv[i+1]);
      i=i+2;
      if(deadtime<0) deadtime=0;
      else if(deadtime>255) deadtime=255;

    } else if (strncasecmp(argv[i],"-mjr",4)==0) {
      majority=atoi(argv[i+1]);
      i=i+2;
      if(majority<0) majority=0;
      else if(majority>255) majority=255;

    } else if (strncasecmp(argv[i],"-msk",4)==0) {
      mask=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-pulse",6)==0) {
      pulse=1;
      i=i+1;

    } else if (strncasecmp(argv[i],"-",1)==0) {
      printf("\n  ?unknown command line arg: %s\n\n",argv[i]);
      exit(1);
      
    } else {
      break;
    }

  }

  return;
}



int
main(int argc, char *argv[])
{
  int nv812, ii, id;
  unsigned short thresholds[16];

  decode_command_line(argc,argv);

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

vmeBusLock();
  nv812 = v812Init();
vmeBusUnlock();

  printf("\nWriting following settings to all channels to all %d v812 boards:\n",nv812);
  printf("   Threshold=%d (mV)\n",threshold);
  printf("   Width=%d (ns)\n",width);
  printf("   Deadtime=%d\n",deadtime);
  printf("   Majority=%d\n",majority);
  printf("   Mask=%d\n",mask);
  printf("   Pulse=%d\n\n",pulse);

  for(ii=0; ii<16; ii++) thresholds[ii] = threshold;

  for(id=0; id<nv812; id++)
  {
vmeBusLock();
    v812SetThresholds(id, thresholds);
    v812SetWidth(id, width);
    v812SetDeadtime(id, deadtime);
    v812SetMajority(id, majority);
    v812DisableChannels(id, mask);
    if(pulse) v812TestPulse(id);
vmeBusUnlock();
  }


  exit(0);
}



#else

int
main()
{
  printf("\nDo nothing - you have to run that program on VME crate\n\n");
  return(0);
}

#endif

