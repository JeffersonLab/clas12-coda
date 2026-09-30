/*
 * File:
 *    vtpMpdInit.c
 *
 * Description:
 *    Initialize VTP and their attached MPDs
 *
 *   if a filename is provided, only initialize those MPD defined
 *
 *   otherwise, only attempt to initialize MPDs found via the fiber+serial
 *    connection (channel must be up).
 *
 *
 * Usage:
 *      vtpMpdInit <optional filename>
 *
 */
#define VTP

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#ifdef Linux_armv7l
//#ifdef Linux_armv7l_RHEL7

#include "vtp.h"
#include "vtpLib.h"
#include "vtpConfig.h"
#include "vtpMpdConfig.h"
#include "mpdLib.h"

//#include "vtp_mpd_setup.c"
char COMMON_MODE_FILENAME[250], PEDESTAL_FILENAME[250];
#include "vtp_mpdro.c"


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 250
#endif
int getShortHostname(char *shortHostname);

int
main(int argc, char *argv[])
{
  int stat;
  int useConfigFile = 1;
  char filename[250];
  char shortHostname[HOST_NAME_MAX];

  /*set debug print level*/
  mpdSetPrintDebug(0xFFFFFFFF);
  vtpSetDebugMask(0xFFFFFFFF);

  //char rol_usrConfig[250] = "/home/sbs-onl/vtp/cfg/sbsvtp4.config";
  char rol_usrConfig[250];
  char *clonparms = getenv("CLON_PARMS");
  char dirname[256];
  sprintf(dirname, "%s/vtp", clonparms);

  if(argc > 1)
  {
    /* assume the only argument is the path to the config file */
    strncpy(filename, argv[1], sizeof(filename));
  }
  else
  {
    stat = getShortHostname(shortHostname);

    filename[0] = '\0';
    
    sprintf(rol_usrConfig, "%s/%s.cnf",dirname,shortHostname);
    //sprintf(rol_usrConfig, "%s/%s.bla",dirname,shortHostname);
    
    printf("rol_usrConfig is >%s<\n",rol_usrConfig);
  }




  /*sergey
  vtpOpen(VTP_FPGA_OPEN|VTP_I2C_OPEN|VTP_SPI_OPEN);
  vtpInit(VTP_INIT_CLK_VXS_250);
  vtpInitGlobals();
  if(strncasecmp(rol_usrConfig,"none",4)) vtpConfig(rol_usrConfig);
  */
  char *expid = getenv("EXPID");
  vtpSetExpid(expid);


  
  //while(1)
{
  vtpInitGlobals();
  vtpConfig(""); /*vtpOpen() called inside !!!*/
  vtpConfig(rol_usrConfig);


/*loop1*/
  
  vtpMpdDownload();
  vtpCheckMutexHealth(1);
vtpLock();
  vtp_mpd_setup(filename);
vtpUnlock();

/*loop1*/

  vtpMpdPrintStatus(0,0);
  mpdGStatus(1);


  vtpClose(VTP_FPGA_OPEN|VTP_I2C_OPEN|VTP_SPI_OPEN);

  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("+++++++++++++*++++++++++++++ finished +++++++++++++++++++++++++++++\n");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n\n");

  sleep(1);
}





  return 0;
}

int
getShortHostname(char *shortHostname)
{
  char longHostname[HOST_NAME_MAX];
  char *tempShort;
  int rval;

  rval = gethostname(longHostname, HOST_NAME_MAX);
  if(rval < 0)
  {
    perror("gethostname");
    return rval;
  }

  printf("long Hostname : %s\n", longHostname);

  tempShort = strtok((char *)&longHostname,".");
  if(tempShort != NULL)
  {
    printf("short Hostname : >%s<\n", tempShort);
    strcpy(shortHostname,tempShort);
  }
  else
  {
    printf("null\n");
  }
  
  return(rval);
}

/*
  Local Variables:
  compile-command: "make -k -B vtpMpdInit"
  End:
 */

#else

int
main()
{
  exit(0);
}

#endif
