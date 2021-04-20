/* FASTBUS Library routines for the STRUCK SFI 340. These are non-standard
   routines adapted from Struck distributed example code. The FASTBUS standard
   Level 1 routines reside in fb_sfi_1.h .

   Adapter: David Abbott  March 1996
   CEBAF Data Acquisition Group

   April 2011: Ported to Linux - Bryan Moffit

******************************************************************************/

#if defined(VXWORKS) || defined(Linux_vme)

#ifdef VXWORKS
#include <vxWorks.h>
#include <cacheLib.h>
#include <logLib.h>
#else
#include <jvme.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "libsfifb.h"

/* Some local defines for debugging */
#define SFI_READ_ADDR_OFFSET    0x100

#ifdef VXWORKS
#define EIEIO    __asm__ volatile ("eieio")
#define SYNC     __asm__ volatile ("sync")
#else
#define EIEIO    
#define SYNC     
#endif

/* Locks/Unlocks for SFI access */
pthread_mutex_t   sfiMutex = PTHREAD_MUTEX_INITIALIZER;
#define SFILOCK     {							\
    if(pthread_mutex_lock(&sfiMutex)<0) perror("pthread_mutex_lock");	\
  }									

#define SFIUNLOCK   {							\
    if(pthread_mutex_unlock(&sfiMutex)<0) perror("pthread_mutex_unlock"); \
  }									

/* Define external Functions */
#ifdef VXWORKS
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
#endif

/* Include some useful utility routines and SFI structure definition*/
#include "sfi.h"
#include "coda_sfi.h"
#include "sfiTrigLib.h"

extern unsigned int *dma_dabufp;


/* This Module holds:  */
/* 
   unsigned int fpwc(PAddr, SAddr,WData)
   unsigned int fprc(PAddr, SAddr,rData)
   unsigned int fpwd(PAddr, SAddr,WData)
   unsigned int fprd(PAddr, SAddr,rData)
   unsigned int fpwcm(PAddr, SAddr,WData)
   unsigned int fprcm(PAddr, SAddr,rData)
   unsigned int fpwdm(PAddr, SAddr,WData)
   unsigned int fprdm(PAddr, SAddr,rData)
   unsigned int fpac(PAddr, SAddr)
   unsigned int fpad(PAddr, SAddr)
   unsigned int fpsaw(SAddr)
   unsigned int fpr()
   unsigned int fpw(WData)
   unsigned int fprdb(PAddr,SAddr,Buffer,next_buffer,Max_ExpLWord,cnt_RecLWord,Mode,Wait)
   unsigned int fprel()
   unsigned int fpfifo(fflag)
*/

/* Include the FASTBUS standard Level 1 routines */
#include "fb_sfi_1.h"

/*======================================================================*/
/*
  Funktion     : FWC

  In           : pAddr	   primary address
  sAddr     secondary address
  lc        limit count

  Out          : returns 0, if no error, else number of errors

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpwc(PAddr, SAddr,WData)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  vmeWrite32(fastPrimCsr, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);  /* secondary address cycle */ 
  vmeWrite32(fastRndmWDis, WData);

  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);
#ifdef DEBUG
      printf("fpwc: WData = 0x%08x   reg32 = 0x%08x\n",WData, reg32);
#endif

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);
  SFIUNLOCK;



  return(Return);

}



unsigned int 
fprc(PAddr, SAddr,rData)

     unsigned int PAddr;
     unsigned int SAddr;
     volatile unsigned int *rData;
{
  volatile unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  volatile unsigned int *vaddr;
  unsigned int    kk;

  SFILOCK;
  vmeWrite32(fastPrimCsr, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);  /* secondary address cycle */ 
  vmeWrite32(fastRndmRDis, reg32);

  sfiSeq2VmeFifoVal=0;
  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;


      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  /* read seq fifo flags */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */


	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) == 0x00000010)  /* Empty */
	    {
	      kk=0;
	      vaddr = sfi.readSeqFifoFlags;
	      vaddr += (SFI_READ_ADDR_OFFSET>>2);
	      reg32 = vmeRead32(vaddr);
	      while ( (kk<10) && ((reg32&0x00000010) == 0x00000010) ) 
		{
		  sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);  /* Dummy read */
		  vaddr += (SFI_READ_ADDR_OFFSET>>2);
		  reg32 = vmeRead32(vaddr);
		  kk++;
		}
	      if(kk > 1) 
		{
		  /* logMsg("fprc: kk = %d\n",kk,0,0,0,0,0); */
		  if(kk >= 10) /* Still Empty */
		    {
		      /* Error !!! */
		      sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);
		      logMsg("fprc: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
		      Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		      Return |= 0x00100000;
		   
		      break;
		    }
		}
	    }
        
	  /* read fifo */

	  *rData = vmeRead32(sfi.readSeq2VmeFifoBase);
         
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;


  return(Return);



}


