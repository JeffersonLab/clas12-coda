/****************************************************************************** 
 * 
 *  sspLib.c    -  Driver library for configuration of JLAB Subsystem Processor 
 *                 (SSP) using a VxWorks 5.4 or later, or Linux based Single  
 *                 Board computer. 
 * 
 *                 Currently Supports SSP Type = 1 (Hall D) 
 * 
 *  Authors: Ben Raydo 
 *           Jefferson Lab Fast Electronics Group 
 *           August 2013 
 * 
 *           Bryan Moffit 
 *           Jefferson Lab Data Acquisition Group 
 *           September 2013 
 * 
 * delay scans:

 0. Kill DiagGuiServer on trig2

 1. set delays in file 'trigger_delay_scan.trg' to 'almost' desired values:

SSP_GT_HTCC_DELAY         140

SSP_GT_FTOF_DELAY          80
SSP_GT_ECAL_CLUSTER_DELAY   0
SSP_GT_PCAL_CLUSTER_DELAY   0
SSP_GT_CTOF_DELAY          212
SSP_GT_CND_DELAY           220
SSP_GT_PCAL_PCU_DELAY      76

 2. run following:

sspGt_HtccDelayScan(int delay_min, int delay_max, int idle)

 3. Found 'middles' of distributions and add/subtract it to
existing delays in corresponding detectors


OTHER USEFUL PROCEDURES:

tcpClient trig2 'sspGt_SetPcal_PcuDelay(3,200)'
tcpClient trig2 'sspGt_SetPcal_PcuDelay(4,200)'
tcpClient trig2 'sspGt_SetPcal_PcuDelay(5,200)'
tcpClient trig2 'sspGt_SetPcal_PcuDelay(6,200)'
tcpClient trig2 'sspGt_SetPcal_PcuDelay(7,200)'
tcpClient trig2 'sspGt_SetPcal_PcuDelay(8,200)'
sspGt_FtofPcuDelayScan(100,500,1)




other delay scans:
  sspGt_FtofPcuDelayScan(int delay_min, int delay_max, int idle)
  sspGt_FtofFtDelayScan(int delay_min, int delay_max, int idle)
  sspGt_CtofFtDelayScan(int delay_min, int delay_max, int idle)
  sspGt_CndFtDelayScan(int delay_min, int delay_max, int idle)
  sspGt_EcPcDelayScan(int delay_min, int delay_max, int idle)
  sspGt_DcDelayScan(int delay_min, int delay_max, int idle)
  sspGt_FtDelayScan(int delay_min, int delay_max, int idle)

 * 
 */ 
/*
#if defined(VXWORKS) || defined(Linux_vme)
*/
#if defined(Linux_vme)

 
#ifdef VXWORKS 
#include <vxWorks.h> 
/*#include "vxCompat.h"*/
#include <logLib.h> 
#include <taskLib.h> 
#include <intLib.h> 
#include <iv.h> 
#include <semLib.h> 
#include <vxLib.h> 
#else 
#include <unistd.h> 
#include <stddef.h> 
#include "jvme.h" 
#endif 
#include <pthread.h> 
#include <stdio.h> 
#include <string.h> 
 
#include "sspLib.h"
#include "xxxConfig.h"

#include "ipc.h"

#ifdef VXWORKS 
#define SYNC()		{ __asm__ volatile("eieio"); __asm__ volatile("sync"); } 
#endif 
 
/* Global Variables */ 
static int active;

static int nSSP;                                 /* Number of SSPs found with sspInit(..) */ 
volatile SSP_regs *pSSP[MAX_VME_SLOTS+1];        /* pointers to SSP memory map */ 
volatile unsigned int *SSPpf[MAX_VME_SLOTS + 1]; /* pointers to VSCM FIFO memory */
volatile unsigned int *SSPpmb;                   /* pointer to Multiblock Window */
int sspSL[MAX_VME_SLOTS+1];                      /* array of slot numbers for SSPs */ 
unsigned int sspAddrList[MAX_VME_SLOTS+1];       /* array of a24 addresses for SSPs */ 
int sspFirmwareType[MAX_VME_SLOTS+1];               /* array of firmware type for SSPs */ 

static int sspA32Base   = 0x08800000;
//static int sspA32Base   = 0x11000000/*0x08800000*/;                   /* Minimum VME A32 Address for use by FADCs */
static int sspA32Offset = 0x00080000;                   /* Difference in CPU A32 Base - VME A32 Base */
static int sspA24Offset=0;                              /* Difference in Local A24 Base and VME A24 Base */ 

static int minSlot = 21;
static int maxSlot = 1;

/* Mutex to guard read/writes */ 
pthread_mutex_t   sspMutex = PTHREAD_MUTEX_INITIALIZER; 
 
/* Static routine prototypes */ 
static void sspSelectSpi(int id, int sel); 
static void sspFlashGetId(int id, unsigned char *rsp); 
static void sspReloadFirmware(int id); 
static unsigned char sspFlashGetStatus(int id); 
static unsigned char sspTransferSpi(int id, unsigned char data); 

void SSPLOCK()
{
  if(pthread_mutex_lock(&sspMutex)<0)
    perror("pthread_mutex_lock"); 
}

void SSPUNLOCK()
{
  if(pthread_mutex_unlock(&sspMutex)<0)
    perror("pthread_mutex_unlock"); 
}

unsigned int  
sspReadReg(volatile unsigned int *addr) 
{ 
#ifdef VXWORKS 
  unsigned int result = *addr; 
  SYNC(); 
  return result; 
#else
  return vmeRead32(addr); 
#endif 
} 
 
void  
sspWriteReg(volatile unsigned int *addr, unsigned int val) 
{ 
#ifdef VXWORKS 
  unsigned int *addr0 = (unsigned int *)( ((unsigned int)addr) & 0xFFFF0000); 
  *addr = val; 
  *addr0 = 0;	// nasty hack for 5500 cpus that have kernel write optimizations enabled (ensures no sequential address writes exist in write queue) 
  SYNC(); 
#else 
  vmeWrite32(addr, val);
#endif
}

/*
 * If id = 0 change id to first SSP slot
 * Returns 1 if SSP in slot id is not initalized or sspFirmwareType doesn't match requirement
 * Returns 0 if SSP is initalized
 */
int
sspIsNotInit(int *id, const char *func, int reqFirmwareType)
{
  if(*id == 0) *id = sspSL[0];

  if((*id <= 0) || (*id > 21) || (pSSP[*id] == NULL))
  {
    logMsg("ERROR: %s: SSP in slot %d is not initialized\n", func, *id);
    return(1);
  }
  
  if( (reqFirmwareType != SSP_CFG_SSPTYPE_COMMON) && (reqFirmwareType != sspFirmwareType[*id]) )
  {
    logMsg("ERROR: %s: SSP in slot %d incorrect firmware type: has %d, needs %d\n",
           func, *id, sspFirmwareType[*id], reqFirmwareType);
    return(1);
  }

  return(0);
}
 
/************************************************************ 
 * SSP Main 
 ************************************************************/ 
 
/******************************************************************************* 
 * 
 * sspInit(unsigned int addr, int iFlag) 
 *    addr: vme a24 base address 
 *    iFlag: 
 *        bits 1:0 - Mode 
 *           0 - disabled 
 *               clk src = 0 
 *               sync src = 0 
 *               trig src = 0 
 *           1 - local/P2LVDS 
 *               clk src = LOCAL 
 *               sync src = P2LVDSIN0 
 *               trig src = P2LVDSIN1 
 *           2 - local/FPLVDS 
 *               clk src = LOCAL 
 *               sync src = FPLVDSIN0 
 *               trig src = FPLVDSIN1 
 *           3 - vxs 
 *               clk src = VXS SWB (SD) 
 *               sync src = VXS SWB (SD) 
 *               trig src = VXS SWB (SD) 
 * 
 *        bit 12 - Skip initialization the clock/syncReset/trigger source 
 *                 Setup (keeping the current values the same) 
 * 
 *        bit 13 - Ignore version compatibility between firware and library 
 * 
 *        bit 14 - Exit before board initialization (just map structure pointer) 
 * 
 *        bit 15 -  Use sspAddrList instead of addr and addr_inc 
 *                  for VME addresses 
 * 
 *        bits 23:16 - Fiber Enable 
 *           see TRG_CTRL_FIBER_ENx definitions in ssp.h 
 *        bits 31:24 - GTP data source 
 *           see TRG_CTRL_GTPSRC_* definitions in ssp.h 
 * 
 * Note: sspInit should only be called once the clock source is stable and 
 *       remains so. If the clock source disappears or changes source sspInit() 
 *       must be called again to properly initialize the ssp. 
 * 
 */ 
 
int  
sspInit(unsigned int addr, unsigned int addr_inc, int nfind, int iFlag) 
{
  int useList=0, noBoardInit=0, noFirmwareCheck=0;; 
  unsigned int rdata, laddr, laddr_inc, boardID, a32addr; 
  int issp=0, islot=0, res; 
  int result=OK; 
  volatile SSP_regs *ssp; 
  unsigned int firmwareInfo=0, sspVersion=0, utmp32; 
  int ii;
 
  /* Check if we're skipping the firmware check */ 
  if(iFlag & SSP_INIT_SKIP_FIRMWARE_CHECK) 
  { 
    printf("%s: noFirmwareCheck\n",__FUNCTION__); 
    noFirmwareCheck=1; 
  } 
 
  /* Check if we're skipping initialization, and just mapping the structure pointer */ 
  if(iFlag & SSP_INIT_NO_INIT) 
  { 
    printf("%s: noBoardInit\n",__FUNCTION__); 
    noBoardInit=1; 
  } 
 
  /* Check if we're initializing using a list */ 
  if(iFlag & SSP_INIT_USE_ADDRLIST) 
  { 
    printf("%s: useList\n",__FUNCTION__); 
    useList=1; 
  } 
 
  /* Check for valid address */ 
  if( (addr==0) && (useList==0) ) 
  { 
    useList=1; 
    nfind=16; 
 
    /* Loop through JLab Standard GEOADDR to VME addresses to make a list */ 
    //for(islot=3; islot<11; islot++) /* First 8 */
    for(islot=2; islot<11; islot++) /* First 8-start from 2 for ELMA VXS backplane */
	{
	  //sspAddrList[islot-3] = (islot<<19);
    sspAddrList[islot-2] = (islot<<19); 
    }
   
    /* Skip Switch Slots */ 
       
    for(islot=13; islot<21; islot++) /* Last 8 */ 
	{
	  //sspAddrList[islot-5] = (islot<<19); 
    sspAddrList[islot-4] = (islot<<19); 
	}
  } 
  else if(addr > 0x00ffffff)  
  { /* A32 Addressing */ 
    printf("%s: ERROR: A32 Addressing not allowed for SSP configuration space\n", __FUNCTION__); 
    return(ERROR); 
  } 
  else 
  { /* A24 Addressing for ONE SSP */ 
    if( ((addr_inc==0)||(nfind==0)) && (useList==0) ) nfind = 1; /* assume only one SSP to initialize */ 
  } 
 
  /* Get the SSP address */ 
#ifdef VXWORKS 
  res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr); 
#else 
  res = vmeBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr); 
#endif 
 
  if (res != 0)
  { 
#ifdef VXWORKS 
    printf("%s: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n", 
	     __FUNCTION__,addr); 
#else 
    printf("%s: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n", 
	     __FUNCTION__,addr); 
#endif 
    return(ERROR); 
  } 
  sspA24Offset = laddr - addr; 
 



  nSSP = 0;
  for(issp=0; issp<nfind; issp++)
  { 
    if(useList==1) 
    { 
	  laddr_inc = sspAddrList[issp] + sspA24Offset; 
	} 
    else 
	{ 
	  laddr_inc = laddr + issp*addr_inc; 
	} 
 
    ssp = (volatile SSP_regs *)laddr_inc; 


      /* Check if Board exists at that address */ 
#ifdef VXWORKS 
    res = vxMemProbe((char *) &(ssp->Cfg.BoardId),VX_READ,4,(char *)&rdata); 
#else 
    res = vmeMemProbe((char *) &(ssp->Cfg.BoardId),4,(char *)&rdata); 
#endif 
 
    if(res < 0)  
	{ 
#ifdef VXWORKS 
	  printf("%s: ERROR: No addressable board at addr=0x%x\n", 
		 __FUNCTION__,(UINT32) ssp); 
#else 
	  printf("%s: ERROR: No addressable board at VME (Local) addr=0x%x (0x%x)\n", 
		 __FUNCTION__, 
		 (UINT32) laddr_inc-sspA24Offset, (UINT32) ssp); 
#endif 
	} 
    else  
	{ 
	  /* Check that it is a ssp */ 
	  if(rdata != SSP_CFG_BOARDID) 
	  { 
#ifdef DEBUG
	    printf(" WARN: For board at 0x%x, Invalid Board ID: 0x%x\n", (UINT32) laddr_inc-sspA24Offset, rdata); 
#endif
	    continue; 
	  } 
	  else  
	  {
	    /* Check if this is board has a valid slot number */ 
	    boardID =  (sspReadReg(&ssp->Cfg.FirmwareRev)&SSP_CFG_SLOTID_MASK)>>24; 
	    if((boardID <= 0)||(boardID >21))  
		{ 
		  printf(" WARN: Board Slot ID is not in range: %d (this module ignored)\n" ,boardID); 
		  continue; 
		} 
	    else 
		{ 
		  pSSP[boardID] = (volatile SSP_regs *)(laddr_inc); 
		  sspSL[nSSP] = boardID; 

 
		  /* Get the Firmware Information and print out some details */ 
		  firmwareInfo = sspReadReg(&pSSP[boardID]->Cfg.FirmwareRev); 
		  if(firmwareInfo>0) 
		  { 
            sspFirmwareType[boardID] = (firmwareInfo & SSP_CFG_SSPTYPE_MASK)>>16;
		    printf("  Slot %2d: Type %d \tFirmware (major.minor): %d.%d\n", 
			     boardID, 
			     (firmwareInfo & SSP_CFG_SSPTYPE_MASK)>>16,
			     (firmwareInfo & SSP_CFG_FIRMWAREREV_MAJOR_MASK)>>8,  
			     (firmwareInfo & SSP_CFG_FIRMWAREREV_MINOR_MASK)); 
		    sspVersion = firmwareInfo&0xFFF; 
		    if(sspVersion < SSP_SUPPORTED_FIRMWARE) 
			{ 
			  if(noFirmwareCheck) 
			  { 
			    printf("   WARN: Firmware version (0x%x) not supported by this driver.\n", 
				     sspVersion); 
			    printf("          Supported version = 0x%x (IGNORED)\n", 
				     SSP_SUPPORTED_FIRMWARE); 
			  } 
			  else 
			  { 
			    printf("   ERROR: Firmware version (0x%x) not supported by this driver.\n", 
				     sspVersion); 
			    printf("          Supported version = 0x%x\n", 
				     SSP_SUPPORTED_FIRMWARE); 
			    pSSP[boardID] = NULL; 
			    continue; 
			  } 
			} 
		  } 
		  else 
		  { 
		    printf("  Slot %2d:  ERROR: Invalid firmware 0x%08x\n", 
			     boardID,firmwareInfo); 
		    pSSP[boardID] = NULL; 
        sspFirmwareType[boardID] = 0;
		    continue; 
		  } 
  
		  printf("Initialized SSP %2d  Slot # %2d at address 0x%08x (0x%08x)\n", 
			 nSSP, sspSL[nSSP],(UINT32) pSSP[(sspSL[nSSP])], 
				 (UINT32) pSSP[(sspSL[nSSP])]-sspA24Offset); 
		} 
	  } 
	  nSSP++; 
	} 
  }


  /* Program an A32 access address for SSP's FIFO */
  for(ii=0; ii<nSSP; ii++) 
  {
    a32addr = sspA32Base + (ii * SSP_MAX_FIFO);

    vmeWrite32(&pSSP[sspSL[ii]]->EB.AD32, ((a32addr >> 16) & 0xFF80) | 0x0001);

	printf("sspInit: a32addr=0x%08x, write to AD32 register 0x%08x\n",a32addr,((a32addr >> 16) & 0xFF80) | 0x0001);

#ifdef VXWORKS
    res = sysBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
    if (res != 0) 
	{
	  printf("sspInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#else
    res = vmeBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
    if (res != 0) 
	{
	  printf("sspInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#endif

    SSPpf[sspSL[ii]] = (unsigned int *)(laddr);
    sspA32Offset = laddr - a32addr;
    printf("sspInit: laddr(am=0x09) = 0x%08x, sspA32Offset=0x%08x\n",laddr, sspA32Offset);

	printf("SSP %2d  Slot # %2d at address 0x%08x (0x%08x) assigned A32 address 0x%08x (0x%08x)\n", 
			 ii, sspSL[ii],(UINT32) pSSP[(sspSL[ii])], 
		   (UINT32) pSSP[(sspSL[ii])]-sspA24Offset,
           (unsigned int)SSPpf[sspSL[ii]], (unsigned int)SSPpf[sspSL[ii]]-sspA32Offset);
  }





  /*
   * If more than 1 SSP in crate then setup the Muliblock Address
   * window. This must be the same on each board in the crate
   */
  if (nSSP > 1)
  {
    /* set MB base above individual board base */
    a32addr = sspA32Base + (nSSP * SSP_MAX_FIFO);
#ifdef VXWORKS
    res = sysBusToLocalAdrs(0x09, (char *)a32addr, (char **)&laddr);
    if (res != 0)
    {
      printf("ERROR: %s: in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",__func__, a32addr);
      return(ERROR);
    }
#else
    res = vmeBusToLocalAdrs(0x09, (char *)a32addr, (char **)&laddr);
    if (res != 0)
    {
	  printf("ERROR: %s: in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",__func__, a32addr);
	  return(ERROR);
    }
#endif
    SSPpmb = (unsigned int *)laddr;  /* Set a pointer to the FIFO */
	for (ii = 0; ii < nSSP; ii++)
    {
  	  /* Write the register and enable */
      vmeWrite32((volatile unsigned int *)&(pSSP[sspSL[ii]]->EB.Adr32M), \
                    ((a32addr + SSP_MAX_A32MB_SIZE) >> 7) | \
                    (a32addr >> 23) | (1 << 25));
	}


    minSlot = sspSL[0];
    maxSlot = sspSL[nSSP-1];

    utmp32 = vmeRead32((volatile unsigned int *)&(pSSP[minSlot]->EB.Adr32M));
    vmeWrite32((volatile unsigned int *)&(pSSP[minSlot]->EB.Adr32M), utmp32 | (1 << 26));
    utmp32 = vmeRead32((volatile unsigned int *)&(pSSP[maxSlot]->EB.Adr32M));
    vmeWrite32((volatile unsigned int *)&(pSSP[maxSlot]->EB.Adr32M), utmp32 | (1 << 27));
  }



  /* Setup initial configuration */ 
  if(noBoardInit==0) 
  { 
    for(issp=0; issp<nSSP; issp++) 
    { 
      printf("%s: slot %d - type = %d\n", __func__, sspSlot(issp), sspFirmwareType[sspSlot(issp)]);fflush(stdout);
      result = sspSetMode(sspSlot(issp),iFlag,0);
      if(result != OK)
      {
        return ERROR;
      }

      sspDisableBusError(sspSlot(issp));
      if(sspFirmwareType[sspSlot(issp)] == SSP_CFG_SSPTYPE_HALLBGT)
      {
        printf("SSP_CFG_SSPTYPE_HALLBGT\n");
        sspPortEnable(sspSlot(issp), 0x2FF, 1);  /* Enable serdes: VXS, Fiber0, Fiber 1, Fiber 2, Fiber 5, Fiber 6, Fiber 7 (was 0x2EB) */
      }
      else if(sspFirmwareType[sspSlot(issp)] == SSP_CFG_SSPTYPE_HALLBGTC)
      {
        printf("SSP_CFG_SSPTYPE_HALLBGTC\n");
        sspPortEnable(sspSlot(issp), 0x2FF, 1); // Enable serdes: VXS, Fiber0 (HTCC), Fiber1 (ADCFT1), Fiber2 (ADCFT2)
      }
      else if(sspFirmwareType[sspSlot(issp)] == SSP_CFG_SSPTYPE_HPS)
      {
        printf("SSP_CFG_SSPTYPE_HPS\n");
        sspPortEnable(sspSlot(issp), 0x003, 1);  // Enable serdes: Fiber0, Fiber1
      }
      else if(sspFirmwareType[sspSlot(issp)] == SSP_CFG_SSPTYPE_HALLBRICH)
      {
        printf("SSP_CFG_SSPTYPE_HALLBRICH\n");
        sspEnableBusError(sspSlot(issp));
        sspRich_Init(sspSlot(issp));
      }
      else
        printf("SSP_CFG_SSPTYPE_UNKNOWN\n");
      
      /* soft reset (resets EB and fifo) */
      vmeWrite32(&pSSP[sspSlot(issp)]->Cfg.Reset, 1);
      taskDelay(2);
      vmeWrite32(&pSSP[sspSlot(issp)]->Cfg.Reset, 0);
      taskDelay(2);
    }	 
  } 

  printf("sspInit: found %d SSPs\n",nSSP);

  return(nSSP);
} 
 


void 
sspCheckAddresses(int id) 
{ 
  unsigned int offset=0, expected=0, base=0; 
  int iser=0; 
 
  if(id==0) id=sspSL[0]; 
  if((id<=0) || (id>21) || (pSSP[id]==NULL)) 
    { 
      printf("%s: ERROR: SSP in slot %d not initialized\n",__FUNCTION__,id); 
      return; 
    } 
   
  printf("%s:\n\t ---------- Checking SSP address space ---------- \n",__FUNCTION__); 
 
  base = (unsigned int) &pSSP[id]->Cfg; 
 
  offset = ((unsigned int) &pSSP[id]->Clk) - base; 
  expected = 0x100; 
  if(offset != expected) 
    printf("%s: ERROR pSSP[%d]->Clk not at offset = 0x%x (@ 0x%x)\n", 
	   __FUNCTION__,id,expected,offset); 
 
  offset = ((unsigned int) &pSSP[id]->Sd) - base; 
  expected = 0x200; 
  if(offset != expected) 
    printf("%s: ERROR pSSP[%d]->Sd not at offset = 0x%x (@ 0x%x)\n", 
	   __FUNCTION__,id,expected,offset); 
 
  offset = ((unsigned int) &pSSP[id]->Trigger) - base; 
  expected = 0x2100; 
  if(offset != expected) 
    printf("%s: ERROR pSSP[%d]->Trigger not at offset = 0x%x (@ 0x%x)\n", 
	   __FUNCTION__,id,expected,offset); 
 
  for(iser=0; iser<10; iser++) 
    { 
      offset = ((unsigned int) &pSSP[id]->Ser[iser]) - base; 
      expected = 0x1000 + iser*0x100; 
      if(offset != expected) 
	printf("%s: ERROR pSSP[%d]->Ser[%d] not at offset = 0x%x (@ 0x%x)\n", 
	       __FUNCTION__,id,iser,expected,offset); 
    } 
 
} 

/******************************************************************************* 
 * 
 * sspSlot - Convert an index into a slot number, where the index is 
 *           the element of an array of SSPs in the order in which they were 
 *           initialized. 
 * 
 * RETURNS: Slot number if Successfull, otherwise ERROR. 
 * 
 */ 
 

int
sspGetGeoAddress(int id)
{
  int slot;

  /*
  slot =  (sspReadReg(&ssp->Cfg.FirmwareRev)&SSP_CFG_SLOTID_MASK)>>24;
  */
  return(slot);
}


int 
sspSlot(unsigned int id) 
{ 
  if(id>=nSSP) 
    { 
      printf("%s: ERROR: Index (%d) >= SSPs initialized (%d).\n", 
	     __FUNCTION__,id,nSSP); 
      return ERROR; 
    } 
 
  return(sspSL[id]); 
} 

int 
sspId(unsigned int slot) 
{
  int id;

  for(id=0; id<nSSP; id++)
    if(sspSL[id] == slot)
      return(id);

  printf("%s: ERROR: SSP in slot %d does not exist or not initialized.\n",__FUNCTION__,slot);
  return(ERROR);
} 

int
sspEbReset(int id, int reset)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;

  SSPLOCK(); 
  if(reset)
    vmeWrite32(&pSSP[id]->Cfg.Reset, 1);
  else
    vmeWrite32(&pSSP[id]->Cfg.Reset, 0);
  SSPUNLOCK(); 
    
  taskDelay(2);
}

/******************************************************************************* 
 * 
 * sspSetMode(int id, int iFlag) 
 *       id: SSP Slot number 
 *    iFlag: 
 *        bits 1:0 - Mode 
 *           0 - disabled 
 *               clk src = 0 
 *               sync src = 0 
 *               trig src = 0 
 *           1 - local/P2LVDS 
 *               clk src = LOCAL 
 *               sync src = P2LVDSIN0 
 *               trig src = P2LVDSIN1 
 *           2 - local/FPLVDS 
 *               clk src = LOCAL 
 *               sync src = FPLVDSIN0 
 *               trig src = FPLVDSIN1 
 *           3 - vxs 
 *               clk src = VXS SWB (SD) 
 *               sync src = VXS SWB (SD) 
 *               trig src = VXS SWB (SD) 
 * 
 *        bits 23:16 - Fiber Enable 
 *           see TRG_CTRL_FIBER_ENx definitions in ssp.h 
 *        bits 31:24 - GTP data source 
 *           see TRG_CTRL_GTPSRC_* definitions in ssp.h 
 * 
 *  RETURNS: OK if successful, otherwise ERROR. 
 * 
 */ 
 
int 
sspSetMode(int id, int iFlag, int pflag) 
{ 
  int result, clksrc, syncsrc, trigsrc;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  switch((iFlag>>0) & SSP_INIT_MODE_MASK) 
  { 
    case SSP_INIT_MODE_DISABLED: 
      clksrc = SSP_CLKSRC_DISABLED; 
      syncsrc = SD_SRC_SEL_0; 
      trigsrc = SD_SRC_SEL_0; 
      break; 
 
    case SSP_INIT_MODE_P2: 
      clksrc = SSP_CLKSRC_LOCAL; 
      syncsrc = SD_SRC_SEL_P2LVDSIN0; 
      trigsrc = SD_SRC_SEL_P2LVDSIN1;			 
      break; 
 
    case SSP_INIT_MODE_FP: 
      clksrc = SSP_CLKSRC_LOCAL; 
      syncsrc = SD_SRC_SEL_LVDSIN0; 
      trigsrc = SD_SRC_SEL_LVDSIN1;			 
      break; 
 
    case SSP_INIT_MODE_VXS: 
      clksrc = SSP_CLKSRC_SWB; 
      syncsrc = SD_SRC_SEL_SYNC; 
      trigsrc = SD_SRC_SEL_TRIG1; 
      break; 
  } 
	 
  if((iFlag & SSP_INIT_SKIP_SOURCE_SETUP)==0) 
  { 
      /* Setup Clock Source */ 
      result = sspSetClkSrc(id, clksrc); 
      if(result != OK) 
	return ERROR; 
       
      /* Setup Sync Source */ 
      result = sspSetIOSrc(id, SD_SRC_SYNC, syncsrc); 
      if(result != OK) 
	return ERROR; 
       
      /* Setup Trig Source */ 
      result = sspSetIOSrc(id, SD_SRC_TRIG, trigsrc); 
      if(result != OK) 
	return ERROR; 
  } 

  if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBGT)
    sspPortEnable(id, (iFlag & SSP_INIT_FIBER_ENABLE_MASK)>>16, pflag);
  else if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBGTC)
    sspPortEnable(id, (iFlag & SSP_INIT_FIBER_ENABLE_MASK)>>16, pflag);
  else if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HPS)
    sspPortEnable(id, (iFlag & SSP_INIT_FIBER_ENABLE_MASK)>>16, pflag); 
  else if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBRICH)
  {
  }

  return OK; 
} 

