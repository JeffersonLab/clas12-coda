#ifdef Linux_vme

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // sleep, usleep

#include "sspLib.h"
#include "jvme.h"

#define NAMESCALER "ssprich_scaler_138.txt" // if you modify this, please modify below accordingly


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
  int ctestAmplitude= 0;
  int ret;
  int pulserFrequency=1000;
  int duration =1;
  int ref;
  int absChannel;
  FILE * fout;
  char foutName[80]=NAMESCALER;
  int skipSSPRichinit;
  int printscreen=0;

  /* Checks Arguments*/
  printf("argc is %d\n",argc);
  if(argc==4)
  {
    threshold = atoi(argv[1]);
    ctestAmplitude = atoi(argv[2]);
    duration = atoi(argv[3]);
  }
  else
  {
    printf("\nUsage: ssptest [TDC threshold] [CTEST Amplitude] [Scaler Duration]\n");
    exit(0);
  }

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  
  sspInit(0, 0, 1, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);
  nssp = sspGetNssp();
  printf("Found %d SSP\n",nssp);

  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspSetMode(slot, 1 | (0xFF<<16), 1);
  }

//  sspGStatus(1);


  skipSSPRichinit=1;

  if(skipSSPRichinit){

     nssp = sspRich_LoadConfig("./setup.txt");

 }else{


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
    printf("SLOT[%d] sspRich_GetConnectedFibers  fibers = 0x%08X (%d)\n", slot, fibers,fibers);
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

  // Check complete tiles
  if(nmarocs!=nfibers){
    printf("Warning: there are incomplete tiles (%d)\n",nfibers-nmarocs);
  }
 
  sspRich_PrintConnectedAsic_All();


  // Configure the Pulser
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    // SSP setup
    // Note: SSP RICH is 125MHz clocked, so sspPulserSetup frequency assumes 250MHz, so:
    //       asking for 20kHz will give 10kHz on SSP RICH (will be fixed at some point)
    sspPulserSetup(slot, pulserFrequency/2., 0.5, 0xFFFFFFFF);
    sspSetIOSrc(slot, SD_SRC_TRIG2, SD_SRC_SEL_PULSER);
  }

}// end of SSPRICHinit


 printf("Configuring %d MAROC boards",nmarocs);
  //  TILES setup
  for(i=0;i<nssp;i++)
  {
   fprintf(stdout,".");
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
  
    for(j=0; j < 32; j++)
    {
      if(fibers & (1<<j))
      {
        if(sspRich_IsAsicInvalid(slot,j)) continue;

        // CTEST Amplitude
        sspRich_SetCTestAmplitude(slot, j, ctestAmplitude);


        // MAROC Slow Control (Gains,thresholds,)
        for(asic = 0; asic < 3; asic++) sspRich_InitMarocReg(slot,j,asic,threshold,gain);
        if(printscreen)printf("******** WR **** SLOT %d FIBER %2d *********\n",slot,j);

        //sspRich_PrintMarocRegs(slot, j, 0, RICH_MAROC_REGS_WR);
        sspRich_UpdateMarocRegs(slot, j);// First update shift into MAROC ASIC
        sspRich_UpdateMarocRegs(slot, j);// Second update shift into MAROC ASIC, and out of MAROC ASIC into FPGA
        if(printscreen)printf("******** RD **** SLOT %d FIBER %2d *********\n",slot,j);
        // sspRich_PrintMarocRegs(slot, j, 0, RICH_MAROC_REGS_RD);

        // CTEST signal source
        if(ctestAmplitude>0)sspRich_SetCTestSource(slot,j, RICH_SD_CTEST_SRC_SEL_SSP);
        else sspRich_SetCTestSource(slot,j, RICH_SD_CTEST_SRC_SEL_0);

      }
    }
  }
  printf("OK\n");


  // SCALERS
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
    {
      if(fibers & (1<<j))
      {
        if(sspRich_IsAsicInvalid(slot,j)<=0) continue;

        // dummy read to reset scalers
        sspRich_ReadScalers(slot, j, &ref, maroc);
//        printf("Scaler Reset for Slot %d Fiber %d ...DONE\n",slot,j);
      }
    }
  }


  printf("Counting... %d  seconds at thr %d\n",duration, threshold);
  sleep(duration);

  // READ SCALER

  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
    {
      if(fibers & (1<<j))
      {
        if(sspRich_IsAsicInvalid(slot,j)) continue;

        sprintf(foutName,"ssprich_scaler_%03d.txt",(slot-3)*32+j); // ABSOLUTE TILE!!
        fout = fopen(foutName,"w");
        if(fout==NULL){printf("Error: file %s not opened\n",foutName); return -1;}

        // reset the counts array
        for(asic =0 ; asic<3 ; asic++){
          for(ch=0 ; ch<64; ch++){
            maroc[asic*64+ch]=0;
          }
        }

        sspRich_ReadScalers(slot, j, &ref, maroc);
           
        if(printscreen){
         // print screen
          printf("\nScaler Print:\n");
          printf("THR %d\n", threshold);
          printf("GAIN %d\n", gain);
          printf("Ref = %u\n", ref);
          printf("Pulser = %d [Hz]\n", pulserFrequency);
          for(ch = 0; ch < 64; ch++){
            printf("Slot % d Fiber %2d Ch%2d:",slot,j, ch);
            for(asic = 0; asic < 3; asic++){
              printf(" %10u", maroc[asic*64+ch]);
            }
            printf("\n");
          }
        }
        // export txt
        fprintf(fout,"%d\n", threshold);
        fprintf(fout,"%d\n", gain);
        fprintf(fout, "%u\n", ref);
        fprintf(fout,"%d\n", pulserFrequency);
        for(asic = 0; asic < 3; asic++){
          for (ch = 0; ch < 64; ch++){
            absChannel = ch+64*asic+192*j+192*32*(slot-3); // ABSOLUTE CHANNEL!!
            fprintf(fout,"%8d %10u\n",absChannel,(int) maroc[asic*64+ch]);
          }
        }
        fclose(fout);
      }
    }
  }// slot

  return 0;
}


//----------------------------------------
int  sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain){
//----------------------------------------

    int i;

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
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CTEST, i, 1);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_MASKOR, i, 0);
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

