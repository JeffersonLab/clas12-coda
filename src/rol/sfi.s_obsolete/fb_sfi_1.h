/*-----------------------------------------------------------------------------
 * Copyright (c) 1991,1992 Southeastern Universities Research Association,
 *                         Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: coda@cebaf.gov  Tel: (804) 249-7101  Fax: (804) 249-7363
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *	Fastbus Standard Routines Library for SFI, level 1
 *	
 * Author:  David Abbott, CEBAF Data Acquisition Group
 *
 * Revision History:
 *
 *
 *	This file contains low level routines to be called by the Fastbus
 *	standard routines to perform Fastbus actions on the STRUCK SFI.
 *
 *  Public Routines:
 *  ---------------
 *      fb_init_1()              fastbus initialization
 *
 *      fb_frd_1()               read data
 *      fb_fwd_1()               write data
 *      fb_frc_1()               read control
 *      fb_fwc_1()               write control
 *      fb_frdm_1()              read data multiple listener
 *      fb_fwdm_1()              write data multiple listener
 *      fb_frcm_1()              read control multiple listener
 *      fb_fwcm_1()              write control multiple listener
 *
 *      fb_frdb_1()              read data block
 */

/*======================================================================*/
/*
  Functions    : fb_init_1

  In           : SFI Base Address on VME as seen from CPU
		  
  Out          : returns 0

  Description  : Initialization of SFI/fastbus
*/
/*======================================================================*/

void 
fb_init_1(SFIAddr)

     unsigned int SFIAddr;
{

  if(SFIAddr > 0) 
    InitSFI(SFIAddr);
  SFI_ShowStatusReg();
  InitFastbus(0x20,0x33);

}



/*======================================================================*/
/*
  Functions    : FWC FRC

  In           : See generic input description at top of file
		  
  Out          : returns 0, if no error, else number of errors

  Description  : Single Cycle Write and Read routines to control space
*/
/*======================================================================*/