/*======================================================================*/
/*
  Funktion     : FWD

  In           : pAddr	   primary address
  sAddr     secondary address
  lc        limit count
  &wc        address where word count is stored
  &ap        address where address pointer is stored
		  
  Out          : returns 0, if no error, else number of errors
  *wc        word count
  *ap        address pointer

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpwd(PAddr, SAddr, WData)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  vmeWrite32(fastPrimDsr, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);  /* secondary address cycle */ 
  vmeWrite32(fastRndmWDis, WData);


  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);
  SFIUNLOCK;



  return(Return);

}



unsigned int 
fprd(PAddr, SAddr,rData)

     unsigned int PAddr;
     unsigned int SAddr;
     volatile unsigned int *rData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  unsigned int kk;

  SFILOCK;
  vmeWrite32(fastPrimDsr, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);  /* secondary address cycle */ 
  vmeWrite32(fastRndmRDis, reg32);


  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  /* read seq fifo flags */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */


	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) == 0x00000010)  /* Empty */
	    {
	      kk=0;
	      reg32 = vmeRead32(sfi.readSeqFifoFlags);
	      while ( (kk<20) && ((reg32&0x00000010) == 0x00000010) ) {
		sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase); /* Dummy read */
		reg32 = vmeRead32(sfi.readSeqFifoFlags);
		kk++;
	      }
	      if(kk >= 20) /* Still Empty */
		{
		  /* Error !!! */
		  logMsg("fprd: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
		  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		  Return |= 0x00100000;
		  break;
		}
	    }
        
	  /* read fifo */

	  *rData = vmeRead32(sfi.readSeq2VmeFifoBase);
         
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);



}

/*======================================================================*/
/*
  Funktion     : FWC

  In           : pAddr     primary address
  sAddr     secondary address
  lc        limit count
  &wc        address where word count is stored
  &ap        address where address pointer is stored
		  
  Out          : returns 0, if no error, else number of errors
  *wc        word count
  *ap        address pointer

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpwcm(PAddr, SAddr,WData)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  vmeWrite32(fastPrimCsrM, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);   /* secondary address cycle */ 
  vmeWrite32(fastRndmWDis, WData);



  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;


      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);

}



unsigned int 
fprcm(PAddr, SAddr,rData)

     unsigned int PAddr;
     unsigned int SAddr;
     volatile unsigned int *rData;
{
  volatile unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  volatile unsigned int *vaddr;
  volatile unsigned int *vfaddr;
  unsigned int kk;


  SFILOCK;
  vmeWrite32(fastPrimCsrM, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);   /* secondary address cycle */ 
  vmeWrite32(fastRndmRDis, reg32);

  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  /* read seq fifo flags */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */


	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
#ifdef DEBUG
	  printf("0: reg32 = 0x%08x\n",reg32);
#endif
	  if((reg32 & 0x00000010) == 0x00000010)  /* Empty */
	    {
	      kk=0;
	      vaddr = sfi.readSeqFifoFlags;
	      vfaddr = sfi.readSeq2VmeFifoBase;
	      vaddr += (SFI_READ_ADDR_OFFSET>>2);
	      reg32 = vmeRead32(vaddr);
#ifdef DEBUG
	      printf("1: vaddr = 0x%08x   reg32 = 0x%08x\n",vaddr, reg32);
#endif
	      while ( (kk<10) && ((reg32&0x00000010) == 0x00000010) ) 
		{
		  sfiSeq2VmeFifoVal = vmeRead32(vfaddr++);  /* Dummy read */
		  vaddr += (SFI_READ_ADDR_OFFSET>>2);
		  reg32 =  vmeRead32(vaddr);
#ifdef DEBUG
		  printf("2: vaddr = 0x%08x   reg32 = 0x%08x   vfaddr = 0x%08x    sfiSeq2VmeFifoVal =0x%08x\n",
			 vaddr, reg32, vfaddr, sfiSeq2VmeFifoVal);
#endif
		  kk++;
		}
	      if(kk > 1) 
		{
		  /*logMsg("fprcm: kk = %d\n",kk,0,0,0,0,0); */
		  if(kk >= 10) /* Still Empty */
		    {
		      /* Error !!! */
		      sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);
		      logMsg("fprcm: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
		      Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		      Return |= 0x00100000;
		   
		      break;
		    }
		}
	    }
        
	  /* read fifo */

	  *rData = vmeRead32(sfi.readSeq2VmeFifoBase);
         
	  break;
	case 0x00000001:
	  /* Not Finished */
	  continue;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);



}