int
sspPrintEbStatus(int id)
{
  unsigned int blockcnt, wordcnt, eventcnt;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
  
  SSPLOCK(); 
  blockcnt = sspReadReg(&pSSP[id]->EB.FifoBlockCnt);
  wordcnt = sspReadReg(&pSSP[id]->EB.FifoWordCnt);
  eventcnt = sspReadReg(&pSSP[id]->EB.FifoEventCnt);
  SSPUNLOCK(); 
  
  printf(" B=%3d E=%3d W=%6d", blockcnt, eventcnt, wordcnt);
  
  return wordcnt;
}

int
sspGetEbWordCnt(int id)
{
  unsigned int blockcnt, wordcnt, eventcnt;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
  
  SSPLOCK(); 
  blockcnt = sspReadReg(&pSSP[id]->EB.FifoBlockCnt);
  wordcnt = sspReadReg(&pSSP[id]->EB.FifoWordCnt);
  eventcnt = sspReadReg(&pSSP[id]->EB.FifoEventCnt);
  SSPUNLOCK(); 
  
 // printf(" B=%3d E=%3d W=%6d", blockcnt, eventcnt, wordcnt);
  
  return wordcnt;
}

int 
sspStatus(int id, int rflag) 
{ 
  int showregs=0; 
  int i=0; 
  unsigned int fiberEnabledMask=0; 
  unsigned int SSPBase=0; 
  SSP_regs st;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  if(rflag & SSP_STATUS_SHOWREGS) 
    showregs=1; 
 
 
  SSPLOCK(); 
  SSPBase             = (unsigned int)pSSP[id]; 
  
  st.EB.BlockCfg      = sspReadReg(&pSSP[id]->EB.BlockCfg);
  st.EB.ReadoutCfg    = sspReadReg(&pSSP[id]->EB.ReadoutCfg);
  st.EB.WindowWidth   = sspReadReg(&pSSP[id]->EB.WindowWidth);
  st.EB.Lookback      = sspReadReg(&pSSP[id]->EB.Lookback);
  st.EB.FifoBlockCnt  = sspReadReg(&pSSP[id]->EB.FifoBlockCnt);
  st.EB.FifoWordCnt   = sspReadReg(&pSSP[id]->EB.FifoWordCnt);
  st.EB.FifoEventCnt  = sspReadReg(&pSSP[id]->EB.FifoEventCnt);
  st.Cfg.BoardId      = sspReadReg(&pSSP[id]->Cfg.BoardId); 
  st.Cfg.FirmwareRev  = sspReadReg(&pSSP[id]->Cfg.FirmwareRev); 
  st.Clk.Ctrl         = sspReadReg(&pSSP[id]->Clk.Ctrl); 
  st.Clk.Status       = sspReadReg(&pSSP[id]->Clk.Status); 
  for(i=0; i < SD_SRC_NUM; i++) 
    st.Sd.SrcSel[i]   = sspReadReg(&pSSP[id]->Sd.SrcSel[i]); 
  for(i=0; i < SSP_SER_NUM; i++) 
  { 
    st.Ser[i].Ctrl    = sspReadReg(&pSSP[id]->Ser[i].Ctrl); 
    st.Ser[i].Status = sspReadReg(&pSSP[id]->Ser[i].Status); 
    if((st.Ser[i].Ctrl & SSP_SER_CTRL_POWERDN)==0) 
      fiberEnabledMask |= (1<<i); 
  } 
  SSPUNLOCK(); 
 
#ifdef VXWORKS 
  printf("\nSTATUS for SSP in slot %d at base address 0x%x \n", 
	 id, (UINT32) pSSP[id]); 
#else 
  printf("\nSTATUS for SSP in slot %d at VME (Local) base address 0x%x (0x%x)\n", 
	 id, (UINT32) pSSP[id] - sspA24Offset, (UINT32) pSSP[id]); 
#endif 
  printf("--------------------------------------------------------------------------------\n"); 
 
  if(showregs)
  { 
    printf("\n"); 
    printf(" Registers (offset):\n"); 
    printf("  Cfg.BoardID    (0x%04x) = 0x%08x\t", (unsigned int)(&pSSP[id]->Cfg.BoardId) - SSPBase, st.Cfg.BoardId); 
    printf("  Cfg.FirmwareRev(0x%04x) = 0x%08x\n", (unsigned int)(&pSSP[id]->Cfg.FirmwareRev) - SSPBase, st.Cfg.FirmwareRev); 
    printf("  Clk.Ctrl       (0x%04x) = 0x%08x\t", (unsigned int)(&pSSP[id]->Clk.Ctrl) - SSPBase, st.Clk.Ctrl); 
    printf("  Clk.Status     (0x%04x) = 0x%08x\n", (unsigned int)(&pSSP[id]->Clk.Status) - SSPBase, st.Clk.Status); 
 
    for(i=0; i < SD_SRC_NUM; i=i+2) 
	 { 
	   printf("  Sd.SrcSel[%2d]  (0x%04x) = 0x%08x\t", i, (unsigned int)(&pSSP[id]->Sd.SrcSel[i]) - SSPBase, st.Sd.SrcSel[i]); 
	   printf("  Sd.SrcSel[%2d]  (0x%04x) = 0x%08x\n", i+1, (unsigned int)(&pSSP[id]->Sd.SrcSel[i+1]) - SSPBase, st.Sd.SrcSel[i+1]); 
	 } 
    for(i=0; i < SSP_SER_NUM; i=i+2) 
	 { 
	   printf("  Ser[%2d].Ctrl   (0x%04x) = 0x%08x\t", i, (unsigned int)(&pSSP[id]->Ser[i].Ctrl) - SSPBase, st.Ser[i].Ctrl); 
	   printf("  Ser[%2d].Ctrl   (0x%04x) = 0x%08x\n", i+1, (unsigned int)(&pSSP[id]->Ser[i+1].Ctrl) - SSPBase, st.Ser[i+1].Ctrl); 
	 } 
  } 
  printf("\n"); 
 
  printf(" Board Firmware Rev/ID = 0x%04x\n", 
	 st.Cfg.FirmwareRev&0x0000FFFF); 
 
  printf("\n Signal Sources: \n"); 
  printf("   Ref Clock : %s - %s\n", 
	 ((st.Clk.Ctrl & CLK_CTRL_SERDES_MASK)>>24)<SSP_CLKSRC_NUM ? 
	 ssp_clksrc_name[(st.Clk.Ctrl & CLK_CTRL_SERDES_MASK)>>24] : 
	 "unknown", 
	 (st.Clk.Status & CLK_STATUS_GCLKLOCKED) ? 
	 "PLL Locked" : 
	 "*** PLL NOT Locked ***"); 
 
  printf("   Trig1     : %s\n", 
	 (st.Sd.SrcSel[SD_SRC_TRIG]<SD_SRC_SEL_NUM) ? 
	 ssp_signal_names[st.Sd.SrcSel[SD_SRC_TRIG]] : 
	 "unknown"); 
 
  printf("   SyncReset : %s\n", 
	 (st.Sd.SrcSel[SD_SRC_SYNC]<SD_SRC_SEL_NUM) ? 
	 ssp_signal_names[st.Sd.SrcSel[SD_SRC_SYNC]] : 
	 "unknown"); 

  printf(" Readout configuration: \n");
  printf("    Block size = %d\n", st.EB.BlockCfg);
  printf("    Bus error enabled = %d\n", st.EB.ReadoutCfg & 0x1);
  printf("    Readout window width = %dns\n", st.EB.WindowWidth*4);
  printf("    Readout lookback = %dns\n", st.EB.Lookback*4);
  
  printf(" Event builder status: \n");
  printf("    FifoBlockCnt = %d\n", st.EB.FifoBlockCnt);
  printf("    FifoWordCnt = %d\n", st.EB.FifoWordCnt);
  printf("    FifoEventCnt = %d\n", st.EB.FifoEventCnt);
  
  printf("\n"); 
 
  if(fiberEnabledMask) 
  { 
    printf(" Fiber Ports Enabled (0x%x) =\n",fiberEnabledMask); 
    for(i=0; i <= SSP_SER_FIBER7; i++) 
    {
      if(fiberEnabledMask & (1<<i)) 
        printf("   %-10s: -%-12s-\n", 
          ssp_serdes_names[i], 
          (st.Ser[i].Status & SSP_SER_STATUS_CHUP) ? "CHANNEL UP" : "CHANNEL DN"); 
    } 
    printf(" VXS Ports Enabled (0x%x) =\n",fiberEnabledMask); 
    for(i=SSP_SER_VXS0; i <= SSP_SER_VXSGTP; i++) 
    {
      if(fiberEnabledMask & (1<<i)) 
        printf("   %-10s: -%-12s-\n", 
          ssp_serdes_names[i], 
          (st.Ser[i].Status & SSP_SER_STATUS_CHUP) ? "CHANNEL UP" : "CHANNEL DN"); 
    } 
  } 
  else 
    printf(" No Serdes Ports Enabled\n"); 
 
  printf("\n"); 
  printf(" I/O Configuration: \n"); 
  sspPrintIOSrc(id,2); 
  printf("\n");
  
  if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HPS)
     sspPrintHpsConfig(id);
     
  if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBGT)
     sspPrintGtConfig(id);

  if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBGTC)
     sspPrintGtcConfig(id);
  
  if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBRICH)
    sspRich_PrintFiberStatus(id);
  
  printf("--------------------------------------------------------------------------------\n"); 
  printf("\n"); 
 
  return OK; 
} 
 
void 
sspGStatus(int rflag) 
{ 
  int showregs=0; 
  int issp=0, id=0, i=0, iport=0; 
  SSP_regs st[20]; 
  int portUsedInTrigger[20][SSP_SER_NUM]; 
 
  if(rflag & SSP_STATUS_SHOWREGS) 
    showregs=1; 
 
  SSPLOCK(); 
  for(issp=0; issp<nSSP; issp++) 
  { 
    id = sspSlot(issp); 
    for(i=0; i < SSP_SER_NUM; i++) 
	 { 
	   st[id].Ser[i].Ctrl       = sspReadReg(&pSSP[id]->Ser[i].Ctrl); 
	   st[id].Ser[i].Status     = sspReadReg(&pSSP[id]->Ser[i].Status); 
	   st[id].Ser[i].Latency     = sspReadReg(&pSSP[id]->Ser[i].Latency); 
//	   st[id].Ser[i].MonStatus  = sspReadReg(&pSSP[id]->Ser[i].MonStatus); 
//	   st[id].Ser[i].CrateId    = sspReadReg(&pSSP[id]->Ser[i].CrateId) & SER_CRATEID_MASK; 
//	   st[id].Ser[i].ErrTile0   = sspReadReg(&pSSP[id]->Ser[i].ErrTile0); 
//	   st[id].Ser[i].ErrTile1   = sspReadReg(&pSSP[id]->Ser[i].ErrTile1); 
	 } 
  } 
  SSPUNLOCK(); 
 
  printf("\n"); 
 
  printf("                            SSP Port Status Summary\n\n"); 
  printf("                Channel  Trig Latency(ns)   Bit errors   \n"); 
  printf("Sl- P    Status   RX      TX                             \n"); 
  printf("---------------------------------------------------------\n"); 
  for(issp=0; issp<nSSP; issp++) 
  { 
    id = sspSlot(issp); 
    for(iport=0; iport<SSP_SER_NUM; iport++) 
	 { 
	   /* Slot and port number */ 
	   printf("%2d-%2d ",id, iport+1); 
 
	   /* Channel Status */ 
	   printf("%s      ", 
		 (st[id].Ser[iport].Status & SSP_SER_STATUS_CHUP) ?    
		 " UP ":"DOWN"); 
 
	  /* Trigger latency */ 
	  printf("%5d   ", 
		 ((st[id].Ser[iport].Latency>>16)&0xFFFF)*4);

	  printf("%5d   ", 
		 ((st[id].Ser[iport].Latency>>0)&0xFFFF)*4); 

	  printf("%5d   ", 
		 (st[id].Ser[iport].Status>>24)&0xFF); 

	   printf("\n"); 
	 } 
  } 
  printf("--------------------------------------------------------------------------------\n"); 
  printf("\n"); 
  printf("\n"); 
 
} 
 
/************************************************************ 
 * SSP CLK Functions 
 ************************************************************/ 
 
/************************************************************ 
 * int sspSetClkSrc(int src) 
 *    src options: 
 *        SSP_CLKSRC_DISABLED 
 *        SSP_CLKSRC_SWB 
 *        SSP_CLKSRC_P2 
 *        SSP_CLKSRC_LOCAL 
*/ 
 