unsigned int 
fb_fwc_1(PAddr, SAddr, WData, EG, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
     unsigned int EG;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimCsr, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmW, WData);
    else
      vmeWrite32(fastRndmWDis, WData);
  }

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
fb_frc_1(PAddr, SAddr, rData, EG, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     volatile unsigned int *rData;
     unsigned int EG;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  volatile unsigned int *vaddr;
  unsigned int kk;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimCsr, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmR, reg32);
    else
      vmeWrite32(fastRndmRDis, reg32);
  }


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
	      while ( (kk<10) && ((reg32&0x00000010) == 0x00000010) ) {
		sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase); /* Dummy read */
		if(((unsigned int)vaddr & 0xF00)==0xF00)
		  vaddr = vaddr - (0xF00>>2);
		else
		  vaddr += (SFI_READ_ADDR_OFFSET>>2);
		reg32 = vmeRead32(vaddr);
		kk++;
	      }
	      if(kk > 1) {
		/* logMsg("fb_frc_1: kk = %d\n",kk,0,0,0,0,0); */
		if(kk >= 10) /* Still Empty */
		  {
		    /* Error !!! */
		    sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);
		    logMsg("fb_frc_1: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
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
  Functions    : FWD FRD

  In           : See generic input description at top of file
		  
  Out          : returns 0, if no error, else number of errors

  Description  : Single Cycle Write and Read routines to data space

*/
/*======================================================================*/

unsigned int 
fb_fwd_1(PAddr, SAddr, WData, EG, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
     unsigned int EG;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimDsr, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmW, WData);
    else
      vmeWrite32(fastRndmWDis, WData);
  }

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
fb_frd_1(PAddr, SAddr, rData, EG, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int *rData;
     unsigned int EG;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  unsigned int kk;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimDsr, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmR, reg32);
    else
      vmeWrite32(fastRndmRDis, reg32);
  }


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
		  logMsg("fb_frd_1: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
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
  Functions    : FWCM FRCM

  In           : See generic input description at top of file
		  
  Out          : returns 0, if no error, else number of errors

  Description  : Single Cycle Write and Read routines to control space
  for Multi-Listeners.
*/
/*======================================================================*/

unsigned int 
fb_fwcm_1(PAddr, SAddr, WData, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimCsrM, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmW, WData);
    else
      vmeWrite32(fastRndmWDis, WData);
  }

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
fb_frcm_1(PAddr, SAddr, rData, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     volatile unsigned int *rData;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  volatile unsigned int *vaddr;
  volatile unsigned int *vfaddr;
  unsigned int kk;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimCsrM, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmR, reg32);
    else
      vmeWrite32(fastRndmRDis, reg32);
  }

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
	      vfaddr = sfi.readSeq2VmeFifoBase;
	      vaddr += (SFI_READ_ADDR_OFFSET>>2);
	      reg32 = vmeRead32(vaddr);
	      while ( (kk<10) && ((reg32&0x00000010) == 0x00000010) ) {
		sfiSeq2VmeFifoVal = vmeRead32(vfaddr++);    /* Dummy read */
		if(((unsigned int)vaddr & 0xF00)==0xF00)
		  vaddr = vaddr - (0xF00>>2);
		else
		  vaddr += (SFI_READ_ADDR_OFFSET>>2);
		reg32 = vmeRead32(vaddr);
		kk++;
	      }
	      if(kk > 1) {
		/* logMsg("fb_frcm_1: kk = %d\n",kk,0,0,0,0,0); */
		if(kk >= 10) /* Still Empty */
		  {
		    /* Error !!! */
		    sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);
		    logMsg("fb_frcm_1: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
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
  Functions    : FWDM FRDM

  In           : See generic input description at top of file
		  
  Out          : returns 0, if no error, else number of errors

  Description  : Single Cycle Write and Read routines to data space
                 for Multi-Listeners.
*/
/*======================================================================*/

unsigned int 
fb_fwdm_1(PAddr, SAddr, WData, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int WData;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimDsrM, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmW, WData);
    else
      vmeWrite32(fastRndmWDis, WData);
  }

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
fb_frdm_1(PAddr, SAddr, rData, noarb, nopa, nosa, nodata, holdas, hold)

     unsigned int PAddr;
     unsigned int SAddr;
     unsigned int *rData;
     unsigned int noarb;
     unsigned int nopa;
     unsigned int nosa;
     unsigned int nodata;
     unsigned int holdas;
     unsigned int hold;

{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;

  unsigned int kk;

  SFILOCK;
  if (!nopa)
    vmeWrite32(fastPrimDsrM, PAddr); /* primary address cycle   */
  if (!nosa)
    vmeWrite32(fastSecadW, SAddr); /* secondary address cycle */ 

  if(!nodata) {
    if (holdas)
      vmeWrite32(fastRndmR, reg32);
    else
      vmeWrite32(fastRndmRDis, reg32);
  }


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
		  logMsg("fb_frdm_1: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
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


unsigned int
fb_frdb_1(PAddr, SAddr, Buffer, MaxBytes, RetBytes, noarb, nopa, nosa, noda, pipe, holdas, hold, Wait)

     unsigned int PAddr;           /* Primary Address */
     unsigned int SAddr;	       /* Secondary Address */
     unsigned int *Buffer;         /* Address of LWord-Buffer (VME-Slave Address!!! )*/
     unsigned int MaxBytes;        /* Max. Count of bytes transfered  */
     volatile unsigned int *RetBytes;       /* Actual byte count of transfer */
     unsigned int noarb;           /* No arbitration cycle*/
     unsigned int nopa;            /* No Primary address cycle */
     unsigned int nosa;            /* No Secondary address cycle */
     unsigned int noda;            /* No data cycle */
     unsigned int pipe;            /* Readout-Modus (Direct and/or VME) */
                                    /* 0x10 only Direct            */
                                    /* 0x09 VMED32 Data cycle      */
                                    /* 0x0A VMED32 Blocktransfer   */
                                    /* 0x0C VMED64 Blocktransfer   */
                                    /* 0x19 Direct and VME32Datacycl. */
                                    /* 0x1A Direct and VME32Blocktr.  */
                                    /* 0x1C Direct and VME64Blocktr.  */
     unsigned int holdas;          /* Hold address lock              */
     unsigned int hold;            /* Hold address lock and bus      */
     unsigned int Wait;            /* Count for CPU idle loop before polling the Sequencer status */
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;
  unsigned int ii,kk;
  unsigned int Max_ExpLWord = 0;

  volatile unsigned int *vaddr;

  Max_ExpLWord = (MaxBytes>>2)&(0x00ffffff);
  Max_ExpLWord |= (pipe << 24);

  SFILOCK;
  if(!noda) {
    vmeWrite32(fastLoadDmaAddressPointer, (unsigned int) Buffer); /* Load local addr pointer */

    if(!nopa)
      vmeWrite32(fastPrimDsr, PAddr);                /* primary address cycle   */

    if(!nosa)
      vmeWrite32(fastSecadW, SAddr);                /* secondary address cycle */  

    vmeWrite32(fastStartFrdbWithClearWc, Max_ExpLWord);           /* start dma               */

    if(!holdas)
      vmeWrite32(fastDiscon, reg32);                /* disconnect              */

    vmeWrite32(fastStoreFrdbWc, reg32);                /* get DMA Transfer Word Count */
    vmeWrite32(fastStoreFrdbWc, 1);                /* get DMA Transfer Word Count */
  }

  /* wait for sequencer done */
  for (ii=0;ii<Wait;ii++)
    {
      kk = ii*ii;
    }
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
	  *RetBytes = 0;

	  /* Word Count is in SEQ2VME FIFO */
	  /* check if EMPTY ; if EMPTY then Dummy read and Loop until not Empty */
	  /*                   else if not empty then read fifo */
	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) == 0x00000010)  /* Empty */
	    {
	      kk=0;
	      vaddr = sfi.readSeqFifoFlags;
	      vaddr += (SFI_READ_ADDR_OFFSET>>2);
	      reg32 = vmeRead32(vaddr);
	      while ( (kk<100) && ((reg32&0x00000010) == 0x00000010) ) {
		sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase); /* Dummy read */
		if(((unsigned int)vaddr & 0xF00)==0xF00)
		  vaddr = vaddr - (0xF00>>2);
		else
		  vaddr += (SFI_READ_ADDR_OFFSET>>2);
		reg32 = vmeRead32(vaddr);
		kk++;
	      }
	      if(kk > 1) {
		/* logMsg("fb_frdb_1: kk = %d\n",kk,0,0,0,0,0); */
		if(kk >= 100) /* Still Empty */
		  {
		    /* Error !!! */
		    logMsg("fb_frdb_1: Empty Fifo Status=0x%x Val=0x%x\n",reg32,sfiSeq2VmeFifoVal,0,0,0,0);
		    Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		    Return |= 0x00100000;
		    break;
		  }
	      }
	    }
	  /* read fifo */
	  sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);
	  kk=0;
	  while ( (kk<10) &&
		 ( ((sfiSeq2VmeFifoVal&SFI_DMA_MASK) != SFI_DMA_LIMIT)
		  && ((sfiSeq2VmeFifoVal&SFI_DMA_MASK) != SFI_DMA_SS2) ) ) {
	    sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);   /* Read Again */
	    kk++;
	  }
	  if (kk>=10) {
	    logMsg("fb_rdb_1: Bad Fifo Value Val=0x%x\n",sfiSeq2VmeFifoVal,0,0,0,0,0);
	    Return = (vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff);
	    Return |= 0x00060000;
	    break;
	  } else {
#ifdef DEBUG
	    printf("%s: kk = %d   sfiSeq2VmeFifoVal = 0x%08x\n",
		   __FUNCTION__,kk,sfiSeq2VmeFifoVal);
#endif
	    *RetBytes = (sfiSeq2VmeFifoVal&0xffffff)<<2;
	  }


	  /* Read once more and Check for FIFO Empty (should be empty now) */
	  sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);
	  reg32 = vmeRead32(sfi.readSeqFifoFlags);
	  if((reg32 & 0x00000010) != 0x00000010) /* Not Empty */
            {
	      /* Error !!! */
	      /* Read until empty (up to 20 times) then return error */
	      kk=0;
	      while ( (kk<20) && ((reg32&0x00000010) != 0x00000010) ) {
		sfiSeq2VmeFifoVal = vmeRead32(sfi.readSeq2VmeFifoBase);
		reg32 = vmeRead32(sfi.readSeqFifoFlags);
		kk++;
	      }
	      if (kk >= 20) /* Still Not Empty */
		{
		  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
		  Return |= 0x00040000;
		  break;
		}
	      if (kk>1) logMsg("fb_rdb_1: Extra Words in fifo kk=%d  flags=%x\n",kk,reg32,0,0,0,0);
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
