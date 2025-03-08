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
 *     Firmware update for the Pipeline Trigger Interface (TI) module.
 *
 *----------------------------------------------------------------------------*/

#if defined(VXWORKS) || defined(Linux_vme)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef VXWORKS
/*sergey#include "vxCompat.h"*/
#else
#include "jvme.h"
#endif

#include "tiLib.h"


#ifdef VXWORKS
extern  int sysBusToLocalAdrs(int, char *, char **);
extern STATUS vxMemProbe(char *, int, int, char*);
extern UINT32 sysTimeBaseLGet();
extern STATUS taskDelay(int);
#ifdef TEMPE
extern unsigned int sysTempeSetAM(unsigned int, unsigned int);
#else
extern unsigned int sysUnivSetUserAM(int, int);
extern unsigned int sysUnivSetLSI(unsigned short, unsigned short);
#endif /*TEMPE*/
#endif


extern volatile struct TI_A24RegStruct *TIp;
unsigned int BoardSerialNumber;
unsigned int firmwareInfo;
char *programName;

int tiMasterID(int sn);
int tiEMInit();
void tiFirmwareEMload(char *filename);
#ifndef VXWORKS
static void tiFirmwareUsage();
#endif

#define TI_ADDR   (21<<19)
#define TI_READOUT 2

int
main(int argc, char *argv[])
{
  int stat, ret;
  int BoardNumber;
  char *filename;
  int inputchar=10;
  unsigned int vme_addr=0;
  char rSN[100];
  int iFlag = 0x1; // no init

  vmeSetQuietFlag(1);
  stat = vmeOpenDefaultWindows();
  if(stat != OK) exit(1);

  /*
  stat = tiInit(0,0,0);
  if(stat != OK) exit(1);
*/

  ret = tiInit(TI_ADDR,TI_READOUT,iFlag);
  printf("INFO1: tiInit() returns %d \n",ret);
  if(ret<0)
  {
    ret = tiInit(0,TI_READOUT,0);
    printf("INFO2: tiInit() returns %d \n",ret);
  }
  if(ret<0)
  {
    printf("ERROR: tiInit() returns %d \n",ret);
    exit(1);
  }

  printf("\n-----------------------\n");
  tiGetSerialNumber((char **)&rSN);
  printf("\n-----------------------\n");
  tiStatus(1);

  vmeCloseDefaultWindows();

  exit(0);
}

#else

int
main()
{
}

#endif
