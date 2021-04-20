/****************************************************************
 *
 * header file for use of Struck SFI 340 with CODA crl
 *
 *                                DJA   March 1996
 *
 *****************************************************************/


volatile struct sfiStruct sfi;         /* global parameter block */

unsigned int sfiSeq2VmeFifoVal;

/* constants for fast access to fastbus functions */
/* all these addresses are found in the write-to-vme2seq-address-space */
volatile unsigned int *fastPrimDsr;
volatile unsigned int *fastPrimCsr;
volatile unsigned int *fastPrimDsrM;
volatile unsigned int *fastPrimCsrM;
volatile unsigned int *fastPrimHmDsr;
volatile unsigned int *fastPrimHmCsr;
volatile unsigned int *fastPrimHmDsrM;
volatile unsigned int *fastPrimHmCsrM;
volatile unsigned int *fastSecadR;
volatile unsigned int *fastSecadW;
volatile unsigned int *fastSecadRDis;
volatile unsigned int *fastSecadWDis;
volatile unsigned int *fastRndmR;
volatile unsigned int *fastRndmW;
volatile unsigned int *fastRndmRDis;
volatile unsigned int *fastRndmWDis;
volatile unsigned int *fastDiscon;
volatile unsigned int *fastDisconRm;
volatile unsigned int *fastStartFrdbWithClearWc;
volatile unsigned int *fastStartFrdb;
volatile unsigned int *fastStoreFrdbWc;
volatile unsigned int *fastStoreFrdbAp;
volatile unsigned int *fastLoadDmaAddressPointer;
volatile unsigned int *fastDisableRamMode;
volatile unsigned int *fastEnableRamSequencer;
volatile unsigned int *fastWriteSequencerOutReg;
volatile unsigned int *fastDisableSequencer;


/*======================================================================*/
/*
  Function     : SFI_ShowStatusReg

  In           : -
  Out          : -

  Description  : displays sfi sequencer/FASTBUS status register set

*/
/*======================================================================*/

void SFI_ShowStatusReg()
{
  SFILOCK;
  printf("Sequencer Status Register = 0x%08x\n",
	 vmeRead32(sfi.sequencerStatusReg));
  printf("Last Sequencer KeyAddress = 0x%08x\n",
	 vmeRead32(sfi.lastSequencerProtocolReg));
  printf("Fastbus Status Register1  = 0x%08x\n",
	 vmeRead32(sfi.FastbusStatusReg1));
  printf("Fastbus Status Register2  = 0x%08x\n",
	 vmeRead32(sfi.FastbusStatusReg2));
  printf("Fastbus Last Primary Addr = 0x%08x\n",
	 vmeRead32(sfi.fastbusreadback));
  SFIUNLOCK;
}

/*======================================================================*/
/*
  Funktion     : GetFastbusPtr

  In           : keyAddress - function key address on SFI
  Out          : returns    - absolute memory address of this function

  Bemerkungen  :
*/
/*======================================================================*/

unsigned int *GetFastbusPtr(keyAddress)
     unsigned int keyAddress;
{
  unsigned int address;

  address = (unsigned int) sfi.writeVme2SeqFifoBase;
  address += keyAddress;

  /*       printf("generated address 0x%08x\n",address); */

  return ((unsigned int*) (address));
}

/*======================================================================*/
/*
  Funktion     : InitSFI

  In           : 32 bit Base address for SFI board
  Out          : -

  Bemerkungen  : initializes program parameters
*/
/*======================================================================*/


