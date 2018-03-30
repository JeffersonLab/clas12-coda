#if defined(VXWORKS) || defined(Linux_vme)

/*****************************************************************************
 *
 *  tdc1190.c - Driver library for readout of C.A.E.N. Model 1190
 *                  TDC using a VxWorks 5.2 or later based Single Board computer.
 *
 *  Author: David Abbott 
 *          Jefferson Lab Data Acquisition Group
 *          August 2002
 *
 *  Revision  1.0 - Initial Revision
 *                    - Supports up to 20 CAEN Model 1190s in a Crate
 *                    - Programmed I/O reads
 *                    - Interrupts from a Single 1190
 *
 *  adjusted for CLAS by Sergey Boyarinov - May 2003-present
 *
 *  Ported to Linux by Bryan Moffit - Jan 2010
 *
 *    - Linked List DMA supported
 *       vxworks: requires modified tsi148 DMA library 
 *
 * SVN: $Rev: 416 $
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#ifdef VXWORKS
#include <vxWorks.h>
#include <logLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <vxLib.h>
#else
#include "jvme.h"
#endif

/*
#ifdef Linux_vme
#ifndef VOIDFUNCPTR
typedef void (*VOIDFUNCPTR) ();
#endif
#endif
*/

#include "tdc1190.h"
#include "xxxConfig.h"


/* Prototype of Common Initialization of Modules */
STATUS tdc1190CommonInit(int itdc, UINT32 laddr);

/* Macro to check id and c1190p */
#define CHECKID(id) { \
    if((id<0) || (c1190p[id] == NULL)) { \
      logMsg("%s: ERROR : TDC id %d not initialized \n", \
	     (int)__FUNCTION__,id,3,4,5,6);	\
      return ERROR;	\
    }  \
    if(use1190[id]==0) \
    { \
	  logMsg("%s: TDC id %d flagged to NOT be used.\n", \
	       (int)__FUNCTION__,id,3,4,5,6); \
	  return ERROR; \
    } \
  }

#ifdef VXWORKS
/* Define external Functions */
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT  STATUS intDisconnect(int);
IMPORT  STATUS sysIntEnable(int);
IMPORT  STATUS sysIntDisable(int);
#endif

#ifdef VXWORKS
#define EIEIO    __asm__ volatile ("eieio")
#define SynC     __asm__ volatile ("sync")
#else
#define EIEIO    
#define SynC     
#endif

/* Mutex to hold of reads and writes from competing threads */
pthread_mutex_t c1190_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_1190 {				\
    if(pthread_mutex_lock(&c1190_mutex)<0)	\
      perror("pthread_mutex_lock");	\
  }
#define UNLOCK_1190 {				\
    if(pthread_mutex_unlock(&c1190_mutex)<0)	\
      perror("pthread_mutex_unlock");	\
  }

/* Define global variables */

unsigned int tdcAddrOffset = 0;                /* Difference in CPU (USERSPACE) Base */

int Nc1190 = 0;                              /* Number of TDCs in crate */

volatile struct v1190_struct *c1190p[V1190_MAX_MODULES];    /* pointers to TDC memory map */
static unsigned int c1190vme[V1190_MAX_MODULES]; /* jumper addresses for Dma */

static int use1190[V1190_MAX_MODULES] = {           /* Switch to turn on (1) or off (0) specific TDCs */
  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0};

volatile struct v1190_struct *c1190MCSTp;    /* pointer to TDC MCST memory map */
int tdcMaxSlot=-1;                           /* Highest element in use1190 hold an TDC */
int tdcMinSlot=-1;                           /* Lowest element in use1190 holding an TDC */

/* Define Interrupts variables */
BOOL              c1190IntRunning = FALSE; /* running flag */
int               c1190IntID      = -1; /* id number of TDC generating interrupts */
LOCAL VOIDFUNCPTR c1190IntRoutine = NULL; /* user interrupt service routine */
LOCAL int         c1190IntArg     = 0; /* arg to user routine */
LOCAL int         c1190IntEvCount = 0; /* Number of Events to generate Interrupt */
LOCAL UINT32      c1190IntLevel   = V1190_VME_INT_LEVEL; /* default VME interrupt level */
LOCAL UINT32      c1190IntVec     = V1190_INT_VEC; /* default interrupt Vector */
int               c1190IntCount   = 0; /* Count of interrupts from TDC */


/*Sergey*/

static int active;

/* window parameters */
static int window_width  = 4600;
static int window_offset = -4000;
static int window_extra  = 25;
static int window_reject = 50;

/* readout options */
static int cblt_not, a24_a32, sngl_blt_mblt, berr_fifo, sst_rate;
static int blt_Events, n_Hits, almostFullLevel, outProgControl;

/* variable filled by DmaStart and used by DmaDone */
static int nbytes_save[21];
/*Sergey*/

/*S.P.*/
static TDC1290_CONF tdc[NBOARD]; /*index in that array is slot number */
/*S.P.*/


/*******************************************************************************
 *
 * tdc1190Init - Initialize tdc1190 Library. 
 *
 *
 * RETURNS: the number of tdc boards detected, or ERROR if the address is invalid
 *   or board is not present.
 */