int  
sspSetClkSrc(int id, int src) 
{ 
  unsigned int clksrc;  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
	 
  if((src < SSP_CLKSRC_DISABLED) || (src > SSP_CLKSRC_LOCAL)) 
    { 
      printf("%s: ERROR: invalid clock source: %d [unknown]\n",  
	     __FUNCTION__,src); 
      return ERROR; 
    } 
	 
  clksrc = (src<<24) | (src<<26); 
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Clk.Ctrl, CLK_CTRL_GCLKRST | clksrc ); 
  taskDelay(1); 
  sspWriteReg(&pSSP[id]->Clk.Ctrl, clksrc); 
  taskDelay(1); 
  SSPUNLOCK(); 
	 
  if(sspGetClkStatus(id) == ERROR) 
    { 
      printf("%s: ERROR: PLL not locked - no clock at source: %d [%s]\n", 
	     __FUNCTION__, 
	     src, ssp_clksrc_name[src]); 
      return ERROR; 
    }	 
	 
  printf("%s:  Clock source successfully set to: %d [%s]\n",  
	 __FUNCTION__, 
	 src, ssp_clksrc_name[src]); 
  return OK; 
} 
 
int  
sspGetClkStatus(int id) 
{ 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  if(!(sspReadReg(&pSSP[id]->Clk.Status) & CLK_STATUS_GCLKLOCKED)) 
    { 
      printf("%s: ERROR: PLL not locked\n", 
	     __FUNCTION__); 
      SSPUNLOCK(); 
      return ERROR; 
    }	 
	 
  printf("%s: PLL locked\n", 
	 __FUNCTION__); 
  SSPUNLOCK(); 
 
  return OK; 
} 
 
int 
sspGetClkSrc(int id, int pflag) 
{ 
  int rval=0; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->Clk.Ctrl) & CLK_CTRL_SERDES_MASK)>>24; 
  SSPUNLOCK(); 
	 
  if(pflag) 
    { 
      printf("%s: Clock Source = %d [%s]\n", 
	     __FUNCTION__, 
	     rval, 
	     (rval<SSP_CLKSRC_NUM) ? 
	     ssp_clksrc_name[rval] : 
	     "unknown"); 
    } 
 
  return rval; 
} 
 
/************************************************************ 
 * SSP SD.IO Functions 
 ************************************************************/ 
 
int  
sspSetIOSrc(int id, int ioport, int signal) 
{ 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  if((ioport < 0) || (ioport >= SD_SRC_NUM)) 
    { 
      printf("%s: ERROR: invalid ioport (%d)\n",  
	     __FUNCTION__, 
	     ioport); 
      return ERROR; 
    } 
	 
  if((signal < 0) || (signal >= SD_SRC_SEL_NUM)) 
    { 
      printf("%s: ERROR: invalid signal source (%d)\n",  
	     __FUNCTION__, 
	     signal); 
      return ERROR; 
    } 
		 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.SrcSel[ioport], signal); 
  SSPUNLOCK(); 
 
  return OK; 
} 
 
int 
sspGetIOSrc(int id, int ioport, int pflag) 
{ 
  int rval=0; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  if((ioport < 0) || (ioport >= SD_SRC_NUM)) 
    { 
      printf("%s: ERROR: invalid ioport (%d)\n",  
	     __FUNCTION__,ioport); 
      return ERROR; 
    } 
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->Sd.SrcSel[ioport]) & SD_SRC_SEL_MASK; 
  SSPUNLOCK(); 
 
  if(pflag) 
    { 
      if(rval < SD_SRC_SEL_NUM) 
	printf("%s:   %15s mapped to: %s\n",  
	       __FUNCTION__, 
	       ssp_ioport_names[ioport], ssp_signal_names[rval]); 
      else 
	printf("%s:   %15s mapped to: unknown\n",  
	       __FUNCTION__, 
	       ssp_ioport_names[ioport]); 
    } 
 
  return rval; 
} 
 
void  
sspPrintIOSrc(int id, int pflag) 
{ 
  int i; 
  unsigned int val; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
	 
  if(pflag!=2) 
    printf(" %s: \n",__FUNCTION__); 
  SSPLOCK(); 
  for(i = 0; i < SD_SRC_NUM; i++) 
    { 
      val = sspReadReg(&pSSP[id]->Sd.SrcSel[i]) & SD_SRC_SEL_MASK; 
      if(val < SD_SRC_SEL_NUM) 
	printf("   %15s mapped to: %s\n",  
	       ssp_ioport_names[i], ssp_signal_names[val]); 
      else 
	printf("   %15s mapped to: unknown\n", 
	       ssp_ioport_names[i]); 
    } 
  SSPUNLOCK(); 
 
} 
 
/************************************************************ 
 * SSP Trigger Functions 
 ************************************************************/ 
 
/************************************************************ 
 * SSP SD.PULSER Functions 
 ************************************************************/ 
 
int  
sspPulserStatus(int id) 
{ 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  if(sspReadReg(&pSSP[id]->Sd.PulserDone) & SD_PULSER_DONE) 
    { 
      SSPUNLOCK(); 
      return 1;	// pulser has finished sending npulses 
    } 
 
  SSPUNLOCK(); 
  return 0;		// pulser is active 
} 
 
void  
sspPulserStart(int id) 
{ 
  if(id==0) id=sspSL[0]; 
  if((id<=0) || (id>21) || (pSSP[id]==NULL)) 
    { 
      printf("%s: ERROR: SSP in slot %d not initialized\n",__FUNCTION__,id); 
      return; 
    } 
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.PulserStart, 0); 
  SSPUNLOCK(); 
} 
 
 
/************************************************************ 
 * int sspPulserSetup(float freq, float duty, unsigned npulses) 
 *    freq: 
 *        0.01 to 25E6 pulser frequency in Hz 
 *    duty: 
 *        0 to 1 pulser duty cycle 
 *    npulses: 
 *        0: pulser disabled 
 *        1 to 0xFFFFFFFE: pulser fires this number of times before being disabled.  
 *                         Must write to Sd.PulserStart to start pulser in this mode 
 *        0xFFFFFFFF: pulser fires forever 
 */ 
 
void
sspPulserSetup(int id, float freq, float duty, unsigned int npulses) 
{ 
  unsigned int per, low; 	 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  if(freq < SD_PULSER_FREQ_MIN) 
    { 
      printf("%s: ERROR: Frequency input (%f) too low. Setting to minimum...\n",  
	     __FUNCTION__,freq); 
      freq = SD_PULSER_FREQ_MIN; 
    } 
	 
  if(freq > SD_PULSER_FREQ_MAX) 
    { 
      printf("%s: ERROR: Frequency input (%f) too high. Setting to maximum...\n",  
	     __FUNCTION__,freq); 
      freq = SD_PULSER_FREQ_MAX; 
    } 
	 
  if((duty < 0.0) || (duty > 1.0)) 
    { 
      printf("%s: ERROR: Invalid duty cycle %f. Setting to 0.5\n",  
	     __FUNCTION__,duty); 
      duty = 0.5; 
    } 
 
  SSPLOCK();	 
  /* Set to disable pulser output during setup */
  sspWriteReg(&pSSP[id]->Sd.PulserLowCycles, 0xFFFFFFFF); 
  sspWriteReg(&pSSP[id]->Sd.PulserPeriod, 0);
  sspWriteReg(&pSSP[id]->Sd.PulserNPulses, npulses);
  /* Hack to let npulses saturate in case burst mode is needed with no glitches */
  taskDelay(60);
  
  // Setup period register...
  //
  if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBRICH)
    per = (int)( ((float)GCLK_FREQ/2.0) / freq );
  else
    per = (int)( ((float)GCLK_FREQ) / freq );

  if(!per) per = 1;
  printf("%s: per = GCLK_FREQ=%f / freq=%f = %d\n",__FUNCTION__,(float)GCLK_FREQ,freq,per);
  sspWriteReg(&pSSP[id]->Sd.PulserPeriod, per);
	 
  // Setup duty cycle register...	 
  low = (int)( ((float)per) * duty ); 
  if(!low) low = 1; 
  sspWriteReg(&pSSP[id]->Sd.PulserLowCycles, low); 

  printf("%s: Actual frequency = %f, duty = %f\n",__FUNCTION__,(float)GCLK_FREQ/(float)per, (float)low/(float)per); 
  SSPUNLOCK(); 

  sspPulserStart(id);
} 

int
sspGetPulserFreq(int id)
{
  int val;
  float fval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
  
  SSPLOCK();
  val = sspReadReg(&pSSP[id]->Sd.PulserPeriod);
  SSPUNLOCK();
  printf("sspGetPulserFreq: val=%d\n",val);
  if(!val) return(0);

  if(sspFirmwareType[id] == SSP_CFG_SSPTYPE_HALLBRICH)
    fval = ((float)GCLK_FREQ/2.0)/((float)val);
  else
    fval = ((float)GCLK_FREQ)/((float)val);
  printf("sspGetPulserFreq: val=%d -> fval = %f / %f = %f (%d)\n",val,(float)GCLK_FREQ,(float)val,fval,(int)fval);
  
  return((int)fval);
}

/************************************************************ 
 * SSP SERDES Functions 
 ************************************************************/ 
 
/************************************************************ 
 * int sspPortEnable(int mask) 
 *    mask bits (set bit enables serdes): 
 *       SSP_SER_FIBER0				0 
 *       SSP_SER_FIBER1				1 
 *       SSP_SER_FIBER2				2 
 *       SSP_SER_FIBER3				3 
 *       SSP_SER_FIBER4				4 
 *       SSP_SER_FIBER5				5 
 *       SSP_SER_FIBER6				6 
 *       SSP_SER_FIBER7				7 
 *       SSP_SER_VXS0				8 
 *       SSP_SER_VXSGTP				9 
 */ 
 
void 
sspPortEnable(int id, int mask, int pflag) 
{ 
  int i, j; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  if(pflag) 
    printf("%s - \n",__FUNCTION__); 
  SSPLOCK();
  for(i = 0; i < SSP_SER_NUM; i++) 
  { 
    if(mask & (1<<i)) 
    { 
      if(pflag) 
        printf("   Enabling channel: %s...\n", ssp_serdes_names[i]); 
 
      for(j = 0; j < 2/*10 sergey: temporary*/; j++) 
		{
        /* if the port already has its channel up, skip it */ 
        if((sspReadReg(&pSSP[id]->Ser[i].Status) & SSP_SER_STATUS_CHUP) == 0) 
        { 
	       sspWriteReg(&pSSP[id]->Ser[i].Ctrl, SSP_SER_CTRL_LINKRST | 
            SSP_SER_CTRL_GTXRST | 
            SSP_SER_CTRL_POWERDN); 
 
          sspWriteReg(&pSSP[id]->Ser[i].Ctrl, SSP_SER_CTRL_LINKRST | 
            SSP_SER_CTRL_GTXRST); 
		 
          sspWriteReg(&pSSP[id]->Ser[i].Ctrl, SSP_SER_CTRL_LINKRST); 
		 
          sspWriteReg(&pSSP[id]->Ser[i].Ctrl, 0); 
          sspWriteReg(&pSSP[id]->Ser[i].Ctrl, SSP_SER_CTRL_ERRCNT_EN);
		  
          usleep(20000);
		  }
      }
      if((sspReadReg(&pSSP[id]->Ser[i].Status) & SSP_SER_STATUS_CHUP) == 0)
        printf("%s: ERROR: SSP in slot %d FIBER %d NOT UP\n", __FUNCTION__,id,i);
    } 
    else 
    { 
      if(pflag) 
        printf("   Disabling channel: %s...\n", ssp_serdes_names[i]); 
      sspWriteReg(&pSSP[id]->Ser[i].Ctrl, SSP_SER_CTRL_LINKRST | 
        SSP_SER_CTRL_GTXRST | 
        SSP_SER_CTRL_POWERDN); 
    } 
  } 
  SSPUNLOCK(); 
  sspPortResetErrorCount(id, mask); 
  sspPortPrintStatus(id); 
} 
 
void  
sspPortResetErrorCount(int id, int mask) 
{ 
  int i; 
  unsigned int val; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  SSPLOCK(); 
  for(i = 0; i < SSP_SER_NUM; i++) 
    { 
      if(mask & (1<<i)) 
	{ 
	  val = sspReadReg(&pSSP[id]->Ser[i].Ctrl); 
	  sspWriteReg(&pSSP[id]->Ser[i].Ctrl, val | SSP_SER_CTRL_ERRCNT_RST); 
	  sspWriteReg(&pSSP[id]->Ser[i].Ctrl, val & ~SSP_SER_CTRL_ERRCNT_RST); 
	} 
    } 
  SSPUNLOCK(); 
} 
 
void  
sspPortPrintStatus(int id)
{ 
  int i; 
  unsigned int ctrl, status, latency; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  printf("\n");
  printf("    ---Lane---    Error  Link  Latency(ns)\n");
  printf("FB  0 1 2 3    Ch Count  Reset RX     TX   \n");
  printf("------------------------------------------------------------------------------\n");
  for(i = 0; i < 8; i++) 
  {
    SSPLOCK();       
    ctrl = sspReadReg(&pSSP[id]->Ser[i].Ctrl); 
    status = sspReadReg(&pSSP[id]->Ser[i].Status); 
    latency = sspReadReg(&pSSP[id]->Ser[i].Latency);
    SSPUNLOCK(); 
    
    printf("%d ", i);
    printf("%s ", (status & 0x2)?"U":"D");
    printf("%s ", (status & 0x4)?"U":"D");
    printf("%s ", (status & 0x8)?"U":"D");
    printf("%s ", (status & 0x10)?"U":"D");
    printf("%s  ", (status & 0x1)?"U":"D");
    printf("%3d    ", status>>24);
    printf("%s     ", (ctrl & 0x1)?"1":"0");
    printf("%5d ", ((latency>>16)&0xFFFF)*4);
    printf("%5d ", ((latency>>0)&0xFFFF)*4);
    printf("\n");
  }
  
  printf("    ---Lane---    Error  Link  Latency(ns)\n");
  printf("VXS 0 1        Ch Count  Reset RX     TX   \n");
  printf("------------------------------------------------------------------------------\n");
  for(i = 8; i < 10; i++) 
  {
    SSPLOCK();       
    ctrl = sspReadReg(&pSSP[id]->Ser[i].Ctrl); 
    status = sspReadReg(&pSSP[id]->Ser[i].Status); 
    latency = sspReadReg(&pSSP[id]->Ser[i].Latency);
    SSPUNLOCK(); 
    
    printf("%d   ", i-8);
    printf("%s ", (status & 0x2)?"U":"D");
    printf("%s        ", (status & 0x4)?"U":"D");
    printf("%s  ", (status & 0x1)?"U":"D");
    printf("%3d    ", status>>24);
    printf("%s     ", (ctrl & 0x1)?"1":"0");
    printf("%5d ", ((latency>>16)&0xFFFF)*4);
    printf("%5d ", ((latency>>0)&0xFFFF)*4);
    printf("\n");
  }
} 
 
int 
sspGetConnectedFiberMask(int id) 
{ 
  int rval=0; 
  int iport=0; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  for(iport=SSP_SER_FIBER0; iport<=SSP_SER_FIBER7; iport++) 
    { 
      if(vmeRead32(&pSSP[id]->Ser[iport].Status) & SSP_SER_STATUS_CHUP) 
	rval |= (1<<iport); 
    } 
  SSPUNLOCK(); 
 
  return rval; 
} 
 
int 
sspGetCrateID(int id, int port) 
{ 
  int crateid=0; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  if((port<SSP_SER_FIBER0) || (port>SSP_SER_FIBER7)) 
    { 
      printf("%s: ERROR: Invalid port (%d)\n", 
	     __FUNCTION__, 
	     port); 
      return ERROR; 
    } 
 
  SSPLOCK(); 
  crateid = sspReadReg(&pSSP[id]->Ser[port].CrateId) & SER_CRATEID_MASK; 
  SSPUNLOCK(); 
   
  return crateid; 
} 
 
void 
sspSerdesEnable(int id, int mask, int pflag) 
{ 
  sspPortEnable(id,mask,pflag); 
} 
 
void  
sspSerdesPrintStatus(int id) 
{ 
  sspPortPrintStatus(id); 
} 

/************************************************************ 
 * SSP SD.SCALERS Functions 
 ************************************************************/ 

int
sspGSendScalers()
{
  int r = OK;
  int issp, id;

  for(issp=0; issp<nSSP; issp++)
  {
    id = sspSlot(issp);
    r = sspSendScalers(id);
    if(r != OK)
      break;
  }
  return r;
}

int
sspSendScalers(int id)
{
  int r = OK;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;

  switch(sspFirmwareType[id])
  {
    case SSP_CFG_SSPTYPE_HALLBGT:
      sspGtSendErrors(id);
      r = sspGtSendScalers(id);
      break;
    case SSP_CFG_SSPTYPE_HALLBGTC:
      sspGtcSendErrors(id);
      r = sspGtcSendScalers(id);
      break;
  }
  return r;
}

void  
sspPrintScalers(int id) 
{ 
  double ref, rate; 
  int i; 
  unsigned int scalers[SD_SCALER_NUM]; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);
	 
  for(i = 0; i < SD_SCALER_NUM; i++) 
    scalers[i] = sspReadReg(&pSSP[id]->Sd.Scalers[i]); 
	 
  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
  SSPUNLOCK(); 
 
  printf("%s - \n", 
	 __FUNCTION__); 
  if(!scalers[SD_SCALER_SYSCLK]) 
    { 
      printf("Error: sspPrintScalers() reference time is 0. Reported rates will not be normalized.\n"); 
      ref = 1.0; 
    } 
  else 
    { 
      ref = (double)scalers[SD_SCALER_SYSCLK] / (double)SYSCLK_FREQ; 
    } 
	 
  for(i = 0; i < SD_SCALER_NUM; i++) 
    { 
      rate = (double)scalers[i]; 
      rate = rate / ref; 
      if(scalers[i] == 0xFFFFFFFF) 
	printf("   %-25s %10u,%.3fHz [OVERFLOW]\n", ssp_scaler_name[i], scalers[i], rate); 
      else 
	printf("   %-25s %10u,%.3fHz\n", ssp_scaler_name[i], scalers[i], rate); 
    } 
} 
 


/************************************************************ 
 * SSP SSPCFG Firmware Functions 
 ************************************************************/ 
 
static void  
sspSelectSpi(int id, int sel) 
{ 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  if(sel) 
    sspWriteReg(&pSSP[id]->Cfg.SpiCtrl, SSPCFG_SPI_NCSCLR); 
  else 
    sspWriteReg(&pSSP[id]->Cfg.SpiCtrl, SSPCFG_SPI_NCSSET); 
} 
 
static unsigned char  
sspTransferSpi(int id, unsigned char data) 
{ 
  int i; 
  unsigned int val; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  sspWriteReg(&pSSP[id]->Cfg.SpiCtrl, data | SSPCFG_SPI_START); 
 
  for(i = 0; i < 1000; i++) 
    { 
      val = sspReadReg(&pSSP[id]->Cfg.SpiStatus); 
      if(val & SSPCFG_SPI_DONE) 
	break; 
    } 
  if(i == 1000) 
    printf("%s: ERROR: Timeout!!!\n", 
	   __FUNCTION__); 
	 
  return val & 0xFF; 
} 
 
static void  
sspFlashGetId(int id, unsigned char *rsp) 
{ 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  sspSelectSpi(id,1); 
  sspTransferSpi(id,SPI_CMD_GETID); 
  rsp[0] = sspTransferSpi(id,0xFF); 
  rsp[1] = sspTransferSpi(id,0xFF); 
  rsp[2] = sspTransferSpi(id,0xFF); 
  rsp[3] = sspTransferSpi(id,0xFF);
  rsp[4] = sspTransferSpi(id,0xFF); 
  sspSelectSpi(id,0); 
} 
 
static unsigned char  
sspFlashGetStatus(int id) 
{ 
  unsigned char rsp; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  sspSelectSpi(id,1); 
  sspTransferSpi(id,SPI_CMD_GETSTATUS); 
  rsp = sspTransferSpi(id,0xFF); 
  sspSelectSpi(id,0); 
	 
  return rsp; 
} 
 
