#ifdef Linux_vme

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // sleep, usleep

#include "sspLib.h"
#include "jvme.h"
#include "sspConfig.h"
#include "tiLib.h"



#define gBUF_LEN  500000
unsigned int gBuf[gBUF_LEN];



#define SSP_RICH_CONFIG_FILE "./config/dafarm35.cnf"
#define SSP_RICH_OUT_FILE "./ssprich_tdc.bin"
#define SSP_RICH_Q_EVENTS 5 // number of events at fixed charge


// functions
int sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain);
int sspRich_ParseData(unsigned int *  buf,int wordcnt, int printFlag);
int sspRich_GetNfibersAll();
int sspRich_GetNmarocAll();

//----------------------------------------
int main(int argc, char *argv[]){
//----------------------------------------

  int DMAt = 1; // 1 enable DMA transfer

  // run variables
  int trigExt=0; // 0 for internal trigger; 1 for external trigger
  int maxEvent = 100000;

  int pulserSrc = 1; // 0 =local SSP pulser, 1 = use TI pulser 

  // tdc settings
  int window = 300; // ns  // 200
  int lookback = 1550; // ns // 2700

  // maroc settings
  int threshold = 300;
  int gain = 64;
  int ctestAmplitude= 0;
  int pulserFrequency=1000000;



  // ti variables
  unsigned int tiData[256];
  int tibready=0;
  int timeout=0;
  int dCnt;
  int prescale=13;


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
  int nevent=0;
  int loop=0;
  int nread=0;
  int ready=0;


  //output file
  FILE * fout;
  char foutName[80]=SSP_RICH_OUT_FILE;
 

 /* Checks Arguments*/
  if(argc==3){
    threshold = atoi(argv[1]);
     maxEvent = atoi(argv[2]);
  }
  else if(argc==4){
    threshold = atoi(argv[1]);
     maxEvent = atoi(argv[2]);
     trigExt = atoi(argv[3]);
  }
  else if(argc==6){
    threshold = atoi(argv[1]);
    maxEvent = atoi(argv[2]);
    trigExt = atoi(argv[3]);
    pulserSrc = atoi(argv[4]); 
    ctestAmplitude= atoi(argv[5]); 
  }
  else{
    printf("\n");
    printf("Usage Mode 1: ssptest [TDC threshold; 0 to enable external file] [Event Preset]\n");
    printf("Usage Mode 2: ssptest [TDC threshold; 0 to enable external file] [Event Preset] [Trigger Type: 0 internal, 1 external]\n");
    printf("Usage Mode 3: ssptest [TDC threshold; 0 to enable external file] [Event Preset] [Trigger Type: 0 internal, 1 external] [PulserSource: 0 local ssp, 1 use TI pulser] [CTEST]\n");
    printf("\n");
    exit(0);
  }

  //-----------
  // VME Init
  //----------
  vmeOpenDefaultWindows();

  int i1,i2,i3;

  if(DMAt==1){ 
    usrVmeDmaInit();
    usrVmeDmaMemory(&i1, &i2, &i3);
    i2 = (i2 & 0xFFFFFFF0) + 16;    
    usrVmeDmaSetConfig(2,5,1); /*A32,2eSST,267MB/s*/
  }
  unsigned int * tdcbuf = (unsigned int *) i2;



  //-----------
  // TI Init
  //----------
  tiInit(21,TI_READOUT_EXT_POLL,0); // ti slot is 21
  tiCheckAddresses();

  if(trigExt){
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

//  printf("Total fibers %d \n",nfibers);
//  printf("Total maroc %d \n",nmarocs);

//  sspRich_PrintConnectedAsic_All();

  //------------------------
  // Front End Configuration 
  //------------------------  
  //sspConfig(SSP_RICH_CONFIG_FILE);
  fprintf(stderr, "Configuring %d MAROC boards...",nmarocs);
  int vv;
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
        for(asic = 0; asic < 3; asic++) sspRich_InitMarocReg(slot,j,asic,threshold,gain);
        //printf("******** WR **** SLOT %d FIBER %2d *********\n",slot,j);
        //sspRich_PrintMarocRegs(slot, j, 0, RICH_MAROC_REGS_WR);
        sspRich_UpdateMarocRegs(slot, j);// First update shift into MAROC ASIC
        sspRich_UpdateMarocRegs(slot, j);// Second update shift into MAROC ASIC, and out of MAROC ASIC into FPGA
        //printf("******** RD **** SLOT %d FIBER %2d *********\n",slot,j);
        // sspRich_PrintMarocRegs(slot, j, 0, RICH_MAROC_REGS_RD);

        // CTEST Amplitude
        sspRich_SetCTestAmplitude(slot, j, ctestAmplitude);

        // CTEST signal source
        if(ctestAmplitude>0)sspRich_SetCTestSource(slot,j, RICH_SD_CTEST_SRC_SEL_SSP); // this is TRIG2 in the firmware
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

    // RESET Event Builder (redundant? checj sspRich_Init)
    sspSetBlockLevel(slot, 1);
   // sspSetBlockLevel(slot, 8);
    
    //sspSetIOSrc(slot, SD_SRC_SYNC, SD_SRC_SEL_0);
    //sspSetIOSrc(slot, SD_SRC_TRIG, SD_SRC_SEL_0);
    sspEbReset(slot, 1);
    sspEbReset(slot, 0);

    // Front Panel  Signals
    sspSetIOSrc(slot, SD_SRC_LVDSOUT4, SD_SRC_SEL_PULSER);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT3, SD_SRC_SEL_TRIG1);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT2, SD_SRC_SEL_TRIG2);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT1, SD_SRC_SEL_TRIG1);
    sspSetIOSrc(slot, SD_SRC_LVDSOUT0, SD_SRC_SEL_TRIG2);


    // ASSIGN SD_SRC_TRIG2

    if(ctestAmplitude>0){
      if(pulserSrc ==1){
        sspSetIOSrc(slot, SD_SRC_TRIG2, SD_SRC_SEL_TRIG2); // use TI pulser (does'it work?)
        printf("Use TI  pulser\n");
      }
      else if(pulserSrc ==0){
        sspPulserSetup(slot, pulserFrequency/2., 0.5, 0xFFFFFFFF);
        sspSetIOSrc(slot, SD_SRC_TRIG2, SD_SRC_SEL_PULSER); // use local uncorrelated pulser
        printf("Use local SSP pulser\n");

      }
      else{
        printf("No pulser selected\n");
      }
    }
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

  if(trigExt){
    tiLoadTriggerTable(0);
    tiEnableTSInput(TI_TSINPUT_1); // plug logical trigger source to TS#1 
  }else{
    tiSetRandomTrigger(2,prescale); // prescale 15
   /* unsigned int nevents = 100000; // integer number of events to trigger
    unsigned int period = 83;// periof multiplier 0-0x7FFF (0-32767) 
    int range = 0; // min 120 ns; increments od 120 ns; 245.76 us 
    tiSoftTrig(2,nevents,period,range);
  */
  }
  
 /* tiStatus(1);
  printf("Press return to start\n");
  getchar();
*/
  //--------------
  // DEBUG 
  //--------------  