int
tdc1190Init(UINT32 addr, UINT32 addr_inc, int ntdc, int iFlag)
{
  int ii, jj, itdc, res=0, errFlag = 0;
  unsigned short rdata=0;
  int boardID = 0;
  unsigned int laddr;
  volatile struct v1190_ROM_struct *rp;
  char boardmodel[3][7] = {"v1190","v1290","v1290N"};

  /* Check for valid address */
  if(addr==0) 
  {
    printf("tdc1190Init: ERROR: Must specify a Bus (VME-based A32/A24) address for TDC 0\n");
    return(ERROR);
  }
  else if(addr < 0x00ffffff) /* A24 Addressing */
  {
    if((addr_inc==0)||(ntdc==0)) ntdc = 1; /* assume only one TDC to initialize */

    /* get the TDCs address */
#ifdef VXWORKS
    res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
    if (res != 0) 
	{
	  printf("tdc1190Init: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
	  return(ERROR);
	}
    tdcAddrOffset = (unsigned int)laddr-(unsigned int)addr;
#else
    res = vmeBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
    if (res != 0) 
	{
	  printf("tdc1190Init: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
	  return(ERROR);
	}
    tdcAddrOffset = (unsigned int)laddr-(unsigned int)addr;
#endif
  }
  else /* A32 Addressing */
  {
    if((addr_inc==0)||(ntdc==0)) ntdc = 1; /* assume only one TDC to initialize */

    /* get the TDC address */
#ifdef VXWORKS
    res = sysBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
    if (res != 0) 
	{
	  printf("tdc1190Init: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",addr);
	  return(ERROR);
	}
    tdcAddrOffset = (unsigned int)laddr-(unsigned int)addr;
#else
    res = vmeBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
    if (res != 0) 
	{
	  printf("tdc1190Init: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",addr);
	  return(ERROR);
	}
    tdcAddrOffset = (unsigned int)laddr-(unsigned int)addr;
#endif
  }

  /*sergey: ii -> Nc1190 in two places below*/
  Nc1190 = 0;
  for(ii=0; ii<ntdc; ii++) 
  {
    if(tdc1190CommonInit(Nc1190/*ii*/,laddr + ii*addr_inc) == OK)
	{
      c1190vme[Nc1190/*ii*/] = /*0x08000000 +*/ addr + ii*addr_inc;
      Nc1190++;
      itdc = Nc1190-1;
      printf("Initialized TDC %s with ID=%d, DMA address 0x%08x, USER address 0x%08x in SLOT=%d\n",
		boardmodel[use1190[itdc]-1],itdc,(UINT32)c1190vme[itdc],(UINT32)c1190p[itdc],tdc1190Slot(itdc));
	}
	else
	{
      /*break;*/
	}
  }

  /* Disable/Clear all TDCs */
  for (ii=0; ii<Nc1190; ii++) 
  {
    if(!use1190[ii]) continue;
      
    /* Determine the first TDC for MCST or CBLT */
    if (tdcMinSlot==-1) tdcMinSlot = ii;

    /* Determine the last TDC for MCST or CBLT */
    tdcMaxSlot = ii;

    vmeWrite16(&(c1190p[ii]->moduleReset),1);
    vmeWrite16(&(c1190p[ii]->clear),1);
  }

  printf("Fount %d tdc1190 or/and tdc1290 boards\n",Nc1190);

  return(Nc1190);
}


/*******************************************************************************
 *
 * tdc1190CommonInit - Called by tdc1190Init or tdc1190InitList.
 *                     Initialization of individual modules, checking
 *                     for a valid VME address and BOARD_ID and 
 *                     Firmware Version.
 *
 * RETURNS: OK, or ERROR if the address is invalid or board is not present.
 */

STATUS
tdc1190CommonInit(int itdc, UINT32 laddr)
{
  int ii, res=0, errFlag = 0;
  unsigned short rdata=0;
  int boardID = 0;
  int boardVersion = 0;
  volatile struct v1190_ROM_struct *rp;

  c1190p[itdc] = (struct v1190_struct *)(laddr);

  /* Check if Board exists at that address */
#ifdef VXWORKS
  res = vxMemProbe((char *) &(c1190p[itdc]->firmwareRev),0,2,(char *)&rdata);
#else
  res = vmeMemProbe((char *) &(c1190p[itdc]->firmwareRev),2,(char *)&rdata);
#endif
  if(res < 0 )
  /*       if(res < 0 || rdata==0xffff)  */
  {
    printf("tdc1190CommonInit: ERROR: No addressable board at addr=0x%x\n",
        (UINT32) c1190p[itdc]);
    c1190p[itdc] = NULL;
    use1190[itdc] = 0;
    errFlag = 1;
    return ERROR;
  } 
  else 
  {
    /* Check if this is a Model 1190/1290 */
    rp = (struct v1190_ROM_struct *)((UINT32)c1190p[itdc] + V1190_ROM_OFFSET);
    boardVersion = vmeRead16(&(rp->vers)) & 0xff;
    boardID = ((vmeRead16(&(rp->board2))&(0xff))<<16) +
	  ((vmeRead16(&(rp->board1))&(0xff))<<8) +
	  (vmeRead16(&(rp->board0))&(0xff));
    if(((boardID&0xffff) != V1190_BOARD_ID) && ((boardID&0xffff) != V1290_BOARD_ID)) 
	{
	  printf("tdc1190CommonInit: ERROR: Board ID does not match: 0x%x \n",boardID);
	  use1190[itdc] = 0;
	  return ERROR;
	}
    else
	{
      printf(">>> Found board with BOARD_ID=0x%08x, firmware=0x%08x\n",boardID,rdata);
	}

    /* Check if this is the firmware we expect V1190_FIRMWARE_REV or V1190_FIRMWARE_REV+1 */
    if( (rdata != V1190_FIRMWARE_REV) && (rdata != (V1190_FIRMWARE_REV+1)) && (rdata != 0xc) && (rdata != 0x11) )
	{
	  printf("WARN: Firmware does not match: 0x%08x (expected 0x%08x) (laddr=0x%08x)\n",
			 rdata,V1190_FIRMWARE_REV, laddr);
      return ERROR;
	} 
  }

  if((boardID&0xffff)==V1190_BOARD_ID)                             use1190[itdc] = 1;  /* 1 for v1190 */
  else if(((boardID&0xffff)==V1290_BOARD_ID) && (boardVersion==0)) use1190[itdc] = 2;  /* 2 for v1290 */
  else if(((boardID&0xffff)==V1290_BOARD_ID) && (boardVersion==2)) use1190[itdc] = 3;  /* 3 for v1290N */

  return OK;
}

UINT32
tdc1190ScanMask()
{
  int itdc=0;
  UINT32 rval=0;

  for(itdc=0; itdc<Nc1190; itdc++)
    rval |= 1<<itdc;

  return rval;
}


/*Sergey start*/

/* set global variables which will be used to program all boards in crate */
void
tdc1190SetDefaultWindowWidth(int width)
{
  window_width = width;
  return;
}

void
tdc1190SetDefaultWindowOffset(int offset)
{
  window_offset = offset;
  return;
}


/*Sergey end*/



int
tdc1190Type(int id)
{
  if(id>=0 && id<Nc1190) return(use1190[id]);

  printf("%s: ERROR: TDC with id %d does not exist.\n",__FUNCTION__,id);
  return(ERROR);
}

/*****************************************************************************/
/*****************************************************************************/
/*S.P.start*/

int
tdc1190Slot(int id)
{
  if(id>=0 && id<Nc1190)
    return(tdc1190GetGeoAddress(id));

  printf("%s: ERROR: TDC with id %d does not exist.\n",__FUNCTION__,id);
  return(ERROR);
}

int
tdc1190Id(int slot)
{
  int id;

  for(id=0; id<Nc1190; id++)
    if(tdc1190GetGeoAddress(id)==slot)
      return(id);

  printf("%s: ERROR: TDC in slot %d does not exist or not initialized.\n",__FUNCTION__,slot);
  return(ERROR);
}

int
tdc119GetBoardID(int id)
{
  int rval;
  volatile struct v1190_ROM_struct *rp;

  CHECKID(id);
  rp = (struct v1190_ROM_struct *)((UINT32)c1190p[id] + V1190_ROM_OFFSET);
  LOCK_1190;
  rval = ((vmeRead16(&(rp->board1))&(0xff))<<8) | (vmeRead16(&(rp->board0))&(0xff));
  UNLOCK_1190;

  return(rval);
}

int
tdc119GetBoardRev(int id)
{
  int rval;
  volatile struct v1190_ROM_struct *rp;

  CHECKID(id);
  rp = (struct v1190_ROM_struct *)((UINT32)c1190p[id] + V1190_ROM_OFFSET);
  LOCK_1190;
  /*
  printf("  rp->revis3 addr=0x%08x  val=0x%04x \n", &(rp->revis3), *(&(rp->revis3)));
  printf("  rp->revis2 addr=0x%08x  val=0x%04x \n", &(rp->revis2), *(&(rp->revis2)));
  printf("  rp->revis1 addr=0x%08x  val=0x%04x \n", &(rp->revis1), *(&(rp->revis1)));
  printf("  rp->revis0 addr=0x%08x  val=0x%04x \n", &(rp->revis0), *(&(rp->revis0)));
  */
  rval = ((vmeRead16(&(rp->revis3))&(0xff))<<24) |
	 ((vmeRead16(&(rp->revis2))&(0xff))<<16) |
	 ((vmeRead16(&(rp->revis1))&(0xff))<<8)  |
	  (vmeRead16(&(rp->revis0))&(0xff));
  UNLOCK_1190;

  return(rval);
}

int
tdc119GetSerialNumber(int id)
{
  int rval;
  volatile struct v1190_ROM_struct *rp;

  CHECKID(id);
  rp = (struct v1190_ROM_struct *)((UINT32)c1190p[id] + V1190_ROM_OFFSET);
  LOCK_1190;
  /*
  printf("  rp->sernum1 addr=0x%08x  val=0x%04x \n", &(rp->sernum1), *(&(rp->sernum1)));
  printf("  rp->sernum2 addr=0x%08x  val=0x%04x \n", &(rp->sernum2), *(&(rp->sernum2)));
  */
  rval = ((vmeRead16(&(rp->sernum1))&(0xff))<<8) + (vmeRead16(&(rp->sernum2))&(0xff));
  UNLOCK_1190;

  return(rval);
}

#define SSWAP(x)        ((((x) & 0x00ff) << 8) | \
                         (((x) & 0xff00) >> 8))
/*
to see all ROM from checksum to sernum2:
      tcpClient rocbcal1 "tdc1190PrintROM(0x4000,34)"
*/
void
tdc1190PrintROM(UINT32 addr_add, int loop)
{
  volatile unsigned short *addr;
  unsigned short rval;
  int jj;

  if (loop<=0) loop=0;

  addr = (unsigned short *)((UINT32)c1190p[0] + addr_add);

  LOCK_1190;

  for(jj=0; jj<loop; jj++)
  {
    rval = *addr;
    rval = SSWAP(rval);
    printf("  addr=0x%08x  rval=0x%02x \n", addr, rval);
    addr += 2;
  }
  UNLOCK_1190;
}

/* channels[0] - low 16 channels */
STATUS
tdc1290EnableChannels(int id, UINT16 channels[2])
{
  int i, nnn=2;
  CHECKID(id);
  tdc1190WriteMicro(id,0x4400);
  if(use1190[id]==3) nnn=1;
  for(i=0; i<nnn; i++) tdc1190WriteMicro(id,channels[i]);
  return(OK);
}

STATUS
tdc1290GetChannels(int id, UINT16 channels[2])
{
  int i, nnn=2;
  CHECKID(id);
  tdc1190WriteMicro(id,0x4500);
  if(use1190[id]==3) nnn=1;
  for(i=0; i<nnn; i++) tdc1190ReadMicro(id,&channels[i],1);
  return(OK);
}








/*****************************************************************************/

static char *expid= NULL;

void
tdc1190SetExpid(char *string)
{
  tdc1290SetExpid(string);
}

void
tdc1290SetExpid(char *string)
{
  expid = strdup(string);
}

void
tdc1290InitGlobals()
{
  int ii, jj;

  cblt_not      = 0;
  berr_fifo     = 1;

  blt_Events      = 1;
  n_Hits          = 64;
  almostFullLevel = 16384;
  outProgControl  = 2;

  window_width  = 750;
  window_offset = -1750;
  window_extra  = 25;
  window_reject = 50;

  for(ii=0; ii<NBOARD; ii++)
  {
    /*tdc[ii].group = 0;*/
    tdc[ii].edge = 2;
    tdc[ii].compensation = 1;
    for(jj=0; jj<8; jj++) tdc[ii].mask[jj] = 0xffff;
  }

  /* obtain CPU DMA setting information */
  usrVmeDmaGetConfig(&a24_a32, &sngl_blt_mblt, &sst_rate);

  return;
}


#define SCAN_MSK \
	args = sscanf (str_tmp, "%*s %d %d %d %d %d %d %d %d   \
                                 %d %d %d %d %d %d %d %d", \
		       &msk[ 0], &msk[ 1], &msk[ 2], &msk[ 3], \
		       &msk[ 4], &msk[ 5], &msk[ 6], &msk[ 7], \
		       &msk[ 8], &msk[ 9], &msk[10], &msk[11], \
		       &msk[12], &msk[13], &msk[14], &msk[15])


int
tdc1290ReadConfigFile(char *filename)
{
  FILE   *fd;
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch, gr;
  char   str_tmp[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  char   str2[STRLEN];
  int    args, i1, msk[NCHAN];
  int    slot, slot1, slot2, chan;
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
      sprintf(fname, "%s/tdc1190/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/tdc1190/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/tdc1190/%s.cnf", clonparms, expid);
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
  gr = 0;

  /* by default apply settings for all slots */
  slot1 = tdc1190Slot(0);
  slot2 = tdc1190Slot(Nc1190-1) + 1;
  printf("\nReadConfigFile: use by default slot1=%d, slot2=%d\n",slot1,slot2);

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


      /* Start parsing real config inputs */
      if((strcmp(keyword,"TDC1190_CRATE")==0)||(strcmp(keyword,"TDC1290_CRATE")==0))
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


      else if(active && ((strcmp(keyword,"TDC1190_SLOT")==0)||(strcmp(keyword,"TDC1290_SLOT")==0)))
      {
        gr++;
        sscanf (str_tmp, "%*s %s", str2);
        printf("nReadConfigFile: str2>%s<\n",str2);
        if(isdigit(str2[0]))
        {
          slot1 = atoi(str2);
          slot2 = slot1 + 1;
          if( slot1<tdc1190Slot(0) || slot1>tdc1190Slot(Nc1190-1) )
          {
            printf("\nReadConfigFile: Wrong slot number %d\n\n",slot1);
            return(-4);
          }
        }
        else if(!strcmp(str2,"all"))
        {
          slot1 = tdc1190Slot(0);
          slot2 = tdc1190Slot(Nc1190-1) + 1;
        }
        else
        {
          printf("\nReadConfigFile: Wrong slot >%s<, must be 'all' or actual slot number\n\n",str2);
          return(-4);
        }
        printf("\nReadConfigFile: slot1=%d slot2=%d\n",slot1,slot2);
	  }



      else if(active && ((strcmp(keyword,"TDC1190_BERR_FIFO")==0)||(strcmp(keyword,"TDC1290_BERR_FIFO")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    if(i1<0||i1>1)
	    {
	      printf("\nReadConfigFile: Wrong DMA Nwords method %d, %s\n",i1,str_tmp);
	      return(-8);
	    }
	    berr_fifo = i1;
        printf("\nReadConfigFile: berr_fifo=%d\n",berr_fifo);
      }

      else if(active && ((strcmp(keyword,"TDC1190_BLT_EVENTS")==0)||(strcmp(keyword,"TDC1290_BLT_EVENTS")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    blt_Events = i1;
        printf("\nReadConfigFile: blt_Events=%d\n",blt_Events);
      }

      else if(active && ((strcmp(keyword,"TDC1190_N_HITS")==0)||(strcmp(keyword,"TDC1290_N_HITS")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    n_Hits = i1;
        printf("\nReadConfigFile: n_Hits=%d\n",n_Hits);
      }

      else if(active && ((strcmp(keyword,"TDC1190_ALMOSTFULL")==0)||(strcmp(keyword,"TDC1290_ALMOSTFULL")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    almostFullLevel = i1;
        printf("\nReadConfigFile: almostFullLevel=%d\n",almostFullLevel);
      }

      else if(active && ((strcmp(keyword,"TDC1190_OUT_PROG")==0)||(strcmp(keyword,"TDC1290_OUT_PROG")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    outProgControl = i1;
        printf("\nReadConfigFile: outProgControl=%d\n",outProgControl);
      }

	  else if(active && ((strcmp(keyword,"TDC1190_W_WIDTH")==0)||(strcmp(keyword,"TDC1290_W_WIDTH")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    window_width = i1;
        printf("\nReadConfigFile: window_width=%d\n",window_width);
      }

      else if(active && ((strcmp(keyword,"TDC1190_W_OFFSET")==0)||(strcmp(keyword,"TDC1290_W_OFFSET")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    window_offset = i1;
        printf("\nReadConfigFile: window_offset=%d\n",window_offset);
      }

      else if(active && ((strcmp(keyword,"TDC1190_W_EXTRA")==0)||(strcmp(keyword,"TDC1290_W_EXTRA")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    window_extra = i1;
        printf("\nReadConfigFile: window_extra=%d\n",window_extra);
      }

      else if(active && ((strcmp(keyword,"TDC1190_W_REJECT")==0)||(strcmp(keyword,"TDC1290_W_REJECT")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    window_reject = i1;
        printf("\nReadConfigFile: window_reject=%d\n",window_reject);
      }

      else if(active && ((strcmp(keyword,"TDC1190_EDGE")==0)||(strcmp(keyword,"TDC1290_EDGE")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    for(slot=slot1; slot<slot2; slot++)  tdc[slot].edge = i1 & 0x3;
      }
	  
      else if(active && ((strcmp(keyword,"TDC1190_COMPENSATION")==0)||(strcmp(keyword,"TDC1290_COMPENSATION")==0)))
      {
        sscanf (str_tmp, "%*s %d", &i1);
	    for(slot=slot1; slot<slot2; slot++)  tdc[slot].compensation = i1 & 0x1;
      }
	  
      else if(active && ((strncmp(keyword,"TDC1190_MASK",12) == 0)||(strncmp(keyword,"TDC1290_MASK",12) == 0)))
      {
	    SCAN_MSK;
	    ui1 = 0;
	    for(jj=0; jj<NCHAN; jj++)
	    {
	      if((msk[jj] < 0) || (msk[jj] > 1))
	      {
	        printf("\nReadConfigFile: Wrong mask bit value, %d\n\n",msk[jj]); return(-9);
	      }
	      ui1 |= (msk[jj]<<jj);
	    }

        str2[0] = keyword[12];
        str2[1] = '\0';
        jj = atoi(str2) - 1;
        if(jj<0 || jj>7)
		{
          printf("ERROR: mask jj = %d, must be between 0 and 7\n",jj);
          return(-9);
		}
        /*printf("mask jj = %d\n",jj);*/
	    for(slot=slot1; slot<slot2; slot++)
		{
	        tdc[slot].mask[jj] = ui1;
            /*printf("slot=%d, mask[%d] = %d\n",slot,jj,tdc[slot].mask[jj]);*/
		}
	  }
      else
      {
        ; /* unknown key - do nothing */
		/*
        printf("ReadConfigFile: Unknown Field or Missed Field in\n");
        printf("   %s \n", fname);
        printf("   str_tmp=%s", str_tmp);
        printf("   keyword=%s \n\n", keyword);
        return(-10);
		*/
      }

    }
  } /* end of while */

  fclose(fd);

  return(0/*gr*/);
}






int
tdc1290DownloadAll()
{
  int ii, jj, nnn;
  unsigned short value;
  short tdata;

  for(ii=0; ii<Nc1190; ii++) tdc1190Clear(ii);
  for(ii=0; ii<Nc1190; ii++) tdc1190Reset(ii);


  /* set Trigger Window Width (ns) */
  for(ii=0; ii<Nc1190; ii++)
  {
    /*tdc1190SetWindowWidth(ii, window_width);*/
    tdc1190WriteMicro(ii,0x1000);
  }
  if(window_width < 25)         value = 1;
  else if(window_width > 51175) value = 0x7FF;
  else                          value = window_width/25;
  printf("Set Window Width to %d ns\n",value*25);
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190WriteMicro(ii,value);
  }

  /* set Trigger Window Offset (ns) */
  for(ii=0; ii<Nc1190; ii++)
  {
    /*tdc1190SetWindowOffset(ii, window_offset);*/
    tdc1190WriteMicro(ii,0x1100);
  }
  if(window_offset < -800000) tdata = -32000;
  else                        tdata = window_offset/25;
  value = (unsigned short)tdata;
  printf("Set Window Offset to %d ns, tdata=0x%04x, value=0x%04x\n",tdata*25,tdata,value);
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190WriteMicro(ii,/*tdata*/value);
  }

  /* set Extra Search Margin (after window) (ns) */
  for(ii=0; ii<Nc1190; ii++)
  {
    /*tdc1190SetExtraSearchMargin(ii, window_extra);*/
    tdc1190WriteMicro(ii,0x1200);
  }
  if(window_extra < 0)         value = 0;
  else if(window_extra > 1250) value = 50;
  else                         value = window_extra/25;
  printf("Set Extra Search Margin to %d ns\n",value*25);
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190WriteMicro(ii,value);
  }

  /* set Reject Margin (before window) (ns) */
  for(ii=0; ii<Nc1190; ii++)
  {
    /*tdc1190SetRejectMargin(ii, window_reject);*/
    tdc1190WriteMicro(ii,0x1300);
  }
  if(window_reject < 0)         value = 0;
  else if(window_reject > 1250) value = 50;
  else                          value = window_reject/25;
  printf("Set Reject Margin to %d ns\n",value*25);
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190WriteMicro(ii,value);
  }

  /* set Edge Detection */
  for(ii=0; ii<Nc1190; ii++)
  {
    /*tdc1190SetEdgeDetectionConfig(ii, tdc[tdc1190Slot(ii)].edge);*/
    tdc1190WriteMicro(ii,0x2200);
  }
  for(ii=0; ii<Nc1190; ii++)
  {
    value = tdc[tdc1190Slot(ii)].edge & 0x3;
    printf("SetEdgeDetectionConfig: ");
    if(value==0x0)      printf("set pair mode\n");
    else if(value==0x1) printf("set trailing edge only\n");
    else if(value==0x2) printf("set leading edge only\n");
    else                printf("set both leading and trailing edges\n");
    tdc1190WriteMicro(ii,value);
  }

  /* set compensation */
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190Compensation(ii, tdc[tdc1190Slot(ii)].compensation);
  }

  /* set Channels Enable Mask */
  /*
  for(ii=0; ii<Nc1190; ii++)
  {
    if(use1190[ii]==1) tdc1190EnableChannels(ii, tdc[tdc1190Slot(ii)].mask);
    else               tdc1290EnableChannels(ii, tdc[tdc1190Slot(ii)].mask);
  }
  */
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190WriteMicro(ii,0x4400);
  }
  for(ii=0; ii<Nc1190; ii++)
  {
    if(use1190[ii]==1)      nnn = 8;
    else if(use1190[ii]==2) nnn = 2;
    else                    nnn = 1;
    for(jj=0; jj<nnn; jj++) tdc1190WriteMicro(ii,tdc[tdc1190Slot(ii)].mask[jj]);
  }

  /* set Max Number of Hits per Event */
  for(ii=0; ii<Nc1190; ii++)
  {
    /*tdc1190SetMaxNumberOfHitsPerEvent(ii, n_Hits);*/
    tdc1190WriteMicro(ii,0x3300);
  }
  if(n_Hits==0)        tdata = 0;
  else if(n_Hits==1)   tdata = 1;
  else if(n_Hits==2)   tdata = 2;
  else if(n_Hits==4)   tdata = 3;
  else if(n_Hits==8)   tdata = 4;
  else if(n_Hits==16)  tdata = 5;
  else if(n_Hits==32)  tdata = 6;
  else if(n_Hits==64)  tdata = 7;
  else if(n_Hits==128) tdata = 8;
  else                 tdata = 9;
  if(tdata==9)
    printf("%s(%d): Set Unlimited Number Of Hits Per Event\n",__FUNCTION__,ii);
  else
    printf("%s(%d): Set Maximum Number Of Hits Per Event to %d\n",__FUNCTION__,ii,n_Hits);
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190WriteMicro(ii,tdata);
  }






  for(ii=0; ii<Nc1190; ii++)
  {
    /* program TDC for trigger matching mode */
    tdc1190SetTriggerMatchingMode(ii);

    /* enable subtraction of trigger time */
    tdc1190EnableTriggerTimeSubtraction(ii);

    /* set Readout settings */
    tdc1190EventFifo(ii, berr_fifo);
    tdc1190BusError(ii, (~berr_fifo)&0x1);

    /* enable 64-bit alignment */
    /*  NOTE: 64-bit alignment must be enabled for any DMA readout,
       not only for MBLT(D64) but for BLT(D32) as well; some DMA engines
       (for example the one on mv5100) will cut off last 32-bit word if
       it is not even, for example if event size is equal to 137 D32 words,
       only 136 D32 words will be transfered */
    /* in addition universe library contains check for 64-bit alignment
       in dmastart procedure, it will return error if ... */
    tdc1190Align64(ii, 1);

    /* set BLT Event Number Register */
    tdc1190SetBLTEventNumber(ii, blt_Events);

    /* reset MCST flag in every board in case if it was set before */
    tdc1190ResetMCST(ii);

    /* set 'almost full' level */
    tdc1190SetAlmostFullLevel(ii, almostFullLevel);

    /* program output connector to signal on it */
    tdc1190SetOutProg(ii, outProgControl);
  }

  return(0);
}




int
tdc1190DownloadAll()
{
  tdc1290DownloadAll();
}


/*
cd "/usr/local/clas12/release/0.1/coda/src/rol/VXWORKS_ppc/lib"
ld < librol.so
tdc1190Init(0x11300000,0x80000,20,0)
tdc1190Config("")
tdc1190Mon(0)
*/

/* tdc1190Init() have to be called before this function */
int  
tdc1290Config(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    tdc1290UploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    tdc1290InitGlobals();
  }

  /* read config file */
  if( (res = tdc1290ReadConfigFile(fname)) < 0 )
  {
    printf("tdc1290Config ERROR: res=%d, nothing will be downloaded !!!!!!!!!!\n",res);
    return(res);
  }

  /* download to all boards */
  tdc1290DownloadAll();

  return(0);
}

int
tdc1190Config(char *fname)
{
  return(tdc1290Config(fname));
}



void
tdc1290Mon(int slot)
{
  int id, ii, start, end, res;
  UINT16 channels[2];
  usrVmeDmaGetConfig(&a24_a32, &sngl_blt_mblt, &sst_rate);

  printf("\nCPU DMA settings:\n");
  if(a24_a32==0) printf("  A16 address mode\n");
  else if(a24_a32==1) printf("  A24 address mode\n");
  else if(a24_a32==2) printf("  A32 address mode\n");
  else printf("  unknown address mode\n");

  tdc1290PrintDMAdataType(sngl_blt_mblt);

  if(sst_rate==0) printf("  SST160 rate\n");
  else if(sst_rate==1) printf("  SST267 rate\n");
  else printf("  unknown SST rate\n");

  printf("\n");

  if(slot==0)
  {
    start = 0;
    end   = Nc1190;
  }
  else if((id = tdc1190Id(slot)) >= 0)
  {
    start = id;
    end   = start + 1;
  }
  else
  {
    return;
  }

  for(ii=start; ii<end; ii++)
  {
    printf("\ntdc CAEN 1290 number %d in slot %d\n",ii,tdc1190Slot(ii));
    printf("  Board Type/ID       = 0x%04x\n", tdc119GetBoardID(ii));
    printf("  Board Revision      = 0x%08x\n", tdc119GetBoardRev(ii));
    printf("  Board Serial Number = %04d\n",  tdc119GetSerialNumber(ii));

    if(use1190[ii]==1) tdc1190GetChannels(ii, channels);
    else               tdc1290GetChannels(ii, channels);

    if(use1190[ii] == 1)
	{
      printf("  Channel Mask = 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
        channels[7],channels[6],channels[5],channels[4],channels[3],channels[2],channels[1],channels[0]);
	}
    else if(use1190[ii] == 2)
	{
      printf("  Channel Mask = 0x%04x 0x%04x\n",
        channels[1],channels[0]);
	}
    else
	{
      printf("  Channel Mask = 0x%04x\n",
        channels[0]);
	}

    printf("\n");

    tdc1290PrintDMABerrFifo(berr_fifo);
    printf("\n");

    tdc1190ReadTriggerConfiguration(ii);
    printf("\n");
    tdc1190ReadEdgeDetectionConfig(ii);

    res = tdc1190ReadAcquisitionMode(ii);
    if(res==0)
      printf(" Acquisition Mode = Continuous Storage\n");
    else
      printf(" Acquisition Mode = Trigger Matching\n");

    res = tdc1190StatusFull(ii);
    if(res==0)
      printf(" Output Buffer status = Not Full\n");
    else
      printf(" Output Buffer status = Full\n");

    printf(" BLT Event Number = %d\n",tdc1190GetBLTEventNumber(ii));
    printf(" Max Number of Hits per Event = %d\n",tdc1190GetMaxNumberOfHitsPerEvent(ii));
    tdc1190GetAlmostFullLevel(ii);
  }
}

void
tdc1190Mon(int slot)
{
  tdc1290Mon(slot);
}





/* upload setting from all found TDCs */
int
tdc1290UploadAll(char *string, int length)
{
  int slot, i, ii, jj, kk, nnn, len1, len2;
  char tdcname[8], *str, sss[1024];
  unsigned int tmp, val[NCHAN], val1[NCHAN];
  unsigned short sval[NCHAN];
  unsigned short bypMask;
  unsigned short channels[8];
  short temp[5];
  int ww[NBOARD],wo[NBOARD],esm[NBOARD],rm[NBOARD],tts[NBOARD], nh[NBOARD];
  short tdata;

  /* read trigger configuration */
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190WriteMicro(ii,0x1600);
  }
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190ReadMicro(ii,temp,5);
	/*
    tdc1190ReadMicro(ii,&temp[0],1);
    tdc1190ReadMicro(ii,&temp[1],1);
    tdc1190ReadMicro(ii,&temp[2],1);
    tdc1190ReadMicro(ii,&temp[3],1);
    tdc1190ReadMicro(ii,&temp[4],1);
	*/
    ww[ii]  = temp[0]*25; /*Window Width (ns)*/
    wo[ii]  = temp[1]*25; /*Window Offset (ns)*/
    esm[ii] = temp[2]*25; /*Extra Seach Margin (ns)*/
    rm[ii]  = temp[3]*25; /*Reject Margin (ns)*/
    tts[ii] = temp[4];    /*Trigger Time Subtruction*/
	/*
    printf("tdc1290UploadAll  Window Width             = %6d ns\n",ww[ii]);
    printf("tdc1290UploadAll  Window Offset            = %6d ns\n",wo[ii]);
    printf("tdc1290UploadAll  Extra Seach Margin       = %6d ns\n",esm[ii]);
    printf("tdc1290UploadAll  Reject Margin            = %6d ns\n",rm[ii]);
    printf("tdc1290UploadAll  Trigger Time Subtruction = %6d\n",tts[ii]);
	*/
  }

  /* read max number of hits per event */
  for(ii=0; ii<Nc1190; ii++)
  {
	/*n_Hits = tdc1190GetMaxNumberOfHitsPerEvent(ii);*/
    tdc1190WriteMicro(ii,0x3400);
  }
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190ReadMicro(ii,&tdata,1);
    tdata &= 0xF;
    if(tdata==0)      nh[ii] = 0;
    else if(tdata==1) nh[ii] = 1;
    else if(tdata==2) nh[ii] = 2;
    else if(tdata==3) nh[ii] = 4;
    else if(tdata==4) nh[ii] = 8;
    else if(tdata==5) nh[ii] = 16;
    else if(tdata==6) nh[ii] = 32;
    else if(tdata==7) nh[ii] = 64;
    else if(tdata==8) nh[ii] = 128;
    else if(tdata==9) nh[ii] = 9999;
    else printf("%s(%d): ERROR: tdata=%d (0x%x)\n",__FUNCTION__,ii,tdata,tdata);
  }

  /* read edge detection configuration */
  for(ii=0; ii<Nc1190; ii++)
  {
	/*tdc[tdc1190Slot(ii)].edge = tdc1190ReadEdgeDetectionConfig(ii);*/
    tdc1190WriteMicro(ii,0x2300);
  }
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc1190ReadMicro(ii,&tdata,1);
    tdata &= 0x3;
    if(tdata<3)
    {
	  /*
      printf(" %s(%d): Mode =",__FUNCTION__,ii);
      if(tdata==0) printf(" pair mode\n");
      else if(tdata==1) printf(" trailing edge\n");
      else if(tdata==2) printf(" leading edge\n");
      else if(tdata==3) printf(" both leading and trailing\n");
	  */
      tdc[tdc1190Slot(ii)].edge = tdata;
    }
    else printf("%s(%d) ERROR: tdata=%d (0x%x)\n",__FUNCTION__,ii,tdata,tdata);
  }

  /* read compensation configuration */
  for(ii=0; ii<Nc1190; ii++)
  {
    tdc[tdc1190Slot(ii)].compensation = tdc1190GetCompensation(ii);
  }

  /* read channel mask */
  for(ii=0; ii<Nc1190; ii++)
  {
    /*if(use1190[ii]==1) tdc1190GetChannels(ii, channels);
	else               tdc1290GetChannels(ii, channels);*/
    tdc1190WriteMicro(ii,0x4500);
  }
  for(ii=0; ii<Nc1190; ii++)
  {
    if(use1190[ii] == 1)      nnn = 8;
    else if(use1190[ii] == 2) nnn = 2;
    else                      nnn = 1;
	/*
    for(jj=0; jj<nnn; jj++) tdc1190ReadMicro(ii,&channels[jj],1);
	*/
    tdc1190ReadMicro(ii,tdc[tdc1190Slot(ii)].mask,nnn);
	/*
    for(jj=0; jj<nnn; jj++)
	{
	  tdc[tdc1190Slot(ii)].mask[jj] = channels[jj];
	}
	*/
  }


  /* read the rest of configuration */
  for(ii=0; ii<Nc1190; ii++)
  {
	berr_fifo = tdc1190GetEventFifo(ii);
	blt_Events = tdc1190GetBLTEventNumber(ii);
	almostFullLevel = tdc1190GetAlmostFullLevel(ii);
	outProgControl = tdc1190GetOutProg(ii);
  }

  if(length)
  {
    str = string;
    str[0] = '\0';
    for(ii=0; ii<Nc1190; ii++)
    {
      slot = tdc1190Slot(ii);

      if(use1190[ii] == 1)      sprintf(tdcname,"TDC1190");
      else if(use1190[ii] == 2) sprintf(tdcname,"TDC1290");
      else                      sprintf(tdcname,"TDC1290");

      sprintf(sss,"%s_SLOT %d\n",tdcname,slot);
      ADD_TO_STRING;

      sprintf(sss,"%s_BERR_FIFO %d\n",tdcname,berr_fifo);
      ADD_TO_STRING;

      sprintf(sss,"%s_BLT_EVENTS %d\n",tdcname,blt_Events);
      ADD_TO_STRING;

      sprintf(sss,"%s_N_HITS %d\n",tdcname,n_Hits);
      ADD_TO_STRING;

      sprintf(sss,"%s_ALMOSTFULL %d\n",tdcname,almostFullLevel);
      ADD_TO_STRING;

      sprintf(sss,"%s_OUT_PROG %d\n",tdcname,outProgControl);
      ADD_TO_STRING;

      sprintf(sss,"%s_W_WIDTH %d\n",tdcname,ww[ii]);
      ADD_TO_STRING;

      sprintf(sss,"%s_W_OFFSET %d\n",tdcname,wo[ii]);
      ADD_TO_STRING;

      sprintf(sss,"%s_W_EXTRA %d\n",tdcname,esm[ii]);
      ADD_TO_STRING;

      sprintf(sss,"%s_W_REJECT %d\n",tdcname,rm[ii]);
      ADD_TO_STRING;

      sprintf(sss,"%s_EDGE %d\n",tdcname,tdc[slot].edge);
      ADD_TO_STRING;

      sprintf(sss,"%s_COMPENSATION %d\n",tdcname,tdc[slot].compensation);
      ADD_TO_STRING;

      if(use1190[ii] == 1)      nnn = 8;
      else if(use1190[ii] == 2) nnn = 2;
      else                      nnn = 1;
      for(jj=0; jj<nnn; jj++)
	  {
        sprintf(sss,"%s_MASK%1d",tdcname,jj+1);
        ADD_TO_STRING;
        for(kk=0; kk<16; kk++)
	    {
          sprintf(sss," %d",(tdc[slot].mask[jj]>>kk)&0x1);
          ADD_TO_STRING;
	    }
        sprintf(sss,"\n");
        ADD_TO_STRING;
	  }
    }

    CLOSE_STRING;
  }
}


int
tdc1190UploadAll(char *string, int length)
{
  return(tdc1290UploadAll(string, length));
}



int
tdc1190UploadAllPrint()
{
  char str[16001];
  tdc1190UploadAll(str, 16000);
  printf("%s",str);
}

int
tdc1290UploadAllPrint()
{
  char str[16001];
  tdc1290UploadAll(str, 16000);
  printf("%s",str);
}







void
tdc1290PrintDMAdataType(int dataType)
{
  if(dataType==0x01)
  {
    printf("  D32 single word readout\n");
  }
  else if(dataType==0x02)
  {
    printf("  D32 DMA (BLT) readout\n");
  }
  else if(dataType==0x03)
  {
    printf("  D64 DMA (MBLT) readout\n");
  }
  else if(dataType==0x04)
  {
    printf("  D64 DMA (2eVME) readout\n");
  }
  else if(dataType==0x05)
  {
    printf("  D64 DMA (2eSST) readout\n");
  }
  else
  {
    printf("  unknown readout mode !!!\n");
  }
}

void
tdc1290PrintDMABerrFifo(int method)
{
  if(method==0x00)
  {
    printf("  DMA Nwords method: use VME BUS error\n");
  }
  else if(method==0x01)
  {
    printf("  DMA Nwords method: use event FIFO\n");
  }
  else
  {
    printf("  unknown DMA Nwords method, use VME BUS error\n");
  }

}

/*S.P.end*/
/*****************************************************************************/
/*****************************************************************************/




/*******************************************************************************
 *
 * tdc1190Status - Gives Status info on specified TDC
 *
 *
 * RETURNS: None
 */

STATUS
tdc1190Status(int id)
{
  int lock;
  int drdy=0, afull=0, bfull=0;
  int berr=0, testmode=0;
  int trigMatch=0, headers=0, tdcerror[4];
  UINT16 statReg, cntlReg, afullLevel, bltEvents;
  UINT16 iLvl, iVec, evStored;
  UINT16 evCount, res=0;
  char   tdcname[8] = { "v1190" };

  CHECKID(id);
  if(use1190[id]==2) sprintf(tdcname, "%s", "v1290");
  if(use1190[id]==3) sprintf(tdcname, "%s", "v1290N");

  /* read various registers */
  LOCK_1190;
  statReg = vmeRead16(&(c1190p[id]->status))&V1190_STATUS_MASK;
  cntlReg = vmeRead16(&(c1190p[id]->control))&V1190_CONTROL_MASK;
  afullLevel = vmeRead16(&(c1190p[id]->almostFullLevel));
  bltEvents = vmeRead16(&(c1190p[id]->bltEventNumber))&V1190_BLTEVNUM_MASK;
  
  iLvl = vmeRead16(&(c1190p[id]->intLevel))&V1190_INTLEVEL_MASK;
  iVec = vmeRead16(&(c1190p[id]->intVector))&V1190_INTVECTOR_MASK;
  evCount  = vmeRead32(&(c1190p[id]->evCount))&V1190_EVCOUNT_MASK;
  evStored = vmeRead16(&(c1190p[id]->evStored));
  UNLOCK_1190;

  drdy  = statReg&V1190_STATUS_DATA_READY;
  afull = statReg&V1190_STATUS_ALMOST_FULL;
  bfull = statReg&V1190_STATUS_FULL;

  trigMatch = statReg&V1190_STATUS_TRIG_MATCH;
  headers  = statReg&V1190_STATUS_HEADER_ENABLE;
  tdcerror[0] = (statReg&V1190_STATUS_ERROR_0)>>6;
  tdcerror[1] = (statReg&V1190_STATUS_ERROR_1)>>7;
  tdcerror[2] = (statReg&V1190_STATUS_ERROR_2)>>8;
  tdcerror[3] = (statReg&V1190_STATUS_ERROR_3)>>9;

  berr = cntlReg&V1190_BUSERROR_ENABLE;
  testmode = cntlReg&V1190_TESTMODE;

  /* print out status info */

#ifdef VXWORKS
  printf("STATUS for %s TDC at base address 0x%x\n",tdcname,(UINT32)c1190p[id]);
#else
  printf("STATUS for %s TDC at VME (USER) base address 0x%x (0x%x)\n",
	 tdcname, (UINT32)c1190p[id] - tdcAddrOffset, (UINT32)c1190p[id]);
#endif
  printf("---------------------------------------------- \n");

  if(iLvl>0)
  {
    printf(" Interrupts Enabled - Request level = %d words\n",afullLevel);
    printf(" VME Interrupt Level: %d   Vector: 0x%x \n",iLvl,iVec);
  } else {
    printf(" Interrupts Disabled\n");
  }
  printf("\n");

  printf("  Data Status \n");
  printf("    Events to transfer via BLT  = 0x%04x\n",bltEvents);
  if(bfull)
  {
    printf("    Status        = 0x%04x (Buffer Full)\n",statReg);
  }
  else if(afull)
  {
    printf("    Status        = 0x%04x (Almost Full)\n",statReg);
  }
  else if(drdy)
  {
    printf("    Status        = 0x%04x (Data Ready )\n",statReg);
  }
  else
  {
    printf("    Status        = 0x%04x (No Data)\n",statReg);
  }
  
  printf("    Events Stored = %d\n",evStored);
  if(evCount == 0xffff)
    printf("    Total Events  = (No Events taken)\n");
  else
    printf("    Total Events  = %d\n",evCount);
  printf("    Almost Full   = %d\n",afullLevel);

  
  printf("\n");

  printf("  TDC Mode/Status \n"); 

  res = tdc1190GetEdgeResolution(id);
  printf("    Resolution      : %d ps\n",res);

  if(testmode)
    printf("    Test Mode       : On\n");

  if(trigMatch)
    printf("    Trigger Matching: On\n");
  else
    printf("    Trigger Matching: Off\n");
  
  if(headers)
    printf("    TDC Headers/EOB : On\n");
  else
    printf("    TDC Headers/EOB : Off\n");

  if(berr)
    printf("    Bus Errors      : On\n");
  else
    printf("    Bus Errors      : Off\n");

  printf("    TDC Errors      : 0:%d   1:%d   2:%d   3:%d\n",
	 tdcerror[0],tdcerror[1],tdcerror[2],tdcerror[3]);
  printf("\n");

  return OK;
}

STATUS
tdc1190GStatus(int mcFlag)
{
  int itdc;
  TDC1190 *st;
  unsigned int addr[V1190_MAX_MODULES+1];
  /* 02xx */ UINT16 acq_mode[V1190_MAX_MODULES+1];
  /* 16xx */ UINT16 trig_config[V1190_MAX_MODULES+1][5];
  /* 23xx */ UINT16 edge_detection[V1190_MAX_MODULES+1];
  /* 26xx */ UINT16 resolution[V1190_MAX_MODULES+1];
  /* 29xx */ UINT16 doublehit_resolution[V1190_MAX_MODULES+1];
  /* 32xx */ UINT16 tdc_headtrail_status[V1190_MAX_MODULES+1];
  /* 34xx */ UINT16 maxhits[V1190_MAX_MODULES+1];
  /* 3axx */ UINT16 errtype_enable[V1190_MAX_MODULES+1];
  /* 3cxx */ UINT16 fifo_size[V1190_MAX_MODULES+1];
  /* 45xx */ UINT32 chanenable_mask[V1190_MAX_MODULES+1];

  st = (TDC1190 *)malloc((V1190_MAX_MODULES+1)*sizeof(TDC1190));

  if(!st)
    {
      printf("%s: ERROR: Cannot allocate memory for memory structures.\n",
	     __func__);
      return ERROR;
    }

  LOCK_1190;
  for(itdc=0; itdc<Nc1190; itdc++)
    {
      addr[itdc] = (UINT32)c1190p[itdc] - tdcAddrOffset;
      st[itdc].control  = vmeRead16(&c1190p[itdc]->control);
      st[itdc].status   = vmeRead16(&c1190p[itdc]->status);
      st[itdc].geoAddr  = vmeRead16(&c1190p[itdc]->geoAddr);
      st[itdc].mcstBaseAddr  = vmeRead16(&c1190p[itdc]->mcstBaseAddr);
      st[itdc].mcstCtrl  = vmeRead16(&c1190p[itdc]->mcstCtrl);
      st[itdc].evCount  = vmeRead32(&c1190p[itdc]->evCount);
      st[itdc].evStored  = vmeRead16(&c1190p[itdc]->evStored);
      st[itdc].almostFullLevel  = vmeRead16(&c1190p[itdc]->almostFullLevel);
      st[itdc].bltEventNumber  = vmeRead16(&c1190p[itdc]->bltEventNumber);
      st[itdc].firmwareRev  = vmeRead16(&c1190p[itdc]->firmwareRev);
      st[itdc].fifo_stored  = vmeRead16(&c1190p[itdc]->fifo_stored);
      st[itdc].fifo_status  = vmeRead16(&c1190p[itdc]->fifo_status);
    }
  UNLOCK_1190;
  if(mcFlag)
    {
      tdc1190GReadAcquisitionMode(acq_mode);
      tdc1190GReadTriggerConfiguration((UINT16 *)trig_config);
      tdc1190GReadEdgeDetectionConfig(edge_detection);
      tdc1190GGetEdgeResolution(resolution);
      tdc1190GGetDoubleHitResolution(doublehit_resolution);
      tdc1190GGetTDCHeaderAndTrailer(tdc_headtrail_status);
      tdc1190GGetMaxNumberOfHitsPerEvent(maxhits);
      tdc1190GGetTDCErrorType(errtype_enable);
      tdc1190GGetEffectiveSizeOfReadoutFIFO(fifo_size);
      tdc1190GGetChannels(chanenable_mask);
    }

  printf("\n");
  /* Parameters from Registers */
  printf("                    CAEN1x90 TDC Module Configuration Summary\n\n");
  printf("            Firmware                                        TDC Error Status\n");
  printf("  #  GEO    Revision     Address      CBLT/MCST Address     0    1    2    3\n");
  printf("--------------------------------------------------------------------------------\n");

  for(itdc=0; itdc<Nc1190; itdc++)
    {
      printf(" %2d  ",itdc);

      printf("%2d     ",st[itdc].geoAddr & V1190_GEOADDR_MASK);

      printf("0x%02x         ",st[itdc].firmwareRev & 0xFF);

      printf("0x%08x   ",addr[itdc]);

      printf("0x%08x - ", (st[itdc].mcstBaseAddr)<<24);

      printf("%s ",
	     (st[itdc].mcstCtrl&0x3)==0?"DISABLED":
	     (st[itdc].mcstCtrl&0x3)==1?"LAST    ":
	     (st[itdc].mcstCtrl&0x3)==2?"FIRST   ":
	     (st[itdc].mcstCtrl&0x3)==3?"MIDDLE  ":
	     "        ");

      printf("%s %s %s %s",
	     (st[itdc].status & V1190_STATUS_ERROR_0)?"ERR ":"GOOD",
	     (st[itdc].status & V1190_STATUS_ERROR_1)?"ERR ":"GOOD",
	     (st[itdc].status & V1190_STATUS_ERROR_2)?"ERR ":"GOOD",
	     (st[itdc].status & V1190_STATUS_ERROR_3)?"ERR ":"GOOD");

      printf("\n");
    }

  printf("--------------------------------------------------------------------------------\n");

  printf("\n");
  printf("                              Readout Configuration\n\n");
  printf("     Block       Res  INL               Bus      Event    Extended TDC Headers+\n"); 
  printf("  #  Level  Mode [ps] Comp     Align64  Errors   FIFO     TrigTime Trailers\n");
  printf("--------------------------------------------------------------------------------\n");

  for(itdc=0; itdc<Nc1190; itdc++)
    {
      printf(" %2d  ",itdc);

      // BL
      printf("%3d    ",st[itdc].bltEventNumber & V1190_BLTEVNUM_MASK);

      // Mode
      printf("%s   ",
	     (st[itdc].status & V1190_STATUS_TRIG_MATCH)?"TM":"CS");

      // Res
      printf("%3d  ",
	     ((st[itdc].status & 0x3000)>>12)==0?800:
	     ((st[itdc].status & 0x3000)>>12)==1?200:
	     ((st[itdc].status & 0x3000)>>12)==2?100:
	     ((st[itdc].status & 0x3000)>>12)==3?25:0);

      printf("%s ",
	     (st[itdc].control & V1190_COMPENSATION_ENABLE)?"ENABLED ":"disabled");

      printf("%s ",
	     (st[itdc].control & V1190_ALIGN64)?"ENABLED ":"disabled");

      printf("%s ",
	     (st[itdc].control & V1190_BUSERROR_ENABLE)?"ENABLED ":"disabled");

      printf("%s ",
	     (st[itdc].control & V1190_EVENT_FIFO_ENABLE)?"ENABLED ":"disabled");

      printf("%s ",
	     (st[itdc].control & V1190_EXT_TRIG_TIME_TAG_ENABLE)?"ENABLED ":"disabled");

      printf("%s",
	     (st[itdc].status & (1<<4))?"ENABLED ":"disabled");

      printf("\n");
    }
  printf("--------------------------------------------------------------------------------\n");
  printf("\n");

  printf("                                 Data Status\n\n");
  printf("     Output      Events    -----Event FIFO----        Event\n");
  printf("  #  Buffer      Stored    Status        Count        Counter  \n");
  printf("--------------------------------------------------------------------------------\n");

  for(itdc=0; itdc<Nc1190; itdc++)
    {
      printf(" %2d  ",itdc);

      printf("%s  ",
	     ((st[itdc].status & 0x4)>>2)==1?"FULL      ":
	     ((st[itdc].status & 0x2)>>1)==0?"Not Full  ":
	     ((st[itdc].status & 0x2)>>1)==1?"AlmostFull":
	     "");

      printf("%5d     ",st[itdc].evStored);

      printf("%s    ",
	     ((st[itdc].fifo_status & 0x2)>>1)==1?"FULL      ":
	     ((st[itdc].fifo_status & 0x1)>>0)==1?"Data Ready":
	     "Empty     ");

      printf("%4d         ",st[itdc].fifo_stored & 0x7ff);

      printf("%10d",st[itdc].evCount);

      printf("\n");
    }

  printf("--------------------------------------------------------------------------------\n");
  printf("\n");

  if(mcFlag)
    {
      /* Parameters from MicroController */

      printf("                       MicroController Parameters\n\n");
      printf("           TriggerTime  Edge                    Channel   TDC Headers+\n");
      printf("  #  Mode  Subtraction  Detection   Resolution  Deadtime  Trailers\n");
      printf("--------------------------------------------------------------------------------\n");

      for(itdc=0; itdc<Nc1190; itdc++)
	{
	  printf(" %2d  ",itdc);

	  printf("%s    ",
		 (acq_mode[itdc] & 0x1)?"TM":"CS");

	  printf("%s     ",
		 (trig_config[itdc][4] & 0x1)?"Enabled ":"Disabled");

	  printf("%s    ",
		 (edge_detection[itdc] & 0x3)==0?"Pair    ":
		 (edge_detection[itdc] & 0x3)==1?"Trailing":
		 (edge_detection[itdc] & 0x3)==2?"Leading ":
		 (edge_detection[itdc] & 0x3)==3?"Both    ":"        ");

	  printf("%3dps       ",
		 (resolution[itdc] & 0x3)==0?800:
		 (resolution[itdc] & 0x3)==1?200:
		 (resolution[itdc] & 0x3)==2?100:
		 (resolution[itdc] & 0x3)==3?25:0);

	  printf("%3dns     ",
		 (doublehit_resolution[itdc] & 0x3)==0?5:
		 (doublehit_resolution[itdc] & 0x3)==1?10:
		 (doublehit_resolution[itdc] & 0x3)==2?30:
		 (doublehit_resolution[itdc] & 0x3)==3?100:0);

	  printf("%s",
		 (tdc_headtrail_status[itdc] & 0x1)?"Enabled ":"Disabled");

	  printf("\n");
	}

      printf("--------------------------------------------------------------------------------\n");
      printf("\n");

      printf("                              Window Parameters\n\n");
      printf("                                           Extra Search       Reject\n");
      printf("  #  Width              Offset             Width              Margin\n");
      printf("--------------------------------------------------------------------------------\n");

      for(itdc=0; itdc<Nc1190; itdc++)
	{
	  printf(" %2d  ",itdc);

	  printf("%6dns (0x%03x)   ",
		 (trig_config[itdc][0] & 0xFFF)*25,
		 (trig_config[itdc][0] & 0xFFF));

	  printf("%4dns (0x%04x)   ",
		 ((trig_config[itdc][1] & 0x7FF) - ((trig_config[itdc][1]&0x800)?2048:0))*25,
		 (trig_config[itdc][1] & 0xFFFF));

	  printf("%6dns (0x%03x)   ",
		 (trig_config[itdc][2] & 0xFFF)*25,
		 (trig_config[itdc][2] & 0xFFF));

	  printf("%6dns (0x%03x)",
		 (trig_config[itdc][3] & 0xFFF)*25,
		 (trig_config[itdc][3] & 0xFFF));

	  printf("\n");
	}

      printf("--------------------------------------------------------------------------------\n");
      printf("\n");

      printf("     MaxHits    TDC Error Types  FIFO        Channel\n");
      printf("  #  PerEvent   Enabled Mask     Size        Enabled Mask\n");
      printf("--------------------------------------------------------------------------------\n");

      for(itdc=0; itdc<Nc1190; itdc++)
	{
	  printf(" %2d  ",itdc);

	  if(maxhits[itdc]==9)
	    printf("No Limit   ");
	  else if (maxhits[itdc]<8)
	    {
	      printf("%3d        ",1<<(maxhits[itdc] & 0xF));
	    }
	  else
	    printf("UNKNOWN    ");

	  printf("0x%03x            ",errtype_enable[itdc] & 0x7ff);

	  printf("%3d         ",2<<fifo_size[itdc]);

	  printf("0x%08x",chanenable_mask[itdc]);

	  printf("\n");
	}


      printf("\n");
      printf("--------------------------------------------------------------------------------\n");
    }

  printf("\n");
  printf("\n");

  if(st)
    free(st);

  return OK;
}


/*******************************************************************************
 *
 *  tdc1190ReadMicro - Read Microcontroller Register
 *
 *  returns  OK or ERROR
 */

STATUS
tdc1190ReadMicro(int id, UINT16 *data, int nwords)
{
  int ii, jj, kk=0;
  UINT16 mstatus;

  CHECKID(id);

  LOCK_1190;

retry:

  mstatus = vmeRead16(&(c1190p[id]->microHandshake)) & V1190_MICRO_READOK;
  if(kk > 10)
  {
    printf("-> ReadMicro: mstatus=%d (0x%x)\n",mstatus,
	     vmeRead16(&(c1190p[id]->microHandshake)));
  }
  if(mstatus)
  {
    for(ii=0; ii<nwords; ii++)
	{
	  jj=0;
	  while(!( vmeRead16(&(c1190p[id]->microHandshake)) & V1190_MICRO_READOK))
	  {
	    jj++;
	    if(jj>20)
		{
		  logMsg("tdc1190ReadMicro: ERROR1: Read Status not OK (read %d)\n",
			 ii,0,0,0,0,0);
		  UNLOCK_1190;
		  return(ERROR);
		}
	  }
	  data[ii] = vmeRead16(&(c1190p[id]->microReg));
	}
  }
  else
  {
    kk++;
    if(kk>=20)
	{
	  logMsg("tdc1190ReadMicro: ERROR2: Read Status not OK\n",0,0,0,0,0,0);
	  UNLOCK_1190;
	  return(ERROR);
	}
    else
	{
	  taskDelay(10);
	  goto retry;
	}
  }

  if(kk > 10) printf("-> ReadMicro: kk=%d\n",kk);

  UNLOCK_1190;

  return(OK);
}

STATUS
tdc1190GReadMicro(UINT16 *data, int nwords_per_module)
{
  int id=0, ntries=0, iword=0;
  volatile UINT32 mstatus=0;
  UINT32 scanmask=0;

  LOCK_1190;

  scanmask = tdc1190ScanMask();

  while((mstatus!=scanmask) && (ntries<20))
    {
      for(id=0; id<Nc1190; id++)
	{
	  if(mstatus & (1<<id)) continue; /* Skip if already ready */

	  if(vmeRead16(&c1190p[id]->microHandshake) & V1190_MICRO_READOK)
	    mstatus |= 1<<id;
	}
      ntries++;
      taskDelay(10);
    }


  if(mstatus==scanmask)
    {
      for(id=0; id<Nc1190; id++)
	{
	  for(iword=0; iword<nwords_per_module; iword++)
	    {
	      ntries=0;
	      while(!(vmeRead16(&(c1190p[id]->microHandshake)) & V1190_MICRO_READOK))
		{
		  ntries++;
		  if(ntries>20)
		    {
		      logMsg("tdc1190GReadMicro: ERROR: Read Status from id=%d not OK (read %d)\n",
			     id, iword,0,0,0,0);
		      UNLOCK_1190;
		      return(ERROR);
		    }
		}
	      data[iword+id*nwords_per_module] = vmeRead16(&(c1190p[id]->microReg));
	    }
	}
    }
  else
    {
      logMsg("tdc1190GReadMicro: ERROR2: Read Status not OK (0x%x != 0x%x)\n",mstatus,scanmask,0,0,0,0);
      UNLOCK_1190;
      return(ERROR);
    }

  if(ntries > 10) logMsg("tdc1190GReadMicro: ntries=%d\n",ntries,2,3,4,5,6);
  UNLOCK_1190;
  return(OK);
}


/*******************************************************************************
 *
 *  tdc1190WriteMicro - Write to Microcontroller Register
 *
 *   returns  OK or ERROR
 */
STATUS
tdc1190WriteMicro(int id, UINT16 data)
{
  int kk=0;
  volatile UINT16 mstatus;

  CHECKID(id);

  LOCK_1190;

retry:

  mstatus = vmeRead16(&(c1190p[id]->microHandshake)) & V1190_MICRO_WRITEOK;

  if(mstatus)
    {
      vmeWrite16(&(c1190p[id]->microReg),data);
    }
  else
    {
      kk++;
      mstatus=0;
      if(kk>=20)
	{
	  logMsg("tdc1190WriteMicro: ERROR: Write Status not OK\n",0,0,0,0,0,0);
	  UNLOCK_1190;
	  return(ERROR);
	}
      else
	{
	  taskDelay(10);
	  goto retry;
	}
    }

  UNLOCK_1190;
  if(kk > 10) printf("-> WriteMicro: kk=%d\n",kk);
  return(OK);
}


STATUS
tdc1190GWriteMicro(UINT16 data)
{
  int id=0, ntries=0;
  volatile UINT32 mstatus=0;
  UINT32 scanmask=0;

  LOCK_1190;
  scanmask = tdc1190ScanMask();

  while((mstatus!=scanmask) || (ntries>=20))
    {
      for(id=0; id<Nc1190; id++)
	{
	  if(mstatus & (1<<id)) continue; /* Skip if it's already ready */
	  mstatus |= ((vmeRead16(&(c1190p[id]->microHandshake)) & V1190_MICRO_WRITEOK)?1:0)<<id;
	}
      ntries++;
      taskDelay(10);
    }

  if(mstatus==scanmask)
    {
      for(id=0; id<Nc1190; id++)
	{
	  vmeWrite16(&(c1190p[id]->microReg),data);
	}
    }
  else
    {
      logMsg("tdc1190GWriteMicro: ERROR: Write Status not OK (0x%x != 0x%x)\n",mstatus,scanmask,0,0,0,0);
      UNLOCK_1190;
      return(ERROR);
    }

  UNLOCK_1190;

  if(ntries > 10) logMsg("tdc1190GWriteMicro: ntries=%d\n",ntries,2,3,4,5,6);

  return(OK);
}


/*******************************************************************************
 *
 * tdc1190PrintEvent - Print an event from TDC to standard out. 
 *
 *
 * RETURNS: Number of Data words read from the TDC (including Header/Trailer).
 */

int
tdc1190PrintEvent(int id, int pflag)
{
  int ii, jj, nWords, evID, bunchID, evCount, headFlag, trigMatch;
  UINT32 gheader, gtrailer, theader, ttrailer, tmpData, dCnt;
  int tdcID, chanID, dataVal, tEdge;

  CHECKID(id);


  LOCK_1190;
  /* Check if there is a valid event */
  if(vmeRead16(&(c1190p[id]->status))&V1190_STATUS_DATA_READY) {
    dCnt = 0;
    headFlag  = vmeRead16(&(c1190p[id]->status))&V1190_STATUS_HEADER_ENABLE;
    trigMatch = vmeRead16(&(c1190p[id]->status))&V1190_STATUS_TRIG_MATCH;

    if(trigMatch) {  /* If trigger match mode then print individual event */

      /* Read Global Header - Get event count */
      gheader = vmeRead32(&(c1190p[id]->data[0]));
      if((gheader&V1190_DATA_TYPE_MASK) != V1190_GLOBAL_HEADER_DATA)
	{
	  logMsg("tdc1190PrintEvent: ERROR: Invalid Global Header Word 0x%08x\n",
		 gheader,2,3,4,5,6);
	  UNLOCK_1190;
	  return(ERROR);
	}
      else
	{
	  logMsg("  TDC DATA for Module at address 0x%08x\n",
		 (UINT32)c1190p[id],2,3,4,5,6);
	  evCount = (gheader&V1190_GHEAD_EVCOUNT_MASK)>>5;
	  dCnt++;
	  logMsg("  Global Header: 0x%08x   Event Count = %d \n",
		 gheader,evCount,3,4,5,6);
	}

      /* Loop over four TDC chips and get data for each */
      for(ii=0; ii<4; ii++)
	{
	  /* Read TDC Header - Get event ID, Bunch ID */
	  theader = vmeRead32(&(c1190p[id]->data[0]));
	  if((theader&V1190_DATA_TYPE_MASK) != V1190_TDC_HEADER_DATA)
	    {
	      logMsg("ERROR: Invalid TDC Header Word 0x%08x for TDC %d\n",
		     theader,ii,3,4,5,6);
	      UNLOCK_1190;
	      return(ERROR);
	    }
	  else
	    {
	      evID = (theader&V1190_TDCHEAD_EVID_MASK)>>12;
	      bunchID = (theader&V1190_TDCHEAD_BUNCHID_MASK);
	      dCnt++;
	      logMsg("    TDC %d Header: 0x%08x   EventID = %d  Bunch ID = %d ",
		     ii,theader,evID,bunchID,5,6);
	    }
	  jj=0;
	  tmpData = vmeRead32(&(c1190p[id]->data[0]));
	  dCnt++;
	  while((tmpData&V1190_DATA_TYPE_MASK) != V1190_TDC_EOB_DATA)
	    {
/* 	      if((jj % 5) == 0) printf("\n     "); */
	      logMsg("ch %d:   0x%08x (0x%08x)\n",
		     (tmpData&0x3e00000)>>21,(tmpData&0x1ffff),
		     (tmpData),4,5,6);
	      jj++;
	      tmpData = vmeRead32(&(c1190p[id]->data[jj]));
	    }
	  /* reached EOB for TDC */
	  logMsg("\n",1,2,3,4,5,6);
	  ttrailer = tmpData;
	  if((ttrailer&V1190_DATA_TYPE_MASK) != V1190_TDC_EOB_DATA)
	    {
	      logMsg("ERROR: Invalid TDC EOB Word 0x%08x for TDC %d\n",
		     ttrailer,ii,3,4,5,6);
	      UNLOCK_1190;
	      return(ERROR);
	    }
	  else
	    {
	      nWords = (ttrailer&V1190_TDCEOB_WORDCOUNT_MASK);
	      dCnt++;
	      logMsg("    TDC %d EOB   : 0x%08x   Word Count = %d \n",
		     ii,ttrailer,nWords,4,5,6);
	    }
	}

      /* next data word should be Global EOB */
      gtrailer = vmeRead32(&(c1190p[id]->data[dCnt]));
      if((gtrailer&V1190_DATA_TYPE_MASK) != V1190_GLOBAL_EOB_DATA)
	{
	  logMsg("tdc1190PrintEvent: ERROR: Invalid Global EOB Word 0x%08x\n",
		 gtrailer,2,3,4,5,6);
	  UNLOCK_1190;
	  return(ERROR);
	}
      else
	{
	  nWords = (gtrailer&V1190_GEOB_WORDCOUNT_MASK)>>5;
	  dCnt++;
	  logMsg("  Global EOB   : 0x%08x   Total Word Count = %d \n",
		 gtrailer,nWords,3,4,5,6);
	}
    }
    else /* Continuous Storage mode */
      {
	tmpData = vmeRead32(&(c1190p[id]->data[dCnt]));
	logMsg("  TDC Continuous Storage DATA\n",1,2,3,4,5,6);
	while((tmpData&V1190_DATA_TYPE_MASK) != V1190_FILLER_DATA)
	  {
	    tdcID  = (tmpData&V1190_TDC_MASK)>>24;
	    chanID = (tmpData&V1190_CHANNEL_MASK)>>19;
	    tEdge = (tmpData&V1190_EDGE_MASK)>>19;
	    dataVal = (tmpData&V1190_DATA_MASK);
	    logMsg("    %d   %d   %d    %d\n",tdcID, chanID, tEdge, dataVal,5,6);
	    dCnt++;
	    tmpData = vmeRead32(&(c1190p[id]->data[dCnt]));
	  }
	printf("\n");
      }
    UNLOCK_1190;
    return(dCnt);
  }
  else
    {
      logMsg("tdc1190PrintEvent: No data available for readout!\n",1,2,3,4,5,6);
      UNLOCK_1190;
      return(0);
    }
  UNLOCK_1190;
}

/*******************************************************************************
 *
 * tdc1190StatusFull - Return the status of the Output Buffer Full register
 *
 * RETURNS: 1 if Full, 0 if Not Full, -1 if TDC is disabled
 *
 */

int
tdc1190StatusFull(int id)
{
  int res;

  CHECKID(id);

  if(use1190[id]==0) 
    return -1; 

  LOCK_1190;
  res = vmeRead16(&(c1190p[id]->status)) & V1190_STATUS_FULL;
  UNLOCK_1190;

  return(res);
}

/*******************************************************************************
 *
 * tdc1190StatusAlmostFull - Return the status of the Output Buffer 
 *                           Almost Full register
 *
 * RETURNS: 1 if Almost Full level is met, 0 if not, -1 if TDC is disabled
 *
 */

int
tdc1190StatusAlmostFull(int id)
{
  int res;

  CHECKID(id);

  if(use1190[id]==0) return(-1); 

  LOCK_1190;
  res = vmeRead16(&(c1190p[id]->status)) & V1190_STATUS_ALMOST_FULL;
  UNLOCK_1190;

  return(res);
}


/******************************************************************************
 *
 * tdc1190Dready - data readyness
 *
 *
 * RETURNS: 0(No Data) or the number of events
 */

int
tdc1190Dready(int id)
{
  UINT16 stat=0, nevents;
  
  CHECKID(id);

  if(use1190[id]==0) return(ERROR); 

  LOCK_1190;
  stat = vmeRead16(&(c1190p[id]->status)) & V1190_STATUS_DATA_READY;
  if(stat)
  {
    nevents = vmeRead16(&(c1190p[id]->evStored));
    UNLOCK_1190;
    return(nevents);
  }
  else
  {
    /*logMsg("stat=%d\n",stat,2,3,4,5,6);*/
    UNLOCK_1190;
    return(0);
  }
}

uint32_t
tdc1190GDready(int blocklevel)
{
  uint32_t rval=0;
  int id;

  LOCK_1190;
  for(id=0; id<Nc1190; id++)
  {
    if(vmeRead16(&(c1190p[id]->evStored))>=blocklevel)
	rval |= 1<<id;
  }
  UNLOCK_1190;

  return(rval);

}



/******************************************************************************
 *
 * tdc1190Compensation - Set the status of the compensation of the INL
 *                flag = 1 : enable compensation
 *                       0 : disable compensation
 *
 *
 * RETURNS: 0 if successful, ERROR otherwise.
 */

int
tdc1190Compensation(int id, UINT32 flag)
{

  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  if(flag>1)
    {
      printf("%s: ERROR: Invalid flag = %d",
	     __FUNCTION__, flag);
      return ERROR;
    }

  printf("tdc1190Compensation reached, flag=%d\n",flag);fflush(stdout);

  LOCK_1190;
  if(flag == 1)
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) | V1190_COMPENSATION_ENABLE);
  else if(flag == 0)
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) & ~V1190_COMPENSATION_ENABLE);

  UNLOCK_1190;

  return(0);
}

int
tdc1190GetCompensation(int id)
{
  unsigned short reg;
  int flag;

  printf("tdc1190GetCompensation reached\n");fflush(stdout);

  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 


  LOCK_1190;
  reg = vmeRead16(&(c1190p[id]->control));
  reg = reg & V1190_COMPENSATION_ENABLE;
  UNLOCK_1190;

  if(reg) flag = 1;
  else    flag = 0;
  printf("tdc1190GetCompensation returns %d\n",flag);fflush(stdout);

  return(flag);
}

int
tdc1190GCompensation(UINT32 flag)
{
  int itdc=0;

  if(flag>1)
    {
      printf("%s: ERROR: Invalid flag = %d",
	     __FUNCTION__, flag);
      return ERROR;
    }

  LOCK_1190;
  for(itdc=0; itdc<Nc1190; itdc++)
    {
      if(use1190[itdc]==0) continue;
      if(flag == 1)
	vmeWrite16(&(c1190p[itdc]->control),
		   vmeRead16(&(c1190p[itdc]->control)) | V1190_COMPENSATION_ENABLE);
      else if(flag == 0)
	vmeWrite16(&(c1190p[itdc]->control),
		   vmeRead16(&(c1190p[itdc]->control)) & ~V1190_COMPENSATION_ENABLE);
    }
  UNLOCK_1190;

  return(0);

}

/******************************************************************************
 *
 * tdc1190ReadCompensation - Set the readback of the compensation of the INL
 *                flag = 1 : enable compensation readback
 *                       0 : disable compensation readback
 *
 *
 * RETURNS: 0 if successful, ERROR otherwise.
 */

int
tdc1190ReadCompensation(int id, UINT32 flag)
{

  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  if(flag>1)
    {
      printf("%s: ERROR: Invalid flag = %d",
	     __FUNCTION__, flag);
      return ERROR;
    }

  LOCK_1190;
  if(flag == 1)
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) | V1190_READ_COMP_SRAM_ENABLE);
  else if(flag == 0)
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) & ~V1190_READ_COMP_SRAM_ENABLE);

  UNLOCK_1190;

  return(0);
}

