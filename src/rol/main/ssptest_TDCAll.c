#ifdef Linux_vme

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // sleep, usleep

#include "sspLib.h"
#include "sspLib_rich.h"
#include "jvme.h"
#include "sspConfig.h"
#include "tiLib.h"

#define SSP_RICH_CONFIG_FILE "./config/dafarm35.cnf"
#define SSP_RICH_OUT_FILE "./ssprich_tdc.bin"


#define TMAP "/home/clasrun/rich/suite/maps/threshold.txt";
#define GMAP "/home/clasrun/rich/suite/maps/gain.txt";


#define BUFSIZE 10000000
// functions
int sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain, int ctestChannel);
int GetThreshold(int slot,int fiber,int asic);
int ResetGains();
int LoadGains();

int sspRich_ParseData(unsigned int *  buf,int wordcnt, int printFlag);
int sspRich_GetNfibersAll();
int sspRich_GetNmarocAll();


// gloabls
int gmap[MAX_VME_SLOTS+1][RICH_FIBER_NUM][3][64];// slot, fiber, asic, channel
unsigned int dabuf[BUFSIZE];

//----------------------------------------
int GetGain(int slot,int fiber,int asic,int channel){
//----------------------------------------

  return gmap[slot][fiber][asic][channel];
}

//----------------------------------------
int main(int argc, char *argv[]){
//----------------------------------------

  int bLevel = 1; // Block Level

  // run variables
  int trigMode=0; // 0 for external trigger; 1 to 15 is prescale for internal trigger
  int maxEvent = 100000;

  // tdc settings
  int window = 4000; //300; // ns  // 200
  int lookback = 4000; //1550; // ns // 2700

  // maroc settings
  int threshold = 230;
  int gain = 64;
  int ctestAmplitude= 0;
  int ctestChannel=-1;

  // ti variables
  unsigned int tiData[256];
  int tibready=0;
  int timeout=0;
  int dCnt;
  int prescale=15;


  // ssp variables
  int nssp = 0;
  int id;
  int geo;
  int slot=0;
  int i;
  int j;
  int k;
  int fibers;
  int asic;
  int ch;
  int nfibers = 0;
  int nmarocs = 0;
  int mode;


  // scalers variables
  unsigned int  maroc[RICH_CHAN_NUM];
  int duration =1;
  int ref;
  int absChannel;

  // event readout
  int wordcnt;
  int wordRd;
  int ready=0;


  //DMA variables
  int i1,i2,i3;
  unsigned int * tdcbuf;


  //output file
  FILE * fout;
  char foutName[80]=SSP_RICH_OUT_FILE;


 /* Checks Arguments*/
  if(argc==7){
    threshold = atoi(argv[1]);
    gain      = atoi(argv[2]);
    maxEvent  = atoi(argv[3]);
    trigMode  = atoi(argv[4]);
    prescale  = trigMode;
    ctestAmplitude  = atoi(argv[5]);
    ctestChannel  = atoi(argv[6]);
  }
  else
  {
    printf("\n");
    printf("Usage: ssptest_TDCAll");
    printf("[Threshold; 0 to use map] ");
    printf("[Gain; 0 to use map] ");
    printf("[Event Preset] ");
    printf("[Trigger type; 0 external, 1 to 15 prescale for internal] ");
    printf("[CTEST Amplitude;  0 OFF, 1 to 4095 charge] ");
    printf("[CTEST Channel; -1 OFF, 0 to 63 to select MAROC channel ] ");

    printf("\n\n");
    exit(0);
  }

  //-----------
  // VME Init
  //----------
  vmeOpenDefaultWindows();


  //DMA init
  usrVmeDmaInit();
  usrVmeDmaMemory(&i1, &i2, &i3);
  i2 = (i2 & 0xFFFFFFF0) + 16;
  usrVmeDmaSetConfig(2,5,1); /*A32,2eSST,267MB/s*/
  tdcbuf = (unsigned int *) i2;


  //-----------
  // TI Init
  //----------
  tiInit(21,TI_READOUT_EXT_POLL,0); // ti slot is 21
  tiCheckAddresses();

  if(trigMode==0){
    tiSetTriggerSource(TI_TRIGGER_TSINPUTS); // Front Panel
  }else{
    tiSetTriggerSource(TI_TRIGGER_TRIG21); // Internal generator
    tiSetTrig21Delay(0); // ti Delay is 0 + about 2.6 microseconds
  }

  //---------------
  // SSP Init
  //---------------
  sspInit(0, 0, 1,SSP_INIT_MODE_VXS |  SSP_INIT_SKIP_FIRMWARE_CHECK);
  nssp = sspGetNssp();

  nfibers = sspRich_GetNfibersAll();
  if(nfibers<=0) {printf("No fibers connected. Exit\n");return -1;}
  nmarocs = sspRich_GetNmarocAll();
  if(nmarocs<=0) {printf("No maroc connected. Exit\n");return -1;}

  printf("Total fibers %d \n",nfibers);
  printf("Total maroc %d \n",nmarocs);

  sspRich_PrintConnectedAsic_All();

  //------------------------
  // Front End Configuration
  //------------------------
  //sspConfig(SSP_RICH_CONFIG_FILE);

  fprintf(stderr, "Configuring %d MAROC boards...\n",nmarocs);
  ResetGains();
  LoadGains();

  for(i=0;i<nssp;i++)
  {
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
    {
      if(fibers & (1<<j))
      {
        if(sspRich_IsAsicInvalid(slot,j)) continue;

        // MAROC Slow Control (Gains,thresholds,)
        for(asic = 0; asic < 3; asic++) sspRich_InitMarocReg(slot,j,asic,threshold,gain,ctestChannel);
        sspRich_UpdateMarocRegs(slot, j);// First update shift into MAROC ASIC
        sspRich_UpdateMarocRegs(slot, j);// Second update shift into MAROC ASIC, and out of MAROC ASIC into FPGA

        // Test Pulse
        sspRich_SetCTestAmplitude(slot, j, ctestAmplitude); // charge amplitude
        if(ctestAmplitude>0)sspRich_SetCTestSource(slot,j, RICH_SD_CTEST_SRC_SEL_SSP); // signal source is TRIG2 from SSP
        else sspRich_SetCTestSource(slot,j, RICH_SD_CTEST_SRC_SEL_0);

        // TDC enable
        sspRich_SetTDCEnableChannelMask(slot,j,0xFFFFFFFF,0xFFFFFFFF,
                                               0xFFFFFFFF,0xFFFFFFFF,
                                               0xFFFFFFFF,0xFFFFFFFF);
        // TDC window
        sspRich_SetLookback(slot, j, lookback);
        sspRich_SetWindow(slot, j, window);

      }
    }
  }
  printf("\n");
  printf("====================\n");
  printf("Total Tiles connected %d \n",nmarocs);
  printf("====================\n");


  //------------------
  // SSP prestart
  //------------------

  for(i=0;i<nssp;i++){
    slot = sspSlot(i);

    sspSetBlockLevel(slot, bLevel);

    //sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_0);
    //sspSetIOSrc(slot, SD_SRC_TRIG, SD_SRC_SEL_0);

    // RESET Event Builder (redundant? checj sspRich_Init)
    sspEbReset(slot, 1);
    sspEbReset(slot, 0);

    // Front Panel  Signals
    sspSetIOSrc(slot, SD_SRC_LVDSOUT4, SD_SRC_SEL_PULSER);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT3, SD_SRC_SEL_TRIG1);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT2, SD_SRC_SEL_TRIG2);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT1, SD_SRC_SEL_TRIG1);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT0, SD_SRC_SEL_TRIG2);

    // Test Pulse from TI
    if(ctestAmplitude>0)sspSetIOSrc(slot, SD_SRC_TRIG2, SD_SRC_SEL_TRIG2); // SSP TRIG2 comes from TI TRIG2 
    // as alterntive you can use local SSP pulser (good in case of single SSp board)
    // int pulserFrequency=1000000;
    //  sspPulserSetup(slot, pulserFrequency/2., 0.5, 0xFFFFFFFF);
    //    sspSetIOSrc(slot, SD_SRC_TRIG2, SD_SRC_SEL_PULSER); // use local uncorrelated pulser
    //    printf("Use local SSP pulser\n");
  }

  // Open Outfile
  fout = fopen(foutName,"w");
  if(!fout)
  {
    printf("Error: File %s cannot be opened...exit\n",foutName);
    exit(0);
  }

  //------------------
  // TI prestart
  //------------------

  tiClockReset();
  taskDelay(1);
  tiTrigLinkReset();
  taskDelay(1);
  tiEnableVXSSignals();
  tiSyncReset(1);
  taskDelay(1);
  tiEnableTriggerSource();
  tiSetBlockLimit(0);

  if(trigMode==0){
    tiLoadTriggerTable(0);
    tiEnableTSInput(TI_TSINPUT_1); // plug logical trigger source to TS#1
  }else{
    tiSetRandomTrigger(2,prescale);
   /* or
    unsigned int nevents = 100000; // integer number of events to trigger
    unsigned int period = 83;// periof multiplier 0-0x7FFF (0-32767)
    int range = 0; // min 120 ns; increments od 120 ns; 245.76 us
    tiSoftTrig(2,nevents,period,range);
    */
  }

  int evnt=0;
  int bready[nssp];
  int z;

  int bcount=0;
  int bn,sl;
  for(i=0;i<nssp;i++)bready[i]=0;

 if(trigMode==0)printf("External Trigger Mode, wait for triggers..\n");


  //--------------
  // EVENT READOUT
  //--------------
  while(evnt<maxEvent){
    ready=0;

    while(!tibready) tibready=tiBReady();
    tibready=0;

    dCnt = tiReadBlock((unsigned int *)&tiData,256,1);
    if(dCnt<=0){printf("TI No data or error.  dCnt = %d\n",dCnt);continue;}

    while(ready<nssp)
    {
      for(i=0;i<nssp;i++){
        if(sspBReady(sspSlot(i)))
          bready[i]=1;
      }
      ready=0;
      for(i=0;i<nssp;i++){
        ready +=bready[i];
      }
    }

    for(i=0;i<nssp;i++)
    {
      bready[i]=0;
      slot = sspSlot(i);
      wordcnt = sspGetEbWordCnt(slot);
      if(wordcnt > BUFSIZE){printf("ERROR - %d words, event too large...\n",wordcnt);exit(1);}
      wordRd = sspReadBlock(slot, tdcbuf, BUFSIZE, 1);
      for(z=0;z<wordRd;z++){
        dabuf[z] =(tdcbuf[z]&0x000000FF)<<24;
        dabuf[z]|=(tdcbuf[z]&0x0000FF00)<<8;
        dabuf[z]|=(tdcbuf[z]&0x00FF0000)>>8;
        dabuf[z]|=(tdcbuf[z]&0xFF000000)>>24;
      }
      fwrite(dabuf,wordRd, sizeof(dabuf[0]), fout);

      bn = (dabuf[0] >>  8 ) & 0x3FF;
      sl = (dabuf[0] >> 22 ) & 0x1F;

      if(sl==7){
        evnt=bn+bcount*1024;
        if(bn==1023)bcount++;      
      }
    }
    tiIntAck();  // Tell TI we're ready for next event
    if(evnt%(maxEvent/20)==0)printf("Event %6d\n",evnt); // Tell user how many events we have
  } // end of while

  fclose(fout);
  printf("Binary Data written on %s (%d events)\n",foutName,evnt);
  return 0;
}


