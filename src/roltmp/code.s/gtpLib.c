
/* gtpLib.c */

#if defined(NIOS)

#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gtpLib.h"
#include "xxxConfig.h"

static int active;

static Gtp_regs *pGtpRegPtr;
static int fdGtpMem = 0; 

static int gtpSyncEventFlag;


static fd_set read_fds, write_fds, except_fds;
struct timeval timeout;
struct pollfd pollfdGtp;

unsigned int
gtpRead32(volatile unsigned int *addr)
{
  unsigned int rval;
  rval = *addr;
  return rval;
}

void
gtpWrite32(volatile unsigned int *addr, unsigned int val)
{
  *addr = val;
}

Gtp_regs *
gtpGetRegsPtr()
{
  int *pGtp = NULL;

  if(fdGtpMem > 0)
  {
  	close(fdGtpMem);
	fdGtpMem = 0;
  }

  fdGtpMem = open("/dev/uio0", O_RDWR);
  if(fdGtpMem < 1)
  {
    printf("ERROR: %s: failed to open /dev/uio0\n", __func__);
    return NULL;
  }

  /* Set isr data poll mode */ 
  pollfdGtp.fd = fdGtpMem;
  pollfdGtp.events = POLLIN;
	
  /* Set timeout */
  timeout.tv_sec = 0;
  timeout.tv_usec = 10000;


  pGtp = (int *)mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fdGtpMem, (off_t)0);
  if(pGtp == MAP_FAILED)
  {
    printf("ERROR: %s: mmap /dev/uio0 Gtp Mem failed\n", __func__);
    pGtp = NULL;
    return NULL;
  }

  printf("INFO: %s: GTP mapped @ CPU address 0x%08X\n", __func__, (unsigned int)pGtp);

  return (Gtp_regs *)pGtp;
}




int
gtpGetBlockLevel()
{
  /* Request block level setting from TI over TiGtp link */
  gtpWrite32(&pGtpRegPtr->TiGtp.LinkCtrl, 0x1);
  usleep(10000);

  return(gtpRead32(&pGtpRegPtr->TiGtp.BlockLength));
}


/*
  void gtpSetClock(int mode)
    mode: 0=disable, 1=vxs, 2=internal
*/
void
gtpSetClock(int mode)
{
  printf("INFO: %s: clock source = %d\n", __func__, mode);
  if(mode == 1)
  {
    gtpWrite32(&pGtpRegPtr->Clk.Ctrl, GTP_CLK_CTRL_P0	| GTP_CLK_CTRL_RESET);
    gtpWrite32(&pGtpRegPtr->Clk.Ctrl, GTP_CLK_CTRL_P0);
  }
  else if(mode == 2)
  {
    gtpWrite32(&pGtpRegPtr->Clk.Ctrl, GTP_CLK_CTRL_INT | GTP_CLK_CTRL_RESET);
    gtpWrite32(&pGtpRegPtr->Clk.Ctrl, GTP_CLK_CTRL_INT);
  }
  else
    gtpWrite32(&pGtpRegPtr->Clk.Ctrl, GTP_CLK_CTRL_RESET);

  usleep(10000);
  gtpGetClock();
}

void
gtpGetClock()
{
  int v;

  printf("INFO: %s\n", __func__);

  v = gtpRead32(&pGtpRegPtr->Clk.Ctrl);
  if((v & 0x3) == 1)
    printf("Gtp clock source = VXS\n");
  else if((v & 0x3) == 2)
    printf("Gtp clock source = Internal\n");
  else
  {
    printf("Gtp clock source = Disabled\n");
    return;
  }

  v = gtpRead32(&pGtpRegPtr->Clk.Status);
  if(v & 0x1) printf("  GCLK PLL Locked");
  else        printf("  ERROR: GCLK PLL not Locked");

  if(v & 0x2) printf("  ATX L PLL Locked");
  else        printf("  ERROR: ATX L PLL not Locked");

  if(v & 0x4) printf("  ATX R PLL Locked");
  else        printf("  ERROR: ATX R PLL not Locked");
}

/*
  void gtpSetSync(int mode)
    mode: GTP_SD_SRC_SEL_*
*/
void
gtpSetSync(int mode)
{
  printf("INFO: %s: sync source = %d\n", __func__, mode);
  gtpWrite32(&pGtpRegPtr->Sd.SrcSel[GTP_SD_SRC_SYNC], mode);
  gtpGetSync();
}