/*
  while(1){
    nread=0;
    while((tibready==0) && (timeout<100)){
      tibready=tiBReady();
      timeout++;
    }

    if(timeout>=100){timeout=0;continue;}
  
    timeout=0;
    tibready=0;
    dCnt = tiReadBlock((unsigned int *)&tiData,256,0);
    if(dCnt<=0){printf("TI No data or error.  dCnt = %d\n",dCnt);continue;}

    tiIntAck(); 
  } // end of while
*/

  int bcomplete;
  int bready[nssp];


  for(i=0;i<nssp;i++)bready[i]=0;

  //--------------
  // EVENT READOUT
  //--------------  
  while(nevent<maxEvent){
    nread=0;
    ready=0;

    while(!tibready)
     tibready=tiBReady();
/*
    while((tibready==0) && (timeout<100)){
      tibready=tiBReady();
      timeout++;
    }

    if(timeout>=100){timeout=0;continue;}
  
    timeout=0;
*/
    tibready=0;
  
    dCnt = tiReadBlock((unsigned int *)&tiData,256,0);
    if(dCnt<=0){printf("TI No data or error.  dCnt = %d\n",dCnt);continue;}

    while(ready<nssp)
    {
      for(i=0;i<nssp;i++){
    //    printf("SLOT %d",i+3);
      //  sspPrintEbStatus(slot);
      //  printf("\n");

        if(sspBReady(sspSlot(i)))
          bready[i]=1;
       }       
       ready=0;
       for(i=0;i<nssp;i++){
         ready +=bready[i];
//          printf("Ready[%d]=%d ",i,bready[i]);
       }
  //     printf("ready=%d\n",ready);
    }   


    for(i=0;i<nssp;i++)
    {
      bready[i]=0;

      slot = sspSlot(i);
    //  printf("SLOT %d\n",slot);
    //  sspPrintEbStatus(slot);
    //  printf("\n");
      wordcnt = sspGetEbWordCnt(slot); 
      if(wordcnt > gBUF_LEN){printf("ERROR - %d words, event too large...\n",wordcnt);exit(1);}


      if(DMAt!=1){
        wordRd = sspReadBlock(slot, gBuf, wordcnt, 0);
        if(wordRd <= 0){printf("ERROR - event readout error...\n");exit(1);}
        fwrite(gBuf,wordcnt, sizeof(*gBuf), fout);
        bcomplete =  sspRich_ParseData(gBuf,wordcnt,0);// use 1 to enable  print
        if(!bcomplete){// printf("Trailer missing, make a second read\n");
          wordcnt = sspGetEbWordCnt(slot);
          if(wordcnt > gBUF_LEN){printf("ERROR - %d words, event too large...\n",wordcnt);exit(1);}
          wordRd = sspReadBlock(slot, gBuf, wordcnt, 0);
          if(wordRd <= 0){printf("ERROR - event readout error...\n");exit(1);}
          fwrite(gBuf,wordcnt, sizeof(*gBuf), fout);
          bcomplete =  sspRich_ParseData(gBuf,wordcnt,0);
          //  if(bcomplete)printf("Block complete after second read OK\n");
         //  else{printf("Block INCOMPLETE\n\n\n");}
        }
      }else{ // DMA transfer
        wordRd = sspReadBlock(slot, tdcbuf, 1000000, 1); // DMA
        fwrite(tdcbuf,wordRd, sizeof(tdcbuf[0]), fout);
        bcomplete =  sspRich_ParseData(tdcbuf,wordRd,0);// use 1 to enable  print
      }
      nread++; 
    } 
  
    tiIntAck();  // Tell TI we're ready for next event
    nevent++; 

    if(nevent%100==0)printf("Event %d\n",nevent);
  } // end of while

  fclose(fout);
  printf("Binary Data written on %s (%d events)\n",foutName,nevent);
  return 0;
}