static void  
sspReloadFirmware(int id) 
{ 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return;
 
  printf("%s: ERROR: Not implemented yet. Issue power cycle or VME SYSRESET to reload firmware.\n", 
	 __FUNCTION__); 
  /* 
    int i; 
    unsigned short reloadSequence[] = { 
    0xFFFF, 0xAA99, 0x5566, 0x3261, 
    0x0000, 0x3281, 0x0B00, 0x32A1, 
    0x0000, 0x32C1, 0x0B00, 0x30A1, 
    0x000E, 0x2000 
    }; 
 
    VSCM_WriteReg((unsigned int)&pVSCM_BASE->ICap, 0x40000 | 0x00000); 
    VSCM_WriteReg((unsigned int)&pVSCM_BASE->ICap, 0x40000 | 0x20000); 
    for(i = 0; i < sizeof(reloadSequence)/sizeof(reloadSequence[0]); i++) 
    { 
    VSCM_WriteReg((unsigned int)&pVSCM_BASE->ICap, 0x00000 | reloadSequence[i]); 
    VSCM_WriteReg((unsigned int)&pVSCM_BASE->ICap, 0x20000 | reloadSequence[i]); 
    } 
    for(i = 0; i < 10; i++) 
    { 
    VSCM_WriteReg((unsigned int)&pVSCM_BASE->ICap, 0x40000 | 0x00000); 
    VSCM_WriteReg((unsigned int)&pVSCM_BASE->ICap, 0x40000 | 0x20000); 
    } 
    taskDelay(120);
  */ 
} 

int  
sspGFirmwareUpdateVerify(const char *filename) 
{
  int issp=0, id=0; 
  for(issp=0; issp<nSSP; issp++) 
  { 
    id = sspSlot(issp);
    sspFirmwareUpdateVerify(id, filename);
  }
  return OK;
} 
 
int  
sspFirmwareUpdateVerify(int id, const char *filename) 
{ 
  int result; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  printf("Updating firmware..."); 
  result = sspFirmwareUpdate(id, filename); 
  if(result != OK) 
    { 
      printf("failed.\n"); 
      return result; 
    } 
  else 
    printf("succeeded."); 
	 
  printf("\nVerifying..."); 
  result = sspFirmwareVerify(id, filename); 
  if(result != OK) 
    { 
      printf("failed.\n"); 
      return result; 
    } 
  else 
    printf("ok.\n"); 
 
  sspReloadFirmware(id); 
		 
  return OK; 
} 
 
int  
sspFirmwareUpdate(int id, const char *filename) 
{ 
  FILE *f; 
  int i; 
  unsigned int page = 0, page_size = 0; 
  unsigned char buf[1056], rspId[5]; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  sspSelectSpi(id,0); 
  sspFlashGetId(id, rspId); 
  sspSelectSpi(id,0); 
  sspFlashGetId(id, rspId); 
	 
  printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X, EDI Len=0x%02X, EDI=0x%02X\n", rspId[0], rspId[1], rspId[2], rspId[3], rspId[4]); 
 
  if( (rspId[0] == SPI_MFG_ATMEL) && (rspId[1] == (SPI_DEVID_AT45DB642D>>8)) && (rspId[2] == (SPI_DEVID_AT45DB642D&0xFF)) ) 
    { 
    if(rspId[3] == 0x01)
    {
      printf("Found AT45DB641E ");
      page_size = 264;
    }
    else
    {
      printf("Found AT45DB642D ");
      page_size = 1056;
    }
    printf("page size set to: %d bytes\n", page_size);

      f = fopen(filename, "rb"); 
      if(!f) 
	{ 
	  printf("%s: ERROR: invalid file %s\n", __FUNCTION__, filename); 
	  return ERROR; 
	  SSPUNLOCK(); 
	} 
	 
    memset(buf, 0xff, page_size); 
    while(fread(buf, 1, page_size, f) > 0) 
	{ 
	  sspSelectSpi(id,1);	// write buffer 1 
	  sspTransferSpi(id,SPI_CMD_WRBUF1); 
	  sspTransferSpi(id,0x00); 
	  sspTransferSpi(id,0x00); 
	  sspTransferSpi(id,0x00); 
      for(i = 0; i < page_size; i++)
	    sspTransferSpi(id,buf[i]); 
	  sspSelectSpi(id,0); 
 
	  sspSelectSpi(id,1);	// buffer 1 to flash w/page erase 
	  sspTransferSpi(id,SPI_CMD_PGBUF1ERASE); 
      if(page_size == 264)
      { 
        sspTransferSpi(id,(page>>7) & 0xFF); 
        sspTransferSpi(id,(page<<1) & 0xFF); 
        sspTransferSpi(id,0x00); 
      }
      else
      {
	  sspTransferSpi(id,(page>>5) & 0xFF); 
	  sspTransferSpi(id,(page<<3) & 0xFF); 
	  sspTransferSpi(id,0x00); 
      }
	  sspSelectSpi(id,0); 
			 
	  i = 0; 
	  while(1) 
	    { 
	      if(sspFlashGetStatus(id) & 0x80) 
		break; 
	      if(i == 40000)	// 40ms maximum page program time 
		{ 
		  fclose(f); 
		  printf("%s: ERROR: failed to program flash\n", __FUNCTION__); 
		  SSPUNLOCK(); 
		  return ERROR; 
		} 
	      i++; 
	    }			 
      memset(buf, 0xff, page_size); 
	  page++; 
	} 
      fclose(f); 
    } 
  else 
    { 
      printf("%s: ERROR: failed to identify flash id 0x%02X 0x%02X 0x%02X\n",  
	     __FUNCTION__, (int)rspId[0], (int)rspId[1], (int)rspId[2]); 
      SSPUNLOCK(); 
      return ERROR; 
    } 
 
  SSPUNLOCK(); 
  return OK; 
} 
 
int  
sspFirmwareRead(int id, const char *filename) 
{ 
  FILE *f; 
  int i; 
  unsigned char rspId[5]; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  sspSelectSpi(id,0); 
  sspFlashGetId(id, rspId); 
	 
  printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId[0], rspId[1], rspId[2]); 
 
  if( (rspId[0] == SPI_MFG_ATMEL) && 
      (rspId[1] == (SPI_DEVID_AT45DB642D>>8)) && 
      (rspId[2] == (SPI_DEVID_AT45DB642D&0xFF)) ) 
    { 
      f = fopen(filename, "wb"); 
      if(!f) 
	{ 
	  printf("%s: ERROR: invalid file %s\n", __FUNCTION__, filename); 
	  SSPUNLOCK(); 
	  return ERROR; 
	} 
		 
      sspSelectSpi(id,1); 
      sspTransferSpi(id,SPI_CMD_RD);	// continuous array read 
      sspTransferSpi(id,0); 
      sspTransferSpi(id,0); 
      sspTransferSpi(id,0); 
		 
      for(i = 0; i < SPI_BYTE_LENGTH; i++) 
	{ 
	  fputc(sspTransferSpi(id,0xFF), f); 
	  if(!(i% 65536)) 
	    { 
	      printf("."); 
	      taskDelay(1); 
	    } 
	} 
			 
      sspSelectSpi(id,0); 
      fclose(f); 
    } 
  else 
    { 
      printf("%s: ERROR: failed to identify flash id 0x%02X 0x%02X 0x%02X\n",  
	     __FUNCTION__, (int)rspId[0], (int)rspId[1], (int)rspId[2]); 
      SSPUNLOCK(); 
      return ERROR; 
    } 
 
  SSPUNLOCK(); 
  return OK; 
} 
 
int  
sspFirmwareVerify(int id, const char *filename) 
{ 
  FILE *f; 
  int i,len; 
  unsigned int addr = 0; 
  unsigned char buf[256]; 
  unsigned char rspId[5], val; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  sspSelectSpi(id,0); 
  sspFlashGetId(id, rspId); 
	 
  printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId[0], rspId[1], rspId[2]); 
 
  if( (rspId[0] == SPI_MFG_ATMEL) && 
      (rspId[1] == (SPI_DEVID_AT45DB642D>>8)) && 
      (rspId[2] == (SPI_DEVID_AT45DB642D&0xFF)) ) 
    { 
      f = fopen(filename, "rb"); 
      if(!f) 
	{ 
	  printf("%s: ERROR: invalid file %s\n", __FUNCTION__, filename); 
	  SSPUNLOCK(); 
	  return ERROR; 
	} 
		 
      sspSelectSpi(id,1); 
      sspTransferSpi(id,SPI_CMD_RD);	// continuous array read 
      sspTransferSpi(id,0); 
      sspTransferSpi(id,0); 
      sspTransferSpi(id,0); 
 
      while((len = fread(buf, 1, 256, f)) > 0) 
	{ 
	  for(i = 0; i < len; i++) 
	    { 
	      val = sspTransferSpi(id,0xFF); 
	      if(buf[i] != val) 
		{ 
		  sspSelectSpi(id,0); 
		  fclose(f);					 
		  printf("%s: ERROR: failed verify at addess 0x%08X[%02X,%02X]\n",  
			 __FUNCTION__, addr+i, buf[i], val); 
		  SSPUNLOCK(); 
		  return ERROR; 
		} 
	    } 
	  addr+=256; 
	  if(!(addr & 0xFFFF)) 
	    printf(".");					 
	} 
      sspSelectSpi(id,0); 
      fclose(f); 
    } 
  else 
    { 
      printf("%s: ERROR: failed to identify flash id 0x%02X 0x%02X 0x%02X\n",  
	     __FUNCTION__, (int)rspId[0], (int)rspId[1], (int)rspId[2]); 
      SSPUNLOCK(); 
      return ERROR; 
    } 
 
  SSPUNLOCK(); 
  return OK; 
} 
 
int  
sspGetSerialNumber(int id, char *mfg, int *sn) 
{ 
  int i; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  // need to parse this and extract MFG string and Serial int	 
  sspSelectSpi(id,0); 
  sspSelectSpi(id,1); 
  sspTransferSpi(id,SPI_CMD_RD); 
  sspTransferSpi(id,0xFF); 
  sspTransferSpi(id,0xF8); 
  sspTransferSpi(id,0x00); 
	 
  for(i = 0; i < 256; i++) 
    { 
      if(!(i & 0xF)) 
	printf("\n0x%04X: ", i); 
      printf("%02X ", sspTransferSpi(id,0xFF)); 
    } 
	 
  sspSelectSpi(id,0); 
  SSPUNLOCK(); 
		 
  return OK; 
} 
 
unsigned int 
sspGetFirmwareVersion(int id) 
{ 
  unsigned int rval=0; 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->Cfg.FirmwareRev) & SSP_CFG_FIRMWAREREV_MASK; 
  SSPUNLOCK(); 
 
  return rval; 
} 

/***************************************
     GT Routines
***************************************/
int
sspGtSendErrors(int id)
{
  char host[100];
  char name[100];
  unsigned int val;
  int i, data = 0;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  gethostname(host,sizeof(host));
  for(i=0; i<strlen(host); i++)
  {
    if(host[i] == '.')
    {
      host[i] = '\0';
      break;
    }
  }

  SSPLOCK();
  for(i=0;i<8;i++)
  {
    if(sspReadReg(&pSSP[id]->Ser[i].Status) & 0x1)
      val |= (1<<i);
    else
      val &= ~(1<<i);
  }
  SSPUNLOCK();

  if(id>=3 && id<=8) // Slot 3-8: SSP Sector
  {
    int sector = id-2;
    if(!(val & 0x1))  // Fiber 0 = adcecalXvtp
    {
      sprintf(name, "err: crate=%s,SSP_SLOT%d link to adcecal%dvtp down", host, id, sector);
      epics_json_msg_send(name, "int", 1, &data);
    }
    if(!(val & 0x2))  // Fiber 1 = adcpcalXvtp
    {
      sprintf(name, "err: crate=%s,SSP_SLOT%d link to adcpcal%dvtp down", host, id, sector);
      epics_json_msg_send(name, "int", 1, &data);
    }
    if(!(val & 0x4))  // Fiber 2 = adcftofXvtp
    {
      sprintf(name, "err: crate=%s,SSP_SLOT%d link to adcftof%dvtp down", host, id, sector);
      epics_json_msg_send(name, "int", 1, &data);
    }
    if(!(val & 0x8))  // Fiber 3 = trig2,SSP_SLOT10
    {
      sprintf(name, "err: crate=%s,SSP_SLOT%d link to SSP_SLOT10 down", host, id);
      epics_json_msg_send(name, "int", 1, &data);
    }
    if(!(val & 0x80))  // Fiber 7 = dcX3vtp
    {
      sprintf(name, "err: crate=%s,SSP_SLOT%d link to dc%d3vtp down", host, id, sector);
      epics_json_msg_send(name, "int", 1, &data);
    }
  }
  
  return OK;
}

int
sspGtSendScalers(int id)
{
  char name[100];
  float ref, data[8];
  unsigned int val;
  int i;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1); 

  // Read/normalize reference 
  val = sspReadReg(&pSSP[id]->Sd.Scalers[SD_SCALER_SYSCLK]); 
  if(!val) val = 1;
  ref = 100000.0f / (float)val;

  // EC cluster
  data[0] = ref * ((float)sspReadReg(&pSSP[id]->gt.ssec.Scaler_cluster));
  sprintf(name, "SSPGT_SLOT%d_ECCLUSTER_RATE", id);
  epics_json_msg_send(name, "float", 1, data);

  // EC pixel
  data[0] = ref * ((float)sspReadReg(&pSSP[id]->gt.ssec.Scaler_inner_cosmic));
  data[1] = ref * ((float)sspReadReg(&pSSP[id]->gt.ssec.Scaler_outer_cosmic));
  sprintf(name, "SSPGT_SLOT%d_ECPIXEL_RATE", id);
  epics_json_msg_send(name, "float", 2, data);

  // PC cluster
  data[0] = ref * ((float)sspReadReg(&pSSP[id]->gt.sspc.Scaler_cluster));
  sprintf(name, "SSPGT_SLOT%d_PCCLUSTER_RATE", id);
  epics_json_msg_send(name, "float", 1, data);

  // PC pixel
  data[0] = ref * ((float)sspReadReg(&pSSP[id]->gt.sspc.Scaler_cosmic));
  sprintf(name, "SSPGT_SLOT%d_PCPIXEL_RATE", id);
  epics_json_msg_send(name, "float", 1, data);

  // PCU 
  data[0] = ref * ((float)sspReadReg(&pSSP[id]->gt.sspc.Scaler_pcu));
  sprintf(name, "SSPGT_SLOT%d_PCU_RATE", id);
  epics_json_msg_send(name, "float", 1, data);

  // GT trigger bit scalers
  for(i=0; i<8; i++)
    data[i] = ref * ((float)sspReadReg(&pSSP[id]->gt.strigger[i].Scaler_trigger));

  sprintf(name, "SSPGT_Slot%d_TriggerBits", id);
  epics_json_msg_send(name, "float", 8, data);

  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0); 
  SSPUNLOCK(); 

  return OK;
}


int
sspGt_HtccDelayScan(int delay_min, int delay_max, int idle)
{ 
  int i, id, delay, val, vals[8];
  float ref, fval;
  for(id=3; id<=8; id++)
  {
    if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
      return ERROR;
  }
  
  for(delay = delay_min; delay <= delay_max; delay+=4)
  {
    for(id=3; id<=8; id++)
      sspGt_SetHtcc_Delay(id, delay);

    system("tcpClient trig1 tsSyncReset");

    // Reset Scalers
    for(id=3; id<=8; id++)
    {
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();
    }

    sleep(idle);

    system("caget IPM2C21A");
    printf("Delay = %4dns, STRIGGER(sec1-6): ", delay);

    // Read Scalers
    for(id=3; id<=8; id++)
    {
      printf("slot=%d ", id);
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);

      val = sspReadReg(&pSSP[id]->Sd.Scalers[SD_SCALER_SYSCLK]);
      if(!val)
        ref = 1.0;
      else
        ref = ((float)val) / 100000.0;

      for(i=0;i<8;i++)
        vals[i] = sspReadReg(&pSSP[id]->gt.strigger[i].Scaler_trigger);

      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();

      for(i=0;i<8;i++)
      {
        fval = ((float)vals[i]) / ref;
        printf(" %9.3f", fval);
      }
      printf("\n");
    }
  }
}


int
sspGt_FtofPcuDelayScan(int delay_min, int delay_max, int idle)
{ 
  int i, id, delay, val, vals;
  float ref, fval;
  for(id=3; id<=8; id++)
  {
    if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
      return ERROR;
  }
  
  for(delay = delay_min; delay <= delay_max; delay+=4)
  {
    for(id=3; id<=8; id++)
      sspGt_SetFtof_Delay(id, delay);

    //system("tcpClient trig1 tsSyncReset");

    // Reset Scalers
    for(id=3; id<=8; id++)
    {
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();
    }

    sleep(idle);

    system("caget IPM2C21A");
    printf("Delay = %4dns, STRIGGER(sec1-6): ", delay);

    // Read Scalers
    for(id=3; id<=8; id++)
    {
      printf("slot=%d ", id);
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);

      val = sspReadReg(&pSSP[id]->Sd.Scalers[SD_SCALER_SYSCLK]);
      if(!val)
        ref = 1.0;
      else
        ref = ((float)val) / 100000.0;

       vals = sspReadReg(&pSSP[id]->gt.sspcuftof.Scaler);

      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();

      fval = ((float)vals) / ref;
      printf(" %9.3f", fval);

      printf("\n");
    }
  }
}



/* new scan */
int
sspGt_FtofFtDelayScan(int delay_min, int delay_max, int idle)
{ 
  int id, delay, val;
  float ref, fval;
  for(id=3; id<=8; id++)
  {
    if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
      return ERROR;
  }

  for(delay = delay_min; delay <= delay_max; delay+=4)
  {
    for(id=3; id<=8; id++) sspGt_SetFtof_Delay(id, delay);
    system("tcpClient trig1 tsSyncReset");
    sleep(idle);
    system("caget IPM2C21A");
    printf("Delay = %4dns, ", delay);
    system("tcpClient trig2vtp vtpGtPrintScalers | grep Trigger0");
  }
}

int
sspGt_CtofFtDelayScan(int delay_min, int delay_max, int idle)
{ 
  int id, delay, val;
  float ref, fval;
  for(id=3; id<=8; id++)
  {
    if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
      return ERROR;
  }

  for(delay = delay_min; delay <= delay_max; delay+=4)
  {
    for(id=3; id<=8; id++) sspGt_SetCtof_Delay(id, delay);
    system("tcpClient trig1 tsSyncReset");
    sleep(idle);
    system("caget IPM2C21A");
    printf("Delay = %4dns, ", delay);
    system("tcpClient trig2vtp vtpGtPrintScalers | grep Trigger0");
  }
}

int
sspGt_CndFtDelayScan(int delay_min, int delay_max, int idle)
{ 
  int id, delay, val;
  float ref, fval;
  for(id=3; id<=8; id++)
  {
    if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
      return ERROR;
  }

  for(delay = delay_min; delay <= delay_max; delay+=4)
  {
    for(id=3; id<=8; id++) sspGt_SetCnd_Delay(id, delay);
    system("tcpClient trig1 tsSyncReset");
    sleep(idle);
    system("caget IPM2C21A");
    printf("Delay = %4dns, ", delay);
    system("tcpClient trig2vtp vtpGtPrintScalers | grep Trigger0");
  }
}
/* new scan */





int
sspGt_EcPcDelayScan(int delay_min, int delay_max, int idle)
{ 
  int id, delay, val;
  float ref, fval;
  for(id=3; id<=8; id++)
  {
    if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
      return ERROR;
  }
  
  for(delay = delay_min; delay <= delay_max; delay+=32)
  {
    for(id=3; id<=8; id++)
    {
      sspGt_SetPcal_ClusterDelay(id, delay);
      sspGt_SetEcal_ClusterDelay(id, delay);
      sspGt_SetHtcc_Delay(id, delay+156);
    }

    system("tcpClient trig1 tsSyncReset");

    // Reset Scalers
    for(id=3; id<=8; id++)
    {
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();
    }

    sleep(idle);

    system("caget IPM2C21A");
    printf("Delay = %4dns, STRIGGER3(sec1-6): ", delay);

    // Read Scalers
    for(id=3; id<=8; id++)
    {
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);

      val = sspReadReg(&pSSP[id]->Sd.Scalers[SD_SCALER_SYSCLK]);
      if(!val)
        ref = 1.0;
      else
        ref = ((float)val) / 100000.0;

      val = sspReadReg(&pSSP[id]->gt.strigger[3].Scaler_trigger);

      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();

      fval = ((float)val) / ref;
      printf(" %9.3f", fval);
    }
  }
}

