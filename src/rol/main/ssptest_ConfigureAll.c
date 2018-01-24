#ifdef Linux_vme

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // sleep, usleep

#include "sspLib.h"
#include "jvme.h"

int  sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain);


//----------------------------------------
int main(int argc, char *argv[]){
//----------------------------------------

  int nssp = 0;
  int id;
  int geo;
  int slot=0;
  int i;
  int j;
  int fibers;
  int nfibers = 0;
  int nmarocs = 0;
  int  asic;
  int ch;
  unsigned int  maroc[RICH_CHAN_NUM];
  int threshold = 300;
  int gain = 64;
  int ctestAmplitude= 1000;
  int ret;
  int pulserFrequency=1.0E3;

 /* Checks Arguments*/
  if(argc==3)
  {
    threshold = atoi(argv[1]);
    ctestAmplitude = atoi(argv[2]);
  }
  else
  {
    printf("\nUsage: ssptest [TDC threshold; 0  enable external file] [CTEST Amplitude]; \n");
    exit(0);
  }

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  
  sspInit(0, 0, 1, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);  
  nssp = sspGetNssp();


  for(i=0;i<nssp;i++){
    geo = sspGetGeoAddress(i);
    slot = sspSlot(i);
    id =  sspId(slot);
    printf("SSP [%d] geo 0x%X, slot %d, id %d'\n",i,geo, slot, id);

  }

  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspSetMode(slot, 1 | (0xFF<<16), 1);
  }

  sspGStatus(1);


  // Reset event builder
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspSetBlockLevel(slot, 1);
    sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_0);
    sspSetIOSrc(slot, SD_SRC_TRIG, SD_SRC_SEL_0);
    sspEbReset(slot, 1);
    sspEbReset(slot, 0);
  }


  // Init SPP and Discovery fibers and marocs
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_Init(slot);
    sspRich_PrintFiberStatus(slot);
  }



// Check Number of fibers (fpga boards)
  nfibers = 0;
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    printf("SLOT[%d] sspRich_GetConnectedFibers  fibers = 0x%08X\n", slot, fibers);
    if(fibers) nfibers++;
  }

  printf("Total fibers %d \n",nfibers);
  if(nfibers==0){
    printf("No fibers connected. Exit\n");
    return -1;
  }

  // Check number of marocs (maroc boards)
  nmarocs =0;
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
    {
      if(fibers & (1<<j))
      {
        if(!sspRich_IsAsicInvalid(slot,j))
        {
          nmarocs++;
        }
      }
    }
  }
  printf("Total marocs %d \n",nmarocs);
  if(nmarocs==0){
    printf("No maroc connected. Exit\n");
    return -1;
  }

 
 // sspRich_PrintConnectedAsic_All();

  // Configure the Pulser if CTEST Amplitude !=0
  if(ctestAmplitude>0){
    for(i=0;i<nssp;i++){
      slot = sspSlot(i);
      // SSP setup
      // Note: SSP RICH is 125MHz clocked, so sspPulserSetup frequency assumes 250MHz, so:
      //       asking for 20kHz will give 10kHz on SSP RICH (will be fixed at some point)
      sspPulserSetup(slot, 2*pulserFrequency, 0.5, 0xFFFFFFFF);
      sspSetIOSrc(slot, SD_SRC_TRIG2, SD_SRC_SEL_PULSER);
    }
  }

  sspRich_SaveConfig("./setup.txt");