STATUS 
InitSFI(unsigned int SFIBaseAddr)
{
  unsigned int laddr;
  int res;

  /* Assume Default Base Address */
  if (SFIBaseAddr <= 0)
    SFIBaseAddr = SFI_VME_BASE_ADDR;
    
  /* Check for valid address */
  if(SFIBaseAddr < 0x00010000)  /* A16 Addressing */
    {
      printf("InitSFI: ERROR: A16 Addressing not allowed for the SFI\n");
      return(ERROR);
    }
  else
    {
      if(SFIBaseAddr > 0x00ffffff) 
	{
	  /* 32 bit address - assume it is already translated */
	  sfi.VMESlaveAddress = SFIBaseAddr;      
	} 
      else 
	{
#ifdef VXWORKS
	  res = sysBusToLocalAdrs(0x39,(char *)SFIBaseAddr,(char **)&laddr);
#else
	  res = vmeBusToLocalAdrs(0x39,(char *)SFIBaseAddr,(char **)&laddr);
#endif
	  if (res != 0) 
	    {
#ifdef VXWORKS
	      printf("InitSFI: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",SFIBaseAddr);
#else
	      printf("InitSFI: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",SFIBaseAddr);
#endif
#define TEST
#ifndef TEST
	      return(ERROR);
#else
	      laddr = SFIBaseAddr;
#endif
	    }
	  sfi.VMESlaveAddress = laddr;
	}
    }


  /* Setup pointers */
  sfi.writeVme2SeqFifoBase = (unsigned int *)
    ( sfi.VMESlaveAddress + SFI_WRITE_VME2SEQ_FIFO_BASE);
  
  sfi.readSeq2VmeFifoBase = (unsigned int *)
    ( sfi.VMESlaveAddress + SFI_READ_SEQ2VME_FIFO_BASE);
  
  
  sfi.sequencerOutputFifoSize = 1024;
  
  sfi.sequencerReset = (unsigned int*) 
    ( sfi.VMESlaveAddress + SFI_SEQUENCER_RESET);
  
  sfi.sequencerEnable = (unsigned int*) 
    ( sfi.VMESlaveAddress + SFI_SEQUENCER_ENABLE);
  
  sfi.sequencerDisable = (unsigned int*) 
    ( sfi.VMESlaveAddress + SFI_SEQUENCER_DISABLE);
  
  sfi.sequencerRamLoadEnable = (unsigned int*) 
    ( sfi.VMESlaveAddress + SFI_SEQUENCER_RAM_LOAD_ENABLE);
  
  sfi.sequencerRamLoadDisable = (unsigned int*) 
    ( sfi.VMESlaveAddress + SFI_SEQUENCER_RAM_LOAD_DISABLE);
  
  sfi.sequencerTryAgain = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_SEQUENCER_TRY_AGAIN);
  
  sfi.readLocalFbAdBus = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_READ_LOCAL_FB_AD_BUS);
  
  sfi.readVme2SeqDataFifo = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_READ_VME2SEQ_DATA_FIFO);
  
  sfi.readSeqFifoFlags = (unsigned int *)
    ( sfi.VMESlaveAddress + SFI_SEQUENCER_FIFO_FLAG_AND_ECL_NIM_INPUT_REGISTER);
  
  
  sfi.writeAuxReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_WRITE_AUX_REGISTER);
  
  sfi.generateAuxB40Pulse = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_GENERATE_AUX_B40_PULSE);
  
  sfi.writeVmeOutSignalReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_WRITE_VME_OUT_SIGNAL_REGISTER);
  
  sfi.clearBothLca1TestReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_CLEAR_BOTH_LCA1_TEST_REGISTER);
  
  sfi.writeVmeTestReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_WRITE_VME_TEST_REGISTER);
  
  sfi.FastbusArbitrationLevelReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_FASTBUS_ARBITRATION_LEVEL_REGISTER);
  
  sfi.FastbusProtocolInlineReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_FASTBUS_PROTOCOL_INLINE_REGISTER);
  
  sfi.sequencerFifoFlagAndEclNimInputReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_SEQUENCER_FIFO_FLAG_AND_ECL_NIM_INPUT_REGISTER);
  
  sfi.nextSequencerRamAddressReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_NEXT_SEQUENCER_RAM_ADDRESS_REGISTER);
  
  sfi.lastSequencerProtocolReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_LAST_SEQUENCER_PROTOCOL_REGISTER);
  
  sfi.sequencerStatusReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_SEQUENCER_STATUS_REGISTER);
  
  sfi.FastbusStatusReg1 = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_FASTBUS_STATUS_REGISTER1);
  
  sfi.FastbusStatusReg2 = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_FASTBUS_STATUS_REGISTER2);
  
  sfi.fastbusreadback = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_READ_LAST_FB_PRIM_ADDR_REGISTER);

  sfi.FastbusTimeoutReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_FASTBUS_TIMEOUT_REGISTER);
  
  sfi.FastbusArbitrationLevelReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_FASTBUS_ARBITRATION_LEVEL_REGISTER);
  
  sfi.VmeIrqLevelAndVectorReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_VME_IRQ_LEVEL_AND_VECTOR_REGISTER);
  
  sfi.VmeIrqMaskReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_VME_IRQ_MASK_REGISTER);
  
  sfi.sequencerRamAddressReg = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_SEQUENCER_RAM_ADDRESS_REGISTER);
  
  sfi.resetRegisterGroupLca2 = (unsigned int *)
    (sfi.VMESlaveAddress + SFI_RESET_REGISTER_GROUP_LCA2);
  
  printf("resetRegisterGroupLca2 = 0x%08x\n",(unsigned int)
	 sfi.resetRegisterGroupLca2);
  printf("FastbusTimeoutRegister = 0x%08x\n",(unsigned int)
	 sfi.FastbusTimeoutReg);
  printf("FastbusdArbitrationLevelReg = 0x%08x\n",(unsigned int)
	 sfi.FastbusArbitrationLevelReg);
  
  
  
  /* intialize global variables for fast FASTBUS access */
  fastPrimDsr     = GetFastbusPtr(PRIM_DSR);
  printf("fastPrimDsr = 0x%08x\n", (unsigned int) fastPrimDsr);
  fastPrimCsr     = GetFastbusPtr(PRIM_CSR);
  fastPrimDsrM    = GetFastbusPtr(PRIM_DSRM);
  fastPrimCsrM    = GetFastbusPtr(PRIM_CSRM);
  fastPrimHmDsr   = GetFastbusPtr(PRIM_HM_DSR);
  fastPrimHmCsr   = GetFastbusPtr(PRIM_HM_CSR);
  fastPrimHmDsrM  = GetFastbusPtr(PRIM_HM_DSRM);
  fastPrimHmCsrM  = GetFastbusPtr(PRIM_HM_CSRM);
  fastSecadR      = GetFastbusPtr(SECAD_R);
  fastSecadW      = GetFastbusPtr(SECAD_W);
  fastSecadRDis   = GetFastbusPtr(SECAD_R_DIS);
  fastSecadWDis   = GetFastbusPtr(SECAD_W_DIS);
  fastRndmR       = GetFastbusPtr(RNDM_R);
  fastRndmW       = GetFastbusPtr(RNDM_W);
  fastRndmRDis    = GetFastbusPtr(RNDM_R_DIS);
  fastRndmWDis    = GetFastbusPtr(RNDM_W_DIS);
  fastDiscon      = GetFastbusPtr(DISCON);
  fastDisconRm    = GetFastbusPtr(DISCON_RM);
  fastStartFrdbWithClearWc = GetFastbusPtr(START_FRDB_WITH_CLEAR_WORD_COUNTER);
  fastStartFrdb   = GetFastbusPtr(START_FRDB);
  fastStoreFrdbWc = GetFastbusPtr(STORE_FRDB_WC);
  fastStoreFrdbAp = GetFastbusPtr(STORE_FRDB_AP);
  fastLoadDmaAddressPointer = GetFastbusPtr(LOAD_DMA_ADDRESS_POINTER);
  /* initialize global variables for RAM LIST Sequencer functions */
  fastDisableRamMode = GetFastbusPtr(DISABLE_RAM_MODE);
  fastEnableRamSequencer = GetFastbusPtr(ENABLE_RAM_SEQUENCER);
  fastWriteSequencerOutReg = GetFastbusPtr(WRITE_SEQUENCER_OUT_REG);
  fastDisableSequencer = GetFastbusPtr(DISABLE_SEQUENCER);


  return(OK);
}