int
sspGt_DcDelayScan(int delay_min, int delay_max, int idle)
{ 
  int id, delay, val;
  float ref, fval;
  for(id=3; id<=8; id++)
  {
    if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
      return ERROR;
  }
  
  for(delay = delay_min; delay <= delay_max; delay+=32)
  {
    for(id=3; id<=8; id++)
      sspGt_SetDc_SegDelay(id, delay);

    system("tcpClient trig1 tsSyncReset");

    // Reset Scalers
    for(id=3; id<=8; id++)
    {
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();
    }

    sleep(idle);

    system("caget IPM2C21A");
    printf("Delay = %4dns, STRIGGER3(sec1-6): ", delay);

    // Read Scalers
    for(id=3; id<=8; id++)
    {
      SSPLOCK();
      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1);

      val = sspReadReg(&pSSP[id]->Sd.Scalers[SD_SCALER_SYSCLK]);
      if(!val)
        ref = 1.0;
      else
        ref = ((float)val) / 100000.0;

      val = sspReadReg(&pSSP[id]->gt.strigger[3].Scaler_trigger);

      sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0);
      SSPUNLOCK();

      fval = ((float)val) / ref;
      printf(" %9.3f", fval);
    }
  }
}




/*Andrea start*/
int
sspGt_FtDelayScan(int delay_min, int delay_max, int idle)
{ 
  int id, delay, val;
  float ref, fval;
  id=9;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;
  
  
  for(delay = delay_min; delay <= delay_max; delay+=4)
  {
    id=9;
    sspGtc_SetFt_ClusterDelay(id,delay); /*9 IS SLOT ID FOR ssp central*/
    //   system("tcpClient trig1 tsSyncReset");

 
    sleep(idle);

    system("caget IPM2C21A");
    printf("Delay = %4dns, ", delay);
    system("tcpClient trig2vtp vtpGtPrintScalers | grep -B1 Trigger30");
  
  }
}

/*Andrea end*/











int sspGt_SetLatency(int id, int latency)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (latency < 0) || (latency > 4*2047) )
  {
    printf("%s: ERROR: latency is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  latency/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.gtpif.Latency, latency);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetLatency(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.gtpif.Latency);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGt_SetEcal_EsumDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  delay/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.ssec.Delay_esum, delay);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetEcal_EsumDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.ssec.Delay_esum);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGt_SetEcal_ClusterDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  delay/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.ssec.Delay_cluster, delay);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetEcal_ClusterDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.ssec.Delay_cluster);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGt_SetEcal_CosmicDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.ssec.Delay_cosmic, delay);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetEcal_CosmicDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->gt.ssec.Delay_cosmic);
  SSPUNLOCK();

  return val*4;
}

int sspGt_SetEcal_EsumIntegrationWidth(int id, int width)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (width < 0) || (width > 4*63) )
  {
    printf("%s: ERROR: width is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  width/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.ssec.WidthInt_esum, width);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetEcal_EsumIntegrationWidth(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.ssec.WidthInt_esum);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGt_SetPcal_EsumDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.sspc.Delay_esum, delay);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetPcal_EsumDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.sspc.Delay_esum);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGt_SetPcal_ClusterDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.sspc.Delay_cluster, delay);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetPcal_ClusterDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.sspc.Delay_cluster);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGt_SetPcal_CosmicDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  
  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.sspc.Delay_cosmic, delay);
  SSPUNLOCK();

  return OK;
} 
  
int sspGt_GetPcal_CosmicDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->gt.sspc.Delay_cosmic);
  SSPUNLOCK();

  return val*4;
}

int sspGt_SetPcal_PcuDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  
  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.sspc.Delay_pcu, delay);
  SSPUNLOCK();

  return OK;
} 
  
int sspGt_GetPcal_PcuDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->gt.sspc.Delay_pcu);
  SSPUNLOCK();

  return val*4;
}

int sspGt_SetPcal_EsumIntegrationWidth(int id, int width)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (width < 0) || (width > 4*63) )
  {
    printf("%s: ERROR: width is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  width/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.sspc.WidthInt_esum, width);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetPcal_EsumIntegrationWidth(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.sspc.WidthInt_esum);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGt_SetDc_SegDelay(int id, int delay)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4*1023) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.ssdc.Delay_seg, delay);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetDc_SegDelay(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->gt.ssdc.Delay_seg);
  SSPUNLOCK();

  return val*4;
}




int sspGt_SetPcuFtof_FtofWidth(int id, int width)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (width < 0) || (width > 4*63) )
  {
    printf("%s: ERROR: width is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  width/=4;
  SSPLOCK();
  val = sspReadReg(&pSSP[id]->gt.sspcuftof.Ctrl);
  val = (val & 0xFFFFFF00) | width;
  sspWriteReg(&pSSP[id]->gt.sspcuftof.Ctrl, val);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetPcuFtof_FtofWidth(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->gt.sspcuftof.Ctrl)>>0) & 0xFF;
  SSPUNLOCK();

  return val*4;
}




int sspGt_SetPcuFtof_PcuWidth(int id, int width)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (width < 0) || (width > 4*63) )
  {
    printf("%s: ERROR: width is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  width/=4;
  SSPLOCK();
  val = sspReadReg(&pSSP[id]->gt.sspcuftof.Ctrl);
  val = (val & 0xFFFF00FF) | (width<<8);
  sspWriteReg(&pSSP[id]->gt.sspcuftof.Ctrl, val);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetPcuFtof_PcuWidth(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->gt.sspcuftof.Ctrl)>>8) & 0xFF;
  SSPUNLOCK();

  return val*4;
}



int sspGt_SetPcuFtof_MatchTable(int id, int table)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (table < 0) || (table > 1) )
  {
    printf("%s: ERROR: table is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->gt.sspcuftof.Ctrl);
  val = (val & 0xFEFFFFFF) | (table<<24);
  sspWriteReg(&pSSP[id]->gt.sspcuftof.Ctrl, val);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetPcuFtof_MatchTable(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->gt.sspcuftof.Ctrl)>>24) & 0x1;
  SSPUNLOCK();

  return val;
}



int sspGt_SetTrigger_Enable(int id, int trg, int en_mask)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.strigger[trg].Ctrl, en_mask);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_Enable(int id, int trg)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gt.strigger[trg].Ctrl);
  SSPUNLOCK(); 
 
  return val;
}

int sspGt_SetTrigger_EcalEsumEmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 16383) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_esum);
  rval = val | (rval & ~GT_STRG_ECCTRL_ESUM_EMIN_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].ECCtrl_esum, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_EcalEsumEmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_esum) & GT_STRG_ECCTRL_ESUM_EMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_EcalEsumWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 255) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_esum);
  rval = (val<<16) | (rval & ~GT_STRG_ECCTRL_ESUM_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].ECCtrl_esum, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_EcalEsumWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_esum) & GT_STRG_ECCTRL_ESUM_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_CosmicWidth(int id, int trg, int val)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 1023) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  val/=4;
  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].Cosmic);
  rval = (val<<16) | (rval & ~GT_STRG_COSMIC_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].Cosmic, rval);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetTrigger_CosmicWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].Cosmic) & GT_STRG_COSMIC_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK();

  return rval*4;
}

int sspGt_SetTrigger_PcalEsumEmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 16383) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_esum);
  rval = val | (rval & ~GT_STRG_ECCTRL_ESUM_EMIN_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].PCCtrl_esum, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_PcalEsumEmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_esum) & GT_STRG_ECCTRL_ESUM_EMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_PcalEsumWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 255) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_esum);
  rval = (val<<16) | (rval & ~GT_STRG_ECCTRL_ESUM_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].PCCtrl_esum, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_PcalEsumWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_esum) & GT_STRG_ECCTRL_ESUM_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_EcalClusterEmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (val < 0) || (val > 65535) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_cluster);
  rval = val | (rval & ~GT_STRG_ECCTRL_CLUSTER_EMIN_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].ECCtrl_cluster, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_EcalClusterEmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_cluster) & GT_STRG_ECCTRL_CLUSTER_EMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_EcalClusterEmax(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (val < 0) || (val > 65535) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.strigger[trg].ECCtrl1, val);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_EcalClusterEmax(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl1) & 0xFFFF;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_EcalClusterWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 255) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_cluster);
  rval = (val<<16) | (rval & ~GT_STRG_ECCTRL_CLUSTER_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].ECCtrl_cluster, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_EcalClusterWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECCtrl_cluster) & GT_STRG_ECCTRL_CLUSTER_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_PcalClusterEmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 65535) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_cluster);
  rval = val | (rval & ~GT_STRG_PCCTRL_CLUSTER_EMIN_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].PCCtrl_cluster, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_PcalClusterEmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_cluster) & GT_STRG_PCCTRL_CLUSTER_EMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_PcalClusterEmax(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 65535) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.strigger[trg].PCCtrl1, val);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_PcalClusterEmax(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl1) & 0xFFFF;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_PcalClusterWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 255) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_cluster);
  rval = (val<<16) | (rval & ~GT_STRG_PCCTRL_CLUSTER_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].PCCtrl_cluster, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_PcalClusterWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].PCCtrl_cluster) & GT_STRG_PCCTRL_CLUSTER_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK(); 
 
  return rval*4;
}




int sspGt_SetTrigger_ECPCClusterEmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 65535) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECPCCtrl_cluster);
  rval = val | (rval & ~GT_STRG_ECPCCTRL_CLUSTER_EMIN_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].ECPCCtrl_cluster, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_ECPCClusterEmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECPCCtrl_cluster) & GT_STRG_ECPCCTRL_CLUSTER_EMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_ECPCClusterWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 255) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECPCCtrl_cluster);
  rval = (val<<16) | (rval & ~GT_STRG_ECPCCTRL_CLUSTER_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].ECPCCtrl_cluster, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGt_GetTrigger_ECPCClusterWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].ECPCCtrl_cluster) & GT_STRG_ECPCCTRL_CLUSTER_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK(); 
 
  return rval*4;
}








int sspGt_SetTrigger_DcMultMin(int id, int trg, int val)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 6) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].DCCtrl);
  rval = val | (rval & ~GT_STRG_DCCTRL_MULT_MIN_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].DCCtrl, rval);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetTrigger_DcMultMin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].DCCtrl) & GT_STRG_DCCTRL_MULT_MIN_MASK;
  SSPUNLOCK();

  return rval;
}

int sspGt_SetTrigger_DcWidth(int id, int trg, int val)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 511*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  val/=4;
  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].DCCtrl);
  rval = (val<<16) | (rval & ~GT_STRG_DCCTRL_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].DCCtrl, rval);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetTrigger_DcWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].DCCtrl) & GT_STRG_DCCTRL_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK();

  return rval*4;
}

int sspGt_SetTrigger_DcRoad(int id, int trg, int mask)
{
  int rval, mask_all;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  mask_all = GT_STRG_DCCTRL_ROAD_MASK |
             GT_STRG_DCCTRL_ROAD_OUTBEND_MASK |
             GT_STRG_DCCTRL_ROAD_INBEND_MASK;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].DCCtrl);
  rval = (rval & ~mask_all) | (mask & mask_all);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].DCCtrl, rval);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetTrigger_DcRoad(int id, int trg)
{
  int rval, mask_all;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  mask_all = GT_STRG_DCCTRL_ROAD_MASK |
             GT_STRG_DCCTRL_ROAD_OUTBEND_MASK |
             GT_STRG_DCCTRL_ROAD_INBEND_MASK;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].DCCtrl) & mask_all;
  SSPUNLOCK();

  return rval;
}

/*
static int sspGt_FtofPcalMatchTolerance[MAX_VME_SLOTS+1][8];

int sspGt_SetTrigger_FtofPcalMatchTolerance(int id, int trg, int val)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (val < 0) || (val > 61) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if(trg >= 4)
    printf("WARNING: FtofPcal matching not supported on trigger bits 4-7!!!\n");

  sspGt_FtofPcalMatchTolerance[id][trg] = val;

  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofPcalData0, );
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofPcalData1, );
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofPcalCtrl, );
  SSPUNLOCK();

  return OK;
}

int sspGt_GetTrigger_FtofPcalMatchTolerance(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (trg < 0) || (trg > 7) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  return sspGt_FtofPcalMatchTolerance[id][trg];
}
*/
int sspGt_SetHtcc_Delay(int id, int delay)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4095) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.sshtcc.Delay_htcc, delay);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetHtcc_Delay(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.sshtcc.Delay_htcc);
  SSPUNLOCK();

  return rval*4;
}

int sspGt_SetCtof_Delay(int id, int delay)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4095) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.ssctof.Delay_ctof, delay);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetCtof_Delay(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.ssctof.Delay_ctof);
  SSPUNLOCK();

  return rval*4;
}

int sspGt_SetCnd_Delay(int id, int delay)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4095) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.sscnd.Delay_cnd, delay);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetCnd_Delay(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.sscnd.Delay_cnd);
  SSPUNLOCK();

  return rval*4;
}

int sspGt_SetFtof_Delay(int id, int delay)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (delay < 0) || (delay > 4095) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gt.ssftof.Delay_ftof, delay);
  SSPUNLOCK();

  return OK;
}

int sspGt_GetFtof_Delay(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.ssftof.Delay_ftof);
  SSPUNLOCK();

  return rval*4;
}

int sspGt_SetTrigger_HtccWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (val < 0) || (val > 255*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].HtccCtrl);
  rval = (val<<16) | (rval & ~GT_STRG_HTCC_CTRL_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].HtccCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_HtccWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gt.strigger[trg].HtccCtrl) & GT_STRG_HTCC_CTRL_WIDTH_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_HtccMask(int id, int trg, long long mask)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.strigger[trg].HtccMask0, (mask>>0)&GT_STRG_HTCC_MASK0);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].HtccMask1, (mask>>32)&GT_STRG_HTCC_MASK1);
  SSPUNLOCK(); 
 
  return OK;
}

long long sspGt_GetTrigger_HtccMask(int id, int trg)
{
  long long rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].HtccMask0);
  rval |= ((long long)sspReadReg(&pSSP[id]->gt.strigger[trg].HtccMask1))<<32;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_FtofWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (val < 0) || (val > 255*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].FtofCtrl);
  rval = (val<<16) | (rval & ~GT_STRG_FTOF_CTRL_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_FtofWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gt.strigger[trg].FtofCtrl) & GT_STRG_FTOF_CTRL_WIDTH_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_FtofMask(int id, int trg, long long mask)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofMask0, (mask>>0)&GT_STRG_FTOF_MASK0);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofMask1, (mask>>32)&GT_STRG_FTOF_MASK1);
  SSPUNLOCK(); 
 
  return OK;
}

long long sspGt_GetTrigger_FtofMask(int id, int trg)
{
  long long rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].FtofMask0);
  rval |= ((long long)sspReadReg(&pSSP[id]->gt.strigger[trg].FtofMask1))<<32;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_CtofWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (val < 0) || (val > 255*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].CtofCtrl);
  rval = (val<<16) | (rval & ~GT_STRG_CTOF_CTRL_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].CtofCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_CtofWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gt.strigger[trg].CtofCtrl) & GT_STRG_CTOF_CTRL_WIDTH_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_CtofMask(int id, int trg, int mask)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].CtofCtrl);
  rval = (mask & GT_STRG_CTOF_CTRL_HIT_MASK) | (rval & ~GT_STRG_CTOF_CTRL_HIT_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].CtofCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_CtofMask(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].CtofCtrl) & GT_STRG_CTOF_CTRL_HIT_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_CndWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (val < 0) || (val > 255*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].CndCtrl);
  rval = (val<<16) | (rval & ~GT_STRG_CND_CTRL_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].CndCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_CndWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gt.strigger[trg].CndCtrl) & GT_STRG_CND_CTRL_WIDTH_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_CndMask(int id, int trg, int mask)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].CndCtrl);
  rval = (mask & GT_STRG_CND_CTRL_HIT_MASK) | (rval & ~GT_STRG_CND_CTRL_HIT_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].CndCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_CndMask(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].CndCtrl) & GT_STRG_CND_CTRL_HIT_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGt_SetTrigger_FtofPcuWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  if( (val < 0) || (val > 255*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].FtofPcuCtrl);
  rval = (val<<0) | (rval & ~GT_STRG_FTOFPCU_CTRL_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofPcuCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_FtofPcuWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gt.strigger[trg].FtofPcuCtrl) & GT_STRG_FTOFPCU_CTRL_WIDTH_MASK)>>0;
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGt_SetTrigger_FtofPcuMatchMask(int id, int trg, int mask)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gt.strigger[trg].FtofPcuCtrl);
  rval = (mask<<16) | (rval & ~GT_STRG_FTOFPCU_CTRL_MATCH_MASK);
  sspWriteReg(&pSSP[id]->gt.strigger[trg].FtofPcuCtrl, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGt_GetTrigger_FtofPcuMatchMask(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gt.strigger[trg].FtofPcuCtrl) & GT_STRG_FTOFPCU_CTRL_MATCH_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval;
}

/***************************************
     GTC Routines
***************************************/
int
sspGtcSendErrors(int id)
{
  char host[100];
  char name[100];
  unsigned int val;
  int i, data = 0;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return ERROR;

  gethostname(host,sizeof(host));
  for(i=0; i<strlen(host); i++)
  {
    if(host[i] == '.')
    {
      host[i] = '\0';
      break;
    }
  }

  SSPLOCK();
  for(i=0;i<8;i++)
  {
    if(sspReadReg(&pSSP[id]->Ser[i].Status) & 0x1)
      val |= (1<<i);
    else
      val &= ~(1<<i);
  }
  SSPUNLOCK();

  if(id == 9) // Slot 9: SSP Central - FT
  {
    if(!(val & 0x1))  // Fiber 0 = adcft1vtp
    {
      sprintf(name, "err: crate=%s,SSP_SLOT%d link to adcft1vtp down", host, id);
      epics_json_msg_send(name, "int", 1, &data);
    }
    if(!(val & 0x2))  // Fiber 1 = adcft2vtp
    {
      sprintf(name, "err: crate=%s,SSP_SLOT%d link to adcft2vtp down", host, id);
      epics_json_msg_send(name, "int", 1, &data);
    }
  }
  
  return OK;
}

int
sspGtcSendScalers(int id)
{
  char name[100];
  float ref, data[4];
  unsigned int val;
  int i;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1); 

  //Read/normalize reference 
  val = sspReadReg(&pSSP[id]->Sd.Scalers[SD_SCALER_SYSCLK]); 
  if(!val) val = 1;
  ref = 100000.0f / (float)val;


  if(id == 9) // FT is in GTC slot 9 only
  {
    // FT scalers
    for(i=0; i<2; i++)
      data[i] = ref * ((float)sspReadReg(&pSSP[id]->gtc.ssft[i].Scaler_cluster));

    sprintf(name, "SSPGTC_FTCLUSTER_RATE", id);
    epics_json_msg_send(name, "float", 2, data);

    // GTC trigger bit scalers
    for(i=0; i<4; i++)
      data[i] = ref * ((float)sspReadReg(&pSSP[id]->gtc.ctrigger[i].Scaler_trigger));

    sprintf(name, "SSPGTC_TRIGGERBIT_RATE", id);
    epics_json_msg_send(name, "float", 4, data);
  }

  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0); 
  SSPUNLOCK(); 

  return OK;
}

int sspGtc_SetLatency(int id, int latency)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (latency < 0) || (latency > 4*2047) )
  {
    printf("%s: ERROR: latency is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  latency/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gtc.gtpif.Latency, latency);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetLatency(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gtc.gtpif.Latency);
  SSPUNLOCK();

  return rval*4;
}

int sspGtc_SetFt_EsumDelay(int id, int delay)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (delay < 0) || (delay > 4095) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gtc.ssft[0].Delay_esum, delay);
  sspWriteReg(&pSSP[id]->gtc.ssft[1].Delay_esum, delay);
  SSPUNLOCK();

  return OK;
}

int sspGtc_GetFt_EsumDelay(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gtc.ssft[0].Delay_esum);
//  rval = sspReadReg(&pSSP[id]->gtc.ssft[1].Delay_esum);
  SSPUNLOCK();

  return rval*4;
}