/*======================================================================*/
/*
  Funktion     : FWDM

  In           : pAddr	   primary address
  sAddr     secondary address
  lc        limit count
  &wc        address where word count is stored
  &ap        address where address pointer is stored
		  
  Out          : returns 0, if no error, else number of errors
  *wc        word count
  *ap        address pointer

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpwdm(PAddr, SAddr,WData)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  vmeWrite32(fastPrimDsrM, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);   /* secondary address cycle */ 
  vmeWrite32(fastRndmWDis, WData);


  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);

}



unsigned int 
fprdm(PAddr, SAddr,rData)

     unsigned int PAddr;
     unsigned int SAddr;
     volatile unsigned int *rData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  unsigned int kk;

  SFILOCK;
  vmeWrite32(fastPrimDsrM, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);   /* secondary address cycle */ 
  vmeWrite32(fastRndmRDis, reg32);

  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  /* read seq fifo flags */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */


	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) == 0x00000010)  /* Empty */
	    {
	      kk=0;
	      reg32 = vmeRead32(sfi.readSeqFifoFlags);
	      while ( (kk<20) && ((reg32&0x00000010) == 0x00000010) ) 
		{
		  sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase); /* Dummy read */
		  reg32 = vmeRead32(sfi.readSeqFifoFlags);
		  kk++;
		}
	      if(kk >= 20) /* Still Empty */
		{
		  /* Error !!! */
		  logMsg("fb_rdb_1: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
		  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		  Return |= 0x00100000;
		  break;
		}
	    }
        
	  /* read fifo */

	  *rData = vmeRead32(sfi.readSeq2VmeFifoBase);
         
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);



}

/*======================================================================*/
/*
  Funktion     : FAC

  In           : pAddr     primary address
  sAddr     secondary address

		  
  Out          : returns 0, if no error, else number of errors


  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpac(PAddr, SAddr)

     unsigned int PAddr;
     unsigned int SAddr;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;


  SFILOCK;
  vmeWrite32(fastPrimCsr, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);   /* secondary address cycle */ 


  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);

}



/*======================================================================*/
/*
  Funktion     : FAD

  In           : pAddr     primary address
  sAddr     secondary address
		  
  Out          : returns 0, if no error, else number of errors

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpad(PAddr, SAddr)

     unsigned int PAddr;
     unsigned int SAddr;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;


  SFILOCK;
  vmeWrite32(fastPrimDsr, PAddr); /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);   /* secondary address cycle */ 

  /* wait for sequencer done */
  do 
    {
      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);

}


/*======================================================================*/
/*
  Funktion     : FSAR, FSAW   secondary address read/write

  In           : SAddr		secondary address (for write)
		  
  Out          : returns 0, if no error, else seq status register

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpsar()

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;


  SFILOCK;
  vmeWrite32(fastSecadR, reg32);   /* secondary address cycle */ 

  /* wait for sequencer done */
  do 
    {
      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  /* read seq fifo flags */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */


	  reg32 = vmeRead32(sfi.readSeqFifoFlags);

	
	  if((reg32 & 0x00000010) == 0x00000010)
	    {
	      reg32 = vmeRead32(sfi.readSeq2VmeFifoBase); /* dummy read */
	      reg32 = vmeRead32(sfi.readSeqFifoFlags);
	      if((reg32 & 0x00000010) == 0x00000010) 
		{
		  /* Fehler !!! */
		  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		  Return |= 0x00100000;
		}
	    }
        
	  /* read fifo */

	  Return = vmeRead32(sfi.readSeq2VmeFifoBase);
         
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);



}

