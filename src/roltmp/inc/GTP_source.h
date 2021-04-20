/******************************************************************************
*
* header file for use with UNIX rols with CODA crl (version 2.0)
*
*                             DJA   March 1996
*
*******************************************************************************/
#ifndef __GTP_ROL__
#define __GTP_ROL__

#define GTP_READ_CONF_FILE {gtpConfig("");if(strncmp(rol->confFile,"none",4) && strncmp(rol->confFile,"NONE",4)) gtpConfig(rol->confFile);}

/*
#define POLLING_GTP
*/

static int GTP_handlers,GTPflag;
static int GTP_isAsync;
static unsigned long GTP_prescale = 1;
static unsigned long GTP_count = 0;


unsigned int gtpNFIFOEvents();


/*----------------------------------------------------------------------------*/
 /* gtp_trigLib.c -- Dummy trigger routines for UNIX based ROLs

 File : gtp_trigLib.h

 Routines:
	   void gtotenable();        enable trigger
	   void gtptdisable();       disable trigger
	   char gtpttype();          read trigger type
	   int  gtpttest();          test for trigger (POLL)
------------------------------------------------------------------------------*/

void
GTP_int_handler(int arg)
{
  theIntHandler(GTP_handlers);                   /* Call our handler */
}


static void
gtptinit(int code)
{
  /* initialize Gtp and set clock source disabled until prestart */
  gtpInit(0);
  gtpSetClock(0);
  gtpPayloadTriggerReset(0xFFFF);

  return;
}


static void
gtptriglink(int code, VOIDFUNCPTR isr)
{

  switch(code)
  {
  case 1/*TIR_SOURCE*/:
      /* Initialize VME Interrupt variables
      tirClearIntCount();
      */
      /* Connect User Trigger Routine */
#ifdef VXWORKS	  
      tiIntConnect(TI_INT_VEC, isr, 0);
#endif
      break;
	  /*
    case TS_SOURCE:
      if(ts)
      {
        ts->intVec = TIR_INT_VEC;
      }
      else
      {
        printf(" gtptriglink: ERROR: ts uninitialized\n");
        return;
      }

#ifdef VXWORKSPPC
      if((intDisconnect(INUM_TO_IVEC(TI_INT_VEC)) !=0))
      {
        printf("Error disconnecting Interrupt\n");
      }
      intConnect(INUM_TO_IVEC(TI_INT_VEC),isr,0);
#endif

      break;
	  */

    default:
      printf("gtptriglink: ERROR: source type %d undefined\n",code);
      return;
  }

  printf("gtptriglink: register int handler 0x%08x\n",isr);fflush(stdout);
}

static void 
gtptenable(int code, int val)
{
  printf("gtptenable reached: code=%d val=%d\n",code,val);fflush(stdout);
  GTPflag = 1;
}

static void 
gtptdisable(int code, int val)
{
  printf("gtptdisable reached\n");fflush(stdout);
  GTPflag = 0;
}

static void 
gtptack(int code, unsigned int intMask)
{
  /*if(code == TIR_SOURCE)*/
  {
    /*printf("TI_PRIMARY: call tiIntAck()\n");*/
    gtpIntAck();
#ifndef POLLING_GTP
    gtpEnableInt(1);
#endif
  }
}

static unsigned long 
gtpttype(int code)
{
  return(1);
}

static int 
gtpttest(int code)
{
  int val=0;
  if(GTPflag)
  {
    /*printf("gtpttest: befor gtpBReady()\n");fflush(stdout);*/
#ifdef POLLING_GTP
	val = gtpBReady();
#else
	if(gtpWaitForInt())
		val = gtpBReady();
#endif
    /*printf("gtpttest: after gtpBReady(): val=%d\n",val);fflush(stdout);*/

    if(val > 0)
    {
      syncFlag = gtpGetSyncEventFlag();
      return(1);
    }
    else
    {
      /*usleep(1000);*/
      syncFlag = 0;
      return(0);
    }
  }
  else
  {
    return(0);
  }
}


/* define CODA readout list specific routines/definitions */


#define GTP_TEST  gtpttest

#define GTP_INIT(code) {GTP_handlers=0; GTP_isAsync=0; GTPflag=0; gtptinit(code);}

#define GTP_ASYNC(code)  {GTP_handlers=1; GTP_isAsync=0; /*gtptriglink(code,GTP_int_handler);*/}

#define GTP_SYNC(code)   {GTP_handlers=1; GTP_isAsync=0;}

#define GTP_SETA(code) GTPflag = code;

#define GTP_SETS(code) GTPflag = code;

#define GTP_ENA(code,val) gtptenable(code,val);

#define GTP_DIS(code,val) gtptdisable(code,val);

#define GTP_ACK(code,val) gtptack(code,val);

#define GTP_CLRS(code) GTPflag = 0;

#define GTP_TTYPE gtpttype

#define GTP_START(val)	 {;}

#define GTP_STOP(val)	 {;}

#define GTP_ENCODE(code) (code)


#endif