void
gtpGetSync()
{
  const char *src_names[] = {
    "GTP_SD_SRC_SEL_0",
    "GTP_SD_SRC_SEL_1",
    "GTP_SD_SRC_SEL_SYNC",
    "GTP_SD_SRC_SEL_TRIG1",
    "GTP_SD_SRC_SEL_TRIG2",
    "GTP_SD_SRC_SEL_LVDSIN",
    "GTP_SD_SRC_SEL_PULSER",
    "GTP_SD_SRC_SEL_BUSY",
    "GTP_SD_SRC_SEL_TRIGGER"
  };

  int v;

  printf("INFO: %s\n", __func__);

  v = gtpRead32(&pGtpRegPtr->Sd.SrcSel[GTP_SD_SRC_SYNC]) & 0x3F;
  printf("Gtp sync source = ");
  if(v < 4)
    printf("%s\n", src_names[v]);
  else if(v < 9)
    printf("%s%d\n", src_names[5], v-5);
  else if((v >= 9) && (v <= 10))
    printf("%s\n", src_names[v-9+6]);
  else
    printf("%s%d\n", src_names[8], v-32);
}

void
gtpPayloadTriggerStatus()
{
  int i, payload, status, errors, status2;
  const char *port_names[] = {
    "Slot10-PP1", "Slot13-PP2", "Slot9-PP3", "Slot14-PP4", "Slot8-PP5", "Slot15-PP6", "Slot7-PP7", "Slot16-PP8",
    "Slot6-PP9", "Slot17-PP10", "Slot5-PP11", "Slot18-PP12", "Slot4-PP13", "Slot19-PP14", "Slot3-PP15", "Slot20-PP16"
  };

  int slot_to_payload[] = {
    0,0,15,13,11,9,7,5,3,1,0,0,2,4,6,8,10,12,14,16
  };

  printf("gtpPayloadTriggerStatus()\n");
  printf("%-11s %-9s %-12s %-11s\n", "Port", "ChannelUp", "Latency(ns)", "Errors");

  for(i = 0; i < sizeof(slot_to_payload)/sizeof(slot_to_payload[0]); i++)
  {
    payload = slot_to_payload[i];
    if(payload)
    {
      status = gtpRead32(&pGtpRegPtr->Ser[payload-1].Status);
      errors = gtpRead32(&pGtpRegPtr->Ser[payload-1].ErrTile0);
      status2 = gtpRead32(&pGtpRegPtr->Ser[payload-1].Status2);
      printf("%-11s %-9d %-12d %-5d %-5d\n", port_names[payload-1], (status & 0x1000)>>12, (status2 & 0xFFFF)*4, errors & 0xFFFF, errors>>16);
    }
  }
}

void
gtpFiberTriggerStatus()
{
  int i, payload, status, errors, errors2;
  printf("gtpFiberTriggerStatus()\n");
  printf("%-11s %-9s %-22s\n", "Port", "ChannelUp", "Errors");

  status = gtpRead32(&pGtpRegPtr->QsfpSer.Status);
  errors = gtpRead32(&pGtpRegPtr->QsfpSer.ErrTile0);
  errors2 = gtpRead32(&pGtpRegPtr->QsfpSer.ErrTile1);
  printf("%-11s %-9d %-5d %-5d %-5d %-5d\n", "QSFP", (status & 0x1000)>>12, errors & 0xFFFF, errors>>16, errors2 & 0xFFFF, errors2>>16);
}

void
gtpFiberTriggerEnable()
{
  int i;

  printf("gtpFiberTriggerEnable\n");

  for(i = 0; i < 10; i++)
  {
    if(!(gtpRead32(&pGtpRegPtr->QsfpSer.Status) & 0x1000))
	 {
      gtpWrite32(&pGtpRegPtr->QsfpSer.Ctrl, 0x401);
	    /* release resets */
      gtpWrite32(&pGtpRegPtr->QsfpSer.Ctrl, 0x0);
	 }
    else
      break;

    /* delay to allow channel to lock */
    sleep(1);
  }

  if(!(gtpRead32(&pGtpRegPtr->QsfpSer.Status) & 0x1000))
    printf("*** ERROR: FIBER LINK NOT UP!!! ***\n");

  /* enable error counters */
  gtpWrite32(&pGtpRegPtr->QsfpSer.Ctrl, 0x800);

  /* print status of ports */
  gtpFiberTriggerStatus();
}

