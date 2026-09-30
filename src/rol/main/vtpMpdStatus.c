/*
 * File:
 *    vtpMpdStatus.c
 *
 * Description:
 *    Show status of VTP and their attached MPDs
 *
 *   This first looks at the connections between VTP <---> MPD.
 *
 *   If a channel is up (good fiber+serial connections), it will be
 *    added to the list to be initialized.
 *
 *   Final status shown is for the MPDs discovered just from the
 *    connection up status
 *
 * Usage:
 *      vtpMpdStatus
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int
main()
{
  printf("\n\nvtpMpdStatus obsolete, use vtpMpdInit instead\n\n\n");
  exit(0);
}




/*OBSOLETE, USE vtpMpdInit INSTEAD !!!*/
#if 0

#define VTP

#define OLD

#ifdef Linux_armv7l


#include "vtp.h"
#include "vtpLib.h"
#include "vtpConfig.h"
#include "vtpMpdConfig.h"
#include "mpdLib.h"

#ifndef OLD
char COMMON_MODE_FILENAME[250], PEDESTAL_FILENAME[250];
#include "vtp_mpdro.c"
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 250
#endif
int getShortHostname(char *shortHostname);

int main(int argc, char *argv[])
{
  int stat;
  char rol_usrConfig[250];
  char shortHostname[HOST_NAME_MAX];


  /*set debug print level*/
  mpdSetPrintDebug(0);



  
  stat = getShortHostname(shortHostname);
#ifndef OLD
  sprintf(rol_usrConfig, "%s/vtp/%s.cnf",getenv("CLON_PARMS"),shortHostname);
#else
  sprintf(rol_usrConfig, "%s/vtp/%s.bla",getenv("CLON_PARMS"),shortHostname);
#endif
  printf("rol_usrConfig = %s\n", rol_usrConfig);


#ifdef OLD
  vtpOpen(VTP_FPGA_OPEN|VTP_I2C_OPEN|VTP_SPI_OPEN);
  vtpInit(VTP_INIT_CLK_VXS_250);
  vtpInitGlobals();
  if(strncasecmp(rol_usrConfig,"none",4)) vtpConfig(rol_usrConfig);
#else
  char *expid = getenv("EXPID");
  vtpSetExpid(expid);
  vtpInitGlobals();
  vtpConfig(""); /*vtpOpen() called inside !!!*/
  vtpConfig(rol_usrConfig);
  vtpMpdDownload();
#endif

  
#ifdef OLD
  vtpStatus(1);
  vtpMpdFiberReset();
  vtpMpdFiberLinkReset(0xffffffffffffffff);
  vtpMpdDisable(0xffffffffffffffff);
  vtpMpdEnable(0xffffffffffffffff);
  vtpStatus(0);
  vtpMpdPrintStatus(0,0);
  vtpMpdPrintStatus(0,1);
  unsigned int chanmask = vtpMpdGetChanUpMask();
  mpdInitVTP(chanmask, MPD_INIT_FIBER_MODE | MPD_INIT_NO_CONFIG_FILE_CHECK);
  mpdGStatus(1);
#else
  vtpCheckMutexHealth(1);
  vtpLock();
  vtp_mpd_setup("");
  vtpUnlock();
#endif


  vtpClose(VTP_FPGA_OPEN|VTP_I2C_OPEN|VTP_SPI_OPEN);

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
    printf("null\n");

  return rval;
}


/*
  Local Variables:
  compile-command: "make -k -B vtpMpdStatus"
  End:
 */

#else

int
main()
{
  exit(0);
}

#endif

#endif /*if 0*/