int
tdc1190GReadCompensation(UINT32 flag)
{
  int itdc=0;

  if(flag>1)
    {
      printf("%s: ERROR: Invalid flag = %d",
	     __FUNCTION__, flag);
      return ERROR;
    }

  LOCK_1190;
  for(itdc=0; itdc<Nc1190; itdc++)
    {
      if(use1190[itdc]==0) continue;
      if(flag == 1)
	vmeWrite16(&(c1190p[itdc]->control),
		   vmeRead16(&(c1190p[itdc]->control)) | V1190_READ_COMP_SRAM_ENABLE);
      else if(flag == 0)
	vmeWrite16(&(c1190p[itdc]->control),
		   vmeRead16(&(c1190p[itdc]->control)) & ~V1190_READ_COMP_SRAM_ENABLE);
    }
  UNLOCK_1190;

  return(0);

}


int
tdc1190GetReadCompensation(int id)
{
  int rval = 0;
  CHECKID(id);

  LOCK_1190;
  rval = (vmeRead16(&c1190p[id]->control) & V1190_READ_COMP_SRAM_ENABLE) ? 1 : 0;
  UNLOCK_1190;

  return rval;
}





/******************************************************************************
 *
 * tdc1190EventFifo - Set the status of the event fifo
 *                flag = 1 : enable event fifo
 *                       0 : disable event fifo
 *
 *
 * RETURNS: 0 if successful, ERROR otherwise.
 */