void
gtpFiberTriggerReset()
{
  int i;

  printf("gtpFiberTriggerReset\n");

  gtpWrite32(&pGtpRegPtr->QsfpSer.Ctrl, 0x401);
}

void
gtpPayloadTriggerEnable(int mask)
{
  int i, j, allup = 1;

  printf("gtpPayloadTriggerEnable payload 0x%04X\n", mask);

  for(j = 0; j < 10; j++)
  {
    allup = 1;
	 
    /* release resets */
    for(i = 0; i < 16; i++)
    {
      if(!(mask & (1<<i)))
        continue;
      gtpWrite32(&pGtpRegPtr->Ser[i].Ctrl, 0x0);
    }

    /* delay to allow channel to lock */
    usleep(20000);

    for(i = 0; i < 16; i++)
    {
      if(!(mask & (1<<i)))
        continue;
	 
	   if(!(gtpRead32(&pGtpRegPtr->Ser[i].Status) & 0x1000))
		{
	     gtpWrite32(&pGtpRegPtr->Ser[i].Ctrl, 0x401);
		  allup = 0;
		}
    }
    
    if(allup)
      break;
	 
    sleep(1);
  }
  
  if(!allup)
    printf("*** ERROR: NOT ALL FADC LINKS UP!!! ***\n");
  
  /* enable error counters */
  for(i = 0; i < 16; i++)
  {
    if(!(mask & (1<<i)))
      continue;
    gtpWrite32(&pGtpRegPtr->Ser[i].Ctrl, 0x800);
  }

  /* enable payloads in trigger processing */
  gtpWrite32(&pGtpRegPtr->Trg.Ctrl, mask);

  /* print status of ports */
  gtpPayloadTriggerStatus();
}

void
gtpPayloadTriggerReset(int mask)
{
  int i;

  printf("gtpPayloadTriggerReset payload 0x%04X\n", mask);

  for(i = 0; i < 16; i++)
  {
    if(!(mask & (1<<i)))
      continue;
    gtpWrite32(&pGtpRegPtr->Ser[i].Ctrl, 0x401);
  }
}

void
gtpSetHpsPulseCoincidence(int ticks_before, int ticks_after)
{
  if((ticks_before < 0) || (ticks_before > 4) || (ticks_after < 0) || (ticks_after > 4))
  {
    printf("ERROR: %s - ticks out of range: before %d, after %d\n", __func__, ticks_before, ticks_after);
    return;
  }
  printf("gtpSetHpsPulseCoincidence(%d, %d)\n", ticks_before, ticks_after);
  gtpWrite32(&pGtpRegPtr->Trg.ClusterPulseCoincidence, ticks_before | (ticks_after<<16));
}

int
gtpGetHpsPulseCoincidenceBefore()
{
  return gtpRead32(&pGtpRegPtr->Trg.ClusterPulseCoincidence) & 0x7;
}

int
gtpGetHpsPulseCoincidenceAfter()
{
  return (gtpRead32(&pGtpRegPtr->Trg.ClusterPulseCoincidence)>>16) & 0x7;
}

void
gtpSetHpsPulseThreshold(int threshold)
{
  if((threshold < 0) || (threshold > 8191))
  {
    printf("ERROR: %s - threshold out of range: %d\n", __func__, threshold);
    return;
  }
  gtpWrite32(&pGtpRegPtr->Trg.ClusterPulseThreshold, threshold);
}

int
gtpGetHpsPulseThreshold()
{
  return gtpRead32(&pGtpRegPtr->Trg.ClusterPulseThreshold);
}

/*******************************************************************************
 *
 *  gtpIntAck
 *  - Acknowledge an interrupt or latched trigger.  This "should" effectively 
 *  release the "Busy" state of the TI-GTP.
 *  Execute a user defined routine, if it is defined.  Otherwise, use
 *  a default prescription.
 *
 */

