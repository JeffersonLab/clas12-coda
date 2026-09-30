
/* tdc1190init.c */

/*USAGE: must be on the VME controller, type: 'tdc1190init' */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "tiLib.h"
#include "tdc1190.h"
#include "jvme.h"

int
main(int argc, char *argv[])
{
  int ntdc;

  vmeCloseDefaultWindows();

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

vmeBusLock();
  tiInit((21<<19),2,0);
  tiStatus(1);
vmeBusUnlock();

vmeBusLock();
  ntdc = tdc1190Init(0x11100000,0x80000,20,0);
  if(ntdc)
  {
    tdc1290SetExpid("clasrun");
    tdc1190Config("");
    tdc1190Config("/usr/clas12/release/2.0.0/parms/trigger/sergey/random.cnf");
  }
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

/*
ReadConfigFile: window_offset=-8700

ReadConfigFile: window_extra=25

ReadConfigFile: window_reject=50

ReadConfigFile: crate = end  host = test4 - disactivated
tdc1190WriteMicro: ERROR: Write Status not OK
Set Window Width to 800 ns
tdc1190WriteMicro: ERROR: Write Status not OK
.....
 */
