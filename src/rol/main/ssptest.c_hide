
/* sspfirmware.c */
/*
sspfirmware("ssp.bin",13)


     cd $CLON_PARMS/firmwares
     sspfirmware fe_ssp_hallb_gt.bin 3
     sspfirmware fe_ssp_hallb_gt.bin 4
     sspfirmware fe_ssp_hallb_gt.bin 5
     sspfirmware fe_ssp_hallb_gt.bin 6
     sspfirmware fe_ssp_hallb_gt.bin 7
     sspfirmware fe_ssp_hallb_gt.bin 8

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sspLib.h"


#ifdef Linux_vme

#define gBUF_LEN  10000
unsigned int gBuf[gBUF_LEN];

int
main(int argc, char *argv[])
{
  int d;
  int res;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0, i, j, dac;
  int boardId, fibers, asic, ch, wordcnt;
  unsigned int ref, maroc[RICH_CHAN_NUM];
  unsigned int mask0, mask1;
  unsigned int mask2, mask3;
  unsigned int mask4, mask5;

  printf("\n");
  if(argc==2)
    slot = atoi(argv[1]);
  else
  {
    printf("Usage: sspfirmware <slot>\n");
    exit(0);
  }
  printf("\n");
  
  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  //sspConfig(filename)
  
  sspInit(slot<<19, 0, 1, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);
  sspSetMode(slot, 1 | (0xFF<<16), 1);
  sspGStatus(1);

  // Reset event builder
  sspSetBlockLevel(slot, 1);
  
  sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_0);
  sspSetIOSrc(slot, SD_SRC_TRIG, SD_SRC_SEL_0);
  sspEbReset(slot, 1);
  sspEbReset(slot, 0);

  sspRich_Init(slot);
  sspRich_PrintFiberStatus(slot);
  
  sspRich_GetConnectedFibers(slot, &fibers);
  printf("sspRich_GetConnectedFibers(%d) fibers = 0x%08X\n", slot, fibers);

  sspRich_SetCTestAmplitude(slot, 0, 1000);

  for(asic = 0; asic < 3; asic++)
  {
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_CMD_FSU, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_CMD_SS, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_CMD_FSB, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SWB_BUF_250F, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SWB_BUF_500F, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SWB_BUF_1P, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SWB_BUF_2P, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_ONOFF_SS, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_SS_300F, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_SS_600F, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_SS1200F, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_EN_ADC, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_H1H2_CHOICE, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSU_20F, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSU_40F, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSU_25K, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSU_50K, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSU_100K, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB1_50K, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB1_100K, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB1_100F, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB1_50F, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_CMD_FSB_FSU, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_VALID_DC_FS, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB2_50K, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB2_100K, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB2_100F, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SW_FSB2_50F, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_VALID_DC_FSB2, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_ENB_TRISTATE, 0, 1);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_POLAR_DISCRI, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_INV_DISCRIADC, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_D1_D2, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_CMD_CK_MUX, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_ONOFF_OTABG, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_ONOFF_DAC, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SMALL_DAC, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_ENB_OUTADC, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_INV_STARTCMPTGRAY, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_RAMP_8BIT, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_RAMP_10BIT, 0, 0);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_DAC0, 0, 300);
    sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_DAC1, 0, 0);
    
    for(i = 0; i < 64; i++)
    {
      sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_GAIN, i, 64);
      sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_SUM, i, 0);
      sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_CTEST, i, 1);
      sspRich_SetMarocReg(slot, 0, asic, RICH_MAROC_REG_MASKOR, i, 0);
    }
  }
  
  printf("************** WR *************************\n");
  sspRich_PrintMarocRegs(slot, 0, 0, RICH_MAROC_REGS_WR);
  
  // First update shift into MAROC ASIC
  sspRich_UpdateMarocRegs(slot, 0);
  // Second update shift into MAROC ASIC, and out of MAROC ASIC into FPGA
  sspRich_UpdateMarocRegs(slot, 0);
  
  printf("************** RD *************************\n");
  sspRich_PrintMarocRegs(slot, 0, 0, RICH_MAROC_REGS_RD);

  // Note: SSP RICH is 125MHz clocked, so sspPulserSetup frequency assumes 250MHz, so:
  //       asking for 20kHz will give 10kHz on SSP RICH (will be fixed at some point)
  sspPulserSetup(slot, 2.0E6, 0.5, 0xFFFFFFFF);
  sspSetIOSrc(slot, SD_SRC_TRIG2, SD_SRC_SEL_PULSER);
  sspRich_SetCTestSource(slot, 0, RICH_SD_CTEST_SRC_SEL_SSP);

  sspRich_SetTDCEnableChannelMask(slot, 0,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
    );

  sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_1);
  usleep(1000);
  sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_0);
  usleep(1000);
  
  for(i=0; i < 32; i++)
  {
    if(fibers & (1<<i))
    {
      sspRich_SetLookback(slot, i, 4000);
      sspRich_SetWindow(slot, i, 2000);
    }
  }
  
  while(1)
  {
//    printf("Before trigger:\n");
//    if(sspPrintEbStatus(slot))
//      exit(1);
//    printf("\n");
    
    sspSetIOSrc(slot, SD_SRC_TRIG, SD_SRC_SEL_1);
    sspSetIOSrc(slot, SD_SRC_TRIG, SD_SRC_SEL_0);

    for(i=1000; i>0; i--)
    {
      if(sspBReady(slot))
        break;
    }

    printf("i = %d\n", i);
    
//    printf("After trigger:\n");
    wordcnt = sspPrintEbStatus(slot)+1;
    printf("\n");

    if(!i)
    {
      printf("ERROR - sspBReady() timeout...\n");
      exit(1);
    }
    
    if(wordcnt > gBUF_LEN)
    {
      printf("ERROR - event too large...\n");
      exit(1);
    }
    
    wordcnt = sspReadBlock(slot, gBuf, wordcnt, 0);
    if(wordcnt <= 0)
    {
      printf("ERROR - event readout error...\n");
      exit(1);
    }
    
    printf("ssp buf[%5d]:", wordcnt);
    
    if(wordcnt > 10) wordcnt = 10;
    
    for(i=0;i<wordcnt;i++)
      printf(" 0x%08x",gBuf[i]);
    printf("\n");
    
//    printf("After readout:\n");
//    if(sspPrintEbStatus(slot))
//      exit(1);
//    printf("\n");
    
    
    /*
    sspRich_ReadScalers(slot, 0, &ref, maroc);
    
    printf("Scaler Print:\n");
    printf("Ref = %d\n", ref);
    for(ch = 0; ch < 64; ch++)
    {
      printf("Ch%2d:", ch);
      for(asic = 0; asic < 3; asic++)
        printf(" %10d", maroc[64*asic+ch]);
      printf("\n");
    }
    sleep(1);
    */
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