void
gtpIntAck()
{
/*  int resetbits=0;*/
  if(pGtpRegPtr == NULL) {
    printf("gtpIntAck: ERROR: GTP not initialized\n");
    return;
  }

  /*printf("gtpIntAck reached\n");*/

  /* Send ack from GTP->TI over TiGtp link */
  gtpWrite32(&pGtpRegPtr->TiGtp.LinkCtrl, 0x2);
#if 0
  if (tiAckRoutine != NULL)
    {
      /* Execute user defined Acknowlege, if it was defined */
      TILOCK;
      (*tiAckRoutine) (tiAckArg);
      TIUNLOCK;
    }
  else
    {
      TILOCK;
      tiDoAck = 1;
      tiAckCount++;
      resetbits = TI_RESET_BUSYACK;

      if(!tiReadoutEnabled)
	{
	  /* Readout Acknowledge and decrease the number of available blocks by 1 */
	  resetbits |= TI_RESET_BLOCK_READOUT;
	}
      
      if(tiDoSyncResetRequest)
	{
	  resetbits |= TI_RESET_SYNCRESET_REQUEST;
	  tiDoSyncResetRequest=0;
	}

      vmeWrite32(&TIp->reset, resetbits);
      TIUNLOCK;
    }
#endif
}

void
gtpTiGtpLinkReset()
{
  int result;
  unsigned int val;

  if(pGtpRegPtr == NULL) {
    printf("gtpTiGtpLinkReset: ERROR: GTP not initialized\n");
    return;
  }

  gtpWrite32(&pGtpRegPtr->TiGtp.LinkReset, 0x7);
  gtpWrite32(&pGtpRegPtr->TiGtp.LinkReset, 0x5);
  usleep(20000);

  gtpWrite32(&pGtpRegPtr->TiGtp.LinkReset, 0x4);
  gtpWrite32(&pGtpRegPtr->TiGtp.LinkReset, 0x0);
  usleep(20000);

  /* will be called again in Go() transition */
  gtpTiGtpFifoReset();

  result = gtpRead32(&pGtpRegPtr->TiGtp.LinkStatus);
  if(!(result & 0x10000))
  {
    printf("gtpTiGtpLinkReset: ERROR: TiGtp not up!!!\n");
    return;
  }
}

void
gtpTiGtpFifoReset()
{
  gtpWrite32(&pGtpRegPtr->TiGtp.FifoCtrl, 0x1);
  gtpWrite32(&pGtpRegPtr->TiGtp.FifoCtrl, 0x0);
  gtpWrite32(&pGtpRegPtr->TiGtp.LinkCtrl, 0x4); /* clear sync flag */

  gtpClearIntData();
}

void
gtpTiStatus()
{
  int status[2];

  status[0] = gtpRead32(&pGtpRegPtr->TiGtp.FifoLenStatus);
  status[1] = gtpRead32(&pGtpRegPtr->TiGtp.FifoDataStatus);
  status[2] = gtpRead32(&pGtpRegPtr->TiGtp.LinkStatus);

  printf("GTP: SyncFlag = %d, Fifo Length = %d(0x%08x), Data Status = 0x%08x, Link Status = 0x%08x\n",
		 status[0]>>31, status[0] & 0x7FFFFFFF,status[0] & 0x7FFFFFFF,status[1],status[2]);
}


int
gtpReadBlock(volatile unsigned int *data, int nwrds, int rflag)
{
  int len, ii, status1, status2;

  if(pGtpRegPtr == NULL)
  {
    printf("gtpReadBlock: ERROR: GTP not initialized\n");
    return;
  }

  status1 = gtpRead32(&pGtpRegPtr->TiGtp.FifoLenStatus) & 0x7FFFFFFF; /* bit 31 is sync flag */
  status2 = gtpRead32(&pGtpRegPtr->TiGtp.FifoDataStatus);
  len = gtpRead32(&pGtpRegPtr->TiGtp.FifoLen);
  /*len++;*/ /* TEMPORARY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  if(len > nwrds)
  {
    printf("gtpReadBlock ERROR: output buffer length %d words is not enough (need %d words)\n",nwrds,len);
    len = nwrds;
  }

  /*printf("statuses: 0x%08x 0x%08x (%d %d)\n",status1,status2,status1,status2);*/
  for(ii=0; ii<len; ii++)
  {
    *data++ = gtpRead32(&pGtpRegPtr->TiGtp.FifoData);
  }

  return(len);
}