int
tdc1190EventFifo(int id, UINT32 flag)
{
  unsigned short reg;

  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  if(flag<0||flag>1)
  {
    printf("%s: ERROR: Invalid flag = %d",__FUNCTION__,flag);
    return ERROR;
  }

  LOCK_1190;
  reg = vmeRead16(&(c1190p[id]->control));
  printf("%s before: 0x%04x\n",__FUNCTION__,reg);
  if(flag == 1)
  {
    reg = reg | V1190_EVENT_FIFO_ENABLE;
    vmeWrite16(&(c1190p[id]->control),reg);
  }
  else
  {
    reg = reg & ~V1190_EVENT_FIFO_ENABLE;
    vmeWrite16(&(c1190p[id]->control),reg);
  }
  printf("%s  after: 0x%04x\n",__FUNCTION__,vmeRead16(&(c1190p[id]->control)));
  UNLOCK_1190;

  return(0);
}

int
tdc1190GetEventFifo(int id)
{
  unsigned short reg;

  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  LOCK_1190;
  reg = vmeRead16(&(c1190p[id]->control));
  UNLOCK_1190;

  return((reg&V1190_EVENT_FIFO_ENABLE)>>8);
}



/******************************************************************************
 *
 * tdc1190ResetMSCT - Reset the MSCT/CBLT register
 *                    This effectively disables MSCT/CBLT
 *
 *
 * RETURNS: 0 if successful, ERROR otherwise.
 */