//----------------------------------------
int  sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain,int ctestChannel){
//----------------------------------------

  int i;
  int gain_choice=gain;
  double gain_mean = 0;


  int chtest=ctestChannel;
  int ctest=0;

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
    if(i==chtest)ctest=1;else ctest=0;

    if(gain_choice==0) gain=GetGain(slot,fiber,asic,i);
    gain_mean+=gain;
    //printf("slot %d fiber %d asic %d channel %2d gain %3d\n",slot,fiber,asic,i,gain);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_GAIN, i, gain);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_SUM, i, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_CTEST, i, ctest);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_MASKOR, i, 0);
  }
  printf("slot %d fiber %2d asic %d threshold %3d gain mean %6.3lf\n",slot,fiber,asic,threshold,gain_mean/64.);
  return 0;
}


//----------------------------------------------------------------------------
int sspRich_ParseData(unsigned int *  buf,int wordcnt, int printFlag){
//----------------------------------------------------------------------------

  int i;
  int tag=0;;
  int tag_idx=0;
  int slot;
  int blockNum;
  int blockLevel;
  int nwords;
  int evtNum=0;
  int fiber;
  int timeH,timeL;
  long unsigned int  timestamp=0;
  int edge;
  int channel;
  int time;
  int p =printFlag;

 for(i=0;i<wordcnt;i++){


    if(i==0)p=1;else p=0;

   if(buf[i] & 0x80000000){ // Data Type defininig, bit 31 =1
      tag = ( buf[i] >> 27 ) & 0xF;
      tag_idx = 0;
    }else{ // Data type continuation, bit 31 = 0
      tag_idx++;
    }

   // if(tag!=15){
      if(p)printf("%4d ",i);
      if(p)printf("0x%08x ",buf[i]);
      if(p)printf("%2d ",tag);
   // }

    switch(tag){
      case 0:
        blockLevel = (buf[i] >>  0 ) & 0xFF;
        blockNum   = (buf[i] >>  8 ) & 0x3FF;
        slot       = (buf[i] >> 22 ) & 0x1F;

        if(p){
          printf("[ BLOCK HEADER] ");
          printf("LEVEL %d ",blockLevel);
          printf("SLOT %d ",slot);
          printf("BLKNUM %d ",blockNum);
        }

      break;

      case 1:
        nwords = (buf[i] >>  0 ) & 0x3FFFFF;
        slot   = (buf[i] >> 22 ) & 0x1F;

       if(p){
         printf("[BLOCK TRAILER] ");
         printf("SLOT %d ",slot);
         printf("NWORDS %d ",nwords);
       }
       break;


      case 2:
        evtNum = (buf[i] >>  0 ) & 0x3FFFFF;
        slot   = (buf[i] >> 22 ) & 0x1F;
        if(p){
          printf("[ EVENT HEADER] ");
          printf("SLOT %d ",slot);
          printf("EVTNUM %d ",evtNum);
        }
      break;
      case 3:
       if(p)printf("[ TRIGGER TIME] ");

       if(tag_idx == 0)
       {
          timeH = (buf[i] >> 0) & 0xFFFFFF;
          if(p)printf("TIME H %d ",timeH);
          timestamp=0;
          time |= timeH;
       }
       else if(tag_idx == 1)
       {
          timeL = (buf[i] >> 0) & 0xFFFFFF;
          if(p)printf("TIME L %d ",timeL);
          timestamp |= (timeL << 24);
          if(p)printf("-> (%lu)  %lf sec.",timestamp,timestamp/125000000.);
        }
      break;

      case 7:
        evtNum  = (buf[i] >>  0 ) & 0x3FFFFF;
        fiber   = (buf[i] >> 22 ) & 0x1F;
        if(p){
          printf("[        FIBER] ");
          printf("EVTNUM %d ",evtNum);
          printf("FIBER %d", fiber);
        }
      break;

      case 8:
        edge    = (buf[i] >> 26 ) & 0x1; // 0 = rising, 1 = falling, seen by FPGA
        channel = (buf[i] >> 16 ) & 0xFF;
        time    = (buf[i] >>  0 ) & 0x7FFF;

        if(p){
          printf("[          TDC] ");
          printf("%d %3d %5d",edge,channel,time);
         }
      break;

      case 14: if(p)printf("[          DNV]");
      break;
      case 15: if(p)printf("[       FILLER]");
      break;
      default: if(p)printf("[      UNKNOWN]");
      break;
    }

    if(p)printf("\n");
  }
  return evtNum;
}