unsigned int
gtpBReady()
{
  unsigned int blockBuffer, rval;
  int gtpSyncEventReceived;

  if(pGtpRegPtr == NULL) {
    printf("gtpBReady: ERROR: GTP not initialized\n");
    return;
  }

  blockBuffer = gtpRead32(&pGtpRegPtr->TiGtp.FifoLenStatus);

  rval = blockBuffer & 0x1FF;
  gtpSyncEventReceived = (blockBuffer>>31) & 0x1;

  if( (rval==1) && (gtpSyncEventReceived) )
  {
    gtpSyncEventFlag = 1;
    gtpWrite32(&pGtpRegPtr->TiGtp.LinkCtrl, 0x4); /* clear sync flag */
  }
  else
    gtpSyncEventFlag = 0;

  return(rval);
}




int
gtpGetSyncEventFlag()
{
  int rval=0;
  
  rval = gtpSyncEventFlag;

  return rval;
}





void
gtpClearIntData()
{
  int i, isr_num;
  for(i = 0; i < 1000; i++)
  {
    pollfdGtp.revents = 0;
    poll(&pollfdGtp, 1, 0);
    if(pollfdGtp.revents & POLLIN)
      read(fdGtpMem, &isr_num, sizeof(isr_num));
	 else
      break;
  }
}

int
gtpWaitForInt()
{
  int isr_num;

  /* Wait for input to become ready or until the time out(10ms) */
  pollfdGtp.revents = 0;
  poll(&pollfdGtp, 1, 10);
  if(pollfdGtp.revents & POLLIN)
  {
    read(fdGtpMem, &isr_num, sizeof(isr_num)); /*need it ???*/
    return(1); /* fd is ready for reading */
  }
  return(0);/* timeout or error */
}

void
gtpEnableInt(int en)
{
  write(fdGtpMem, &en, 4);
}

int
gtpInit(int flag)
{
  int v;

  pGtpRegPtr = gtpGetRegsPtr();
  if(!pGtpRegPtr) return -1;

  v = gtpRead32(&pGtpRegPtr->Cfg.BoardId);
  if(v != GTP_BOARDID)
  {
     printf("ERROR: %s: invalid board id = 0x%08X\n", v);
     return -1;
  }
  printf("Gtp board id = 0x%08X\n", v);

  v = gtpRead32(&pGtpRegPtr->Cfg.FirmwareRev);
  printf("Gtp firmware revision = %d.%d, type = ", (v>>0)&0xFF, (v>>8)&0xFF);
  if((v>>16) == GTP_TYPE_HALLD)
    printf("HallD\n");
  else if((v>>16) == GTP_TYPE_HPS)
    printf("HPS\n");
  else
  {
    printf("***** Unknown *****\nERROR: %s: unknown GTP firmware type\n", __func__);
    return -1;
  }
  
  gtpEnableInt(0);
}





/*****************************************************************************/
typedef struct
{
  int ClusterPulseCoincidenceBefore;
  int ClusterPulseCoincidenceAfter;
  int ClusterPulseThreshold;
} GTP_CONFIG_STRUCT;

GTP_CONFIG_STRUCT gtp;


static char *expid = NULL;

void
gtpSetExpid(char *string)
{
  expid = strdup(string);
}

void
gtpInitGlobals()
{
  gtp.ClusterPulseCoincidenceBefore = 1;
  gtp.ClusterPulseCoincidenceAfter = 1;
  gtp.ClusterPulseThreshold = 50;
  
  return;
}



#define NCHAN 8
#define SCAN_MSK \
	args = sscanf (str_tmp, "%*s %d %d %d %d %d %d %d %d   \
                                     %d %d %d %d %d %d %d %d", \
		       &msk[ 0], &msk[ 1], &msk[ 2], &msk[ 3], \
		       &msk[ 4], &msk[ 5], &msk[ 6], &msk[ 7], \
		       &msk[ 8], &msk[ 9], &msk[10], &msk[11], \
		       &msk[12], &msk[13], &msk[14], &msk[15])