int
tdc1190ResetMCST(int id)
{
  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  LOCK_1190;
  vmeWrite16(&(c1190p[id]->mcstCtrl),0);
  UNLOCK_1190;

  return(0);
}

/******************************************************************************
 *
 * tdc1190BusError - Enable/Disable Bus Errors (to finish a block transfer,
 *                   or on an empty buffer read)
 *                    flag = 0 : Disable
 *                           1 : Enable
 *
 * RETURNS: 0 if successful, ERROR otherwise.
 */

int
tdc1190BusError(int id, UINT32 flag)
{
  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  if(flag>1)
  {
    printf("%s: ERROR: Invalid flag = %d",__FUNCTION__,flag);
    return ERROR;
  }

  LOCK_1190;
  if(flag == 1)
  {
    printf("  set BUSerror\n");
    vmeWrite16(&(c1190p[id]->control),
		 vmeRead16(&(c1190p[id]->control)) | V1190_BUSERROR_ENABLE);
  }
  else if(flag == 0)
  {
    printf("  reset BUSerror\n");
    vmeWrite16(&(c1190p[id]->control),
		 vmeRead16(&(c1190p[id]->control)) & ~V1190_BUSERROR_ENABLE);
  }
  UNLOCK_1190;

  return(0);
}

int
tdc1190GetBusError(int id)
{
  int reg;

  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  LOCK_1190;
  reg = vmeRead16(&(c1190p[id]->control));
  UNLOCK_1190;

  return(reg & V1190_BUSERROR_ENABLE);
}

/******************************************************************************
 *
 * tdc1190Align64 - Enable/Disable 64 bit alignment for block transfers
 *                   flag = 0 : Disable
 *                          1 : Enable
 *
 * RETURNS: 0 if successful, ERROR otherwise.
 */

int
tdc1190Align64(int id, UINT32 flag)
{

  CHECKID(id);

  if(use1190[id]==0) 
    return ERROR; 

  if(flag>1)
    {
      printf("%s: ERROR: Invalid flag = %d",__FUNCTION__,flag);
      return ERROR;
    }

  LOCK_1190;
  printf("%s before: 0x%04x\n",__FUNCTION__,
	 vmeRead16(&(c1190p[id]->control)));

  if(flag == 1)
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) | V1190_ALIGN64);

  else if(flag == 0)
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) & ~V1190_ALIGN64);

  else
    printf("%s: unknown flag=%d\n",__FUNCTION__,flag);

  printf("%s  after: 0x%04x\n",__FUNCTION__,
	 vmeRead16(&(c1190p[id]->control)));
  UNLOCK_1190;


  return(0);
}





/*Sergey start: my mid-  and top-level functions*/

int
tdc1190ReadBoard(int itdc, UINT32 *tdata)
{
  volatile UINT32 *data;
  volatile UINT32 *fifo;
  UINT32 *output = tdata - 1;
  int fifodata, ndata, nev, ii;

  UINT32 addr = (unsigned int) c1190p[itdc];
  data = (UINT32 *) addr;
  fifo = (UINT32 *) (addr+0x1038);

  if(berr_fifo == 0x01)
  {
    /* get event length in words */
    /* do not need it here but must read fifo as soon as it is enabled,
       otherwise 'full' condition will happens */

    nev = tdc1190Dready(itdc);
    if(nev > blt_Events) nev = blt_Events;
    for(ii=0; ii<nev; ii++)
	{
      fifodata = (vmeRead32(&(c1190p[itdc]->fifo))&0xffff);
	}
  }

  do
  {
    *(++output) = vmeRead32(&(c1190p[itdc]->data[0]));
  } while( ((*output)&V1190_DATA_TYPE_MASK) != V1190_GLOBAL_EOB_DATA );

  return(((int)(output-tdata))+1);
}


int
tdc1190ReadBoardDmaStart(int ib, UINT32 *tdata)
{
  volatile UINT32 *vmeAdr;
  volatile UINT32 *fifo;
  int mdata, fifodata, res;
  int i, ii, nbytes, nev;
  int ndata_save, extra_save;

  UINT32 addr = (unsigned int) c1190p[ib];
  fifo = (UINT32 *) (addr+0x1038);

  if(berr_fifo == 0x01)
  {
    /* get event length in words */
    nev = tdc1190Dready(ib);
    if(nev > blt_Events) nev = blt_Events;
    ndata_save = 0;
    for(ii=0; ii<nev; ii++)
	{
      fifodata = (vmeRead32(&(c1190p[ib]->fifo))&0xffff);
      ndata_save += fifodata&0xffff;
	}
    /*
    logMsg("tdc1190ReadBoardDmaStart: INFO: event fifo reports %d words\n",
           ndata_save,0,0,0,0,0);
	*/
  }
  else
  {
    ndata_save = V1190_MAX_WORDS_PER_BOARD * blt_Events;
    mdata = 0;
    /*
    logMsg("tdc1190ReadBoardDmaStart: INFO: trying to DMA %d words\n",
           ndata_save,0,0,0,0,0);
    */
  }

  /*usrVmeDmaReset();*/

  if(berr_fifo == 0x01)
  {
    if(sngl_blt_mblt >= 0x04) /* 128 bit alignment */
	{
      extra_save = (4-(ndata_save%4));
	  /*
      logMsg("111: tdc1190ReadBoardDmaStart: ndata_save=%d extra_save=%d\n",
        ndata_save,extra_save,3,4,5,6);
	  */
      if(extra_save==4) extra_save=0;
	}
	else /* 64 bit alignment */
	{
      if( (ndata_save%2) != 0 ) extra_save = 1;
      else                      extra_save = 0;
	}
	/*
    logMsg("tdc1190ReadBoardDmaStart: ndata_save=%d extra_save=%d\n",
      ndata_save,extra_save,3,4,5,6);
	*/
    nbytes_save[ib] = nbytes = ((ndata_save+extra_save)<<2);
  }
  else
  {
    nbytes_save[ib] = nbytes = ndata_save<<2;
  }

  /*
printf("tdc1190ReadBoardDmaStart[%d]: c1190vme=0x%08x, tdata=0x%08x, nbytes=%d\n",
		 ib,c1190vme[ib],tdata, nbytes);
  */
  res = usrVme2MemDmaStart( (UINT32 *)c1190vme[ib], (UINT32 *)tdata, nbytes);

  if(res < 0)
  {
    logMsg("tdc1190ReadEventDmaStart: ERROR: usrVme2MemDmaStart returned %d\n",
           res,0,0,0,0,0);
  }
  /*else
  {
    logMsg("tdc1190ReadBoardDmaStart: usrVme2MemDmaStart returned %d\n",
      res,0,0,0,0,0);
  }
  */

  return(res);
}

/* returns the number of words transfered, or error */
int
tdc1190ReadBoardDmaDone(int ib)
{
  int mbytes, res;

  /* check if transfer is completed; returns nbytes or ERROR  */
  if((res = usrVme2MemDmaDone()) < 0)
  {
    logMsg("tdc1190ReadBoardDmaDone: ERROR: usrVme2MemDmaDone returned = %d\n",
           res,2,3,4,5,6);
    return(-1);
  }

  if(berr_fifo == 0x01) /*fifo readout*/
  {
    mbytes = nbytes_save[ib] - res;
    /*logMsg("%s: nbytes_save=%d res=%d -> mbytes=%d\n",__FUNCTION__,nbytes_save[id],res,mbytes,5,6);*/
    if(mbytes>0)
    {
      logMsg("%s: WRONG: nbytes_save[%d]=%d, res=%d => mbytes=%d\n",(int)__FUNCTION__,
          ib,nbytes_save[ib],res,mbytes,6);
      return(-2);
    }
  }

  return(res>>2); /* return length in words */
}


/* generic readout for v1190/v1290 TDC boards */
/* time profiling data for 2 boards 550 bytes event size */
int
tdc1190ReadStart(INT32 *tdcbuf, INT32 *rlenbuf)
{
  int ii, jj, nev, itdcbuf, itmp1;
  int nn[21];
  unsigned short tdata;
  int notready;

/* whole routine: 42 usec */

/* part1: 6 usec */

  if(Nc1190==0)
  {
    logMsg("tdc1190ReadStart: ERROR: Nc1190=%d\n",Nc1190,2,3,4,5,6);
  }

  notready = 0;
  for(jj=0; jj<Nc1190; jj++)
  {
    /* check if board is full */
    if(tdc1190StatusFull(jj))
    {
      logMsg("ERROR: [%2d] board is full - clear (Nc1190=%d)\n",
        jj,Nc1190,3,4,5,6);
      /* clear board, otherwise cannot use it any more ! */
      tdc1190Clear(jj);
    }

    /* check the number of events */
    nn[jj] = nev = tdc1190Dready(jj);

    /* if trigger matching window 'straddling' trigger, tdc1190Dready returns 0
      (V1190_STATUS_DATA_READY bit is zero),but next call returns 1 - do not know why ..*/
    if(nev == 0) nn[jj] = nev = tdc1190Dready(jj);

    if(nev == 0)
    {
      logMsg("tdc1190ReadStart: [%2d] not ready ! (nev=%d)\n",jj,tdc1190Dready(jj),3,4,5,6);
      notready = 1;
    }

    /* should never have more then 100 events in one block 
    if(nev > 100)      
    {
	  logMsg("tdc1190ReadStart: ERROR: [%2d] nev=%d\n",jj,nev,3,4,5,6);
	}
	*/
  }

  if(notready) return(ERROR);


  /* part2: 36 usec */

  /* readout */
  notready = 0;
  itdcbuf = 0;
  for(jj=0; jj<Nc1190; jj++)
  {

    if(sngl_blt_mblt == 0x01) /*no DMA*/
    {
      rlenbuf[jj] = tdc1190ReadBoard(jj,&tdcbuf[itdcbuf]);
    }
    else
    {
      /* 18usec x 2boards = 36Usec */
      tdc1190ReadBoardDmaStart(jj,&tdcbuf[itdcbuf]);
      rlenbuf[jj] = tdc1190ReadBoardDmaDone(jj);
    }

    if(rlenbuf[jj] <= 0)
    {
      logMsg("[%2d] ERROR: tdc1190ReadEvent[Dma] returns %d\n",
        jj,rlenbuf[jj],3,4,5,6);
      notready = 1;
    }
    else
    {
      itdcbuf += rlenbuf[jj];
    }

  }

  if(notready) return(ERROR);

  return(OK);
}


/* use DMA list */
int
tdc1190ReadListStart(INT32 *tdcbuf, INT32 *rlenbuf)
{
  int fifodata;
  int ii, jj, nev;
  int itdcbuf;
  static int *destination[V1190_MAX_MODULES];
  int ndata_save, extra_save;

  /*
TIMER_VAR;
  */
  if(Nc1190<2)
  {
    logMsg("tdc1190ReadListStart: ERROR: Nc1190=%d\n",Nc1190,2,3,4,5,6);
  }

  for(jj=0; jj<Nc1190; jj++)
  {
    /* check if board is full */
    if(tdc1190StatusFull(jj))
    {
      logMsg("ERROR: [%2d] board is full - clear (Nc1190=%d)\n",
        jj,Nc1190,3,4,5,6);
      /* clear board, otherwise cannot use it any more ! */
      tdc1190Clear(jj);
    }

    /* check the number of events */
    nev = tdc1190Dready(jj);

    /* if trigger matching window 'straddling' trigger, tdc1190Dready returns 0 (V1190_STATUS_DATA_READY bit is zero),
    but next call returns 1 - do not know why ..*/
    if(nev == 0) nev = tdc1190Dready(jj);

    if(nev == 0)
    {
      logMsg("tdc1190ReadListStart: [%2d] not ready !\n",jj,2,3,4,5,6);
    }

    /* Trigger Supervisor has 6 event buffer, but we can get 7
	   if 'parallel' readout is in use 
    if(nev > 7)
    {
	  logMsg("tdc1190ReadListStart: ERROR: [%2d] nev=%d\n",jj,nev,3,4,5,6);
	}
*/
  }


  if(sngl_blt_mblt == 0x01)
  {
    logMsg("ERROR: DMA method must be used: sngl_blt_mblt=%d\n",
        sngl_blt_mblt,2,3,4,5,6);
    return(ERROR);
  }



  /* readout settings */
  if(berr_fifo == 0x01) /* use FIFO reaout */
  {
    itdcbuf = 0;
    for(jj=0; jj<Nc1190; jj++)
    {

      /* get event length in words */
      nev = tdc1190Dready(jj);
      if(nev > blt_Events) nev = blt_Events;
      ndata_save = 0;
      for(ii=0; ii<nev; ii++)
	  {
        fifodata = (vmeRead32(&(c1190p[jj]->fifo))&0xffff);
        ndata_save += (fifodata & 0xffff);
	  }

      if(sngl_blt_mblt >= 0x04) /* 128 bit alignment */
	  {
        extra_save = (4-(ndata_save%4));
        if(extra_save==4) extra_save=0;
	  }
	  else /* 64 bit alignment */
	  {
        if( (ndata_save%2) != 0 ) extra_save = 1;
        else                      extra_save = 0;
	  }

      nbytes_save[jj] = (ndata_save+extra_save)<<2;
      rlenbuf[jj] = ndata_save+extra_save;

      destination[jj] = &tdcbuf[itdcbuf];

      itdcbuf += rlenbuf[jj];

	/*
logMsg("[%d] ask=%d (%d bytes), got=%d (0x%08x to 0x%08x)\n",
 jj,ndata_save+extra_save,nbytes_save[jj],rlenbuf[jj],(unsigned int)c1190p[jj],destination[jj]);
	*/

    }
  }
  else /* use BERR readout */
  {
    itdcbuf = 0;
    for(jj=0; jj<Nc1190; jj++)
    {
      ndata_save = V1190_MAX_WORDS_PER_BOARD;
      extra_save = 0;

      nbytes_save[jj] = (ndata_save)<<2;
      rlenbuf[jj] = ndata_save;

      destination[jj] = &tdcbuf[itdcbuf];

      itdcbuf += rlenbuf[jj];
	} 
  }


  /*
{
  TIMER_NORMALIZE;
  TIMER_START;
}
*/

  /* set DMA list and start DMA */
  usrVme2MemDmaListSet(c1190vme, destination, nbytes_save, Nc1190);
  usrVmeDmaListStart();

  /*
{
  TIMER_STOP(100000,-1);
}
*/
  return(OK);
}

int
tdc1190ReadDone()
{
  int mbytes, res, ii, nbytes_saved = 0;

  /* check if transfer is completed; returns nbytes or ERROR  */
  if((res = usrVme2MemDmaDone()) < 0)
  {
    logMsg("tdc1190ReadDone: ERROR: usrVme2MemDmaDone returned = %d\n",
           res,2,3,4,5,6);
    return(-1);
  }

  if(berr_fifo == 0x01) /*fifo readout*/
  {
    for(ii=0; ii<Nc1190; ii++) nbytes_saved += nbytes_save[ii];
    mbytes = nbytes_saved - res;
    /*logMsg("%s: nbytes_saved=%d res=%d -> mbytes=%d\n",__FUNCTION__,nbytes_saved,res,mbytes,5,6);*/
    if(mbytes>0)
    {
      logMsg("%s: WRONG: nbytes_saved=%d, res=%d => mbytes=%d\n",(int)__FUNCTION__,
          nbytes_saved,res,mbytes,5,6);
      return(-2);
    }
  }

  return(res>>2); /* return length in words */
}

/*Sergey end: my mid-  and top-level functions*/




/*******************************************************************************
 *
 * tdc1190ReadEvent - Read event from TDC to specified address. 
 *
 * // FIXME: May just remove this routine..
 *
 * RETURNS: Number of Data words read from the TDC (including Header/Trailer).
 */
/* FIXME: Make sure endian-ness is correct */

