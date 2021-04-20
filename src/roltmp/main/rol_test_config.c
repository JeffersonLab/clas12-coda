
/* rol_test_config.c - testing program for boards config files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef Linux_vme

#include "tiLib.h"
#include "tiConfig.h"

#include "tdc1190.h"

#include "sspLib.h"
#include "sspConfig.h"

#include "dsc2Lib.h"
#include "dsc2Config.h"

#include "fadcLib.h"
#include "fadc250Config.h"

#define NDIM 10000



/* possible arguments:

    ""
    "/usr/clas12/release/0.2/parms/fadc250/hps1.cnf"
    "/usr/clas12/release/0.2/parms/trigger/clasdev.cnf"

*/

int
main(int argc,char* argv[])
{
  char txt[128];

  if (argc>1) sprintf(txt,"%s",argv[1]);
  else        sprintf(txt,"%s","/usr/clas12/release/0.2/parms/trigger/clasdev.cnf");

  printf(txt);//"\nReading %s ....\n\n",txt);

  fadc250Config(txt);
  tdc1190Config(txt);
  tiConfig(txt);
  dsc2Config(txt);
  sspConfig(txt);
  /*gtpConfig(txt); NIOS only*/

  exit(0);
}

#else

int
main(int argc,char* argv[])
{
  exit(0);
}

#endif