int sspGtc_SetFt_ClusterDelay(int id, int delay)
{
  int rval;

  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (delay < 0) || (delay > 4095) )
  {
    printf("%s: ERROR: delay is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  delay/=4;
  SSPLOCK();
  sspWriteReg(&pSSP[id]->gtc.ssft[0].Delay_cluster, delay);
  sspWriteReg(&pSSP[id]->gtc.ssft[1].Delay_cluster, delay);
  SSPUNLOCK();

  return OK;
}

int sspGtc_GetFt_ClusterDelay(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK();
  rval = sspReadReg(&pSSP[id]->gtc.ssft[0].Delay_cluster);
//  rval = sspReadReg(&pSSP[id]->gtc.ssft[1].Delay_cluster);
  SSPUNLOCK();

  return rval*4;
}

int sspGtc_SetFt_EsumIntegrationWidth(int id, int width)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (width < 0) || (width > 4*63) )
  {
    printf("%s: ERROR: width is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  width/=4;
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gtc.ssft[0].WidthInt_esum, width);
  sspWriteReg(&pSSP[id]->gtc.ssft[1].WidthInt_esum, width);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetFt_EsumIntegrationWidth(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gtc.ssft[0].WidthInt_esum);
//  val = sspReadReg(&pSSP[id]->gtc.ssft[1].WidthInt_esum);
  SSPUNLOCK(); 
 
  return val*4;
}

int sspGtc_SetTrigger_Enable(int id, int trg, int en_mask)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (trg < 0) || (trg > 3) )
  {
    printf("%s: ERROR: trg is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].Ctrl, en_mask);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_Enable(int id, int trg)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].Ctrl);
  SSPUNLOCK(); 
 
  return val;
}

int sspGtc_SetTrigger_FtEsumEmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 16383) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_esum);
  rval = val | (rval & ~GTC_CTRG_FT_ESUM_CTRL_EMIN_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_esum, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtEsumEmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_esum) & GTC_CTRG_FT_ESUM_CTRL_EMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGtc_SetTrigger_FtEsumWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 255) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_esum);
  rval = (val<<16) | (rval & ~GTC_CTRG_FT_ESUM_CTRL_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_esum, rval);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspGtc_GetTrigger_FtEsumWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_esum) & GTC_CTRG_FT_ESUM_CTRL_WIDTH_MASK;
  rval = (rval>>16);
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGtc_SetTrigger_FtClusterEmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 16383) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster0);
  rval = val | (rval & ~GTC_CTRG_FT_CLUSTER_CTRL0_EMIN_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster0, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtClusterEmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster0) & GTC_CTRG_FT_CLUSTER_CTRL0_EMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGtc_SetTrigger_FtClusterEmax(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 16383) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster0);
  rval = (val<<16) | (rval & ~GTC_CTRG_FT_CLUSTER_CTRL0_EMAX_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster0, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtClusterEmax(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster0) & GTC_CTRG_FT_CLUSTER_CTRL0_EMAX_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGtc_SetTrigger_FtClusterHodoNmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 2) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1);
  rval = val | (rval & ~GTC_CTRG_FT_CLUSTER_CTRL1_HODO_NMIN_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtClusterHodoNmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1) & GTC_CTRG_FT_CLUSTER_CTRL1_HODO_NMIN_MASK;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGtc_SetTrigger_FtClusterNmin(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 9) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1);
  rval = (val<<8) | (rval & ~GTC_CTRG_FT_CLUSTER_CTRL1_NMIN_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtClusterNmin(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1) & GTC_CTRG_FT_CLUSTER_CTRL1_NMIN_MASK)>>8;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGtc_SetTrigger_FtClusterWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 255*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1);
  rval = (val<<16) | (rval & ~GTC_CTRG_FT_CLUSTER_CTRL1_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtClusterWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_Cluster1) & GTC_CTRG_FT_CLUSTER_CTRL1_WIDTH_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGtc_SetTrigger_FtClusterMultWidth(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 63*4) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  val/=4;
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_ClusterMult);
  rval = (val<<16) | (rval & ~GTC_CTRG_FT_CLUSTER_MULT_WIDTH_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_ClusterMult, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtClusterMultWidth(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_ClusterMult) & GTC_CTRG_FT_CLUSTER_MULT_WIDTH_MASK)>>16;
  SSPUNLOCK(); 
 
  return rval*4;
}

int sspGtc_SetTrigger_FtClusterMult(int id, int trg, int val)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  if( (val < 0) || (val > 15) )
  {
    printf("%s: ERROR: val is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_ClusterMult);
  rval = (val<<0) | (rval & ~GTC_CTRG_FT_CLUSTER_MULT_MASK);
  sspWriteReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_ClusterMult, rval);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetTrigger_FtClusterMult(int id, int trg)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = (sspReadReg(&pSSP[id]->gtc.ctrigger[trg].FtCtrl_ClusterMult) & GTC_CTRG_FT_CLUSTER_MULT_MASK)>>0;
  SSPUNLOCK(); 
 
  return rval;
}

int sspGtc_SetFanout_EnableMask(int id, int en_mask)
{
  int rval;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->gtc.ctrigfanout.Ctrl, en_mask);
  SSPUNLOCK(); 
 
  return OK;
}

int sspGtc_GetFanout_EnableMask(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return ERROR;

  SSPLOCK(); 
  rval = sspReadReg(&pSSP[id]->gtc.ctrigfanout.Ctrl);
  SSPUNLOCK(); 
 
  return rval;
}

/* HPS Routines */

/***************************************
           set parameters
***************************************/

/* sspHps_SetLatency() - set trigger latency in 4ns ticks */ 
int sspHps_SetLatency(int id, int latency)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (latency < 0) || (latency > 1023) )
  {
    printf("%s: ERROR: latency is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Trigger.Latency, latency);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetSinglesEmin() - set calorimeter single cluster trigger parameters
     n    - singles trigger 0 or 1
     emin - minimum cluster energy (units: MeV)
*/
int sspHps_SetSinglesEmin(int id, int n, int emin)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (emin < 0) || (emin > 8191) )
  {
    printf("%s: ERROR: emin is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsSingles[n].ClusterEmin, emin);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetSinglesEmax() - set calorimeter single cluster trigger parameters
     n    - singles trigger 0 or 1
     emax - minimum cluster energy (units: MeV)
*/
int sspHps_SetSinglesEmax(int id, int n, int emax)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (emax < 0) || (emax > 8191) )
  {
    printf("%s: ERROR: emax is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsSingles[n].ClusterEmax, emax);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetSinglesNHitsmin() - set calorimeter single cluster trigger parameters
     n    - singles trigger 0 or 1
     nmin - minimum cluster hits
*/
int sspHps_SetSinglesNHitsmin(int id, int n, int nmin)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (nmin < 0) || (nmin > 15) )
  {
    printf("%s: ERROR: nmin is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsSingles[n].ClusterNHitsmin, nmin);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetSinglesEnableEmin() - set calorimeter single cluster trigger parameters
     n    - singles trigger 0 or 1
     en   - '0' disables , '1' enables trigger
*/
int sspHps_SetSinglesEnableEmin(int id, int n, int en)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsSingles[n].Ctrl);
  if(en)
    val |= 0x00000001;
  else
    val &= 0xFFFFFFFE;
  sspWriteReg(&pSSP[id]->hps.HpsSingles[n].Ctrl, val);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetSinglesEnableEmax() - set calorimeter single cluster trigger parameters
     n    - singles trigger 0 or 1
     en   - '0' disables , '1' enables trigger
*/
int sspHps_SetSinglesEnableEmax(int id, int n, int en)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsSingles[n].Ctrl);
  if(en)
    val |= 0x00000002;
  else
    val &= 0xFFFFFFFD;
  sspWriteReg(&pSSP[id]->hps.HpsSingles[n].Ctrl, val);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetSinglesEnableNmin() - set calorimeter single cluster trigger parameters
     n    - singles trigger 0 or 1
     en   - '0' disables , '1' enables trigger
*/
int sspHps_SetSinglesEnableNmin(int id, int n, int en)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsSingles[n].Ctrl);
  if(en)
    val |= 0x00000004;
  else
    val &= 0xFFFFFFFB;
  sspWriteReg(&pSSP[id]->hps.HpsSingles[n].Ctrl, val);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetSinglePrescale() - set singles regional prescaler
     n        - singles trigger 0 or 1
     region   - region number: 0 to 6
     xmin     - region definition minimum cluster X: -31 to 31
     xmax     - region definition maximum cluster X: -31 to 31
     prescale - region prescale value: 0 to 65535
                0 - no prescaling (accepts everything)
                1 - skip 1, accept 1
                2 - skip 2, accept 1
                ...
     Note: All region decisions are OR'd together.
           All regions (xmin,xmax) should be defined mutually exclusive
*/
int sspHps_SetSinglePrescale(int id, int n, int region, int xmin, int xmax, int prescale)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (region < 0) || (region > 6) )
  {
    printf("%s: ERROR: region is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (xmin < -31) || (xmin > 31) )
  {
    printf("%s: ERROR: xmin is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (xmax < -31) || (xmax > 31) )
  {
    printf("%s: ERROR: xmax is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (prescale < 0) || (prescale > 65535) )
  {
    printf("%s: ERROR: xmax is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }


  SSPLOCK();
  vmeWrite32(&pSSP[id]->hps.HpsSingles[n].Prescale[region],
				 ((prescale & 0xFFFF)<<0) | ((xmin & 0x3F)<<16) | ((xmax & 0x3F)<<24)
    );
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetCosmicTimeCoincidence() - set cosmic scintillator coincidence time
     ticks - coincidence time (units: +/-4ns)
*/
int sspHps_SetCosmicTimeCoincidence(int id, int ticks)
{
  int ctrl = 0;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (ticks < 0) || (ticks > 255) )
  {
    printf("%s: ERROR: ticks is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsCosmic.TimeCoincidence, ticks);
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_SetCosmicCoincidencePattern() - set cosmic scintillator coincidence pattern
     pattern  - 3:1 LUT definition for scintillator coincidence pattern that is accepted/rejected
              Scintillator channels are the last 3 channels (14-16) of FADC in slot 20
              pattern = 0xFE will trigger on any hit channels
              pattern = 0x80 will trigger when all 3 channels hit
              pattern = 0x88 will trigger when channels 14&15 are hit
              pattern = 0xE8 will trigger when any 2 channels hit
*/
int sspHps_SetCosmicCoincidencePattern(int id, int pattern)
{
  int ctrl = 0;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (pattern < 0) || (pattern > 65535) )
  {
    printf("%s: ERROR: pattern is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsCosmic.TriggerPattern, pattern);
  SSPUNLOCK(); 
 
  return OK; 
}

int sspHps_SetPairsEnableSum(int id, int n, int en)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  if(en)
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) | 0x00000001;
  else
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0xFFFFFFFE;
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].Ctrl, val); 
  SSPUNLOCK(); 
 
  return OK;
}

int sspHps_SetPairsEnableDiff(int id, int n, int en)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  if(en)
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) | 0x00000002;
  else
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0xFFFFFFFD;
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].Ctrl, val); 
  SSPUNLOCK(); 
 
  return OK;
}

int sspHps_SetPairsEnableCoplanar(int id, int n, int en)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  if(en)
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) | 0x00000004;
  else
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0xFFFFFFFB;
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].Ctrl, val); 
  SSPUNLOCK(); 
 
  return OK;
}

int sspHps_SetPairsEnableED(int id, int n, int en)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  if(en)
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) | 0x00000008;
  else
    val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0xFFFFFFF7;
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].Ctrl, val); 
  SSPUNLOCK(); 
 
  return OK;
}

/* sspHps_PairsTimeCoincidence() - set cluster pair coincidence time window (units: +/-4ns) */
int sspHps_SetPairsTimeCoincidence(int id, int n, int ticks)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;
  
  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (ticks < 0) || (ticks > 15) )
  {
    printf("%s: ERROR: ticks is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterTimeCoincidence, ticks); 
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_PairsSummax() - set cluster pair sum maximum (units: MeV) */
int sspHps_SetPairsSummax(int id, int n, int max)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (max < 0) || (max > 8191) )
  {
    printf("%s: ERROR: ticks is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterSummax, max); 
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_PairsSummin() - set cluster pair sum minimum (units: MeV) */
int sspHps_SetPairsSummin(int id, int n, int min)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;
  
  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (min < 0) || (min > 8191) )
  {
    printf("%s: ERROR: ticks is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterSummin, min); 
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_PairsDiffmax() - set cluster pair difference maximum (units: MeV) */
int sspHps_SetPairsDiffmax(int id, int n, int max)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (max < 0) || (max > 8191) )
  {
    printf("%s: ERROR: ticks is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterDiffmax, max); 
  SSPUNLOCK(); 
 
  return OK;
}


/* sspHps_PairsEmin() - set cluster pair cluster energy minimum (units: MeV) */
int sspHps_SetPairsEmin(int id, int n, int min)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (min < 0) || (min > 8191) )
  {
    printf("%s: ERROR: emin is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterEmin, min); 
  SSPUNLOCK(); 
 
  return OK;
}

/* sspHps_PairsEmin() - set cluster pair cluster energy maximum (units: MeV) */
int sspHps_SetPairsEmax(int id, int n, int max)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (max < 0) || (max > 8191) )
  {
    printf("%s: ERROR: emax is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterEmax, max); 
  SSPUNLOCK(); 
 
  return OK;
}

/* sspHps_PairsNHitsmin() - set minimum cluster hits for calorimeter pair cluster trigger (units: counts) */
int sspHps_SetPairsNHitsmin(int id, int n, int min)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (min < 0) || (min > 9) )
  {
    printf("%s: ERROR: emin is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterNHitsmin, min); 
  SSPUNLOCK(); 
 
  return OK; 
}

/* sspHps_PairsCoplanarTolerance() - set cluster pair-beam coplanarity tolerance (units: degrees) */
int sspHps_SetPairsCoplanarTolerance(int id, int n, int tol)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (tol < 0) || (tol > 255) )
  {
    printf("%s: ERROR: tol is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterCoplanarTol, tol); 
  SSPUNLOCK(); 
 
  return OK;
}

/* sspHps_PairsEDFactor() - set cluster pair radial distance->energy factor (units: MeV/mm) */
int sspHps_SetPairsEDFactor(int id, int n, float f)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (f < 0.0) || (f > 511.0) )
  {
    printf("%s: ERROR: f is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  /* convert to fixed point 8.4 format */
  f = f * 16.0f;

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterEDFactor, (int)f); 
  SSPUNLOCK(); 
 
  return OK;
}

int sspHps_SetPairsEDmin(int id, int n, int min)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (min < 0) || (min > 8191) )
  {
    printf("%s: ERROR: min is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  sspWriteReg(&pSSP[id]->hps.HpsPairs[n].ClusterEDmin, min); 
  SSPUNLOCK(); 
 
  return OK;
}

/***************************************
           get parameters
***************************************/

int sspHps_GetLatency(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  SSPLOCK(); 
  val = sspReadReg(&pSSP[id]->Trigger.Latency);
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetSinglesEmin(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsSingles[n].ClusterEmin);
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetSinglesEmax(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }
  
  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsSingles[n].ClusterEmax);
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetSinglesNHitsmin(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsSingles[n].ClusterNHitsmin);
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetSinglesEnableEmin(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->hps.HpsSingles[n].Ctrl) & 0x00000001) >> 0;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetSinglesEnableEmax(int id, int n)
{
  int val;  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->hps.HpsSingles[n].Ctrl) & 0x00000002) >>1;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetSinglesEnableNmin(int id, int n)
{
  int val;  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->hps.HpsSingles[n].Ctrl) & 0x00000004) >> 2;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetSinglePrescaleXmin(int id, int n, int region)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;
  
  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (region < 0) || (region > 6) )
  {
    printf("%s: ERROR: region is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = vmeRead32(&pSSP[id]->hps.HpsSingles[n].Prescale[region]);
  SSPUNLOCK();
  
  val = (val>>16) & 0x3F;
  if(val & 0x20) val |= 0xFFFFFFC0;
  
  return val;
}

int sspHps_GetSinglePrescaleXmax(int id, int n, int region)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (region < 0) || (region > 6) )
  {
    printf("%s: ERROR: region is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = vmeRead32(&pSSP[id]->hps.HpsSingles[n].Prescale[region]);
  SSPUNLOCK();

  val = (val>>24) & 0x3F;
  if(val & 0x20) val |= 0xFFFFFFC0;
  
  return val;
}

int sspHps_GetSinglePrescalePrescale(int id, int n, int region)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  if( (region < 0) || (region > 6) )
  {
    printf("%s: ERROR: region is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = vmeRead32(&pSSP[id]->hps.HpsSingles[n].Prescale[region]);
  SSPUNLOCK();
  
  return val & 0xFFFF;
}

int sspHps_GetCosmicTimeCoincidence(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsCosmic.TimeCoincidence);
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetCosmicCoincidencePattern(int id)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsCosmic.TriggerPattern);
  SSPUNLOCK(); 
 
  return val; 
}

int sspHps_GetPairsEnableSum(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0x00000001) >> 0;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsEnableDiff(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0x00000002) >> 1;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsEnableCoplanar(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0x00000004) >> 2;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsEnableED(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (sspReadReg(&pSSP[id]->hps.HpsPairs[n].Ctrl) & 0x00000008) >> 3;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsTimeCoincidence(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterTimeCoincidence); 
  SSPUNLOCK(); 
 
  return val; 
}

int sspHps_GetPairsSummax(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterSummax);
  SSPUNLOCK(); 
 
  return val; 
}

int sspHps_GetPairsSummin(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterSummin);
  SSPUNLOCK(); 
 
  return val; 
}

int sspHps_GetPairsDiffmax(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterDiffmax);
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsEmin(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterEmin);
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsEmax(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterEmax); 
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsNHitsmin(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterNHitsmin);
  SSPUNLOCK(); 
 
  return val; 
}

int sspHps_GetPairsCoplanarTolerance(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterCoplanarTol); 
  SSPUNLOCK(); 
 
  return val;
}

float sspHps_GetPairsEDFactor(int id, int n)
{
  float val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = (float)sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterEDFactor) / 16.0f;
  SSPUNLOCK(); 
 
  return val;
}

int sspHps_GetPairsEDmin(int id, int n)
{
  int val;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return ERROR;

  if( (n < 0) || (n > 1) )
  {
    printf("%s: ERROR: n is outside acceptable range.\n",__FUNCTION__);
    return ERROR;
  }

  SSPLOCK();
  val = sspReadReg(&pSSP[id]->hps.HpsPairs[n].ClusterEDmin); 
  SSPUNLOCK(); 
 
  return val;
}