unsigned int 
fpsaw(SAddr)

     unsigned int SAddr;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;


  SFILOCK;
  vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  /* wait for sequencer done */
  do 
    {
      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);


}


/*======================================================================*/
/*
  Funktion     : FPR FPW     (Fastbus single read/write from attached slave)

  In           : NONE
		  
  Out          : returns result of read else the error

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fpr()

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  vmeWrite32(fastRndmR, reg32);


  /* wait for sequencer done */
  do 
    {
      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  /* read seq fifo flags */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */


	  reg32 = vmeRead32(sfi.readSeqFifoFlags);

	
	  if((reg32 & 0x00000010) == 0x00000010)
	    {
	      reg32 = vmeRead32(sfi.readSeq2VmeFifoBase); /* dummy read */
	      reg32 = vmeRead32(sfi.readSeqFifoFlags);
	      if((reg32 & 0x00000010) == 0x00000010) 
		{
		  /* Fehler !!! */
		  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		  Return |= 0x00100000;
		}
	    }
        
	  /* read fifo */

	  Return = vmeRead32(sfi.readSeq2VmeFifoBase);
         
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);



}

unsigned int 
fpw(WData)

     unsigned int WData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;


  SFILOCK;
  vmeWrite32(fastRndmW, WData);

  /* wait for sequencer done */
  do 
    {
      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);

}



/*======================================================================*/
/*
  Funktion     : frdb      (Fastbus block read routine)

  In           : See discription below
		  
  Out          : returns 0, if no error, else number of errors

  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int 
fprdb(PAddr,SAddr,Buffer,next_buffer,Max_ExpLWord,cnt_RecLWord,Mode,Wait)

     unsigned int PAddr;           /* Primary Address */
     unsigned int SAddr;	       /* Secondary Address */
     unsigned int Buffer;          /* Address of LWord-Buffer (VME-Slave Address!!! )*/
     unsigned int *next_buffer;    /* Address of LWord-Buffer after READ */
     unsigned int Max_ExpLWord;    /* Max. Count of 32Bit Datawords  */
     unsigned int *cnt_RecLWord;   /* Count of Received 32Bit Datawords  */ 
     unsigned int Mode;            /* Readout-Modus (Direct and/or VME) */
     /* 0x10 only Direct            */
     /* 0x09 VMED32 Data cycle      */
     /* 0x0A VMED32 Blocktransfer   */
     /* 0x19 Direct and VME32Datacycl. */
     /* 0x1A Direct and VME32Blocktr.  */
     unsigned int Wait;            /* Count for CPU idle loop before
				      polling the Sequencer status */
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;
  unsigned int ii,kk;

  Max_ExpLWord &= 0x00ffffff;
  Max_ExpLWord |= (Mode << 24);

  SFILOCK;
  vmeWrite32(fastLoadDmaAddressPointer, Buffer);
  vmeWrite32(fastPrimDsr, PAddr);          /* primary address cycle   */
  vmeWrite32(fastSecadW, SAddr);           /* secondary address cycle */ 
  vmeWrite32(fastStartFrdbWithClearWc, Max_ExpLWord); /* start dma    */
  vmeWrite32(fastDiscon, reg32);           /*            disconnect   */
  vmeWrite32(fastStoreFrdbWc, reg32);                 /* get wordcount*/
  vmeWrite32(fastStoreFrdbAp, reg32);                 /* get adr.ptr. */

  /* wait for sequencer done */
  for (ii=0;ii<Wait;ii++)
    kk = ii*ii;

  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:   /* OK */
	  Return = 0;
	  Exit = 1;
	  /* read seq fifo flags */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */

	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) == 0x00000010)
	    {
	      reg32 = vmeRead32(sfi.readSeq2VmeFifoBase); /* dummy read */
	      reg32 = vmeRead32(sfi.readSeqFifoFlags);
	      if((reg32 & 0x00000010) == 0x00000010) 
		{
		  /* Error !!! */
		  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		  Return |= 0x00100000;
		  break;
		}
	    }
        
	  /* read fifo */

	  *cnt_RecLWord = vmeRead32(sfi.readSeq2VmeFifoBase);
	  /* Check for FIFO Empty: */
	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) == 0x00000010) 
	    {
	      /* Error !!! */
	      Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	      Return |= 0x00040000;
	      break;
	    }
	 
	  /* read fifo */

	  *next_buffer  = vmeRead32(sfi.readSeq2VmeFifoBase);
	  /* Check for FIFO Empty: */
	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) != 0x00000010) 
	    {
	      /* Error !!! */
	      Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	      Return |= 0x00040000;
	      break;
	    }

	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);


}