//----------------------------------------
int  sspRich_InitMarocReg(int slot,int fiber,int asic,int threshold,int gain){
//----------------------------------------

    int ctest=1; // check logic! 1 = enable, 0 disable?
    int pri = 0;
    int i;
    FILE *  fin;
//    const char * filename = "/home/matt/test/ped/thr_relative/threshold.txt";
    const char * filename = "./threshold.txt";
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
        if(pri)printf("File %s not found...use 230");
        threshold = 230;
      }
    }
    if(threshold!=0)printf("slot %d fiber %2d asic %d threshold %3d\n",slot,fiber,asic,threshold);

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
   // sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_POLAR_DISCRI, 0, 0);
    sspRich_SetMarocReg(slot, fiber, asic, RICH_MAROC_REG_POLAR_DISCRI, 0, 1);
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

  if(gain!=64 || ctest!=1) printf("channel %d gain %d ctest %d\n",i,gain,ctest);

    }

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

  int blockComplete = 0;

 for(i=0;i<wordcnt;i++){

   if(buf[i] & 0x80000000){ // Data Type defininig, bit 31 =1
      tag = ( buf[i] >> 27 ) & 0xF;
      tag_idx = 0;
    }else{ // Data type continuation, bit 31 = 0
      tag_idx++;
    }
  //  if(tag!=15){
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
          //printf("LEVEL %d ",blockLevel);
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
       blockComplete = 1;
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

      case 14: if(p)printf("[          DNV]\n");
      break;
      case 15: if(p)printf("[       FILLER]\n ");
      break;
      default: if(p)printf("[      UNKNOWN]\n ");
      break;
    }

    if(p)printf("\n");
  }
  return blockComplete;;
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

#else

int
main()
{
  return(0);
}

#endif