void sspPrintHpsScalers(int id) 
{
  double ref, rate; 
  int i; 
  unsigned int scalers[SD_SCALER_NUM];
  unsigned int hpsscalers[24];
  const char *hpsscalers_name[24] = {
    "PairTrig0.PairPass",
    "PairTrig0.SumPass",
    "PairTrig0.DiffPass",
    "PairTrig0.EDPass",
    "PairTrig0.CoplanarPass",
    "PairTrig0.TriggerPass",
    "PairTrig1.PairPass",
    "PairTrig1.SumPass",
    "PairTrig1.DiffPass",
    "PairTrig1.EDPass",
    "PairTrig1.CoplanarPass",
    "PairTrig1.TriggerPass",
    "SingleTrig0.SinglesPass",
    "SingleTrig0.SinglesTot",
    "SingleTrig1.SinglesPass",
    "SingleTrig1.SinglesTot",
    "Cosmic.Top0",
    "Cosmic.Top1",
    "Cosmic.Top2",
    "Cosmic.TopTrig",
    "Cosmic.Bot0",
    "Cosmic.Bot1",
    "Cosmic.Bot2",
    "Cosmic.BotTrig"
    };
 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return;
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0); 
 
  for(i = 0; i < SD_SCALER_NUM; i++) 
    scalers[i] = sspReadReg(&pSSP[id]->Sd.Scalers[i]); 

  hpsscalers[0] = sspReadReg(&pSSP[id]->hps.HpsPairs[0].ScalerPairsPass);
  hpsscalers[1] = sspReadReg(&pSSP[id]->hps.HpsPairs[0].ScalerSumPass);
  hpsscalers[2] = sspReadReg(&pSSP[id]->hps.HpsPairs[0].ScalerDiffPass);
  hpsscalers[3] = sspReadReg(&pSSP[id]->hps.HpsPairs[0].ScalerEDPass);
  hpsscalers[4] = sspReadReg(&pSSP[id]->hps.HpsPairs[0].ScalerCoplanarPass);
  hpsscalers[5] = sspReadReg(&pSSP[id]->hps.HpsPairs[0].ScalerTriggerPass);
  hpsscalers[6] = sspReadReg(&pSSP[id]->hps.HpsPairs[1].ScalerPairsPass);
  hpsscalers[7] = sspReadReg(&pSSP[id]->hps.HpsPairs[1].ScalerSumPass);
  hpsscalers[8] = sspReadReg(&pSSP[id]->hps.HpsPairs[1].ScalerDiffPass);
  hpsscalers[9] = sspReadReg(&pSSP[id]->hps.HpsPairs[1].ScalerEDPass);
  hpsscalers[10] = sspReadReg(&pSSP[id]->hps.HpsPairs[1].ScalerCoplanarPass);
  hpsscalers[11] = sspReadReg(&pSSP[id]->hps.HpsPairs[1].ScalerTriggerPass);
  hpsscalers[12] = sspReadReg(&pSSP[id]->hps.HpsSingles[0].ScalerSinglesPass);
  hpsscalers[13] = sspReadReg(&pSSP[id]->hps.HpsSingles[0].ScalerSinglesTot);
  hpsscalers[14] = sspReadReg(&pSSP[id]->hps.HpsSingles[1].ScalerSinglesPass);
  hpsscalers[15] = sspReadReg(&pSSP[id]->hps.HpsSingles[1].ScalerSinglesTot);
  hpsscalers[16] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerScintillatorTop[0]);
  hpsscalers[17] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerScintillatorTop[1]);
  hpsscalers[18] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerScintillatorTop[2]);
  hpsscalers[19] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerCosmicTop);
  hpsscalers[20] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerScintillatorBot[0]);
  hpsscalers[21] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerScintillatorBot[1]);
  hpsscalers[22] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerScintillatorBot[2]);
  hpsscalers[23] = sspReadReg(&pSSP[id]->hps.HpsCosmic.ScalerCosmicBot);

  SSPUNLOCK(); 
 
  printf("%s - \n", __FUNCTION__); 
  if(!scalers[SD_SCALER_SYSCLK]) 
  {
    printf("Error: sspPrintScalers() reference time is 0. Reported rates will not be normalized.\n"); 
    ref = 1.0; 
  } 
  else 
  { 
    ref = (double)scalers[SD_SCALER_SYSCLK] / (double)SYSCLK_FREQ; 
  } 
	 
  for(i = 0; i < SD_SCALER_NUM; i++) 
  { 
    rate = (double)scalers[i]; 
    rate = rate / ref; 
    if(scalers[i] == 0xFFFFFFFF) 
	   printf("   %-25s %10u,%.3fHz [OVERFLOW]\n", ssp_scaler_name[i], scalers[i], rate); 
    else 
	   printf("   %-25s %10u,%.3fHz\n", ssp_scaler_name[i], scalers[i], rate); 
  }

  for(i = 0; i < 24; i++) 
  { 
    rate = (double)hpsscalers[i]; 
    rate = rate / ref; 
    if(hpsscalers[i] == 0xFFFFFFFF) 
	   printf("   %-25s %10u,%.3fHz [OVERFLOW]\n", hpsscalers[i], hpsscalers[i], rate); 
    else 
	   printf("   %-25s %10u,%.3fHz\n", hpsscalers[i], hpsscalers[i], rate); 
  }
}

void sspPrintHpsConfig(int id)
{
  int i, j;
  int triggerLatency, singlesEmin[2], singlesEmax[2], singlesNmin[2],
      singlesEminEn[2], singlesEmaxEn[2], singlesNminEn[2],
      singlesPrescaleXmin[2][7], singlesPrescaleXmax[2][7], singlesPrescalePrescale[2][7], 
      cosmicTimeCoincidence, cosmicPatternCoincidence,
      pairsSumEn[2], pairsDiffEn[2], pairsCoplanarEn[2], pairsEDEn[2],
      pairsTimeCoincidence[2], pairsSummax[2],
      pairsSummin[2], pairsDiffmax[2], pairsEmin[2], pairsEmax[2],
      pairsNHitsmin[2], pairsCoplanarTolerance[2], pairsEDmin[2];
  float pairsEDFactor[2];
		
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HPS))
    return;

  printf("*** HpsConfig ***\n");
  
  triggerLatency = sspHps_GetLatency(id);
  
  for(i = 0; i < 2; i++)
  {
    singlesEmin[i] = sspHps_GetSinglesEmin(id, i);
    singlesEmax[i] = sspHps_GetSinglesEmax(id, i);
    singlesNmin[i] = sspHps_GetSinglesNHitsmin(id, i);
    singlesEminEn[i] = sspHps_GetSinglesEnableEmin(id, i);
    singlesEmaxEn[i] = sspHps_GetSinglesEnableEmax(id, i);
    singlesNminEn[i] = sspHps_GetSinglesEnableNmin(id, i);
  
    for(j = 0; j < 7; j++)
    {
      singlesPrescaleXmin[i][j] = sspHps_GetSinglePrescaleXmin(id, i, j);
      singlesPrescaleXmax[i][j] = sspHps_GetSinglePrescaleXmax(id, i, j);
      singlesPrescalePrescale[i][j] = sspHps_GetSinglePrescalePrescale(id, i, j);
    }
  }

  cosmicTimeCoincidence = sspHps_GetCosmicTimeCoincidence(id);
  cosmicPatternCoincidence = sspHps_GetCosmicCoincidencePattern(id);
  
  for(i = 0; i < 2; i++)
  {
  pairsSumEn[i] = sspHps_GetPairsEnableSum(id, i);
  pairsDiffEn[i] = sspHps_GetPairsEnableDiff(id, i);
  pairsCoplanarEn[i] = sspHps_GetPairsEnableCoplanar(id, i);
  pairsEDEn[i] = sspHps_GetPairsEnableED(id, i);
  pairsTimeCoincidence[i] = sspHps_GetPairsTimeCoincidence(id, i);
  pairsSummax[i] = sspHps_GetPairsSummax(id, i);
  pairsSummin[i] = sspHps_GetPairsSummin(id, i);
  pairsDiffmax[i] = sspHps_GetPairsDiffmax(id, i);
  pairsEmin[i] = sspHps_GetPairsEmin(id, i);
  pairsEmax[i] = sspHps_GetPairsEmax(id, i);
  pairsNHitsmin[i] = sspHps_GetPairsNHitsmin(id, i);
  pairsCoplanarTolerance[i] = sspHps_GetPairsCoplanarTolerance(id, i);
  pairsEDFactor[i] = sspHps_GetPairsEDFactor(id, i);
  pairsEDmin[i] = sspHps_GetPairsEDmin(id, i);
  }

  for(i = 0; i < 2; i++)
  {
  printf("   *** Singles %d Configuration ***\n", i);
  printf("     Emin = %dMeV, Enabled = %d\n", singlesEmin[i], singlesEminEn[i]);
  printf("     Emax = %dMeV, Enabled = %d\n", singlesEmax[i], singlesEmaxEn[i]);
  printf("     NHitsmin = %d, Enabled = %d\n", singlesNmin[i], singlesNminEn[i]);
  for(j = 0; j < 7; j++)
  printf("     Prescale region %d: xmin = %d, xmax = %d, prescale = %d\n", j,
			singlesPrescaleXmin[i][j], singlesPrescaleXmax[i][j], singlesPrescalePrescale[i][j]);  
  printf("\n");
  }
  
  for(i = 0; i < 2; i++)
  {
  printf("   *** Pairs %d Configuration ***\n", i);
  printf("     Emin = %dMeV\n", pairsEmin[i]);
  printf("     Emax = %dMeV\n", pairsEmax[i]);
  printf("     NHitsmin = %d\n", pairsNHitsmin[i]);
  printf("     TimeCoincidence = %d(+/-%dns)\n", pairsTimeCoincidence[i], pairsTimeCoincidence[i]*4);
  printf("     SumMax = %dMeV, SumMin = %dMeV, Enabled = %d\n", pairsSummax[i], pairsSummin[i], pairsSumEn[i]);
  printf("     DiffMax = %dMeV, Enabled = %d\n", pairsDiffmax[i], pairsDiffEn[i]);
  printf("     Coplanarity = +/-%ddegrees, Enabled = %d\n", pairsCoplanarTolerance[i], pairsCoplanarEn[i]);
  printf("     EDFactor = %.3fMeV/mm, EDmin = %dMeV, Enabled = %d\n", pairsEDFactor[i], pairsEDmin[i], pairsEDEn[i]);
  printf("\n");
  }

  printf("   *** Cosmic Configuration ***\n");
  printf("     TimeCoincidence = %d(%dns)\n", cosmicTimeCoincidence, cosmicTimeCoincidence*4);
  printf("     PatternCoincidence = 0x%02X\n", cosmicPatternCoincidence);
  printf("\n");
  
  printf("   *** Trigger Configuration ***\n");
  printf("     Latency = %d(%dns)\n", triggerLatency, triggerLatency*4);
  
  printf("   *** Trigger Output Configuration ***\n");
  for(i = SD_SRC_P2_LVDSOUT0; i <= SD_SRC_P2_LVDSOUT7; i++)
  {
  printf("     %s = %s\n", ssp_ioport_names[i], ssp_signal_names[sspGetIOSrc(id, i, 0)]);
  }
  
  printf("\n");
}

void sspPrintGtcConfig(int id)
{
  int trg, val, mask;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return;

  mask = sspGtc_GetFanout_EnableMask(id);

  printf("*** GtcConfig ***\n");
  printf("   Gtp Interface Latency     = %dns\n", sspGtc_GetLatency(id));
  printf("\n");
  printf("   *** FT Subsystem ***\n");
  printf("      Esum Delay           = %dns\n", sspGtc_GetFt_EsumDelay(id));
  printf("      Integrate Width      = %dns\n", sspGtc_GetFt_EsumIntegrationWidth(id));
  printf("      Cluster Delay        = %dns\n", sspGtc_GetFt_ClusterDelay(id));
  printf("\n");
  printf("   *** Sector Fanout ***\n");
  printf("      HTCC_CTOF Enabled    = %d\n",   (mask>>0) & 0x1);
  printf("      CND Enabled          = %d\n",   (mask>>1) & 0x1);
  printf("\n");

  for(trg = 0; trg<4; trg++)
  {
  printf("   *** Central Trigger Bit %d ***\n", trg);
  val = sspGtc_GetTrigger_Enable(id, trg);
  printf("      Enabled                      = %d\n", (val&GTC_CTRG_CTRL_EN) ? 1:0);
  printf("      *** FTCal Esum ***\n");
  printf("         Require                   = %d\n", (val&GTC_CTRG_CTRL_FTESUM_EMIN_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGtc_GetTrigger_FtEsumEmin(id, trg));
  printf("         Coindicende Width         = %dns\n", sspGtc_GetTrigger_FtEsumWidth(id, trg));
  printf("      *** FTCal Cluster ***\n");
  printf("         Require                   = %d\n", (val&GTC_CTRG_CTRL_FTCLUSTER_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGtc_GetTrigger_FtClusterEmin(id, trg));
  printf("         Emax                      = %d\n", sspGtc_GetTrigger_FtClusterEmax(id, trg));
  printf("         Nmin                      = %d\n", sspGtc_GetTrigger_FtClusterNmin(id, trg));
  printf("         HodoLayerMin              = %d\n", sspGtc_GetTrigger_FtClusterHodoNmin(id, trg));
  printf("         TriggerPulseWidth         = %dns\n", sspGtc_GetTrigger_FtClusterWidth(id, trg));
  printf("      *** FTCal Cluster Multiplicity ***\n");
  printf("         Require                   = %d\n", (val&GTC_CTRG_CTRL_FTCLUSTER_MULT_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGtc_GetTrigger_FtClusterEmin(id, trg));
  printf("         Emax                      = %d\n", sspGtc_GetTrigger_FtClusterEmax(id, trg));
  printf("         Nmin                      = %d\n", sspGtc_GetTrigger_FtClusterNmin(id, trg));
  printf("         HodoLayerMin              = %d\n", sspGtc_GetTrigger_FtClusterHodoNmin(id, trg));
  printf("         TriggerPulseWidth         = %dns\n", sspGtc_GetTrigger_FtClusterWidth(id, trg));
  printf("         ClusterMultMin            = %d\n", sspGtc_GetTrigger_FtClusterMult(id, trg));
  printf("         ClusterMultCoincidence    = %dns\n", sspGtc_GetTrigger_FtClusterMultWidth(id, trg));
  printf("\n");
  }
}

void sspPrintGtConfig(int id)
{
  int trg, val;
  
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return;

  printf("*** GtConfig ***\n");
  printf("   Gtp Interface Latency     = %dns\n", sspGt_GetLatency(id));
  printf("\n");
  printf("   *** Ecal Subsystem ***\n");
  printf("      Esum Delay             = %dns\n", sspGt_GetEcal_EsumDelay(id));
  printf("      Esum Integrate Width   = %dns\n", sspGt_GetEcal_EsumIntegrationWidth(id));
  printf("      Cluster Delay          = %dns\n", sspGt_GetEcal_ClusterDelay(id));
  printf("      Cosmic Delay           = %dns\n", sspGt_GetEcal_CosmicDelay(id));
  printf("\n");
  printf("   *** Pcal Subsystem ***\n");
  printf("      Esum Delay             = %dns\n", sspGt_GetPcal_EsumDelay(id));
  printf("      Esum Integrate Width   = %dns\n", sspGt_GetPcal_EsumIntegrationWidth(id));
  printf("      Cluster Delay          = %dns\n", sspGt_GetPcal_ClusterDelay(id));
  printf("      Cosmic Delay           = %dns\n", sspGt_GetPcal_CosmicDelay(id));
  printf("      Pcu Delay              = %dns\n", sspGt_GetPcal_PcuDelay(id));
  printf("\n");
  printf("   *** DC Subsystem ***\n");
  printf("      Segment Delay          = %dns\n", sspGt_GetDc_SegDelay(id));
  printf("\n");
  printf("   *** HTCC Subsystem ***\n");
  printf("      Mask Delay             = %dns\n", sspGt_GetHtcc_Delay(id));
  printf("\n");
  printf("   *** FTOF Subsystem ***\n");
  printf("      Mask Delay             = %dns\n", sspGt_GetFtof_Delay(id));
  printf("\n");
  printf("   *** FTOF*PCU Subsystem ***\n");
  printf("      FTOF Width             = %dns\n", sspGt_GetPcuFtof_FtofWidth(id));
  printf("      PCU  Width             = %dns\n", sspGt_GetPcuFtof_PcuWidth(id));
  printf("      PCU*FTOF table         = %d\n", sspGt_GetPcuFtof_MatchTable(id));

  printf("\n");
  for(trg = 0; trg<8; trg++)
  {
  printf("   *** Sector Trigger Bit %d ***\n", trg);
  val = sspGt_GetTrigger_Enable(id, trg);
  printf("      Enabled                      = %d\n", (val&GT_STRG_CTRL_EN) ? 1:0);
  printf("      *** Ecal Esum ***\n");
  printf("         Require                   = %d\n", (val&GT_STRG_CTRL_ECESUM_EMIN_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGt_GetTrigger_EcalEsumEmin(id, trg));
  printf("         Coindicende Width         = %dns\n", sspGt_GetTrigger_EcalEsumWidth(id, trg));
  printf("      *** Pcal Esum ***\n");
  printf("         Require                   = %d\n", (val&GT_STRG_CTRL_PCESUM_EMIN_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGt_GetTrigger_PcalEsumEmin(id, trg));
  printf("         Coindicende Width         = %dns\n", sspGt_GetTrigger_PcalEsumWidth(id, trg));
  printf("      *** Ecal Cluster ***\n");
  printf("         Require                   = %d\n", (val&GT_STRG_CTRL_ECCLUSTER_EMIN_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGt_GetTrigger_EcalClusterEmin(id, trg));
  printf("         Emax                      = %d\n", sspGt_GetTrigger_EcalClusterEmax(id, trg));
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_EcalClusterWidth(id, trg));
  printf("      *** Pcal Cluster ***\n");
  printf("         Require                   = %d\n", (val&GT_STRG_CTRL_PCCLUSTER_EMIN_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGt_GetTrigger_PcalClusterEmin(id, trg));
  printf("         Emax                      = %d\n", sspGt_GetTrigger_PcalClusterEmax(id, trg));
  printf("         Coincidence Width         = %dns\n", sspGt_GetTrigger_PcalClusterWidth(id, trg));
  printf("      *** EcalPcal Cluster ***\n");
  printf("         Require                   = %d\n", (val&GT_STRG_CTRL_ECPCCLUSTER_EMIN_EN) ? 1:0);
  printf("         Emin                      = %d\n", sspGt_GetTrigger_ECPCClusterEmin(id, trg));
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_ECPCClusterWidth(id, trg));
  printf("      *** DC ***\n");
  printf("         Require                   = %d\n", (val&GT_STRG_CTRL_DC_EN) ? 1:0);
  printf("         Require Road              = %d\n", (GT_STRG_DCCTRL_ROAD_MASK & sspGt_GetTrigger_DcRoad(id, trg)) ? 1:0);
  printf("         Require Inbend Road       = %d\n", (GT_STRG_DCCTRL_ROAD_INBEND_MASK & sspGt_GetTrigger_DcRoad(id, trg)) ? 1:0);
  printf("         Require Outbend Road      = %d\n", (GT_STRG_DCCTRL_ROAD_OUTBEND_MASK & sspGt_GetTrigger_DcRoad(id, trg)) ? 1:0);
  printf("         Seg Multiplicity Min      = %d\n", sspGt_GetTrigger_DcMultMin(id, trg));
  printf("         Coincidence Width         = %dns\n", sspGt_GetTrigger_DcWidth(id, trg));
  printf("      *** Cosmic ***\n");
  printf("         Require EC Inner Pixel    = %d\n", (val&GT_STRG_CTRL_ECOCOSMIC_EN) ? 1:0);
  printf("         Require EC Outer Pixel    = %d\n", (val&GT_STRG_CTRL_ECICOSMIC_EN) ? 1:0);
  printf("         Require PC Pixel          = %d\n", (val&GT_STRG_CTRL_PCCOSMIC_EN) ? 1:0);
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_CosmicWidth(id, trg));
  printf("      *** HTCC ***\n");
  printf("         Require HTCC Mask Hit     = %d\n", (val&GT_STRG_CTRL_HTCC_EN) ? 1:0);
  printf("         Mask                      = 0x%16llX\n", sspGt_GetTrigger_HtccMask(id, trg));
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_HtccWidth(id, trg));
  printf("      *** FTOF ***\n");
  printf("         Require FTOF Mask Hit     = %d\n", (val&GT_STRG_CTRL_FTOF_EN) ? 1:0);
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_FtofWidth(id, trg));
  printf("         Mask                      = 0x%08llX\n", sspGt_GetTrigger_FtofMask(id, trg)); 
  printf("      *** CTOF ***\n");
  printf("         Require CTOF Mask Hit     = %d\n", (val&GT_STRG_CTRL_CTOF_EN) ? 1:0);
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_CtofWidth(id, trg));
  printf("         Mask                      = 0x%02X\n", sspGt_GetTrigger_CtofMask(id, trg)); 
  printf("      *** CND ***\n");
  printf("         Require CND Mask Hit      = %d\n", (val&GT_STRG_CTRL_CND_EN) ? 1:0);
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_CndWidth(id, trg));
  printf("         Mask                      = 0x%02X\n", sspGt_GetTrigger_CndMask(id, trg));
  printf("      *** FTOF*PCU ***\n");
  printf("         Require                   = %d\n", (val&GT_STRG_CTRL_FTOFPC_EN) ? 1:0);
  printf("         Coindidence Width         = %dns\n", sspGt_GetTrigger_FtofPcuWidth(id, trg));
  printf("         Match Mask                = %02X\n", sspGt_GetTrigger_FtofPcuMatchMask(id, trg));
  printf("\n");
  }
}