/*======================================================================*/
/*
  Funktion     : FREL

  In           : 
                 

		  
  Out          : returns 0, if no error, else number of errors


  Bemerkungen  : 

*/
/*======================================================================*/

unsigned int fprel()

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;


  SFILOCK;
  vmeWrite32(fastDiscon, reg32); /* Release fastbus */


  /* wait for sequencer done */
  do 
    {
      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);

}


/*======================================================================*/
/*
  Function     : FPRAM

  In           : ramAddr - Local RAM address from where to start execution
  (must be on 256 byte boundary i.e. 0x100,0x200 etc...)
  nRet    - Number of Return data words to be read from Sequencer
  FIFO.
  rData   - Pointer to Array for return values to be placed.
		  
  Out          : returns 0, if no error, else error result with Sequencer Status


  Description  : Execute SFI Sequencer RAM list 

*/
/*======================================================================*/

unsigned int 
fpram(ramAddr, nRet, rData)

     unsigned int ramAddr;
     int nRet;
     volatile unsigned int *rData;
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  volatile unsigned int *vaddr;

  int kk;

  SFILOCK;
  vmeWrite32((fastEnableRamSequencer + (ramAddr>>2)), 1);       /* start Sequencer at RAM Address */

  /* wait for sequencer done */
  do 
    {
      EIEIO;
      SYNC;

      reg32 = vmeRead32(sfi.sequencerStatusReg);

      switch(reg32 & 0x00008001)
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;

	  if(nRet) 
	    {
	      /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	      /*                   else if not empty then read fifo */
	      reg32 = vmeRead32(sfi.readSeqFifoFlags);
	      if((reg32 & 0x00000010) == 0x00000010)  /* Empty */
		{
		  kk=0;
		  vaddr = sfi.readSeqFifoFlags;
		  vaddr += (SFI_READ_ADDR_OFFSET>>2);
		  reg32 = vmeRead32(vaddr);
		  while ( (kk<50) && ((reg32&0x00000010) == 0x00000010) ) 
		    {
		      sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase); /* Dummy read */
		      if(kk<10) vaddr += (SFI_READ_ADDR_OFFSET>>2);
		      reg32 =  vmeRead32(vaddr);
		      kk++;
		    }
		  if(kk > 1) 
		    {
		      /* logMsg("fpram: kk = %d\n",kk,0,0,0,0,0); */
		      if(kk >= 50) /* Still Empty */
			{
			  /* Error !!! */
			  logMsg("fpram: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
			  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
			  Return |= 0x00100000;
			  break;
			}
		    }
		}
	   
	      /* read fifo */
	      if(rData) 
		{
		  for (kk=0; kk<nRet; kk++)
		    *rData++ = vmeRead32(sfi.readSeq2VmeFifoBase);
		} 
	      else 
		{
		  logMsg("fpram: Error - Null Pointer for Return Data\n",0,0,0,0,0,0);
		}
	    }
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  SFIUNLOCK;

  return(Return);

}

/* SFI Sequencer RAM Functions */
void
sfiRamLoadEnable(unsigned int a)
{

  vmeWrite32(sfi.sequencerReset, 1);
  vmeWrite32(sfi.sequencerRamAddressReg, a);
  vmeWrite32(sfi.sequencerRamLoadEnable, 1);

}

void
sfiRamLoadDisable()
{
  vmeWrite32(sfi.sequencerRamLoadDisable, 1);
  vmeWrite32(sfi.sequencerEnable, 1);
}

/* sergey: replace 'vmeDmaLocalToVmeAdrs()' with 'usrDmaLocalToVmeAdrs()' */
void
sfiLoadAddr(unsigned int a)
{
  if(a)
    {
      vmeWrite32(fastLoadDmaAddressPointer,
				 /*vme*/usrDmaLocalToVmeAdrs((unsigned int)a));
    }
  else
    {
      vmeWrite32(fastLoadDmaAddressPointer,
				 /*vme*/usrDmaLocalToVmeAdrs((unsigned int)dma_dabufp));
    }
}