//  sspRich_LoadConfig("./setup.txt");

  //  TILES setup
  for(i=0;i<nssp;i++)
  {
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
    {
      if(fibers & (1<<j))
      {
        if(sspRich_IsAsicInvalid(slot,j)) continue;


        printf("================\n");
        printf("SLOT %d FIBER %2d\n",slot, j);
        printf("================\n");

        // CTEST Amplitude
        sspRich_SetCTestAmplitude(slot, j, ctestAmplitude);


        // MAROC Slow Control (Gains,thresholds,)
        for(asic = 0; asic < 3; asic++) sspRich_InitMarocReg(slot,j,asic,threshold,gain);
       
//        printf("******** WR **** SLOT %d FIBER %2d *********\n",slot,j);
        //sspRich_PrintMarocRegs(slot, j, 0, RICH_MAROC_REGS_WR);
        printf("WRITE\n");
        sspRich_UpdateMarocRegs(slot, j);// First update shift into MAROC ASIC
        sspRich_UpdateMarocRegs(slot, j);// Second update shift into MAROC ASIC, and out of MAROC ASIC into FPGA
  //      printf("******** RD **** SLOT %d FIBER %2d *********\n",slot,j);
        printf("READ\n");

        // sspRich_PrintMarocRegs(slot, j, 0, RICH_MAROC_REGS_RD);

        // CTEST signal source
        if(ctestAmplitude>0)sspRich_SetCTestSource(slot,j, RICH_SD_CTEST_SRC_SEL_SSP);
        else sspRich_SetCTestSource(slot,j, RICH_SD_CTEST_SRC_SEL_0);

        // TDC enable
        sspRich_SetTDCEnableChannelMask(slot,j,0xFFFFFFFF,0xFFFFFFFF,
                                               0xFFFFFFFF,0xFFFFFFFF,
                                               0xFFFFFFFF,0xFFFFFFFF);
        // TDC window
        sspRich_SetLookback(slot, j, 4000);
        sspRich_SetWindow(slot, j, 1000);
      }
    }
  }

  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_1);
    usleep(1000);
    sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_0);
    usleep(1000);
  }
  printf("====================\n");
  printf("Total Tiles connected %d \n",nmarocs);
  printf("====================\n");

/*
  FILE * log = fopen("config.log","w");
  char str[1000001];
  ret = sspUploadAll(str, 1000000);
  const int m = 10;
  char str2[m];
  int k;
  for( k=0;k<m;k++){
   printf("%c",str[k+1]); 
   str2[k]=str[k+1];
  }

  printf("UpLoadAll returns %d\n",ret);
  printf("%s",str2);
  fclose(log);
*/
  //exit(0);
  return 0;
}


//----------------------------------------
int  sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain){
//----------------------------------------


    int ctest=1; // check logic! 1 = enable, 0 disable?
    int pri = 0;
    int i;
    FILE *  fin;
    const char * filename = "/home/matt/test/ped/thr_relative/threshold.txt";
    int var[4]; 
    if(threshold==0){
      if(pri) printf("Threshold from External File\n");
      fin = fopen(filename,"r");
      if(fin){
       if(pri) printf("Reading file %s\n",filename);
        while(fscanf(fin,"%d %d %d %d \n",var,var+1,var+2,var+3)!=EOF) // slot, fiber, asic, threshold
        {
           if(pri) printf("%d %3d %d %4d\n",var[0],var[1],var[2],var[3]);
          if(var[0]==slot && var[1]==fiber && var[2]==asic){
            threshold = var[3];
            break;
          } 
        }
      }else{
        if(pri)printf("File %s not found...");
        threshold = 230;
      }
    }
    printf("INIT  asic %d threshold %3d\n",asic,threshold);

    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CMD_FSU, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CMD_SS, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CMD_FSB, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SWB_BUF_250F, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SWB_BUF_500F, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SWB_BUF_1P, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SWB_BUF_2P, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_ONOFF_SS, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_SS_300F, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_SS_600F, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_SS1200F, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_EN_ADC, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_H1H2_CHOICE, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSU_20F, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSU_40F, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSU_25K, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSU_50K, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSU_100K, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB1_50K, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB1_100K, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB1_100F, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB1_50F, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CMD_FSB_FSU, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_VALID_DC_FS, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB2_50K, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB2_100K, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB2_100F, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SW_FSB2_50F, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_VALID_DC_FSB2, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_ENB_TRISTATE, 0, 1);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_POLAR_DISCRI, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_INV_DISCRIADC, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_D1_D2, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CMD_CK_MUX, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_ONOFF_OTABG, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_ONOFF_DAC, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SMALL_DAC, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_ENB_OUTADC, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_INV_STARTCMPTGRAY, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_RAMP_8BIT, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_RAMP_10BIT, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_DAC0, 0, threshold);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_DAC1, 0, 0);

    for(i = 0; i < 64; i++)
    {
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_GAIN, i, gain);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SUM, i, 0);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CTEST, i, ctest);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_MASKOR, i, 0);

     if(gain!=64 || ctest!=1) printf("channel %d gain %d ctest %d\n",i,gain,ctest);

    }

  return 0;
}

#else

int
main()
{
  return(0);
}

#endif