void sspGtPrintScalers(int id)
{
  sspPrintGtScalers(id);
}
void sspPrintGtScalers(int id)
{
  double ref, rate; 
  int i; 
  unsigned int scalers[SD_SCALER_NUM];
  unsigned int gtscalers[24];
  const char *scalers_name[24] = {
    "ssec.cluster",
    "ssec.inner_cosmic",
    "ssec.outer_cosmic",
    "sspc.cluster",
    "sspc.cosmic",
    "sshtcc.hit",
    "ssftof.hit",
    "ssctof.hit",
    "sscnd.hit",
    "ssftofpcu.hit0",
    "ssftofpcu.hit1",
    "ssftofpcu.hit2",
    "ssftofpcu.hit3",
    "ssftofpcu.hit4",
    "ssftofpcu.hit5",
    "sspcu.hit",
    "strigger0",
    "strigger1",
    "strigger2",
    "strigger3",
    "strigger4",
    "strigger5",
    "strigger6",
    "strigger7",
    };
 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGT))
    return;
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1); 
 
  for(i = 0; i < SD_SCALER_NUM; i++) 
    scalers[i] = sspReadReg(&pSSP[id]->Sd.Scalers[i]); 

  gtscalers[0] = sspReadReg(&pSSP[id]->gt.ssec.Scaler_cluster);
  gtscalers[1] = sspReadReg(&pSSP[id]->gt.ssec.Scaler_inner_cosmic);
  gtscalers[2] = sspReadReg(&pSSP[id]->gt.ssec.Scaler_outer_cosmic);
  gtscalers[3] = sspReadReg(&pSSP[id]->gt.sspc.Scaler_cluster);
  gtscalers[4] = sspReadReg(&pSSP[id]->gt.sspc.Scaler_cosmic);
  gtscalers[5] = sspReadReg(&pSSP[id]->gt.sshtcc.Scaler_htcc);
  gtscalers[6] = sspReadReg(&pSSP[id]->gt.ssftof.Scaler_ftof);
  gtscalers[7] = sspReadReg(&pSSP[id]->gt.ssctof.Scaler_ctof);
  gtscalers[8] = sspReadReg(&pSSP[id]->gt.sscnd.Scaler_cnd);
  gtscalers[9] = sspReadReg(&pSSP[id]->gt.sspcuftof.Scaler[0]);
  gtscalers[10] = sspReadReg(&pSSP[id]->gt.sspcuftof.Scaler[1]);
  gtscalers[11] = sspReadReg(&pSSP[id]->gt.sspcuftof.Scaler[2]);
  gtscalers[12] = sspReadReg(&pSSP[id]->gt.sspcuftof.Scaler[3]);
  gtscalers[13] = sspReadReg(&pSSP[id]->gt.sspcuftof.Scaler[4]);
  gtscalers[14] = sspReadReg(&pSSP[id]->gt.sspcuftof.Scaler[5]);
  gtscalers[15] = sspReadReg(&pSSP[id]->gt.sspc.Scaler_pcu);
  for(i=0; i<8; i++)
    gtscalers[16+i] = sspReadReg(&pSSP[id]->gt.strigger[i].Scaler_trigger);

  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0); 
  SSPUNLOCK(); 
 
  printf("%s - \n", __FUNCTION__); 
  if(!scalers[SD_SCALER_SYSCLK]) 
  {
    printf("Error: sspPrintScalers() reference time is 0. Reported rates will not be normalized.\n"); 
    ref = 1.0; 
  } 
  else 
  { 
    ref = (double)scalers[SD_SCALER_SYSCLK] / 100000.0; 
  } 
   
  for(i = 0; i < SD_SCALER_NUM; i++) 
  { 
    rate = (double)scalers[i]; 
    rate = rate / ref; 
    if(scalers[i] == 0xFFFFFFFF) 
     printf("   %-25s %10u,%.3fHz [OVERFLOW]\n", ssp_scaler_name[i], scalers[i], rate); 
    else 
     printf("   %-25s %10u,%.3fHz\n", ssp_scaler_name[i], scalers[i], rate); 
  }

  for(i = 0; i < 24; i++) 
  { 
    rate = (double)gtscalers[i]; 
    rate = rate / ref; 
    if(gtscalers[i] == 0xFFFFFFFF) 
     printf("   %-25s %10u,%.3fHz [OVERFLOW]\n", scalers_name[i], gtscalers[i], rate); 
    else 
     printf("   %-25s %10u,%.3fHz\n", scalers_name[i], gtscalers[i], rate); 
  }
}

void sspPrintGtcScalers(int id)
{
  double ref, rate; 
  int i; 
  unsigned int scalers[SD_SCALER_NUM];
  unsigned int gtcscalers[6];
  const char *scalers_name[6] = {
    "ssft1.cluster",
    "ssft2.cluster",
    "ctrigger0",
    "ctrigger1",
    "ctrigger2",
    "ctrigger3"
    };
 
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_HALLBGTC))
    return;
 
  SSPLOCK(); 
  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 1); 
 
  for(i = 0; i < SD_SCALER_NUM; i++) 
    scalers[i] = sspReadReg(&pSSP[id]->Sd.Scalers[i]); 

  gtcscalers[0] = sspReadReg(&pSSP[id]->gtc.ssft[0].Scaler_cluster);
  gtcscalers[1] = sspReadReg(&pSSP[id]->gtc.ssft[1].Scaler_cluster);
  gtcscalers[2] = sspReadReg(&pSSP[id]->gtc.ctrigger[0].Scaler_trigger);
  gtcscalers[3] = sspReadReg(&pSSP[id]->gtc.ctrigger[1].Scaler_trigger);
  gtcscalers[4] = sspReadReg(&pSSP[id]->gtc.ctrigger[2].Scaler_trigger);
  gtcscalers[5] = sspReadReg(&pSSP[id]->gtc.ctrigger[3].Scaler_trigger);

  sspWriteReg(&pSSP[id]->Sd.ScalerLatch, 0); 
  SSPUNLOCK(); 
 
  printf("%s - \n", __FUNCTION__); 
  if(!scalers[SD_SCALER_SYSCLK]) 
  {
    printf("Error: sspPrintScalers() reference time is 0. Reported rates will not be normalized.\n"); 
    ref = 1.0; 
  } 
  else 
  { 
    ref = (double)scalers[SD_SCALER_SYSCLK] / 100000.0; 
  } 
   
  for(i = 0; i < SD_SCALER_NUM; i++) 
  { 
    rate = (double)scalers[i]; 
    rate = rate / ref; 
    if(scalers[i] == 0xFFFFFFFF) 
     printf("   %-25s %10u,%.3fHz [OVERFLOW]\n", ssp_scaler_name[i], scalers[i], rate); 
    else 
     printf("   %-25s %10u,%.3fHz\n", ssp_scaler_name[i], scalers[i], rate); 
  }

  for(i = 0; i < 6; i++) 
  { 
    rate = (double)gtcscalers[i]; 
    rate = rate / ref; 
    if(gtcscalers[i] == 0xFFFFFFFF) 
     printf("   %-25s %10u,%.3fHz [OVERFLOW]\n", scalers_name[i], gtcscalers[i], rate); 
    else 
     printf("   %-25s %10u,%.3fHz\n", scalers_name[i], gtcscalers[i], rate); 
  }
}

/* Global arrays of strings of names of ports/signals */ 
 
const char *ssp_ioport_names[SD_SRC_NUM] =  
  { 
    "LVDSOUT0", 
    "LVDSOUT1", 
    "LVDSOUT2", 
    "LVDSOUT3", 
    "LVDSOUT4", 
    "GPIO0", 
    "GPIO1", 
    "P2_LVDSOUT0 (TS#1)", 
    "P2_LVDSOUT1 (TS#2)", 
    "P2_LVDSOUT2 (TS#3)", 
    "P2_LVDSOUT3 (TS#4)", 
    "P2_LVDSOUT4 (TS#5)", 
    "P2_LVDSOUT5 (TS#6)", 
    "P2_LVDSOUT6", 
    "P2_LVDSOUT7", 
    "TRIG", 
    "SYNC",
    "TRIG2"
  }; 
	 
const char *ssp_signal_names[SD_SRC_SEL_NUM] =  
  { 
    "SD_SRC_SEL_0", 
    "SD_SRC_SEL_1", 
    "SD_SRC_SEL_SYNC", 
    "SD_SRC_SEL_TRIG1", 
    "SD_SRC_SEL_TRIG2", 
    "SD_SRC_SEL_LVDSIN0", 
    "SD_SRC_SEL_LVDSIN1", 
    "SD_SRC_SEL_LVDSIN2", 
    "SD_SRC_SEL_LVDSIN3", 
    "SD_SRC_SEL_LVDSIN4", 
    "SD_SRC_SEL_P2LVDSIN0", 
    "SD_SRC_SEL_P2LVDSIN1", 
    "SD_SRC_SEL_P2LVDSIN2", 
    "SD_SRC_SEL_P2LVDSIN3", 
    "SD_SRC_SEL_P2LVDSIN4", 
    "SD_SRC_SEL_P2LVDSIN5", 
    "SD_SRC_SEL_P2LVDSIN6", 
    "SD_SRC_SEL_P2LVDSIN7", 
    "SD_SRC_SEL_PULSER", 
    "SD_SRC_SEL_BUSY", 
    "SD_SRC_SEL_TRIGGER0 (HPS SINGLES 0)", 
    "SD_SRC_SEL_TRIGGER1 (HPS SINGLES 1)", 
    "SD_SRC_SEL_TRIGGER2 (HPS PAIRS 0)", 
    "SD_SRC_SEL_TRIGGER3 (HPS PAIRS 1)", 
    "SD_SRC_SEL_TRIGGER4 (HPS LED)", 
    "SD_SRC_SEL_TRIGGER5 (HPS COSMIC)", 
    "SD_SRC_SEL_TRIGGER6", 
    "SD_SRC_SEL_TRIGGER7" 
  }; 
 
const char *ssp_gtpsrc_names[TRG_CTRL_GTPSRC_NUM] =  
  { 
    "TRG_CTRL_GTPSRC_FIBER0", 
    "TRG_CTRL_GTPSRC_FIBER1", 
    "TRG_CTRL_GTPSRC_FIBER2", 
    "TRG_CTRL_GTPSRC_FIBER3", 
    "TRG_CTRL_GTPSRC_FIBER4", 
    "TRG_CTRL_GTPSRC_FIBER5", 
    "TRG_CTRL_GTPSRC_FIBER6", 
    "TRG_CTRL_GTPSRC_FIBER7", 
    "TRG_CTRL_GTPSRC_SUM" 
  }; 
 
const char *ssp_scaler_name[SD_SCALER_NUM] =  
  { 
    "SD_SCALER_SYSCLK", 
    "SD_SCALER_GCLK", 
    "SD_SCALER_SYNC", 
    "SD_SCALER_TRIG1", 
    "SD_SCALER_TRIG2", 
    "SD_SCALER_GPIO0", 
    "SD_SCALER_GPIO1", 
    "SD_SCALER_LVDSIN0", 
    "SD_SCALER_LVDSIN1", 
    "SD_SCALER_LVDSIN2", 
    "SD_SCALER_LVDSIN3", 
    "SD_SCALER_LVDSIN4", 
    "SD_SCALER_LVDSOUT0", 
    "SD_SCALER_LVDSOUT1", 
    "SD_SCALER_LVDSOUT2", 
    "SD_SCALER_LVDSOUT3", 
    "SD_SCALER_LVDSOUT4", 
    "SD_SCALER_BUSY", 
    "SD_SCALER_BUSYCYCLES", 
    "SD_SCALER_P2_LVDSIN0", 
    "SD_SCALER_P2_LVDSIN1", 
    "SD_SCALER_P2_LVDSIN2", 
    "SD_SCALER_P2_LVDSIN3", 
    "SD_SCALER_P2_LVDSIN4", 
    "SD_SCALER_P2_LVDSIN5", 
    "SD_SCALER_P2_LVDSIN6", 
    "SD_SCALER_P2_LVDSIN7", 
    "SD_SCALER_P2_LVDSOUT0", 
    "SD_SCALER_P2_LVDSOUT1", 
    "SD_SCALER_P2_LVDSOUT2", 
    "SD_SCALER_P2_LVDSOUT3", 
    "SD_SCALER_P2_LVDSOUT4", 
    "SD_SCALER_P2_LVDSOUT5", 
    "SD_SCALER_P2_LVDSOUT6", 
    "SD_SCALER_P2_LVDSOUT7" 
  }; 
 
const char *ssp_clksrc_name[SSP_CLKSRC_NUM] =  
  { 
    "DISABLED",  
    "SWB",  
    "P2",  
    "LOCAL" 
  }; 
 
const char *ssp_serdes_names[SSP_SER_NUM] =  
  { 
    "SSP_SER_FIBER0", 
    "SSP_SER_FIBER1",
    "SSP_SER_FIBER2", 
    "SSP_SER_FIBER3", 
    "SSP_SER_FIBER4", 
    "SSP_SER_FIBER5", 
    "SSP_SER_FIBER6", 
    "SSP_SER_FIBER7", 
    "SSP_SER_VXS0", 
    "SSP_SER_VXSGTP" 
  }; 



/*******************************************************************************
 *
 * sspReadBlock - General Data readout routine
 *
 *    id    - SSP to read from
 *    data  - local memory address to place data
 *    nwrds - Max number of words to transfer
 *    rflag - Readout Flag
 *              0 - programmed I/O from the specified board
 *              1 - DMA transfer using Universe/Tempe DMA Engine 
 *                    (DMA VME transfer Mode must be setup prior)
 *              2 - Multiblock DMA transfer (Multiblock must be enabled
 *                     and daisychain in place or SD being used)
 *
 * RETURNS: Number of words transferred to data if successful, ERROR otherwise
 *
 */

int
sspReadBlock(int id, unsigned int *data, int nwrds, int rflag)
{
  int retVal;
  volatile unsigned int *laddr;
  unsigned int val;
  unsigned int vmeAdr;

  if(id==0) id=sspSL[0]; 
  if((id<=0) || (id>21) || (pSSP[id]==NULL)) 
  { 
      printf("%s: ERROR: SSP in slot %d not initialized\n",__FUNCTION__,id); 
      return -1; 
  } 

  if (SSPpf[id] == NULL) {
  logMsg("ERROR: %s: SSP A32 not initialized\n", __func__);
    return -1;
  }

  if (data == NULL)
  {
    logMsg("ERROR: %s: Invalid Destination address\n", __func__);
    return -1;
  }

  SSPLOCK();

  /* Block transfer */
  if (rflag >= 1)
  {
    /* Assume that the DMA programming is already setup. 
    Don't Bother checking if there is valid data - that should be done prior
    to calling the read routine */

    laddr = data;
	/*
	printf("DMA1: 0x%08x 0x%08x 0x%08x\n",
		   vmeRead32(&pSSP[id]->EB.FifoBlockCnt),
		   vmeRead32(&pSSP[id]->EB.FifoWordCnt),
		   vmeRead32(&pSSP[id]->EB.FifoEventCnt));
	*/
    vmeAdr = (unsigned int)(SSPpf[id]) - sspA32Offset;

	/*
    printf("sspReadBlock: vmeAdr=0x%08x (0x%08x - 0x%08x)\n",vmeAdr,(unsigned int)(SSPpf[id]),sspA32Offset);fflush(stdout);
	*/

    retVal = usrVme2MemDmaStart(vmeAdr, (unsigned int *)laddr, (nwrds << 2));

    if (retVal |= 0)
    {
      logMsg("ERROR: %s: DMA transfer Init @ 0x%x Failed\n", __func__, retVal);
      SSPUNLOCK();
      return retVal;
    }

    retVal = usrVme2MemDmaDone();

    if (retVal > 0) {
      int xferCount = (retVal >> 2);

      SSPUNLOCK();
      return xferCount;
    }
    else if (retVal == 0) {
      logMsg("WARN: %s: DMA transfer terminated by word count 0x%x\n", \
              __func__, nwrds);
      SSPUNLOCK();
      return 0;
    }
    /* Error in DMA */
    else {
      logMsg("ERROR: %s: DmaDone returned an Error\n", __func__);
      SSPUNLOCK();
      return 0;
    }
  }
  /* Programmed IO */
  else
  {
    int dCnt = 0;
    int ii = 0;

    while (ii < nwrds)
    {
      val = *SSPpf[id];
#ifndef VXWORKS
      val = LSWAP(val);
#endif
/*
      if (val == TI_EMPTY_FIFO)
        break;
#ifndef VXWORKS
      val = LSWAP(val);
#endif
*/
      data[ii] = val;
      ii++;
    }
    ii++;
    dCnt += ii;

    SSPUNLOCK();
    return dCnt;
  }

  SSPUNLOCK();
  return(0);
}



int
sspGetNssp()
{
  return(nSSP);
}


int
sspBReady(int slot)
{
  unsigned int rval;

  SSPLOCK();
  rval = vmeRead32(&pSSP[slot]->EB.FifoBlockCnt);
  SSPUNLOCK();

  return (rval > 0) ? 1 : 0;
}

unsigned int
sspGBReady()
{
  unsigned int mask = 0;
  int i, stat;

  /*SSPLOCK();*/
  for (i = 0; i < nSSP; i++) {
    stat = sspBReady(sspSL[i]);
    if (stat)
      mask |= (1 << sspSL[i]);
  }
  /*SSPUNLOCK();*/

  return(mask);
}

int
sspGetFirmwareType(int id)
{
  int rval;
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
  
  SSPLOCK();
  rval = vmeRead32(&pSSP[id]->Cfg.FirmwareRev);
  SSPUNLOCK();
  
  return (rval&SSP_CFG_SSPTYPE_MASK)>>16;
}

int sspGetFirmwareType_Shadow(int id)
{
  if(sspIsNotInit(&id, __func__, SSP_CFG_SSPTYPE_COMMON))
    return ERROR;
  
  return sspFirmwareType[id];
}

/* the number of events per block */
int
sspSetBlockLevel(int id, int block_level)
{
  if(id==0) id=sspSL[0]; 
  vmeWrite32(&pSSP[id]->EB.BlockCfg, block_level);
  return(0);
}

int
sspGetBlockLevel(int id)
{
  int ret;
  if(id==0) id=sspSL[0];
  ret = vmeRead32(&pSSP[id]->EB.BlockCfg);
  printf("sspGetBlockLevel returns %d\n",ret),fflush(stdout);
  return(ret);
}

/* Enable Bus Error */
int
sspEnableBusError(int id)
{
  if(id==0) id=sspSL[0]; 
  vmeWrite32(&pSSP[id]->EB.ReadoutCfg, 1);
  return(0);
}

int
sspDisableBusError(int id)
{
  if(id==0) id=sspSL[0]; 
  vmeWrite32(&pSSP[id]->EB.ReadoutCfg, 0);
  return(0);
}

int
sspGetBusError(int id)
{
  int ret;
  if(id==0) id=sspSL[0];
  ret = vmeRead32(&pSSP[id]->EB.ReadoutCfg);
  printf("sspGetBusError returns %d\n",ret),fflush(stdout);
  return(ret);
}


/* window size */
int
sspSetWindowWidth(int id, int window_width)
{
  if(id==0) id=sspSL[0]; 
  vmeWrite32(&pSSP[id]->EB.WindowWidth, window_width/4);
  return(0);
}

int
sspGetWindowWidth(int id)
{
  int ret;
  if(id==0) id=sspSL[0];
  ret = vmeRead32(&pSSP[id]->EB.WindowWidth) * 4;
  printf("sspGetWindowWidth returns %d\n",ret),fflush(stdout);
  return(ret);
}

/* window position */
int
sspSetWindowOffset(int id, int window_offset)
{
  if(id==0) id=sspSL[0]; 
  vmeWrite32(&pSSP[id]->EB.Lookback, window_offset/4);
  return(0);
}

int
sspGetWindowOffset(int id)
{
  int ret;
  if(id==0) id=sspSL[0];
  ret = vmeRead32(&pSSP[id]->EB.Lookback) * 4;
  printf("sspGetWindowOffset returns %d\n",ret),fflush(stdout);
  return(ret);
}

/*sergey*/
void
sspSetA32BaseAddress(unsigned int addr)
{
  sspA32Base = addr;
  printf("ssp A32 base address set to 0x%08X\n",sspA32Base);
}

#endif

