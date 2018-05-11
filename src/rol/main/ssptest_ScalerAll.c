#ifdef Linux_vme

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // sleep, usleep

#include "sspLib.h"
#include "jvme.h"

#define NAMESCALER "ssprich_scaler_138.txt" // if you modify this, please modify below accordingly


//#define RICH_IN_THRESHOLDS   "/home/clasrun/rich/suite1.0/maps/threshold.txt";
//#define RICH_IN_GAINS        "/home/clasrun/rich/suite1.0/maps/gain.txt";
//#define RICH_OUT_TEMPERATURE "/home/clasrun/rich/suite1.0/data/temperature/ssprich_Temperatures.txt";

int  sspRich_ReadTemperature(int slot,int fiber);
int  sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain);
int GetThreshold(int slot,int fiber,int asic);
int ResetGains();
int LoadGains();

int gmap[8][32][3][64];// slot, fiber, asic, channel


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
  if(argc==5)
  {
    threshold = atoi(argv[1]);
    gain = atoi(argv[2]);
    ctestAmplitude = atoi(argv[3]);
    duration = atoi(argv[4]);
   // printscreen=atoi(argv[5]);

  }
  else
  {
    printf("\nUsage: ssptest [threshold; 0 to enable map] [gain; 0 to enable map] [CTEST Amplitude] [Scaler Duration]\n");
    printf("\nExample: ssptest 230 64 0 1\n");
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


  printf("Configuring MAROC boards ");
  ResetGains();
  LoadGains();

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
        if(sspRich_IsAsicInvalid(slot,j)) continue;

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
   //       printf("\nScaler Print:\n");
          printf("Slot % d Fiber %2d: ",slot,j);

          printf("THR %d ", threshold);
//          printf("GAIN %d\n", gain);
          printf("Ref = %10u ", ref);
          printf("Elapsed Time  = %6.3lf\n",1/(125E6/ref));
//          printf("Pulser = %d [Hz]\n", pulserFrequency);
          for(ch = 0; ch < 64; ch++){
           // printf("Slot % d Fiber %2d Ch%2d:",slot,j, ch);
            for(asic = 0; asic < 3; asic++){
           //   printf(" %10u", maroc[asic*64+ch]);
            }
          //  printf("\n");
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

  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
      if(fibers & (1<<j))
        sspRich_ReadTemperature(slot,j);
  }


  return 0;
}


//----------------------------------------
int  sspRich_ReadTemperature(int slot,int fiber){
//----------------------------------------

  float tFPGA = 0.0;
  double limit = 70.0; // Celsius
  sspRich_Monitor mon;
  FILE * fmon;
  //const char * fmonName = RICH_OUT_TEMPERATURE;
  char fmonName[200];
  sprintf(fmonName, "%s/data/temperature/ssprich_Temperatures.txt", getenv("RICH_SUITE"));

  fmon=fopen(fmonName,"a");
  if(!fmon){
    printf("Error in %s: cannot open file %s\n",__FUNCTION__,fmonName);
    exit(0);
  }

  sspRich_ReadMonitor(slot, fiber, &mon);

  fprintf(fmon,"%d ",(int)time(NULL));
  fprintf(fmon,"%d %2d ",slot, fiber);
  fprintf(fmon,"%.3f ",(float)mon.temps.fpga / 1000.0f);
  fprintf(fmon,"%.3f ",(float)mon.temps.regulator[0] / 1000.0f);
  fprintf(fmon,"%.3f ",(float)mon.temps.regulator[1] / 1000.0f);
  fprintf(fmon,"\n");
  fclose(fmon);

  tFPGA = (float)mon.temps.fpga / 1000.0f;
  if(tFPGA>=limit){
    printf("TIME %d ",(int)time(NULL));
    printf("SLOT %d  FIBER %d ",slot, fiber);
    printf("FPGA %6.3f ",(float)mon.temps.fpga / 1000.0f);
    printf("REG1 %6.3f ",(float)mon.temps.regulator[0] / 1000.0f);
    printf("REG2 %6.3f ",(float)mon.temps.regulator[1] / 1000.0f);
    printf("[Celsius]");
    printf("Attention: Temperature too warm!");
    printf("\n");
  }
  return 0;
}

//----------------------------------------
int GetThreshold(int slot,int fiber,int asic){
//----------------------------------------

  FILE *  fin;
  int var[4];
  int pri = 0;
  int thr;
  int thr_default = 230;
  //const char * filename =  RICH_IN_THRESHOLDS;
  char filename[200];
  sprintf(filename, "%s/maps/threshold.txt", getenv("RICH_SUITE"));

  thr = thr_default;

  fin = fopen(filename,"r");
  if(!fin)
  {
   printf("Threshold file %s not found...\n");
   exit(1);
  }
  else
  {
    while(fscanf(fin,"%d %d %d %d \n",var,var+1,var+2,var+3)!=EOF) // slot, fiber, asic, threshold
    {
     if(pri) printf("%d %3d %d %4d\n",var[0],var[1],var[2],var[3]);
      if(var[0]==slot && var[1]==fiber && var[2]==asic){
        thr = var[3];
        break;
      }
    }
    fclose(fin);
  }
  return thr;
}

//----------------------------------------
int ResetGains(){
//----------------------------------------

 int slot, fiber, asic, channel;
 int gain_default=64;
 for(slot=3;slot<=7;slot++)
   for(fiber=0;fiber<=31;fiber++)
     for(asic=0;asic<=2;asic++)
       for(channel=0;channel<=63;channel++)
         gmap[slot][fiber][asic][channel]= gain_default;

  return 0;
}

//----------------------------------------
int LoadGains(){
//----------------------------------------
  FILE *  fin;
  int var[4];
  int slot, fiber,asic, channel, gain;

  //const char * filename = RICH_IN_GAINS;
  char filename[200];
  sprintf(filename, "%s/maps/gain.txt", getenv("RICH_SUITE"));

  fin = fopen(filename,"r");
  if(!fin)
  {
    printf("Gain file %s not found...\n",filename);
    exit(1);
  }
  else
  {
    while(fscanf(fin,"%d %d %d %d\n",var,var+1,var+2,var+3)!=EOF)  // slot, fiber, channel [0,191], gain
    {
      //printf("%d %3d %d %4d\n",var[0],var[1],var[2],var[3]);
      slot   = var[0];
      fiber  = var[1];
      asic   = var[2]/64;
      channel= var[2]%64;
      gain   = var[3];
      gmap[slot][fiber][asic][channel]= gain;
    }
    fclose(fin);
  }
  return 0;
}

//----------------------------------------
int GetGain(int slot,int fiber,int asic,int channel){
//----------------------------------------

  return gmap[slot][fiber][asic][channel];
}



//----------------------------------------
int  sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain){
//----------------------------------------

    int i;
    int gain_choice=gain;
    double gain_mean = 0;

    if(threshold==0) threshold = GetThreshold(slot,fiber,asic);

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
      if(gain_choice==0) gain=GetGain(slot,fiber,asic,i);
      gain_mean+=gain;
      //printf("slot %d fiber %d asic %d channel %2d gain %3d\n",slot,fiber,asic,i,gain);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_GAIN, i, gain);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SUM, i, 0);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CTEST, i, 1);
      sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_MASKOR, i, 0);
    }
  printf("slot %d fiber %d asic %d threshold %3d gain mean %6.3lf\n",slot,fiber,asic,threshold,gain_mean/64.);

  return 0;
}
#else

int
main()
{
  return(0);
}

#endif