void
sfiWait()
{
  taskDelay(1);
}

void
sfiSetOutReg(unsigned int m)
{
  vmeWrite32(fastWriteSequencerOutReg, m);
}

void
sfiClearOutReg(unsigned int m)
{
  vmeWrite32(fastWriteSequencerOutReg, ((m)<<16));
}


/* FASTBUS Routines for RAM List Programming */
void
sfiRAMFBAC(unsigned int p, unsigned int s) 
{ 
  if(s) 
    {
      vmeWrite32(fastPrimCsr, p);
      vmeWrite32(fastSecadW, s);
    }
  else
    {
      vmeWrite32(fastPrimCsr, p);
    }
}

void
sfiRAMFBAD(unsigned int p, unsigned int s) 
{ 
  if(s) 
    {
      vmeWrite32(fastPrimDsr, p);
      vmeWrite32(fastSecadW, s);
    }
  else
    {
      vmeWrite32(fastPrimDsr, p);
    }
}

void 
sfiRAMFBSAW(unsigned int s)  
{
  vmeWrite32(fastSecadW, s);
}

void 
sfiRAMFBWC(unsigned int p, unsigned int s, unsigned int d) 
{ 
  if(s) 
    {
      vmeWrite32(fastPrimCsr, p);
      vmeWrite32(fastSecadW, s);
      vmeWrite32(fastRndmWDis, d);
    }
  else
    {
      vmeWrite32(fastPrimCsr, p);
      vmeWrite32(fastRndmWDis, d);
    }
}

void
sfiRAMFBWD(unsigned int p, unsigned int s, unsigned int d) 
{ 
  if(s) 
    {
      vmeWrite32(fastPrimDsr, p);
      vmeWrite32(fastSecadW, s);
      vmeWrite32(fastRndmWDis, d);
    }
  else
    {
      vmeWrite32(fastPrimDsr, p);
      vmeWrite32(fastRndmWDis, d);
    }
}

void
sfiRAMFBWCM(unsigned int p, unsigned int s, unsigned int d) 
{ 
  if(s) 
    {
      vmeWrite32(fastPrimCsrM, p);
      vmeWrite32(fastSecadW, s);
      vmeWrite32(fastRndmWDis, d);
    }
  else
    {
      vmeWrite32(fastPrimCsrM ,p);
      vmeWrite32(fastRndmWDis, d);
    }
}

void 
sfiRAMFBWDM(unsigned int p, unsigned int s, unsigned int d) 
{ 
  if(s) 
    {
      vmeWrite32(fastPrimDsrM, p);
      vmeWrite32(fastSecadW, s);
      vmeWrite32(fastRndmWDis, d);
    }
  else
    {
      vmeWrite32(fastPrimDsrM, p);
      vmeWrite32(fastRndmWDis, d);
    }
}

void
sfiRAMFBR()
{
  vmeWrite32(fastRndmR, 1);
}

void 
sfiRAMFBW(unsigned int d) 
{
  vmeWrite32(fastRndmW, d);
}

void
sfiRAMFBREL()
{
  vmeWrite32(fastDiscon, 1);
}

void
sfiRAMFBBR(unsigned int c)
{
  vmeWrite32(fastStartFrdbWithClearWc, (0x0a<<24)+c);
}

void
sfiRAMFBBRF(unsigned int c)
{
  vmeWrite32(fastStartFrdbWithClearWc, (0x08<<24)+c);
}

void
sfiRAMFBBRNC(unsigned int c)  
{
  vmeWrite32(fastStartFrdb, (0x0a<<24)+c);
}

void
sfiRAMFBBRNCF(unsigned int c) 
{
  vmeWrite32(fastStartFrdb, (0x08<<24)+c);
}

void
sfiRAMFBSWC()
{
  vmeWrite32(fastStoreFrdbWc, 1);
}

void
sfiRAMFBEND()  
{
  vmeWrite32(fastDisableRamMode, 1);
}




#else /* dummy version*/

void
libsfifb_dummy()
{
  return;
}

#endif
