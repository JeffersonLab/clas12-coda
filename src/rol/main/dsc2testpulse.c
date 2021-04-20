
/* dsc2init.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#ifdef Linux_vme

#include "jvme.h"
#include "dsc2Lib.h"
#include "dsc2Config.h"

int
main(int argc, char *argv[])
{
  int res;
  char myname[256];
  unsigned int addr, laddr;
  int i, slot = 0, n_sets = 0, double_pulse_gap_us = 0, set_gap_us = 0;

  if(argc != 5)
  {
    printf("Usage: dsc2testpulse <slot> <n_sets> <double_pulse_gap_us> <set_gap_us>\n");
    exit(1);
  }

  slot = atoi(argv[1]);
  n_sets = atoi(argv[2]);
  double_pulse_gap_us = atoi(argv[3]);
  set_gap_us = atoi(argv[4]);

  printf("Settings: slot=%d, n_sets=%d, double_pulse_gap_us=%d, set_gap_us=%d\n", slot, n_sets, double_pulse_gap_us, set_gap_us);

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  
  dsc2Init(0x100000,0x80000,20,0/*1<<19*/);
  dsc2Config ("");

  for(i=0;i<n_sets;i++)
  {
    dsc2TestPulse(slot, 1);
    usleep(double_pulse_gap_us);
    dsc2TestPulse(slot, 1);
    usleep(set_gap_us);
  }

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