int
gtpReadConfigFile(char *filename)
{
  FILE   *fd;
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch;
  char   str_tmp[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  char   str2[2];
  int    args, i1, i2, i3, i4, msk[NCHAN];
  float  f1;
  int    slot, chan;
  unsigned int  ui1, ui2;
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
      sprintf(fname, "%s/gtp/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\ngtpReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/gtp/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/gtp/%s.cnf", clonparms, expid);
      if((fd=fopen(fname,"r")) == NULL)
      {
        printf("\ngtpReadConfigFile: Can't open config file >%s<\n",fname);
        return(-2);
	  }
	}

  }
  printf("\ngtpReadConfigFile: Using configuration file >%s<\n",fname);

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
      fgets(str_tmp, /*80*/STRLEN, fd); /* sergey: NIOS cannot handle long lines ??? */
      sscanf (str_tmp, "%s %s", keyword, ROC_name);


      /* Start parsing real config inputs */
      if(strcmp(keyword,"GTP_CRATE") == 0)
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

      else if(active && (strcmp(keyword,"GTP_CLUSTER_PULSE_COIN")==0))
      {
        i1 = i2 = -1;
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
		  gtp.ClusterPulseCoincidenceBefore = i1;
		  gtp.ClusterPulseCoincidenceAfter = i2;
      }

      else if(active && (strcmp(keyword,"GTP_CLUSTER_PULSE_THRESHOLD")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
		  gtp.ClusterPulseThreshold = i1;
      }

      else
      {
        ; /* unknown key - do nothing */
		/*
        printf("gtpReadConfigFile: Unknown Field or Missed Field in\n");
        printf("   %s \n", fname);
        printf("   str_tmp=%s", str_tmp);
        printf("   keyword=%s \n\n", keyword);
        return(-10);
		*/
      }

    }
  } /* end of while */

  fclose(fd);

  return(0);
}

int
gtpDownloadAll()
{
  gtpSetHpsPulseCoincidence(gtp.ClusterPulseCoincidenceBefore, gtp.ClusterPulseCoincidenceAfter);
  gtpSetHpsPulseThreshold(gtp.ClusterPulseThreshold);

  return(0);
}

/* gtpInit() have to be called before this function */
int  
gtpConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    gtpUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    gtpInitGlobals();
  }

  /* read config file */
  if( (res = gtpReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  gtpDownloadAll();

  return(0);
}

void
gtpMon(int slot)
{
  int id, ii, start, end, res;
  int coin_before, coin_after;
  int cluster_threshold;

  coin_before = gtpGetHpsPulseCoincidenceBefore();
  coin_after = gtpGetHpsPulseCoincidenceAfter();
  cluster_threshold = gtpGetHpsPulseThreshold();
  
  printf("GTP HPS cluster threshold: %dMeV\n", cluster_threshold);
  printf("GTP HPS cluster coincidence: -%dns to +%dns\n", coin_before*4, coin_after*4);
  
  gtpFiberTriggerStatus();
  gtpPayloadTriggerStatus();
}



/* upload setting from all found DSC2s */
int
gtpUploadAll(char *string, int length)
{
  int slot, i, ii, jj, kk, ifiber, len1, len2;
  char *str, sss[1024];
  unsigned int tmp, connectedfibers;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];

  gtp.ClusterPulseCoincidenceBefore = gtpGetHpsPulseCoincidenceBefore();
  gtp.ClusterPulseCoincidenceAfter = gtpGetHpsPulseCoincidenceAfter();
  gtp.ClusterPulseThreshold = gtpGetHpsPulseThreshold();

  if(length)
  {
    str = string;
    str[0] = '\0';

    sprintf(sss,"GTP_CLUSTER_PULSE_COIN %d %d\n", gtp.ClusterPulseCoincidenceBefore, gtp.ClusterPulseCoincidenceAfter); ADD_TO_STRING;
    sprintf(sss,"GTP_CLUSTER_PULSE_THRESHOLD %d\n", gtp.ClusterPulseThreshold); ADD_TO_STRING;

    CLOSE_STRING;
  }
}

int
gtpUploadAllPrint()
{
  char str[1025];
  gtpUploadAll(str, 1024);
  printf("%s",str);
}


#else /* dummy version*/

void
gtpLib_dummy()
{
  return;
}

#endif