/*======================================================================*/
/*
  Funktion     : InitFastbus

  In           :       arbReg - value for arbitration level register
  timReg - value for arbitration timeout register
  Out          : -

  Bemerkungen  : reset fastbus sequencer and prepare for new action
*/
/*======================================================================*/

void InitFastbus(arbReg, timReg)
     unsigned int arbReg, timReg;
{
  unsigned int val32out;

  /* reset sequencer */ 
  SFILOCK;
  val32out = 0L;
  vmeWrite32(sfi.sequencerReset, val32out); 

  /* set arbitration level */
  vmeWrite32(sfi.FastbusArbitrationLevelReg, arbReg);

  /* set timeout register  */
  vmeWrite32(sfi.FastbusTimeoutReg, timReg);

  /* enable sequencer */
  vmeWrite32(sfi.sequencerEnable, val32out);

  SFIUNLOCK;
}

/*======================================================================*/
/*
  Function     : sfi_pulse

  In           : pmode - setup pulse mode
  0 - default (set and unset output)
  1 - set output
  2 - unset output
  pmask - mask for which outputs to pulse 
  pcount - number of pulses to generate

  Out          : -

  Description  : General utility to manipulate the SFI output register
*/
/*======================================================================*/
void
sfi_pulse(pmode, pmask, pcount)
     unsigned int pmode, pmask, pcount;
{
  int ii, setMask, clearMask;

  SFILOCK;
  if (pmask) 
    {
      setMask = pmask;
      clearMask = pmask<<16;
      switch (pmode) 
	{
	case 0:
	  for(ii=0; ii<pcount; ii++) 
	    {
	      vmeWrite32(sfi.writeVmeOutSignalReg, setMask);
	      vmeWrite32(sfi.writeVmeOutSignalReg, clearMask);
	    }
	  break;
	case 1:
	  vmeWrite32(sfi.writeVmeOutSignalReg, setMask);
	  break;
	case 2:
	  vmeWrite32(sfi.writeVmeOutSignalReg, clearMask);
	  break;
	default:
	  ;
	}
    }
  SFIUNLOCK;

}