int
tdc1190ReadEvent(int id, UINT32 *tdata)
{
/*   int ii, nWords, evID; */
  UINT32 header, trailer, dCnt, tmpData;
  UINT16 contReg, statReg;
  int fifodata, nev, ii;

  CHECKID(id);

  LOCK_1190;
  /* Check if there is a valid event */
  if(vmeRead16(&(c1190p[id]->evStored))==0) 
    {
      logMsg("tdc1190ReadEvent: Data Buffer is EMPTY!\n",0,0,0,0,0,0);
      UNLOCK_1190;
      return(0);
    }

  statReg = vmeRead16(&(c1190p[id]->status));
  contReg = vmeRead16(&(c1190p[id]->control));
  if(statReg & V1190_STATUS_DATA_READY) 
    {
      dCnt = 0;

    /* Check to see if the Event FIFO is enabled, 
	 if so, read its register */
    if(contReg & V1190_EVENT_FIFO_ENABLE)
	{
      nev = tdc1190Dready(id);
      if(nev > blt_Events) nev = blt_Events;
      for(ii=0; ii<nev; ii++)
	  {
	    fifodata = vmeRead32(&(c1190p[id]->fifo));
	  }
	}

	/* Read Header  */
	header = vmeRead32(&(c1190p[id]->data[dCnt]));
      if((header&V1190_DATA_TYPE_MASK) != V1190_TDC_HEADER_DATA) 
	{
	  logMsg("tdc1190ReadEvent: ERROR: Invalid Header Word 0x%08x\n",header,0,0,0,0,0);
	  UNLOCK_1190;
	  return(-1);
	}
      else
	{
	  tdata[dCnt] = header;
	  dCnt++;
	  tmpData = vmeRead32(&(c1190p[id]->data[0]));
	  while((tmpData&V1190_DATA_TYPE_MASK) != V1190_TDC_EOB_DATA )
	    {
	      tdata[dCnt] = tmpData;
	      dCnt++;
	      tmpData = vmeRead32(&(c1190p[id]->data[0]));
	    }
	}

      /* reached EOB for TDC */
      trailer = tmpData;
      if((trailer&V1190_DATA_TYPE_MASK) != V1190_TDC_EOB_DATA) {
	logMsg("tdc1190ReadEvent: ERROR: Invalid Trailer Word 0x%08x\n",trailer,0,0,0,0,0);
	UNLOCK_1190;
	return(-1);
      }else{
	tdata[dCnt] = trailer;
	dCnt++;
      }
      UNLOCK_1190;
      return (dCnt);
      
    }else{
    logMsg("tdc1190ReadEvent: Data Not ready for readout!\n",0,0,0,0,0,0);
    UNLOCK_1190;
    return(0);
  }

}


/******************************************************************************
 *
 * tdc1190ReadData - available data into a buffer. 
 *
 *
 * RETURNS: Number of Data words read from the TDC (including Header/Trailer).
 */
/* FIXME: Make sure endian-ness is correct */

int
tdc1190ReadData(int id, UINT32 *tdata, int maxWords)
{
  int ii, jj, nWords, evID, bunchID, evCount, headFlag, trigMatch;
  UINT32 gheader, gtrailer, theader, ttrailer, tmpData, dCnt;

  CHECKID(id);

  if(maxWords==0) maxWords = 1024;

  /* Check if there is a valid event */
  LOCK_1190;

  if(vmeRead16(&(c1190p[id]->status)) & V1190_STATUS_DATA_READY)
    {
      dCnt = 0;
      headFlag  = vmeRead16(&(c1190p[id]->status)) & V1190_STATUS_HEADER_ENABLE;
      trigMatch = vmeRead16(&(c1190p[id]->status)) & V1190_STATUS_TRIG_MATCH;

      if(trigMatch) /* If trigger match mode then read individual event */
	{
	  /* Read Global Header - Get event count */
	  gheader = vmeRead32(&(c1190p[id]->data[0]));
	  if((gheader & V1190_DATA_TYPE_MASK) != V1190_GLOBAL_HEADER_DATA)
	    {
	      logMsg("tdc1190ReadData: ERROR: Invalid Global Header Word 0x%08x\n",
		     gheader,2,3,4,5,6);
	      UNLOCK_1190;
	      return(ERROR);
	    }
	  else
	    {
	      tdata[dCnt] = gheader;
	      evCount = (gheader & V1190_GHEAD_EVCOUNT_MASK) >> 5;
	      dCnt++;
	    }

	  /* Loop over four TDC chips and get data for each */
	  for(ii=0; ii<4; ii++)
	    {
	      /* Read TDC Header - Get event ID, Bunch ID */
	      theader = vmeRead32(&(c1190p[id]->data[0]));
	      if((theader&V1190_DATA_TYPE_MASK) != V1190_TDC_HEADER_DATA)
		{
		  logMsg("ERROR: Invalid TDC Header Word 0x%08x for TDC %d\n",
			 theader,ii,3,4,5,6);
		  UNLOCK_1190;
		  return(ERROR);
		}
	      else
		{
		  tdata[dCnt] = theader;
		  evID = (theader&V1190_TDCHEAD_EVID_MASK)>>12;
		  bunchID = (theader&V1190_TDCHEAD_BUNCHID_MASK);
		  dCnt++;
		}
	      jj=0;
	      tmpData = vmeRead32(&(c1190p[id]->data[0]));
	      while((tmpData&V1190_DATA_TYPE_MASK) != V1190_TDC_EOB_DATA)
		{
		  tdata[dCnt] = tmpData;
		  jj++;
		  dCnt++;
		  tmpData = vmeRead32(&(c1190p[id]->data[jj]));
		}

	      /* reached EOB for TDC */
	      ttrailer = tmpData;
	      if((ttrailer&V1190_DATA_TYPE_MASK) != V1190_TDC_EOB_DATA)
		{
		  logMsg("ERROR: Invalid TDC EOB Word 0x%08x for TDC %d\n",
			 ttrailer,ii,3,4,5,6);
		  UNLOCK_1190;
		  return(ERROR);
		}
	      else
		{
		  tdata[dCnt] = ttrailer;
		  nWords = (ttrailer&V1190_TDCEOB_WORDCOUNT_MASK);
		  dCnt++;
		}
	    }

	  /* next data word should be Global EOB */
	  gtrailer = vmeRead32(&(c1190p[id]->data[dCnt]));
	  if((gtrailer&V1190_DATA_TYPE_MASK) != V1190_GLOBAL_EOB_DATA)
	    {
	      logMsg("tdc1190ReadData: ERROR: Invalid Global EOB Word 0x%08x\n",
		     gtrailer,2,3,4,5,6);
	      UNLOCK_1190;
	      return(ERROR);
	    }
	  else
	    {
	      tdata[dCnt] = gtrailer;
	      nWords = (gtrailer&V1190_GEOB_WORDCOUNT_MASK)>>5;
	      dCnt++;
	    }
	}
      else /* Continuous Storage mode */
	{
	  tmpData = vmeRead32(&(c1190p[id]->data[dCnt]));
	  while(((tmpData&V1190_DATA_TYPE_MASK) != V1190_FILLER_DATA) &&
		(dCnt<maxWords))
	    {
	      tdata[dCnt]=tmpData;
	      dCnt++;
	      tmpData = vmeRead32(&(c1190p[id]->data[dCnt]));
	    }
	}
      UNLOCK_1190;
      return(dCnt);
    }
  else
    {
      logMsg("tdc1190ReadData: No data available for readout!\n",1,2,3,4,5,6);
      UNLOCK_1190;
      return(0);
    }
}

/******************************************************************************
 * ACQUISITION MODE OPCODES Routines
 *
 *  All routines return OK or ERROR,
 *   unless otherwise specified.
 * 
 * tdc1190SetTriggerMatchingMode
 * tdc1190SetContinuousStorageMode
 * tdc1190ReadAcquisitionMode         - Returns 0: Continuous storage mode
 *                                              1: Trigger matching mode
 * tdc1190SetKeepToken
 * tdc1190ClearKeepToken
 * tdc1190LoadDefaultConfiguration
 * tdc1190SaveUserConfiguration
 * tdc1190LoadUserConfiguration
 * tdc1190SetAutoLoadUserConfiguration
 * tdc1190SetAutoLoadDefaultConfiguration
 *
*/

STATUS
tdc1190SetTriggerMatchingMode(int id)
{
  STATUS status;
  CHECKID(id);
  status = tdc1190WriteMicro(id,0x0000);
  tdc1190Clear(id);
  return status;
}

STATUS
tdc1190SetContinuousStorageMode(int id)
{
  STATUS status;
  CHECKID(id);
  status = tdc1190WriteMicro(id,0x0100);
  tdc1190Clear(id);
  return status;
}

int
tdc1190ReadAcquisitionMode(int id)
{
  UINT16 tdata;
  int status;
  int rval;

  CHECKID(id);
  tdc1190WriteMicro(id,0x0200);
  status = tdc1190ReadMicro(id,&tdata,1);
  if(status==ERROR)
    {
      rval = ERROR;
    }
  else 
    {
      rval = tdata&0x1;
    }

  return rval;
}

STATUS
tdc1190GReadAcquisitionMode(UINT16 *acqmode)
{
  STATUS status;

  tdc1190GWriteMicro(0x0200);
  status = tdc1190GReadMicro(acqmode, 1);

  return status;
}

STATUS
tdc1190SetKeepToken(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x0300);
}

STATUS
tdc1190ClearKeepToken(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x0400);
}

STATUS
tdc1190LoadDefaultConfiguration(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x0500);
}

STATUS
tdc1190SaveUserConfiguration(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x0600);
}

STATUS 
tdc1190LoadUserConfiguration(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x0700);
}

STATUS
tdc1190SetAutoLoadUserConfiguration(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x0800);
}

STATUS
tdc1190SetAutoLoadDefaultConfiguration(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x0900);
}




/******************************************************************************
 * TRIGGER MODE OPCODES Routines
 *
 *  All routines return OK or ERROR,
 *   unless otherwise specified.
 *  All parameters in tdc1190Set* are in steps of 25ns
 * 
 * tdc1190SetWindowWidth                - Set window width [25ns,51175ns]
 * tdc1190SetWindowOffset               - Set window offset > -800000ns
 * tdc1190SetExtraSearchMargin          - Set window extra search margin [0,1250ns]
 * tdc1190SetRejectMargin               - Set reject margin [0,1250ns]
 * tdc1190EnableTriggerTimeSubtraction
 * tdc1190DisableTriggerTimeSubtraction
 * tdc1190ReadTriggerConfiguration
 *
 */

STATUS
tdc1190SetWindowWidth(int id, UINT32 window_width)
{
  UINT16 tdata;

  CHECKID(id);
  tdc1190WriteMicro(id,0x1000);

  if(window_width < 25)         
    tdata = 1;
  else if (window_width > 51175) 
    tdata = 0x7FF;
  else
    tdata = window_width/25;

  printf("%s(%d): Set Window Width to %d ns\n",__FUNCTION__,id,tdata*25);

  return tdc1190WriteMicro(id,tdata);
}


/* window offset can be set from -2048 (0xF800) to +40 (0x0028) */
/* default: tdata=0xFFD8 means -1 millisec (0xFFFF-0xFFD8=39, 39*25=975ns) */
STATUS
tdc1190SetWindowOffset(int id, INT32 window_offset)
{
  INT16 tdata;

  CHECKID(id);
  tdc1190WriteMicro(id,0x1100);

  if(window_offset < -800000) 
    tdata = -32000;
  else             
    tdata = window_offset/25;

  printf("%s(%d): Set Window Offset to %d ns (tdata=0x%04x)\n",__FUNCTION__,id,tdata*25,(unsigned short)tdata);

  return tdc1190WriteMicro(id,tdata);

}

STATUS
tdc1190SetExtraSearchMargin(int id, UINT32 window_extra)
{
  UINT16 tdata;

  CHECKID(id);
  tdc1190WriteMicro(id,0x1200);

  if(window_extra < 0)         
    tdata = 0;
  else if (window_extra > 1250) 
    tdata = 50;
  else               
    tdata = window_extra/25;
  printf("%s(%d): Set Extra Search Margin to %d ns\n",__FUNCTION__,id,tdata*25);

  return tdc1190WriteMicro(id,tdata);

}

STATUS
tdc1190SetRejectMargin(int id, UINT32 window_reject)
{
  UINT16 tdata;

  CHECKID(id);
  tdc1190WriteMicro(id,0x1300);

  if(window_reject < 0)
    tdata = 0;
  else if (window_reject > 1250) 
    tdata = 50;
  else               
    tdata = window_reject/25;

  printf("%s(%d): Set Reject Margin to %d ns\n",__FUNCTION__,id,tdata*25);

  return tdc1190WriteMicro(id,tdata);

}

STATUS
tdc1190EnableTriggerTimeSubtraction(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x1400);
}

STATUS
tdc1190DisableTriggerTimeSubtraction(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x1500);
}

STATUS
tdc1190ReadTriggerConfiguration(int id)
{
  INT16 tmp[5] = {0,0,0,0,0};
  CHECKID(id);

  EIEIO;
  SynC;
  tdc1190WriteMicro(id,0x1600);
  EIEIO;
  SynC;

  /*tdc1190ReadMicro(id,tmp,5);*/
  tdc1190ReadMicro(id,&tmp[0],1);
  tdc1190ReadMicro(id,&tmp[1],1);
  tdc1190ReadMicro(id,&tmp[2],1);
  tdc1190ReadMicro(id,&tmp[3],1);
  tdc1190ReadMicro(id,&tmp[4],1);

  EIEIO;
  SynC;

  printf(" %s(%d):\n",__FUNCTION__,id);
  printf("  Window Width              = %6d ns\n",tmp[0]*25);
  printf("  Window Offset             = %6d ns\n",tmp[1]*25);
  printf("  Extra Seach Margin        = %6d ns\n",tmp[2]*25);
  printf("  Reject Margin             = %6d ns\n",tmp[3]*25);
  printf("  Trigger Time Subtraction  = %6d\n",tmp[4]);

  return(OK);
}

STATUS
tdc1190GReadTriggerConfiguration(UINT16 *trigcfg)
{
  STATUS status;
  tdc1190GWriteMicro(0x1600);
  status = tdc1190GReadMicro(trigcfg,5);

  return(status);
}

/******************************************************************************
 * EDGE DETECTION AND RESOLUTION OPCODES Routines
 *
 *  All routines return OK or ERROR,
 *   unless otherwise specified.
 *
 * tdc1190SetEdgeDetectionConfig   
 * tdc1190ReadEdgeDetectionConfig - Returns:
 *                                    0 : Pair mode
 *                                    1 : Trailing Edge
 *                                    2 : Leading Edge
 *                                    3 : Both leading and trailing edge
 * tdc1190SetEdgeResolution
 * tdc1190GetEdgeResolution       - Returns edge resolution (ns)
 * tdc1190SetPairResolution 
 * tdc1190GetPairResolution       - Returns 2nd and 3rd argument (ns)
 * tdc1190SetDoubleHitResolution
 * tdc1190GetDoubleHitResolution  - Returns double hit resolution (ns)
 *
*/

STATUS
tdc1190SetEdgeDetectionConfig(int id, UINT16 setedge)
{
  UINT16 tdata=0;

  CHECKID(id);

  printf("%s(%d): ",__FUNCTION__,id);
  if(setedge==0)
    {
      tdata=0x0;
      printf("set pair mode\n");
    }
  else if(setedge==1)
    {
      tdata=0x1;
      printf("set trailing edge only\n");
    }
  else if(setedge==2)
    {
      tdata=0x2;
      printf("set leading edge only\n");
    }
  else
    {
      printf("set both leading and trailing edges\n");
      tdata=0x3;
    }

  tdc1190WriteMicro(id,0x2200);
  return tdc1190WriteMicro(id,tdata);

}

int
tdc1190ReadEdgeDetectionConfig(int id)
{
  UINT16 tdata;

  CHECKID(id);

  tdc1190WriteMicro(id,0x2300);
  tdc1190ReadMicro(id,&tdata,1);

  tdata &= 0x3;
  if(tdata<3)
    {
      printf(" %s(%d): Mode =",__FUNCTION__,id);
      if(tdata==0)
	printf(" pair mode\n");
      else if(tdata==1)
	printf(" trailing edge\n");
      else if(tdata==2)
	printf(" leading edge\n");
      else if(tdata==3)
	printf(" both leading and trailing\n");
    }
  else
    {
      printf("%s(%d) ERROR: tdata=%d (0x%x)\n",__FUNCTION__,id,tdata,tdata);
      return(ERROR);
    }

  return(tdata);
}

STATUS
tdc1190GReadEdgeDetectionConfig(UINT16 *edgedetect)
{
  STATUS status;

  tdc1190GWriteMicro(0x2300);
  status = tdc1190GReadMicro(edgedetect,1);

  return status;
}

STATUS
tdc1190SetEdgeResolution(int id, UINT16 edge_res)
{
  UINT16 tdata;

  CHECKID(id);

  if(edge_res==800)      
    tdata = 0;
  else if(edge_res==200) 
    tdata = 1;
  else if(edge_res==100) 
    tdata = 2;
  else
    {
      printf("%s(%d): ERROR: Invalid edge_res=%d, must be 100, 200 or 800 ps\n",
	     __FUNCTION__,id,edge_res);
      return(ERROR);
    }

  tdc1190WriteMicro(id,0x2400);
  tdc1190WriteMicro(id,tdata);
  printf("%s(%d): Set Edge Resolution to %d ps\n",__FUNCTION__,id,edge_res);

  return(OK);
}

int
tdc1190GetEdgeResolution(int id)
{
  UINT16 tdata=0;
  int rval;

  CHECKID(id);

  tdc1190WriteMicro(id,0x2600);
  tdc1190ReadMicro(id,&tdata,1);

  if(use1190[id]==1)
  {
    tdata &= 0x3;
    if(tdata==0)      rval = 800;
    else if(tdata==1) rval = 200;
    else if(tdata==2) rval = 100;
    else
    {
	  printf("%s(%d): ERROR: tdata=%d (0x%x)\n",__FUNCTION__,id,tdata,tdata);
	  return(ERROR);
    }
  }
  else
  {
    rval = 25;
  }
  /*
  printf("%s(%d): Edge Resolution is %d ps\n",__FUNCTION__,id,rval);
  */

  return(rval);
}

STATUS
tdc1190GGetEdgeResolution(UINT16 *res)
{
  STATUS status;

  tdc1190GWriteMicro(0x2600);
  status = tdc1190GReadMicro(res,1);

  return status;
}

STATUS
tdc1190SetPairResolution(int id, UINT32 leading_res, UINT32 pulse_width_res)
{
  UINT16 tdata;

  CHECKID(id);

  if(leading_res==100)            tdata = 0x0;
  else if(leading_res==200)       tdata = 0x1;
  else if(leading_res==400)       tdata = 0x2;
  else if(leading_res==800)       tdata = 0x3;
  else if(leading_res==1600)      tdata = 0x4;
  else if(leading_res==3120)      tdata = 0x5;
  else if(leading_res==6250)      tdata = 0x6;
  else if(leading_res==12500)     tdata = 0x7;
  else
    {
      printf("%s(%d): ERROR: Invalid leading_res=%d\n",__FUNCTION__,id,
	     leading_res);
      return(ERROR);
    }

  if(pulse_width_res==100)         tdata += 0x0;
  else if(pulse_width_res==200)    tdata += 0x100;
  else if(pulse_width_res==400)    tdata += 0x200;
  else if(pulse_width_res==800)    tdata += 0x300;
  else if(pulse_width_res==1600)   tdata += 0x400;
  else if(pulse_width_res==3200)   tdata += 0x500;
  else if(pulse_width_res==6250)   tdata += 0x600;
  else if(pulse_width_res==12500)  tdata += 0x700;
  else if(pulse_width_res==25000)  tdata += 0x800;
  else if(pulse_width_res==50000)  tdata += 0x900;
  else if(pulse_width_res==100000) tdata += 0xA00;
  else if(pulse_width_res==200000) tdata += 0xB00;
  else if(pulse_width_res==400000) tdata += 0xC00;
  else if(pulse_width_res==800000) tdata += 0xD00;
  else
    {
      printf("%s(%d): ERROR: Invalid pulse_width_res=%d\n",__FUNCTION__,id,
	     pulse_width_res);
      return(ERROR);
    }

  tdc1190WriteMicro(id,0x2500);
  tdc1190WriteMicro(id,tdata);
  printf("%s(%d): Set Pair Resolution to %d(leading time), %d(width) ps\n",
	 __FUNCTION__,id,leading_res,pulse_width_res);

  return(OK);
}

