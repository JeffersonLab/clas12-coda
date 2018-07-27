/****************************************************************************
 *
 *  dsc2Config.c  -  configuration library file for dsc2 board 
 *
 *  Author: Serguei Pozdniakov
 *          Jefferson Lab Online Group
 *          September 2009
 *
 *          Sergey Boyarinov Oct 2013: major revision for the new firmware, config file format changed !!!


config file format:

CRATE      <tdcecal1>     <- crate name, usually IP name

DSC2_SLOT  2   <- slot#

DSC2_WIDTH  20  40   <- TDC width (ns), TRG width (ns)

DSC2_TEST_INPUT 0

DSC2_PULSER F  <- internal pulser frequency (0.01 to 25E6 pulser frequency in Hz)

DSC2_TDCMASK  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1     <- TDC enable mask

DSC2_TRGMASK  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1     <- TRG enable mask

DSC2_TDCORMASK  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1     <- TDC OR mask

DSC2_TRGORMASK  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1     <- TRG OR mask

DSC2_THRESHOLD  20  50   <- board threshold: TDC threshold (mV), TRG threshold (mV)

DSC2_CH_THRESHOLD    0  20   50   <- channel threshold: channel#, TDC threshold (mV), TRG threshold (mV)

following, if used, will supersede TRG width:

DSC2_TRGDIGITAL   40  64    <- board digital TRG output: width (ns), delay (ns)

DSC2_CH_TRGDIGITAL   0   40  64    <- channel digital TRG output: channel#, width (ns), delay (ns)

DSC2_ENABLE_DAQ_READOUT 3   <- enables readout by daq and inhibits readout by tcpserver from slot #

*/

#if defined(VXWORKS) || defined(Linux_vme)

#ifdef VXWORKS
#include <vxWorks.h>
#include <vxLib.h>
#include <logLib.h>
#else
#include "jvme.h"
#endif

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "dsc2Lib.h"
#include "dsc2Config.h"
#include "xxxConfig.h"


/* Global variables */
static int active;

static int Ndsc = 0;                        /* Number of DSCs in Crate */

static int Ndsc_daq = 0;                       /* Number of DSCs for daq scalers readout */
static int dscID_daq[DSC_MAX_BOARDS+1];        /* array of slot numbers for daq scalers readout */

static int Ndsc_tcp = 0;                       /* Number of DSCs for tcpserver's scalers readout */
static int dscID_tcp[DSC_MAX_BOARDS+1];        /* array of slot numbers for tcpserver's scalers readout */

#define DEBUG

#define FNLEN     256       /* length of config. file name */
#define STRLEN    256       /* length of str_tmp */
#define ROCLEN    256       /* length of ROC_name */
#define NBOARD     22
#define NCHAN      16


/* Define global variables (NOTE: index in following arrays is SLOT NUMNER !!!) */
static int     TDCWidth[NBOARD];
static int     TRGWidth[NBOARD];
static int     TestInput[NBOARD];
static float   PulserFreq[NBOARD];
static int     TDCThreshold[NBOARD][NCHAN];
static int     TRGThreshold[NBOARD][NCHAN];
static UINT32  ChannelMask[NBOARD];
static UINT32  ORMask[NBOARD];
static int     TRGDigitalWidth [NBOARD][NCHAN];
static int     TRGDigitalDelay [NBOARD][NCHAN];
static UINT16  bypMask;
static int     scalerRefPrescale[NBOARD];
static int     scalerFlags[NBOARD];

static int     DAQReadoutMask[NBOARD];



/******************************************************************/
/* functions to get info about slots set for daq- and tcp-readout */

int
dsc2GetNdsc_daq()
{
  return(Ndsc_daq);
}

int
dsc2Slot_daq(unsigned int id)
{
  if(id>=Ndsc_daq)
  {
    printf("%s: ERROR: Index (%d) >= dsc2s set for daq readout (%d).\n",__FUNCTION__,id,Ndsc_daq);
    return(ERROR);
  }

  return(dscID_daq[id]);
}

int
dsc2Id_daq(unsigned int slot)
{
  int id;

  for(id=0; id<Ndsc_daq; id++)
  {
    if(dscID_daq[id]==slot)
	{
      return(id);
	}
  }

  printf("%s: ERROR: dsc2 in slot %d does not exist or not set for daq readout\n",__FUNCTION__,slot);
  return(ERROR);
}

