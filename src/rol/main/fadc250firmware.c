

#ifdef Linux_vme

/*
 * File:
 *    firmwareGTest.c
 *
 * Description:
 *    Test JLab Flash ADC firmware updating
 *
 *


NOTE: second file will print: "Will update firmware for LX110 (Processing FPGA)",
make sure you are doing right FPGA !!!!!!!!!!!!







HPS UNIX:

 cd $CLON_PARMS/firmwares
 fadc250firmware 1 fe_fadc_ctrl_b0.mcs
 fadc250firmware 2 fe_fadc_adc_1b01.mcs

 ##fadc250firmware 1 fe_fadc_ctrl.mcs
 ##fadc250firmware 2 fe_fadc_adc.mcs

 ##fadc250firmware 1 FADC_CTRL_HPS.mcs
 ##fadc250firmware 2 FADC_ADC_HPS.mcs




HPS VXWORKS:

 cd "$CLON_PARMS/firmwares"
 faInit((3<<19),(1<<19),20,0x40005)

 fadcFirmwareReadMcsFile("FADC_CTRL_HPS.mcs")
 fadcFirmwareGLoad(1,0)

 fadcFirmwareReadMcsFile("FADC_ADC_HPS.mcs")
 fadcFirmwareGLoad(0,0)


 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef Linux_vme
#include "jvme.h"
#endif
#include "fadcLib.h"

#define FADC_ADDR (3<<19)
#define NFADC     16
#define SKIPSS
#define DIST_ADDR 0x0

DMA_MEM_ID vmeIN, vmeIN2, vmeOUT;
#define MAX_NUM_EVENTS    400
#define MAX_SIZE_EVENTS   1024*10      /* Size in Bytes */

extern int fadcA32Base;
extern int nfadc;
char *progName;

void Usage();

int 
main(int argc, char *argv[]) 
{

    GEF_STATUS status;
    int fpga_choice, firmware_choice=0;
    char *mcs_filename;
    int inputchar=10;
    int ifa=0;
    unsigned int cfw=0;

    printf("\nJLAB fadc firmware update\n");
    printf("----------------------------\n");

    progName = argv[0];

    if(argc<3)
      {
	printf(" ERROR: Must specify two arguments\n");
	Usage();
	exit(-1);
      }
    else
      {
	fpga_choice = atoi(argv[1]);
	mcs_filename = argv[2];
      }

    if( (fpga_choice != 1) &&
	(fpga_choice != 2) )
      {
	printf(" ERROR: Invalid FPGA choice (%d)\n",fpga_choice);
	Usage();
	exit(-1);
      }

    if(fadcFirmwareReadMcsFile(mcs_filename) != OK)
      {
	exit(-1);
      }

    status = vmeOpenDefaultWindows();

    fadcA32Base=0x09000000;

    int iFlag = (DIST_ADDR)<<10;
    /* Sync Source */
    iFlag |= (1<<0); /* P2 */
    /* Trigger Source */
    iFlag |= (1<<2); // VXS Input Trigger
    /* Clock Source */
    iFlag |= (0<<5); // Internal Clock Source

    iFlag |= (1<<18); // Skip firmware check
/*     iFlag |= (1<<16); // Skip initialization */

#ifdef SKIPSS
    faInit((unsigned int)(FADC_ADDR),(1<<19),NFADC+2,iFlag);
#else
    faInit((unsigned int)(FADC_ADDR),(1<<19),NFADC,iFlag);
#endif

    if(nfadc==0)
      {
	printf(" Unable to initialize any FADCs.\n");
	goto CLOSE;
      }

    for(ifa=0; ifa<nfadc; ifa++)
      {
	cfw = faGetFirmwareVersions(faSlot(ifa),0);
	printf("%2d: Control Firmware Version: 0x%04x   Proc Firmware Version: 0x%04x\n",
	       faSlot(ifa),cfw&0xFFFF,(cfw>>16)&0xFFFF);
      }

    printf(" Will update firmware for ");
    if(fpga_choice==1)
      {
	firmware_choice = FADC_FIRMWARE_FX70T;
	printf("FX70T (Control FPGA) ");
      }
    else if(fpga_choice==2)
      {
	firmware_choice = FADC_FIRMWARE_LX110;
	printf("LX110 (Processing FPGA) ");
      }

    printf(" with file: \n   %s\n",mcs_filename);
    printf(" <ENTER> to continue... or q and <ENTER> to quit without update\n");

    inputchar = getchar();

    if((inputchar == 113) ||
       (inputchar == 81))
      {
	printf(" Exiting without update\n");
	goto CLOSE;
      }

    fadcFirmwareGLoad(firmware_choice,0);

    goto CLOSE;

 CLOSE:


    status = vmeCloseDefaultWindows();
    if (status != GEF_SUCCESS)
    {
      printf("vmeCloseDefaultWindows failed: code 0x%08x\n",status);
      return -1;
    }

    exit(0);
}


void
Usage()
{
  printf("\n");
  printf("%s <chip_choice> <firmware MCS file>\n",progName);
  printf("\n");
  printf("   fpga_choice :     1 for FX70T (Control FPGA)\n");
  printf("                     2 for LX110 (Processing FPGA)\n");
  printf("\n");

}


#else

int
main()
{
  return(0);
}

#endif