STATUS
tdc1190GetPairResolution(int id, UINT32 *leading_res, UINT32 *pulse_width_res)
{
  UINT16 tdata, tdata1, tdata2;

  CHECKID(id);

  tdc1190WriteMicro(id,0x2600);
  tdc1190ReadMicro(id,&tdata,1);

  tdata1 = tdata & 0x7;
  if(tdata1 == 0x0)      *leading_res = 100;
  else if(tdata1 == 0x1) *leading_res = 200;
  else if(tdata1 == 0x2) *leading_res = 400;
  else if(tdata1 == 0x3) *leading_res = 800;
  else if(tdata1 == 0x4) *leading_res = 1600;
  else if(tdata1 == 0x5) *leading_res = 3120;
  else if(tdata1 == 0x6) *leading_res = 6250;
  else if(tdata1 == 0x7) *leading_res = 12500;
  else
    {
      printf("ERROR: GetPairResolution: tdata1=%d\n",tdata1);
      return(ERROR);
    }

  tdata2 = tdata & 0xF00;
  if(tdata2 == 0x0)        *pulse_width_res = 100;
  else if(tdata2 == 0x100) *pulse_width_res = 200;
  else if(tdata2 == 0x200) *pulse_width_res = 400;
  else if(tdata2 == 0x300) *pulse_width_res = 800;
  else if(tdata2 == 0x400) *pulse_width_res = 1600;
  else if(tdata2 == 0x500) *pulse_width_res = 3200;
  else if(tdata2 == 0x600) *pulse_width_res = 6250;
  else if(tdata2 == 0x700) *pulse_width_res = 12500;
  else if(tdata2 == 0x800) *pulse_width_res = 25000;
  else if(tdata2 == 0x900) *pulse_width_res = 50000;
  else if(tdata2 == 0xA00) *pulse_width_res = 100000;
  else if(tdata2 == 0xB00) *pulse_width_res = 200000;
  else if(tdata2 == 0xC00) *pulse_width_res = 400000;
  else if(tdata2 == 0xD00) *pulse_width_res = 800000;
  else
    {
      printf("ERROR: GetPairResolution: tdata2=%d\n",tdata2);
      return(ERROR);
    }
  printf("Pair Resolution is %d(leading time), %d(width) ps\n",*leading_res,*pulse_width_res);

  return(OK);
}

STATUS
tdc1190SetDoubleHitResolution(int id, UINT16 doublehit_res)
{
  UINT16 tdata;

  CHECKID(id);

  if(doublehit_res==5)        tdata = 0;
  else if(doublehit_res==10)  tdata = 1;
  else if(doublehit_res==30)  tdata = 2;
  else if(doublehit_res==100) tdata = 3;
  else
    {
      printf("%s(%d): ERROR: Invalid doublehit_res=%d, must be 5, 10, 30 or 100 ns\n",
	     __FUNCTION__,id,doublehit_res);
      return(ERROR);
    }

  tdc1190WriteMicro(id,0x2800);
  tdc1190WriteMicro(id,tdata);
  printf("%s(%d): Set Double Hit Resolution to %d ns\n",__FUNCTION__,id,doublehit_res);

  return(OK);
}

int
tdc1190GetDoubleHitResolution(int id)
{
  UINT16 tdata;
  int rval;

  CHECKID(id);

  tdc1190WriteMicro(id,0x2900);
  tdc1190ReadMicro(id,&tdata,1);

  tdata &= 0x3;
  if(tdata==0)      rval = 5;
  else if(tdata==1) rval = 10;
  else if(tdata==2) rval = 30;
  else if(tdata==3) rval = 100;
  else
    {
      printf("%s(%d): ERROR: tdata=%d (0x%x)\n",__FUNCTION__,id,tdata,tdata);
      return(ERROR);
    }
  printf("w%s(%d): Double Hit Resolution is %d ns\n",__FUNCTION__,id,rval);

  return(rval);
}

STATUS
tdc1190GGetDoubleHitResolution(UINT16 *double_res)
{
  STATUS status;

  tdc1190GWriteMicro(0x2900);
  status = tdc1190GReadMicro(double_res,1);

  return status;
}

/******************************************************************************
 * TDC READOUT OPCODES Routines
 *
 *  All routines return OK or ERROR,
 *   unless otherwise specified.
 *
 * tdc1190EnableTDCHeaderAndTrailer
 * tdc1190DisableTDCHeaderAndTrailer
 * tdc1190GetTDCHeaderAndTrailer        - Returns 
 *                                         0 : header/trailer disabled
 *                                         1 : header/trailer enabled
 * tdc1190SetMaxNumberOfHitsPerEvent
 * tdc1190GetMaxNumberOfHitsPerEvent    - Returns maximum number of hits/event
 * tdc1190EnableTDCErrorMark
 * tdc1190DisableTDCErrorMark
 * tdc1190EnableTDCErrorBypass
 * tdc1190DisableTDCErrorBypass
 * tdc1190SetTDCErrorType
 * tdc1190GetTDCErrorType               - Returns mask of enabled error types
 * tdc1190SetEffectiveSizeOfReadoutFIFO
 * tdc1190GetEffectiveSizeOfReadoutFIFO - Returns number of words in readout FIFO
 *
 */

STATUS
tdc1190EnableTDCHeaderAndTrailer(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x3000);
}

STATUS
tdc1190DisableTDCHeaderAndTrailer(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x3100);
}

int
tdc1190GetTDCHeaderAndTrailer(int id)
{
  UINT16 tdata;

  CHECKID(id);

  tdc1190WriteMicro(id,0x3200);
  tdc1190ReadMicro(id,&tdata,1);

  tdata &= 0x1;
  if(tdata==0)
    printf("%s(%d): TDC Header/Trailer disabled\n",__FUNCTION__,id);
  else if(tdata==1)
    printf("%s(%d): TDC Header/Trailer enabled\n",__FUNCTION__,id);

  return(tdata);
}

STATUS
tdc1190GGetTDCHeaderAndTrailer(UINT16 *headtrl)
{
  STATUS status;

  tdc1190GWriteMicro(0x3200);
  status = tdc1190GReadMicro(headtrl,1);

  return status;
}

STATUS
tdc1190SetMaxNumberOfHitsPerEvent(int id, UINT32 nhits)
{
  UINT16 tdata;

  CHECKID(id);

  tdc1190WriteMicro(id,0x3300);

  if(nhits==0)        tdata = 0;
  else if(nhits==1)   tdata = 1;
  else if(nhits==2)   tdata = 2;
  else if(nhits==4)   tdata = 3;
  else if(nhits==8)   tdata = 4;
  else if(nhits==16)  tdata = 5;
  else if(nhits==32)  tdata = 6;
  else if(nhits==64)  tdata = 7;
  else if(nhits==128) tdata = 8;
  else                tdata = 9;

  if(tdata==9)
    printf("%s(%d): Set Unlimited Number Of Hits Per Event\n",__FUNCTION__,id);
  else
    printf("%s(%d): Set Maximum Number Of Hits Per Event to %d\n",__FUNCTION__,id,nhits);

  return tdc1190WriteMicro(id,tdata);

}

int
tdc1190GetMaxNumberOfHitsPerEvent(int id)
{
  UINT16 tdata;
  int rval;

  CHECKID(id);

  tdc1190WriteMicro(id,0x3400);
  tdc1190ReadMicro(id,&tdata,1);

  tdata &= 0xF;
  if(tdata==0)      rval = 0;
  else if(tdata==1) rval = 1;
  else if(tdata==2) rval = 2;
  else if(tdata==3) rval = 4;
  else if(tdata==4) rval = 8;
  else if(tdata==5) rval = 16;
  else if(tdata==6) rval = 32;
  else if(tdata==7) rval = 64;
  else if(tdata==8) rval = 128;
  else if(tdata==9) rval = 9999;
  else
    {
      printf("%s(%d): ERROR: tdata=%d (0x%x)\n",__FUNCTION__,id,tdata,tdata);
      return(ERROR);
    }

  return(rval);
}

STATUS
tdc1190GGetMaxNumberOfHitsPerEvent(UINT16 *maxhits)
{
  STATUS status;

  tdc1190GWriteMicro(0x3400);
  status = tdc1190GReadMicro(maxhits,1);

  return status;
}

STATUS
tdc1190EnableTDCErrorMark(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x3500);
}

STATUS
tdc1190DisableTDCErrorMark(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x3600);
}

STATUS
tdc1190EnableTDCErrorBypass(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x3700);
}

STATUS
tdc1190DisableTDCErrorBypass(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x3800);
}

STATUS
tdc1190SetTDCErrorType(int id, UINT32 mask)
{
  UINT16 tdata;

  CHECKID(id);

  tdata = mask & 0x7FF;
  tdc1190WriteMicro(id,0x3900);
  return tdc1190WriteMicro(id,tdata);

}

int
tdc1190GetTDCErrorType(int id)
{
  UINT16 tdata;

  CHECKID(id);

  tdc1190WriteMicro(id,0x3A00);
  tdc1190ReadMicro(id,&tdata,1);

  tdata &= 0x7FF;
  if(tdata & 0x1)
    printf("%s(%d): Vernier error\n",__FUNCTION__,id);
  else if(tdata & 0x2)
    printf("%s(%d): Coarse error\n",__FUNCTION__,id);
  else if(tdata & 0x4)
    printf("%s(%d): Channel select error\n",__FUNCTION__,id);
  else if(tdata & 0x8)
    printf("%s(%d): L1 buffer parity error\n",__FUNCTION__,id);
  else if(tdata & 0x10)
    printf("%s(%d): Trigger fifo parity error\n",__FUNCTION__,id);
  else if(tdata & 0x20)
    printf("%s(%d): Trigger matching error\n",__FUNCTION__,id);
  else if(tdata & 0x40)
    printf("%s(%d): Readout fifo parity error\n",__FUNCTION__,id);
  else if(tdata & 0x80)
    printf("%s(%d): Readout state error\n",__FUNCTION__,id);
  else if(tdata & 0x100)
    printf("%s(%d): Set up parity error\n",__FUNCTION__,id);
  else if(tdata & 0x200)
    printf("%s(%d): Control parity error\n",__FUNCTION__,id);
  else if(tdata & 0x400)
    printf("%s(%d): Jtag instruction parity error\n",__FUNCTION__,id);

  return(tdata);
}

STATUS
tdc1190GGetTDCErrorType(UINT16 *errortype)
{
  STATUS status;

  tdc1190GWriteMicro(0x3A00);
  status = tdc1190GReadMicro(errortype,1);

  return status;
}

STATUS
tdc1190SetEffectiveSizeOfReadoutFIFO(int id, UINT32 nwords)
{
  UINT16 tdata;

  CHECKID(id);

  tdc1190WriteMicro(id,0x3B00);

  if(nwords==2)        tdata = 0;
  else if(nwords==4)   tdata = 1;
  else if(nwords==8)   tdata = 2;
  else if(nwords==16)  tdata = 3;
  else if(nwords==32)  tdata = 4;
  else if(nwords==64)  tdata = 5;
  else if(nwords==128) tdata = 6;
  else if(nwords==256) tdata = 7;
  else                 tdata = 7;

  printf("%s(%d): Set Effective Size Of Readout FIFO to %d\n",__FUNCTION__,id,nwords);

  return tdc1190WriteMicro(id,tdata);

}

int
tdc1190GetEffectiveSizeOfReadoutFIFO(int id)
{
  UINT16 tdata;

  CHECKID(id);

  tdc1190WriteMicro(id,0x3C00);
  tdc1190ReadMicro(id,&tdata,1);

  tdata &= 0x7;
  if(tdata==0)
    printf("%s(%d): 2 words\n",__FUNCTION__,id);
  else if(tdata==1)
    printf("%s(%d): 4 words\n",__FUNCTION__,id);
  else if(tdata==2)
    printf("%s(%d): 8 words\n",__FUNCTION__,id);
  else if(tdata==3)
    printf("%s(%d): 16 words\n",__FUNCTION__,id);
  else if(tdata==4)
    printf("%s(%d): 32 words\n",__FUNCTION__,id);
  else if(tdata==5)
    printf("%s(%d): 64 words\n",__FUNCTION__,id);
  else if(tdata==6)
    printf("%s(%d): 128 words\n",__FUNCTION__,id);
  else if(tdata==7)
    printf("%s(%d): 256 words\n",__FUNCTION__,id);

  return(tdata);
}

STATUS
tdc1190GGetEffectiveSizeOfReadoutFIFO(UINT16 *fifosize)
{
  STATUS status;

  tdc1190GWriteMicro(0x3C00);
  status = tdc1190GReadMicro(fifosize,1);

  return status;
}

/******************************************************************************
 * CHANNEL ENABLE OPCODES Routines
 *
 *  All routines return OK or ERROR,
 *   unless otherwise specified.
 *
 * tdc1190EnableChannel
 * tdc1190DisableChannel
 * tdc1190EnableAllChannels
 * tdc1190DisableAllChannels
 * tdc1190EnableChannels
 * tdc1190GetChannels        - Returns mask of enabled channels
 *
*/


STATUS
tdc1190EnableChannel(int id, UINT16 channel)
{
  int opcode = 0x4000 + (channel & 0x7F);

  CHECKID(id);

  return tdc1190WriteMicro(id,opcode);
}

STATUS
tdc1190DisableChannel(int id, UINT16 channel)
{
  int opcode = 0x4100 + (channel & 0x7F);

  CHECKID(id);
  return tdc1190WriteMicro(id,opcode);
}

STATUS
tdc1190EnableAllChannels(int id)
{
  CHECKID(id);
  return tdc1190WriteMicro(id,0x4200);
}

STATUS
tdc1190DisableAllChannels(int id)
{
  CHECKID(id);
  tdc1190WriteMicro(id,0x4300);
  return(OK);
}

/* channels[0] - low 16 channels */
STATUS
tdc1190EnableChannels(int id, UINT16 channels[8])
{
  int i;
  CHECKID(id);

  tdc1190WriteMicro(id,0x4400);
  for(i=0; i<8; i++) tdc1190WriteMicro(id,channels[i]);
  return(OK);
}

STATUS
tdc1190GetChannels(int id, UINT16 channels[8])
{
  int i;
  CHECKID(id);

  tdc1190WriteMicro(id,0x4500);
  for(i=0; i<8; i++) tdc1190ReadMicro(id,&channels[i],1);
  return(OK);
}

STATUS
tdc1190GGetChannels(UINT32 *chanenable_mask)
{
  STATUS status;
  UINT16 read[V1190_MAX_MODULES+1][2];
  int itdc;

  tdc1190GWriteMicro(0x4500);
  status = tdc1190GReadMicro((UINT16 *)read,2);

  for(itdc=0; itdc<Nc1190; itdc++)
    chanenable_mask[itdc] = read[itdc][0] | (read[itdc][1]<<16);

  return status;
}


/******************************************************************************
 * ADVANCED OPCODES Routines
 *
 *  All routines return OK or ERROR,
 *   unless otherwise specified.
 *
 * tdc1190GetTDCError   - Returns internal error from selected TDC
 * tdc1190GetTDCDLLLock - Returns 
 *                         0 : DLL not in LOCK
 *                         1 : DLL in LOCK
 * 
*/


int
tdc1190GetTDCError(int id, UINT16 ntdc)
{
  int opcode = 0x7400 + (ntdc & 0x3);
  unsigned short tmp;
  int rval;

  CHECKID(id);

  tdc1190WriteMicro(id,opcode);
  tdc1190ReadMicro(id,&tmp,1);

  rval = tmp & 0x7FF;

  return(rval);
}

int
tdc1190GetTDCDLLLock(int id, UINT16 ntdc)
{
  int opcode = 0x7500 + (ntdc & 0x3);
  unsigned short tmp;
  int rval;

  CHECKID(id);

  tdc1190WriteMicro(id,opcode);
  tdc1190ReadMicro(id,&tmp,1);

  rval = tmp & 0x1;

  return(rval);
}

/******************************************************************************
 *
 * tdc1190InitMSCT     - Initialize MSCT for all initialized boards
 *                       vmeaddress is set to the MSCT address
 *                       Returns OK, if successful.  ERROR, otherwise.
 *
 */

STATUS
tdc1190InitMCST(UINT32 *vmeaddress)
{
  unsigned int baseadrs;
  int ii;

  baseadrs = (V1190_A32_ADDR >> 24) & 0xFF;
  baseadrs += (V1190_A32_MCST_OFFSET >> 24);

#ifdef VXWORKS
  sysBusToLocalAdrs(0x09,(char *)(baseadrs<<24),(char **)vmeaddress);
#else
  vmeBusToLocalAdrs(0x09,(char *)(baseadrs<<24),(char **)vmeaddress);
#endif
  c1190MCSTp = (struct v1190_struct *)(vmeaddress);

  printf("tdc1190InitMCST: MCST VME (USER) base address 0x%x (0x%x):\n",
	 (baseadrs<<24), (UINT32)c1190MCSTp);

  /* Loop through use1190/c1190p array to set mcstBaseAddr
     and set First/intermediate/Last boards */
  for(ii=0; ii<V1190_MAX_MODULES; ii++)
  {
    if(use1190[ii] == 0) continue;

    vmeWrite16(&(c1190p[ii]->mcstBaseAddr),(baseadrs & V1190_MCSTBASEADDR_MASK));

    if(ii==tdcMinSlot)
	{
	  vmeWrite16(&(c1190p[ii]->mcstCtrl),2); /* active first */
	  printf("\tFirst  board at 0x%08x\n",(UINT32)c1190p[ii]);
	}
      else if(ii==tdcMaxSlot)
	{
	  vmeWrite16(&(c1190p[ii]->mcstCtrl),1); /* active last */
	  printf("\tLast   board at 0x%08x\n",(UINT32)c1190p[ii]);
	}
      else
	{
	  vmeWrite16(&(c1190p[ii]->mcstCtrl),3); /* active intermediate */
	  printf("\tMiddle board at 0x%08x\n",(UINT32)c1190p[ii]);
	}
  }

  return(OK);
}


/******************************************************************************
 *
 * tdc1190Trig         - Issue Software trigger to TDC
 * tdc1190Clear        - Clear TDC
 * tdc1190Reset        - Clear/Reset TDC
 * tdc1190GetEventCounter - Return number of triggers since last clear/reset.
 * tdc1190GetEventStored  - Return number of events currently stored in 
 *                          output buffer
 * tdc1190SetAlmostFullLevel - Set value of number of words in output buffer
 *                             when an IRQ is generated (if enabled) and
 *                             correspond bit in Status reg is set.
 * tdc1190SetOutProg - Set the function for the output on the control connector
 *                     0 : Data Ready
 *                     1 : Full
 *                     2 : Almost Full
 *                     3 : Error
 * tdc1190GetOutProg - Get value set for the output on the control connector.
 * tdc1190TestMode   - Enable (off=0) or disable (off=1) Test mode
 * tdc1190Test       - Set a value (tval) into the Testreg register
 * tdc1190SetGeoAddress - Set the GEO Address (contained in header/trailer words)
 * tdc1190GetGeoAddress - Get the GEO Address (contained in header/trailer words)
 *
 */