/*======================================================================*/
/*
  Function     : sfi_error_decode

  In           : pflag - set various output/reset options
  0 - default 
  1 - suppress messages
  2 - force reset/enable of sequencer
  3 - suppress logMsg and reset/enable
  Out          : -

  Description  : Decode error from last sequencer failure , reset the
  sequencer and pend a message to LogMsg in VxWorks.
*/
/*======================================================================*/

void
sfi_error_decode(pflag)
     int pflag;
{
  unsigned int seqStatus, seqKeyAddr;
  unsigned int fbStatus1=0, fbStatus2=0, paddr=-1;
  int ss,fb1=0,fb2=0;
  int reset = 0;
  int enable = 0;

  /* First check Sequencer Status Register for error type */
  SFILOCK;
  seqKeyAddr = vmeRead32(sfi.lastSequencerProtocolReg);
  seqStatus  = vmeRead32(sfi.sequencerStatusReg) & 0xffff;
  switch(seqStatus) 
    {
    case 0x8001:
    case 0xA001:
      /* logMsg("SFI_SEQ_INFO: Sequencer OK \n",0,0,0,0,0); */
      break;
    case 0x8000:
    case 0xA000:
      if(!(pflag&1)) logMsg("SFI_SEQ_ERR: Sequencer not Enabled\n",0,0,0,0,0,0);
      break;
    case 0x8011:
    case 0xA011:
      if(!(pflag&1)) logMsg("SFI_SEQ_ERR: Invalid Sequencer KeyAddress 0x%x\n",seqKeyAddr,0,0,0,0,0);
      reset = 1;
      enable = 1;
      break;
    case 0xc010:     
      if(!(pflag&1)) logMsg("SFI_CMD_ERR: Sequencer Command error: status=0x%x command = 0x%x\n",seqStatus,seqKeyAddr,0,0,0,0);
      reset = 1;
      enable = 1;
      break;
    case 0xc020:
      fb1=1;
      fbStatus1 = vmeRead32(sfi.FastbusStatusReg1) & 0xfff;
      paddr = vmeRead32(sfi.fastbusreadback);
      break;
    case 0xc040:
      fb2=1;
      fbStatus2 = vmeRead32(sfi.FastbusStatusReg2) & 0xffff;
      paddr = vmeRead32(sfi.fastbusreadback);
      break;
    case 0xc080:
      fb2=1;
      fbStatus2 = vmeRead32(sfi.FastbusStatusReg2) & 0xffff;
      paddr = vmeRead32(sfi.fastbusreadback);
      break;
    default:
      if(!(pflag&1)) logMsg("SFI_SEQ_ERR: Unknown Sequencer error: status=0x%x\n",seqStatus,0,0,0,0,0);
      break;
    }

  /* Now Determine if this is an Addressing Error or Data error */
  if(fb1)            /* Addressing Error */
    {
      switch((fbStatus1 & 0x28F)) 
	{
	case 0x200:
	  if(!(pflag&1)) logMsg("SFI_ADR_ERR: Fastbus AK timeout paddr=%d \n",paddr,0,0,0,0,0);
	  break;
	case 0x002:
	  if(!(pflag&1)) logMsg("SFI_ADR_ERR: Fastbus Arbitration timeout \n",0,0,0,0,0,0);
	  break;
	case 0x001:
	  if(!(pflag&1)) logMsg("SFI_ADR_ERR: Fastbus AS-AK lock already exists\n",0,0,0,0,0,0);
	  break;
	case 0x004:
	  if(!(pflag&1)) logMsg("SFI_ADR_ERR: Fastbus Master long timout\n",0,0,0,0,0,0);
	  break;
	case 0x008:
	  if(!(pflag&1)) logMsg("SFI_ADR_ERR: Fastbus Set AS timeout (WT High)\n",0,0,0,0,0,0);
	  break;
	case 0x080:
	  ss = (fbStatus1&0x70)>>4;
	  if(!(pflag&1)) logMsg("SFI_ADR_ERR: Fastbus SS!=0 paddr=%d SS=%d\n",paddr,ss,0,0,0,0);
	  break;
	default:
	  if(!(pflag&1)) logMsg("SFI_ADR_ERR: Fastbus Address Error status=%x \n",fbStatus1,0,0,0,0,0);
	}
      reset = 1;
      enable = 1;
    }
  else if(fb2)       /* Data Error       */
    {
      switch((fbStatus2 & 0x300F)) {
      case 0x0001:
	if(!(pflag&1)) logMsg("SFI_DAT_ERR: Fastbus No AS-AK lock paddr=%d \n",paddr,0,0,0,0,0);
	break;
      case 0x0004:
	if(!(pflag&1)) logMsg("SFI_DAT_ERR: Fastbus DS timeout (WT High)\n",0,0,0,0,0,0);
	break;
      case 0x0008:
	if(!(pflag&1)) logMsg("SFI_DAT_ERR: Fastbus DK long timeout\n",0,0,0,0,0,0);
	break;
      case 0x1000:
	if(!(pflag&1)) logMsg("SFI_DAT_ERR: Fastbus timeout during DMA\n",0,0,0,0,0,0);
	break;
      case 0x2000:
	if(!(pflag&1)) logMsg("SFI_DAT_ERR: VME Bus timeout during DMA\n",0,0,0,0,0,0);
	break;
      default:
	if(!(pflag&1)) logMsg("SFI_DAT_ERR: Fastbus Data error paddr=%d, status=0x%x\n",paddr,fbStatus2,0,0,0,0);
      }
      reset = 1;
      enable = 1;
    }

  if(pflag&0x2) 
    {
      reset = 1;
      enable = 1;
      if(!(pflag&1)) logMsg("** Resetting Sequencer ** \n",0,0,0,0,0,0);
    }
   
  /* reset sequencer */ 
  if(reset) vmeWrite32(sfi.sequencerReset, 1); 

  /* enable sequencer */
  if(enable) vmeWrite32(sfi.sequencerEnable, 1);

  SFIUNLOCK;
  return;
}

STATUS
checkSFIinit()
{
  if(sfi.VMESlaveAddress<=0) {
    logMsg("%s: ERROR: SFI not initialized\n",__FUNCTION__,2,3,4,5,6);
    return ERROR;
  }

  return OK;
}
