/*----------------------------------------------------------------------------*
 *  Copyright (c) 2013        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Authors: Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Firmware update for the Signal Distribution (SD) module.
 *
 *----------------------------------------------------------------------------*/

#if defined(VXWORKS) || defined(Linux_vme)

/*sergey: 

  VXWORKS:
     ld < $CODA/src/rol/VXWORKS_ppc/bin/sdFirmwareUpdate
     cd "$CLON_PARMS/firmwares"
     sdFirmwareUpdate("SD_Production_Code_VerA5.rbf")

  UNIX:
     cd $CLON_PARMS/firmwares
     sdFirmwareUpdate SD_Production_Code_VerA5.rbf

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "unistd.h"
#ifdef VXWORKS
/*sergey#include "vxCompat.h"*/
#else
#include "jvme.h"
#endif
#include "tiLib.h"
#include "tsLib.h"
#include "sdLib.h"

void Usage();
char bin_name[50];


int
main(int argc, char *argv[])
{
  int res=0;
  char firmware_filename[50];
  int current_fw_version=0;
  int inputchar=10;

  printf("------------------------------------------------\n");

  vmeSetQuietFlag(1);
  res = vmeOpenDefaultWindows();
  if(res!=OK)
    goto CLOSE;

  printf("tiInit called ...%d\n");fflush(stdout);

  res = tiInit(21<<19,0,1);
  printf("tiInit returns %d\n",res);
  if(res!=OK)
    {
      /* try tsInit, instead */
	  printf("trying TS instead of TI ..\n");
      res = tsInit(21<<19,0,1);
      printf("tsInit returns %d\n",res);
      if(res!=OK) goto CLOSE;
    }
  
  res = sdInit(SD_INIT_IGNORE_VERSION);
  printf("sdInit returns %d\n",res);
  if(res!=OK)
    goto CLOSE;

  current_fw_version = sdGetFirmwareVersion(0);
  printf("\n  Current SD Firmware Version = 0x%x\n\n",current_fw_version);

CLOSE:
  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