STATUS
tdc1190Trig(int id)
{
  CHECKID(id);
  LOCK_1190;
  vmeWrite16(&(c1190p[id]->trigger),1);
  UNLOCK_1190;
  return OK;
}

STATUS
tdc1190Clear(int id)
{
  CHECKID(id);
  LOCK_1190;
  vmeWrite16(&(c1190p[id]->clear),1);
  UNLOCK_1190;
  return OK;
}

STATUS
tdc1190Reset(int id)
{
  CHECKID(id);
  LOCK_1190;
  vmeWrite16(&(c1190p[id]->moduleReset),1);
  UNLOCK_1190;
  return OK;
}

int
tdc1190GetEventCounter(int id)
{
  int rval;

  CHECKID(id);
  LOCK_1190;
  rval = vmeRead32(&(c1190p[id]->evCount));
  UNLOCK_1190;
  return rval;
}

int
tdc1190GetEventStored(int id)
{
  int rval;

  CHECKID(id);
  LOCK_1190;
  rval = vmeRead16(&(c1190p[id]->evStored));
  UNLOCK_1190;
  return rval;
}

STATUS
tdc1190SetAlmostFullLevel(int id, UINT16 nwords)
{
  CHECKID(id);
  LOCK_1190;
  vmeWrite16(&(c1190p[id]->almostFullLevel),(nwords & 0x7FFF));
  UNLOCK_1190;
  return OK;
}

int
tdc1190GetAlmostFullLevel(int id)
{
  int rval;

  CHECKID(id);
  LOCK_1190;
  rval = vmeRead16(&(c1190p[id]->almostFullLevel));
  /*printf(" Almost Full Level set to %d (0x%04x) words\n",rval,rval);*/
  UNLOCK_1190;
  return rval;
}


STATUS
tdc1190SetOutProg(int id, UINT16 code)
{
  CHECKID(id);
  LOCK_1190;
  vmeWrite16(&(c1190p[id]->outProgControl),code & 0x3);
  UNLOCK_1190;
  return OK;
}

int
tdc1190GetOutProg(int id)
{
  int rval;

  CHECKID(id);
  LOCK_1190;
  rval = vmeRead16(&(c1190p[id]->outProgControl));
  UNLOCK_1190;
  return rval;
}

STATUS
tdc1190TestMode(int id, int off)
{
  CHECKID(id);
  /* FIXME: Test mode enabled/disabled through OPCODE */

  if(off) 
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) & (~V1190_TESTMODE&0xf));
  else
    vmeWrite16(&(c1190p[id]->control),
	       vmeRead16(&(c1190p[id]->control)) | (V1190_TESTMODE));

  return OK;
}

STATUS
tdc1190Test(int id, UINT16 tval)
{
  /* FIXME: Test mode enabled/disabled through OPCODE */
  UINT16 testMode=0;

  CHECKID(id);

  testMode = vmeRead16(&(c1190p[id]->control))&V1190_TESTMODE;
  if(testMode)
    {
      if(tval==0)
	vmeWrite32(&(c1190p[id]->testReg),0x11223344);
      else
	vmeWrite32(&(c1190p[id]->testReg),tval);
      /* FIXME: Maybe the below is better than above... I dont know... fix it later */
      /* 	(UINT16 *)(c1190p[id]->testReg) = tval; */
    }
  else
    {
      printf("tdc1190Test: ERROR: TestMode not enabled.\n");
    }
     
  return OK;
}


STATUS
tdc1190SetGeoAddress(int id, UINT16 geoaddr)
{
  CHECKID(id);
  LOCK_1190;
  vmeWrite16(&(c1190p[id]->geoAddr),geoaddr & 0x1F);
  UNLOCK_1190;
  return OK;
}

int
tdc1190GetGeoAddress(int id)
{
  int rval;

  CHECKID(id);
  LOCK_1190;
  rval = vmeRead16(&(c1190p[id]->geoAddr)) & 0x1F;
  UNLOCK_1190;
  return rval;
}


STATUS
tdc1190SetBLTEventNumber(int id, UINT16 nevents)
{
  CHECKID(id);
  blt_Events = nevents & 0xFF;
  LOCK_1190;
  vmeWrite16(&(c1190p[id]->bltEventNumber), blt_Events);
  UNLOCK_1190;
  printf("tdc1190SetBLTEventNumber: set blt event number to %d\n",blt_Events);
  return OK;
}

int
tdc1190GetBLTEventNumber(int id)
{
  int rval;

  CHECKID(id);
  LOCK_1190;
  rval = vmeRead16(&(c1190p[id]->bltEventNumber));
  UNLOCK_1190;
  return rval;
}





/* new from Bryan */



STATUS
tdc1190SetRCadjust(int id, int tdc, int adjust)
{
  CHECKID(id);

  if((tdc<0) || (tdc>3))
    {
      printf("%s: ERROR: Invalid tdc (%d)\n",
	     __FUNCTION__,tdc);
      return ERROR;
    }

  if((adjust<0) || (adjust>0xFFF))
    {
      printf("%s: ERROR: Invalid adjust (0x%x)\n",
	     __FUNCTION__,adjust);
      return ERROR;
    }
  
  tdc1190WriteMicro(id, 0x5400 + tdc);
  tdc1190WriteMicro(id, adjust);

  return OK;
}

STATUS
tdc1190GSetRCadjust(int tdc, int adjust)
{
  if((tdc<0) || (tdc>3))
    {
      printf("%s: ERROR: Invalid tdc (%d)\n",
	     __FUNCTION__,tdc);
      return ERROR;
    }

  if((adjust<0) || (adjust>0xFFF))
    {
      printf("%s: ERROR: Invalid adjust (0x%x)\n",
	     __FUNCTION__,adjust);
      return ERROR;
    }
  
  tdc1190GWriteMicro(0x5400 + tdc);
  tdc1190GWriteMicro(adjust);

  return OK;
}

int
tdc1190GetRCadjust(int id, int tdc)
{
  int rval=0;
  unsigned short tmp = 0;

  CHECKID(id);

  if((tdc<0) || (tdc>3))
    {
      printf("%s: ERROR: Invalid tdc (%d)\n",
	     __FUNCTION__,tdc);
      return ERROR;
    }

  tdc1190WriteMicro(id, 0x5500 + tdc);
  tdc1190ReadMicro(id, &tmp, 1);

  rval = (int) ( tmp & 0xFFF );

  printf("tdc1190GetRCadjust[id=%d][tdc=%d] returns %d\n",id,tdc,rval);

  return rval;
}


int
tdc1190SaveRCadjust(int id)
{
  CHECKID(id);

  tdc1190WriteMicro(id, 0x5600);

  return OK;
}

int
tdc1190GSaveRCadjust()
{
  tdc1190GWriteMicro(0x5600);

  return OK;
}



int
tdc1190WriteFlashPage(int id, unsigned char *page, unsigned long pagenum)
{
  unsigned int flash_addr, data;
  unsigned char addr0, addr1, addr2;
  unsigned short i;

  CHECKID(id);

  flash_addr = pagenum << 9;

  addr0 = (unsigned char) flash_addr;
  addr1 = (unsigned char) (flash_addr >> 8);
  addr2 = (unsigned char) (flash_addr >> 16);

  LOCK_1190;
  /* enable flash (NCS = 0) */
  data = 0;
  vmeWrite16(&c1190p[id]->selflash, data);

  /* write opcode */
  data = V1190_MAIN_MEM_PAGE_PROG_TH_BUF1;
  vmeWrite16(&c1190p[id]->flash, data);

  /* write address */
  data = addr2;
  vmeWrite16(&c1190p[id]->flash, data);
  data = addr1;
  vmeWrite16(&c1190p[id]->flash, data);
  data = addr0;
  vmeWrite16(&c1190p[id]->flash, data);

  /* write flash page (264 bytes for page, 256 used) */
  for (i = 0; i < 264; i++)
    {
      data = page[i];
      vmeWrite16(&c1190p[id]->flash, data);
    }

  /* disable flash (NCS = 1) */
  data = 1;
  vmeWrite16(&c1190p[id]->selflash, data);

  /* wait 20ms (max time required by the flash to complete the writing) */
  taskDelay(20);
  UNLOCK_1190;

  return OK;
}

static int
tdc1190EraseFlashPage(int id, unsigned long pagenum)
{
  unsigned int flash_addr, data;
  unsigned char addr0, addr1, addr2;

  CHECKID(id);

  flash_addr = pagenum << 9;

  addr0 = (unsigned char) flash_addr;
  addr1 = (unsigned char) (flash_addr >> 8);
  addr2 = (unsigned char) (flash_addr >> 16);

  LOCK_1190;
  /* enable flash (NCS = 0) */
  data = 0;
  vmeWrite16(&c1190p[id]->selflash, data);

  /* write opcode */
  data = V1190_PAGE_ERASE;
  vmeWrite16(&c1190p[id]->flash, data);

  /* write address */
  data = addr2;
  vmeWrite16(&c1190p[id]->flash, data);
  data = addr1;
  vmeWrite16(&c1190p[id]->flash, data);
  data = addr0;
  vmeWrite16(&c1190p[id]->flash, data);

  /* disable flash (NCS = 1) */
  data = 1;
  vmeWrite16(&c1190p[id]->selflash, data);

  /* wait 20ms (max time required by the flash to complete the writing) */
  taskDelay(20);
  UNLOCK_1190;

  return OK;
}

int
tdc1190ReadFlashPage(int id, unsigned char *page, unsigned long pagenum)
{
  unsigned int flash_addr, data;
  unsigned char addr0, addr1, addr2;
  unsigned short i;

  CHECKID(id);

  flash_addr = pagenum << 9;
  addr0 = (unsigned char) flash_addr;
  addr1 = (unsigned char) (flash_addr >> 8);
  addr2 = (unsigned char) (flash_addr >> 16);

  LOCK_1190;
  /* enable flash (NCS = 0) */
  data = 0;
  vmeWrite16(&c1190p[id]->selflash, data);

  /* write opcode */
  data = V1190_MAIN_MEM_PAGE_READ;
  vmeWrite16(&c1190p[id]->flash, data);

  /* write address */
  data = addr2;
  vmeWrite16(&c1190p[id]->flash, data);
  data = addr1;
  vmeWrite16(&c1190p[id]->flash, data);
  data = addr0;
  vmeWrite16(&c1190p[id]->flash, data);

  /* additional don't care bytes */
  data = 0;
  for (i = 0; i < 4; i++)
    {
      vmeWrite16(&c1190p[id]->flash, data);
    }

  /* read flash page (264 bytes for page, 256 used) */
  for (i = 0; i < 264; i++)
    {
      data = vmeRead16(&c1190p[id]->flash);
      page[i] = data;
    }

  /* disable flash (NCS = 1) */
  data = 1;
  vmeWrite16(&c1190p[id]->selflash, data);

  UNLOCK_1190;

  return OK;
}



int
tdc1190WriteChannelCompensation(int id, int channel, unsigned char *table)
{
  int pagenum_0 = 0, ipage = 0;
  int rval = OK;
  CHECKID(id);

  if((channel < 0) || (channel > 31))
    {
      printf("%s: ERROR: Invalid channel (%d)\n",
	     __FUNCTION__,
	     channel);
      return ERROR;
    }

  /* Calculate starting page number */
  pagenum_0 = channel * 4;

  for(ipage = 0; ipage < 4; ipage++)
    {
      if(tdc1190EraseFlashPage(id, pagenum_0 + ipage) == ERROR)
	{
	  rval = ERROR;
	  printf("%s(%d, %d, *table): ERROR erasing Flash Page %d\n",
		 __FUNCTION__, id, channel, pagenum_0 + ipage);
	  
	}
      else
	{
	  if(tdc1190WriteFlashPage(id, (unsigned char *)&table[ipage*256],
				   pagenum_0 + ipage) == ERROR)
	    {
	      rval = ERROR;
	      printf("%s(%d, %d, *table): ERROR writing to Flash Page %d\n",
		     __FUNCTION__, id, channel, pagenum_0 + ipage);
	    }
	}
    }
  
  return rval;
}

int
tdc1190ReadCompensationSramPage(int id, unsigned char *page, unsigned long pagenum)
{
  int ibin = 0, nbin = 256;
  CHECKID(id);

  if((pagenum < 0) || (pagenum > 255))
    {
      printf("%s: ERROR: Invalid pagenum (%ld)\n",
	     __FUNCTION__, pagenum);
      return ERROR;
    }
  
  LOCK_1190;
  vmeWrite16(&c1190p[id]->compensationSRAMpage, pagenum);

  for(ibin = 0; ibin < nbin; ibin++)
    {
      page[ibin] = vmeRead16(&c1190p[id]->sram[ibin]) & 0xFF;
    }
				   
  
  UNLOCK_1190;

  return OK;
}

unsigned char
tdc1190Float2TCInt(float in_float)
{
  unsigned char rval = 0;

  if((in_float >= 127.5) || (in_float <= -128.5))
    {
      printf("%s: WARN: Float (%lf) out of bounds.",
  	     __FUNCTION__, in_float);
      if(in_float >= 127.5) in_float = 127.;
      if(in_float <= -128.5) in_float = -128.;
      printf(" Truncated to %lf\n", in_float);
    }
  
  rval = ((unsigned char) (rintf(-1 * in_float)) & 0xff);

  rval = (~(rval) & 0xff) + 1;

  return rval;
}

float
tdc1190TCInt2Float(unsigned char in_tcint)
{
  int rval = 0;

  if(in_tcint & 0x80)
    {
      rval = -1 * ((~(in_tcint - 1)) & 0xff);
    }
  else
    rval = in_tcint;
  
  return (float)rval;
}

void
tdc1190ConvertTable_Float2TCInt(float *in_table, unsigned char *out_table, int n)
{
  int i = 0;

  for(i = 0; i < n; i++)
    out_table[i] = tdc1190Float2TCInt(in_table[i]);

}

void
tdc1190ConvertTable_TCInt2Float(unsigned char *in_table, float *out_table, int n)
{
  int i = 0;

  for(i = 0; i < n; i++)
    out_table[i] = tdc1190TCInt2Float(in_table[i]);

}







/***********************/
/* INTERRUPT FUNCTIONS */
/***********************/


/*******************************************************************************
 *
 * tdc1190Int - default interrupt handler
 *
 * This rountine handles the v1190 TDC interrupt.  A user routine is
 * called, if one was connected by tdc1190IntConnect().
 *
 * RETURNS: N/A
 *
 */

LOCAL void 
tdc1190Int (void)
{

  UINT32 nevt=0;
  
  /* Disable interrupts */
#ifdef VXWORKS
  sysIntDisable(c1190IntLevel);
#else
  vmeBusLock();
#endif

  c1190IntCount++;
 
  if (c1190IntRoutine != NULL)  {     /* call user routine */
    (*c1190IntRoutine) (c1190IntArg);
  }else{
    if((c1190IntID<0) || (c1190p[c1190IntID] == NULL)) {
      logMsg("c1190Int: ERROR : TDC id %d not initialized \n",c1190IntID,0,0,0,0,0);
      return;
    }

    logMsg("c1190Int: Processed %d events\n",nevt,0,0,0,0,0);

  }

  /* Enable interrupts */
#ifdef VXWORKS
  sysIntEnable(c1190IntLevel);
#else
  vmeBusUnlock();
#endif

}



/*******************************************************************************
 *
 * c1190IntConnect - connect a user routine to the v1190 TDC interrupt
 *
 * This routine specifies the user interrupt routine to be called at each
 * interrupt. 
 *
 * RETURNS: OK, or ERROR if Interrupts are enabled
 */

STATUS 
tdc1190IntConnect (VOIDFUNCPTR routine, int arg, UINT16 level, UINT16 vector)
{

  if(c1190IntRunning) {
    printf("c1190IntConnect: ERROR : Interrupts already Initialized for TDC id %d\n",
	   c1190IntID);
    return(ERROR);
  }
  
  c1190IntRoutine = routine;
  c1190IntArg = arg;

  /* Check for user defined VME interrupt level and vector */
  if(level == 0) {
    c1190IntLevel = V1190_VME_INT_LEVEL; /* use default */
  }else if (level > 7) {
    printf("c1190IntConnect: ERROR: Invalid VME interrupt level (%d). Must be (1-7)\n",level);
    return(ERROR);
  } else {
    c1190IntLevel = level;
  }

  if(vector == 0) {
    c1190IntVec = V1190_INT_VEC;  /* use default */
  }else if ((vector < 32)||(vector>255)) {
    printf("c1190IntConnect: ERROR: Invalid interrupt vector (%d). Must be (32<vector<255)\n",vector);
    return(ERROR);
  }else{
    c1190IntVec = vector;
  }
      
  /* Connect the ISR */
#ifdef VXWORKSPPC
  if((intDisconnect((int)INUM_TO_IVEC(c1190IntVec)) != 0)) {
    printf("c1190IntConnect: ERROR disconnecting Interrupt\n");
    return(ERROR);
  }
  if((intConnect(INUM_TO_IVEC(c1190IntVec),tdc1190Int,0)) != 0) {
    printf("c1190IntConnect: ERROR in intConnect()\n");
    return(ERROR);
  }
#else
  if(vmeIntConnect(c1190IntVec,c1190IntLevel,tdc1190Int,c1190IntArg) != OK)
    {
      printf("%s: ERROR: vmeIntConnect failed\n");
      return ERROR;
    }
#endif

  return (OK);
}


/*******************************************************************************
 *
 * c1190IntEnable - Enable interrupts from specified TDC
 *
 * Enables interrupts for a specified TDC.
 * 
 * RETURNS OK or ERROR if TDC is not available or parameter is out of range
 */

STATUS 
tdc1190IntEnable (int id, UINT16 evCnt)
{

  if(c1190IntRunning) {
    printf("c1190IntEnable: ERROR : Interrupts already initialized for TDC id %d\n",
	   c1190IntID);
    return(ERROR);
  }

  if((id<0) || (c1190p[id] == NULL)) {
    printf("c1190IntEnable: ERROR : TDC id %d not initialized \n",id);
    return(ERROR);
  }else{
    c1190IntID = id;
  }
  
  
#ifdef VXWORKS
  sysIntEnable(c1190IntLevel);   /* Enable VME interrupts */
#endif  

  /* Zero Counter and set Running Flag */
  c1190IntEvCount = evCnt;
  c1190IntCount = 0;
  c1190IntRunning = TRUE;
  
  return(OK);
}


/*******************************************************************************
 *
 * c1190IntDisable - disable the TDC interrupts
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS 
tdc1190IntDisable (int iflag)
{

  if((c1190IntID<0) || (c1190p[c1190IntID] == NULL)) {
    logMsg("c1190IntDisable: ERROR : TDC id %d not initialized \n",c1190IntID,0,0,0,0,0);
    return(ERROR);
  }

#ifdef VXWORKS
  sysIntDisable(c1190IntLevel);   /* Disable VME interrupts */
#endif

  /* Tell tasks that Interrupts have been disabled */
  if(iflag > 0) {
    c1190IntRunning = FALSE;
  }
  
  return (OK);
}


#else /* dummy version*/

void
tdc1190_dummy()
{
  return;
}

#endif