//----------------------------------------
int sspRich_GetNfibersAll(){
//----------------------------------------
  int i,j,slot;
  int nssp = sspGetNssp();
  int n=0;
  int fibers;
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
      if(fibers & (1<<j))
        n++;
  }
  return n;
}

//----------------------------------------
int sspRich_GetNmarocAll(){
//----------------------------------------

  int i,j,slot;
  int nssp = sspGetNssp();
  int n=0;
  int fibers;
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
    sspRich_GetConnectedFibers(slot, &fibers);
    for(j=0; j < 32; j++)
      if(fibers & (1<<j))
        if(!sspRich_IsAsicInvalid(slot,j))
          n++;
  }
  return n;
}


//----------------------------------------
int GetThreshold(int slot,int fiber,int asic){
//----------------------------------------

  FILE *  fin;
  int var[4];
  int thr;
  int thr_default = 230;
  const char * filename = TMAP;

  thr = thr_default;

  fin = fopen(filename,"r");
  if(!fin)
  {
   printf("Threshold file %s not found...\n");
  }
  else
  {
    while(fscanf(fin,"%d %d %d %d \n",var,var+1,var+2,var+3)!=EOF) // slot, fiber, asic, threshold
    {
      //printf("%d %3d %d %4d\n",var[0],var[1],var[2],var[3]);
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
 for(slot=0;slot<MAX_VME_SLOTS+1;slot++)
   for(fiber=0;fiber<RICH_FIBER_NUM;fiber++)
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

 // const char * filename = "/home/clasrun/rich/calibration_suite/maps/eqmap_all_sorted.txt";
  const char * filename = GMAP;

  fin = fopen(filename,"r");
  if(!fin)
  {
    printf("Gain file %s not found...\n");
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



#else

int
main()
{
  return(0);
}

#endif