int
dsc2GetNdsc_tcp()
{
  return(Ndsc_tcp);
}

int
dsc2Slot_tcp(unsigned int id)
{
  if(id>=Ndsc_tcp)
  {
    printf("%s: ERROR: Index (%d) >= dsc2s set for tcpserver readout (%d).\n",__FUNCTION__,id,Ndsc_tcp);
    return(ERROR);
  }

  return(dscID_tcp[id]);
}

int
dsc2Id_tcp(unsigned int slot)
{
  int id;

  for(id=0; id<Ndsc_tcp; id++)
  {
    if(dscID_tcp[id]==slot)
	{
      return(id);
	}
  }

  printf("%s: ERROR: dsc2 in slot %d does not exist or not set for tcpserver readout\n",__FUNCTION__,slot);
  return(ERROR);
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/



/*
cd "/usr/local/clas12/coda/src/rol/VXWORKS_ppc/lib"
ld < librol.so
dsc2Init(0x100000,0x80000,20,0)
dsc2Config("")
dsc2Mon(0)
*/

static char *expid = NULL;

void
dsc2SetExpid(char *string)
{
  expid = strdup(string);
}


/* main function, have to be called after dsc2Init() */
int
dsc2Config(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  /* dsc2Init() must be called by now; get the number of boards from there */
  Ndsc = dsc2GetNdsc();

  printf("dsc2Config: Ndsc=%d\n",Ndsc);

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    dsc2UploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    dsc2InitGlobals();
  }

  /* read config file */
  if( (res = dsc2ReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  dsc2DownloadAll();

  return(0);
}


void
dsc2InitGlobals()
{
  int ii, jj;

  for(ii=0; ii<NBOARD; ii++)
  {
    TDCWidth[ii] = 40;
    TRGWidth[ii] = 40;
    TestInput[ii] = 0;
    PulserFreq[ii] = 0.0;
    for(jj=0; jj<NCHAN; jj++) TDCThreshold [ii][jj] = 20;
    for(jj=0; jj<NCHAN; jj++) TRGThreshold [ii][jj] = 40;
    ChannelMask[ii] = 0xffffffff;
    ORMask[ii] = 0xffffffff;
    for(jj=0; jj<NCHAN; jj++) TRGDigitalWidth [ii][jj] = 0;
    for(jj=0; jj<NCHAN; jj++) TRGDigitalDelay [ii][jj] = 0;

    scalerRefPrescale[ii] = 0;
    scalerFlags[ii] = DSC_SCALERCFG_RESET_AUTO;

    DAQReadoutMask[ii] = 0;
  }

}


#define DSC2_READ_MASKS(XXMASK,R_MSK,JJ_ADD) \
    else if(active && ((strcmp(keyword,XXMASK) == 0) && (kk >= 0)))	\
	{ \
      /*printf("PROCESSING >%s< >%s<\n",XXMASK,str_tmp);*/				\
	  args = sscanf (str_tmp, "%*s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", \
		       &msk[ 0], &msk[ 1], &msk[ 2], &msk[ 3], \
		       &msk[ 4], &msk[ 5], &msk[ 6], &msk[ 7], \
		       &msk[ 8], &msk[ 9], &msk[10], &msk[11], \
			   &msk[12], &msk[13], &msk[14], &msk[15]);				\
	  /*printf ("   args=%d, msk => %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",args, \
		  msk[ 0], msk[ 1], msk[ 2], msk[ 3], \
		  msk[ 4], msk[ 5], msk[ 6], msk[ 7], \
		  msk[ 8], msk[ 9], msk[10], msk[11], \
		  msk[12], msk[13], msk[14], msk[15]); \
		  printf("   slot1=%d slot2=%d\n",slot1,slot2);*/	\
      for(slot=slot1; slot<slot2; slot++) \
      { \
        /*printf("mask befor clear 0x%08x\n",(R_MSK) [slot]);*/ \
        (R_MSK) [slot] &= ~(0xFFFF<<JJ_ADD); /*clear corresponding part of the mask*/ \
        /*printf("mask after clear 0x%08x\n",(R_MSK) [slot]);*/ \
	    for(jj=0; jj<NCHAN; jj++) \
	    { \
	      if( (msk[jj] < 0) || (msk[jj] > 1) ) \
	      { \
	        printf("\nReadConfigFile: Wrong mask bit value, %d\n\n",msk[jj]); return(-4); \
	      } \
		  /*printf("msk[%d]=%d\n",jj,msk[jj]);*/		\
	      (R_MSK) [slot] |= (msk[jj]<<(jj+JJ_ADD));  /*set corresponding part of the mask*/ \
	    } \
        /*printf("mask after setting 0x%08x\n",(R_MSK) [slot]);*/ \
      } \
	}


/* reading and parsing config file */
/* config file will be selected using following rules:
     1. use 'filename' specified; if name starts from '/' or './', use it,
        otherwise use file from '$CLON_PARMS/dsc2/' area
     2. if name is "", use file 'hostname'.conf from '$CLON_PARMS/dsc2/' area
     3. if previous does not exist, use file '$EXPID.conf' from '$CLON_PARMS/dsc2/' area
 */
int
dsc2ReadConfigFile(char *filename)
{
  FILE   *fd;
  int    ii, jj, ch, kk = 0;
  char   str_tmp[STRLEN], str2[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  int    msk[NCHAN];
  int    args, i1, i2;
  float  f1;
  int    slot, slot1, slot2, chan;
  char fname[FNLEN] = { "" };  /* config file name */
  char *getenv();
  char *clonparms;

  gethostname(host,ROCLEN);  /* obtain our hostname */
  clonparms = getenv("CLON_PARMS");

  if(expid==NULL)
  {
    expid = getenv("EXPID");
    printf("\nNOTE: use EXPID=>%s< from environment\n",expid);
  }
  else
  {
    printf("\nNOTE: use EXPID=>%s< from CODA\n",expid);
  }

  if(strlen(filename)!=0) /* filename specified */
  {
    if ( filename[0]=='/' || (filename[0]=='.' && filename[1]=='/') )
	{
      sprintf(fname, "%s", filename);
	}
    else
	{
      sprintf(fname, "%s/dsc2/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/dsc2/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/dsc2/%s.cnf", clonparms, expid);
      if((fd=fopen(fname,"r")) == NULL)
      {
        printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
        return(-2);
	  }
	}

  }

  printf("\nReadConfigFile: Using configuration file >%s<\n",fname);


  /* Parsing of config file */
  active = 0;
  while ((ch = getc(fd)) != EOF)
  {
    if ( ch == '#' || ch == ' ' || ch == '\t' )
    {
      while (getc(fd) != '\n') {}
    }
    else if( ch == '\n' ) {}
    else
    {
      ungetc(ch,fd);
      fgets(str_tmp, STRLEN, fd);
      sscanf (str_tmp, "%s %s", keyword, ROC_name);
#ifdef DEBUG
      printf("\nfgets returns %s so keyword=%s\n\n",str_tmp,keyword);
#endif

      if(strcmp(keyword,"DSC2_CRATE") == 0)
      {
	    if(strcmp(ROC_name,host) == 0)
        {
	      printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
          active = 1;
        }
	    else if(strcmp(ROC_name,"all") == 0)
		{
	      printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
          active = 1;
		}
        else
		{
	      printf("\nReadConfigFile: crate = %s  host = %s - disactivated\n",ROC_name,host);
          active = 0;
		}
      }

      else if(active && (strcmp(keyword,"DSC2_SLOT")==0))
      {
        kk++;
        sscanf (str_tmp, "%*s %s", str2);
        /*printf("str2=%s\n",str2);*/
        if(isdigit(str2[0]))
        {
          slot1 = atoi(str2);
          slot2 = slot1 + 1;
          if(slot1<0 && slot1>21)
          {
            printf("\nReadConfigFile: Wrong slot number %d\n\n",slot1);
            return(-4);
          }
        }
        else if(!strcmp(str2,"all"))
        {
          slot1 = 0;
          slot2 = NBOARD;
        }
        else
        {
          printf("\nReadConfigFile: Wrong slot >%s<, must be 'all' or actual slot number\n\n",str2);
          return(-4);
        }
        /*printf("slot1=%d slot2=%d\n",slot1,slot2);*/
	  }

      DSC2_READ_MASKS("DSC2_TDCMASK",ChannelMask,0)

      DSC2_READ_MASKS("DSC2_TRGMASK",ChannelMask,16)

      DSC2_READ_MASKS("DSC2_TDCORMASK",ORMask,0)

      DSC2_READ_MASKS("DSC2_TRGORMASK",ORMask,16)

      else if(active && ((strcmp(keyword,"DSC2_WIDTH") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        for(slot=slot1; slot<slot2; slot++) TDCWidth[slot] = i1;
        for(slot=slot1; slot<slot2; slot++) TRGWidth[slot] = i2;
      }

      else if(active && ((strcmp(keyword,"DSC2_TEST_INPUT") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %d", &i1);
        for(slot=slot1; slot<slot2; slot++) TestInput[slot] = i1;
      }

      else if(!strcmp(keyword,"DSC2_PULSER"))
      {
        sscanf (str_tmp, "%*s %f", &f1);
        for(slot=slot1; slot<slot2; slot++) PulserFreq[slot] = f1;
      }

      else if(active && ((strcmp(keyword,"DSC2_THRESHOLD") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        for(slot=slot1; slot<slot2; slot++) for(chan=0; chan<NCHAN; chan++) TDCThreshold[slot][chan] = i1;
        for(slot=slot1; slot<slot2; slot++) for(chan=0; chan<NCHAN; chan++) TRGThreshold[slot][chan] = i2;
      }

      else if(active && ((strcmp(keyword,"DSC2_CH_THRESHOLD") == 0) && (kk >= 0)))
      {
        sscanf (str_tmp, "%*s %d %d %d", &chan, &i1, &i2);
		printf("--> slot1=%d slot2=%d chan=%d i1=%d i2=%d\n",slot1, slot2, chan, i1, i2);
        if((chan<0) || (chan>=NCHAN))
        {
          printf("\nReadConfigFile: Wrong channel number, %d\n\n",chan);
          return(-5);
        }
        for(slot=slot1; slot<slot2; slot++) TDCThreshold[slot][chan] = i1;
        for(slot=slot1; slot<slot2; slot++) TRGThreshold[slot][chan] = i2;

		/*
slot = 9;
for(jj=0;jj<NCHAN;jj++) printf("!!!      chan=%2d, TDCThreshold=%d, TRGThreshold=%d\n",jj,TDCThreshold[slot][jj],TRGThreshold[slot][jj]);
		*/



      }

      else if(active && ((strcmp(keyword,"DSC2_TRGDIGITAL") == 0) && (kk >= 0)))
      {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        for(slot=slot1; slot<slot2; slot++) for(chan=0; chan<NCHAN; chan++) TRGDigitalWidth[slot][chan] = i1;
        for(slot=slot1; slot<slot2; slot++) for(chan=0; chan<NCHAN; chan++) TRGDigitalDelay[slot][chan] = i2;
      }

      else if(active && ((strcmp(keyword,"DSC2_CH_TRGDIGITAL") == 0) && (kk >= 0)))
      {
        sscanf (str_tmp, "%*s %d %d %d", &chan, &i1, &i2);
        if((chan<0) || (chan>=NCHAN))
        {
          printf("\nReadConfigFile: Wrong channel number, %d\n\n",chan);
          return(-6);
        }
        for(slot=slot1; slot<slot2; slot++) TRGDigitalWidth[slot][chan] = i1;
        for(slot=slot1; slot<slot2; slot++) TRGDigitalDelay[slot][chan] = i2;
      }

      else if(active && ((strcmp(keyword,"DSC2_ENABLE_DAQ_READOUT") == 0) && (kk >= 0)))
      {
        sscanf (str_tmp, "%*s %d", &slot);
        if((slot<0) || (slot>=21))
        {
          printf("\nReadConfigFile: Wrong slot number, %d\n\n",slot);
          return(-6);
        }
        DAQReadoutMask[slot] = 1;
      }

      else if(active && ((strcmp(keyword,"DSC2_SCALER_REFPRESCALE") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %d", &i1);
        if(i1<1 || i1>65535)
		{
          printf("\nReadConfigFile: Wrong scaler reference prescale %d, must be between 1 and 65535\n\n",i1);
          return(-7);
        }
        for(slot=slot1; slot<slot2; slot++) scalerRefPrescale[slot] = i1-1;
      }

      else if(active && ((strcmp(keyword,"DSC2_SCALER_FLAGS") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %d", &i1);
        if(i1<0 || i1>2)
		{
          printf("\nReadConfigFile: Wrong scaler flags %d, must be between 0 and 2\n\n",i1);
          return(-7);
        }
        for(slot=slot1; slot<slot2; slot++) scalerFlags[slot] = i1;
      }



      else
      {
        ; /* unknown key - do nothing */
		/*
        printf("ReadConfigFile: Unknown Field or Missed Field in\n");
        printf("   %s \n", fname);
        printf("   str_tmp=%s", str_tmp);
        printf("   keyword=%s \n\n", keyword);
        return(-7);
		*/
      }

    }
  }
  fclose(fd);


  printf("dsc2ReadConfigFile !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

#ifdef DEBUG
  Ndsc = dsc2GetNdsc();
  for(ii=0; ii<Ndsc; ii++)
  {
    slot = dsc2Slot(ii);
    printf("\nslot=%2d\n",slot);
	printf("   TDCWidth=%d\n",TDCWidth[slot]);
	printf("   TRGWidth=%d\n",TRGWidth[slot]);
	printf("   TestInput=%d\n",TestInput[slot]);
	printf("   PulserFreq=%f\n",PulserFreq[slot]);
	printf("   ChannelMask=0x%08x\n",ChannelMask[slot]);
	printf("   ORMask=0x%08x\n",ORMask[slot]);
    for(jj=0;jj<NCHAN;jj++)
    {
      printf("      chan=%2d, TDCThreshold=%d, TRGThreshold=%d\n",
           jj,TDCThreshold[slot][jj],TRGThreshold[slot][jj]);
	}
    for(jj=0;jj<NCHAN;jj++)
    {
      printf("      chan=%2d, TRGDigitalWidth=%d, TRGDigitalDelay=%d\n",
           jj,TRGDigitalWidth[slot][jj],TRGDigitalDelay[slot][jj]);
	}
    printf("   scalerRefPrescale=%d\n",scalerRefPrescale[slot]);
    printf("   scalerFlags=%d\n",scalerFlags[slot]);
	printf("   daq readout=%d\n",DAQReadoutMask[slot]);
  }
#endif

  return(kk);
}





int
dsc2DownloadAll()
{
  int slot, chan, kk;
  UINT16 selMask;

  Ndsc = dsc2GetNdsc();
  for(kk=0; kk<Ndsc; kk++)
  {
    slot = dsc2Slot(kk);
    dsc2SetPulseWidth(slot, TDCWidth[slot], 1);
    dsc2SetPulseWidth(slot, TRGWidth[slot], 2);
    dsc2SetTestInput(slot, TestInput[slot]);
dsc2PulserSetup(slot, PulserFreq[slot], 0.5, 0xFFFFFFFF);
    dsc2SetChannelMask(slot,  ChannelMask[slot]&0xFFFF,      1); /* set tdc mask */
    dsc2SetChannelMask(slot, (ChannelMask[slot]>>16)&0xFFFF, 2); /* set trg mask */
    dsc2SetChannelORMask(slot,  ORMask[slot]&0xFFFF,      1); /* set tdc mask */
    dsc2SetChannelORMask(slot, (ORMask[slot]>>16)&0xFFFF, 2); /* set trg mask */
    for(chan=0; chan<NCHAN; chan++)
    {
      dsc2SetThreshold(slot,chan,TDCThreshold[slot][chan],1);
      dsc2SetThreshold(slot,chan,TRGThreshold[slot][chan],2);
    }

    selMask = 0xFFFF; /* by default, trigger output will use TRG discriminator */
    bypMask = 0xFFFF; /* by default, TRG discriminator output goes directly to trigger output */
    for(chan=0; chan<NCHAN; chan++)
    {
      dsc2SetTRGOut(slot, chan, TRGDigitalWidth[slot][chan], TRGDigitalDelay[slot][chan]);
      /* if TRGDigitalWidth[][]>0, set trigger output to the digital */
      if(TRGDigitalWidth[slot][chan]>0)
	  {
		/*printf("[%2d][%2d] befor 0x%04x\n",slot,chan,bypMask);*/
        bypMask = bypMask ^ (1<<chan);
		/*printf("[%2d][%2d] after 0x%04x\n",slot,chan,bypMask);*/
	  }
	}
	/*printf("[%2d] setup 0x%04x\n",slot,bypMask);*/
    dsc2SetTRGOutSource(slot, selMask, bypMask);
    dsc2SetScalerConfigRefPrescale(slot, scalerRefPrescale[slot]);
    dsc2SetScalerConfigFlags(slot, scalerFlags[slot]);
  }


  /* fills two additional arrays: for scalers daq readout, and for scalers tcpserver readout */
  Ndsc_daq = 0;
  Ndsc_tcp = 0;
  for(kk=0; kk<Ndsc; kk++)
  {
    slot = dsc2Slot(kk);

    if(DAQReadoutMask[slot])
	{
      dscID_daq[Ndsc_daq++] = slot;
	}
    else
	{
      dscID_tcp[Ndsc_tcp++] = slot;
	}
  }

  return(0);
}



/* print board registers; if slot is zero, print all boards */
void
dsc2Mon(int slot)
{
  int id, start, end, kk, jj;

  Ndsc = dsc2GetNdsc();
  if(slot==0)
  {
    start = 0;
    end = Ndsc;
  }
  else if((id = dsc2Id(slot)) >= 0)
  {
    start = id;
    end = start + 1;
  }
  else
  {
    return;
  }

  printf("Ndsc=%d\n",Ndsc);
  for(kk=start; kk<end; kk++)
  {
    slot = dsc2Slot(kk);
    printf("\nDSC2 number %d in slot %d\n",kk,slot);
    printf("   TDCWidth=%d(ns) TRGWidth=%d(ns)\n",
       dsc2GetPulseWidth(slot,1),
       dsc2GetPulseWidth(slot,2));
    printf("   TestInput=%d\n",
       dsc2GetTestInput(slot));
    printf("   PulserFreq=%f\n",dsc2GetPulserFreq(slot));
    printf("   ChannelMask=0x%08x ORMask=0x%08x \n",
       dsc2GetChannelMask(slot, 0),
       dsc2GetChannelORMask(slot, 0));
    for(jj=0;jj<NCHAN;jj++)
    {
      printf("      TDC_threshold[%2d]=%4d(mV)   TRG_threshold[%2d]=%4d(mV)\n",
	   jj, dsc2GetThreshold(slot,jj,1),
	   jj, dsc2GetThreshold(slot,jj,2));
    }
    bypMask = dsc2GetTRGOutSource(slot,1);
    for(jj=0;jj<NCHAN;jj++)
    {
      if( (bypMask&(1<<jj))==0 )
	  {
        printf("      TRGDigitalDelay[%2d]=%4d(ns)   TRGDigitalWidth[%2d]=%4d(ns) <- supersede TRGWidth\n",
	      jj, dsc2GetTRGOutDelay(slot,jj),
	      jj, dsc2GetTRGOutWidth(slot,jj));
	  }
    }
    printf("   scalerRefPrescale=%d\n",scalerRefPrescale[slot]);
    printf("   scalerFlags=%d\n",scalerFlags[slot]);
    printf("\n");
  }

  if(Ndsc_daq)
  {
    printf("Following %d slots are set for daq readout: ",Ndsc_daq);
    for(jj=0; jj<Ndsc_daq; jj++) printf(" %d",dscID_daq[jj]);
    printf("\n");
  }
  else
  {
    printf("There are no daq readout slots\n");
  }

  if(Ndsc_tcp)
  {
    printf("Following %d slots are set for tcpserver readout: ",Ndsc_tcp);
    for(jj=0; jj<Ndsc_tcp; jj++) printf(" %d",dscID_tcp[jj]);
    printf("\n");
  }
  else
  {
    printf("There are no tcpserver readout slots\n");
  }

}




/* upload setting from all found DSC2s */
int
dsc2UploadAll(char *string, int length)
{
  int slot, i, ii, jj, kk, len1, len2;
  char *str, sss[1024];
  unsigned int tmp, val[NCHAN], val1[NCHAN];
  unsigned short sval[NCHAN];

  Ndsc = dsc2GetNdsc();
  for(kk=0; kk<Ndsc; kk++)
  {
    slot = dsc2Slot(kk);

	TDCWidth[slot] = dsc2GetPulseWidth(slot,1);
    TRGWidth[slot] = dsc2GetPulseWidth(slot,2);
    TestInput[slot] = dsc2GetTestInput(slot);
    PulserFreq[slot] = dsc2GetPulserFreq(slot);

    ChannelMask[slot] = dsc2GetChannelMask(slot,1),
    ChannelMask[slot] |= (dsc2GetChannelMask(slot,2) << 16),

    ORMask[slot] = dsc2GetChannelORMask(slot,1);
    ORMask[slot] |= (dsc2GetChannelORMask(slot,2) << 16);

    for(jj=0; jj<NCHAN; jj++)
    {
      TDCThreshold[slot][jj] = dsc2GetThreshold(slot,jj,1);
      TRGThreshold[slot][jj] = dsc2GetThreshold(slot,jj,2);

      TRGDigitalDelay[slot][jj] = dsc2GetTRGOutDelay(slot,jj);
      TRGDigitalWidth[slot][jj] = dsc2GetTRGOutWidth(slot,jj);
	}

    bypMask = dsc2GetTRGOutSource(slot,1);

    scalerRefPrescale[slot] = dsc2GetScalerConfigRefPrescale(slot);
    scalerFlags[slot] = dsc2GetScalerConfigFlags(slot);
  }

  if(length)
  {
    str = string;
    str[0] = '\0';

    for(kk=0; kk<Ndsc; kk++)
    {
      slot = dsc2Slot(kk);
      sprintf(sss,"DSC2_SLOT %d\n",slot);
      ADD_TO_STRING;

      sprintf(sss,"DSC2_WIDTH %d %d\n",TDCWidth[slot],TRGWidth[slot]);
      ADD_TO_STRING;

      sprintf(sss,"DSC2_TEST_INPUT %d\n",TestInput[slot]);
      ADD_TO_STRING;

      sprintf(sss,"DSC2_PULSER %f\n",PulserFreq[slot]);
      ADD_TO_STRING;

      sprintf(sss,"DSC2_TDCMASK");
      ADD_TO_STRING;
      for(jj=0; jj<NCHAN; jj++)
	  {
        sprintf(sss," %d",(ChannelMask[slot]>>(15-jj))&0x1);
        ADD_TO_STRING;
      }
      sprintf(sss,"\n");
      ADD_TO_STRING;

      sprintf(sss,"DSC2_TDCORMASK");
      ADD_TO_STRING;
      for(jj=0; jj<NCHAN; jj++)
	  {
        sprintf(sss," %d",(ORMask[slot]>>(15-jj))&0x1);
        ADD_TO_STRING;
      }
      sprintf(sss,"\n");
      ADD_TO_STRING;

      sprintf(sss,"DSC2_TRGMASK");
      ADD_TO_STRING;
      for(jj=0; jj<NCHAN; jj++)
	  {
        sprintf(sss," %d",((ChannelMask[slot]>>16)>>(15-jj))&0x1);
        ADD_TO_STRING;
      }
      sprintf(sss,"\n");
      ADD_TO_STRING;

      sprintf(sss,"DSC2_TRGORMASK");
      ADD_TO_STRING;
      for(jj=0; jj<NCHAN; jj++)
	  {
        sprintf(sss," %d",((ORMask[slot]>>16)>>(15-jj))&0x1);
        ADD_TO_STRING;
      }
      sprintf(sss,"\n");
      ADD_TO_STRING;

      for(jj=0; jj<NCHAN; jj++)
      {
        sprintf(sss,"DSC2_CH_THRESHOLD %d %d %d\n",jj,TDCThreshold[slot][jj],TRGThreshold[slot][jj]);
        ADD_TO_STRING;
      }

      for(jj=0;jj<NCHAN;jj++)
      {
        if( (bypMask&(1<<jj))==0 )
	    {
          sprintf(sss,"DSC2_CH_TRGDIGITAL %d %d %d\n",jj,TRGDigitalDelay[slot][jj],TRGDigitalWidth[slot][jj]);
	    }
      }

      sprintf(sss,"DSC2_SCALER_REFPRESCALE %d\n",scalerRefPrescale[slot]+1);
      ADD_TO_STRING;

      sprintf(sss,"DSC2_SCALER_FLAGS %d\n",scalerFlags[slot]);
      ADD_TO_STRING;
	}

    CLOSE_STRING;
  }

}


int
dsc2UploadAllPrint()
{
  char str[16001];
  dsc2UploadAll(str, 16000);
  printf("%s",str);
}



#else /* dummy version*/

void
dsc2Config_dummy()
{
  return;
}

#endif
