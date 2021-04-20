/******************************************************************************
*
* header file for use with UNIX rols with CODA crl (version 2.0)
*
*                             DJA   March 1996
*
*******************************************************************************/
#ifndef __VTP_ROL__
#define __VTP_ROL__

#define DAQ_READ_CONF_FILE  {daqSetExpid(expid);                        daqConfig("");     if(strncasecmp(rol->confFile,"none",4)) daqConfig(rol->confFile);}
#define VTP_READ_CONF_FILE {vtpSetExpid(expid); vtpInitGlobals(); vtpConfig(""); if(strncasecmp(rol->confFile,"none",4)) vtpConfig(rol->confFile);}

extern char *mysql_host; /* defined in coda_component.c */
extern char *expid; /* defined in coda_component.c */

#define POLLING_VTP


static int VTP_handlers,VTPflag;
static int VTP_isAsync;
static unsigned long VTP_prescale = 1;
static unsigned long VTP_count = 0;


unsigned int vtpNFIFOEvents();


/*----------------------------------------------------------------------------*/
 /* vtp_trigLib.c -- Dummy trigger routines for UNIX based ROLs

 File : vtp_trigLib.h

 Routines:
	   void gtotenable();        enable trigger
	   void vtptdisable();       disable trigger
	   char vtpttype();          read trigger type
	   int  vtpttest();          test for trigger (POLL)
------------------------------------------------------------------------------*/

void
VTP_int_handler(int arg)
{
  theIntHandler(VTP_handlers);                   /* Call our handler */
}


static void
vtptinit(int code)
{

#if 0
  /* initialize VTP and set clock source disabled until prestart */
  vtpInit(0);
  vtpSetClock(0);
  vtpPayloadTriggerReset(0xFFFF);
#endif

  return;
}


static void
vtptriglink(int code, VOIDFUNCPTR isr)
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
        printf(" vtptriglink: ERROR: ts uninitialized\n");
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
      printf("vtptriglink: ERROR: source type %d undefined\n",code);
      return;
  }

  printf("vtptriglink: register int handler 0x%08x\n",isr);fflush(stdout);
}

static void 
vtptenable(int code, int val)
{
  printf("vtptenable reached: code=%d val=%d\n",code,val);fflush(stdout);
  VTPflag = 1;
}

static void 
vtptdisable(int code, int val)
{
  printf("vtptdisable reached\n");fflush(stdout);
  VTPflag = 0;
}

static void 
vtptack(int code, unsigned int intMask)
{
  /*if(code == TIR_SOURCE)*/
  {
    /*printf("TI_PRIMARY: call tiIntAck()\n");*/

#if 1
    vtpTiAck(1); /* argument=1 forces TI to clear sync event flag */
#ifndef POLLING_VTP
    vtpEnableInt(1);
#endif
#endif

  }
}

static unsigned long 
vtpttype(int code)
{
  return(1);
}

static int 
vtpttest(int code)
{
  int val=0;
  if(VTPflag)
  {


#if 1
    /*printf("vtpttest: befor vtpBReady()\n");fflush(stdout);*/
#ifdef POLLING_VTP
	val = vtpBReady();
#else
	if(vtpWaitForInt()) val = vtpBReady();
#endif
    /*printf("vtpttest: after vtpBReady(): val=%d\n",val);fflush(stdout);*/

    if(val > 0)
    {
      syncFlag = 0/*vtpGetSyncEventFlag()*/;
      return(1);
    }
    else
    {
      /*usleep(1000);*/
      syncFlag = 0;
      return(0);
    }



#endif
    return(0);



  }
  else
  {
    return(0);
  }



}


/* define CODA readout list specific routines/definitions */


#define VTP_TEST  vtpttest

#define VTP_INIT(code) {VTP_handlers=0; VTP_isAsync=0; VTPflag=0; vtptinit(code);}

#define VTP_ASYNC(code)  {VTP_handlers=1; VTP_isAsync=0; /*vtptriglink(code,VTP_int_handler);*/}

#define VTP_SYNC(code)   {VTP_handlers=1; VTP_isAsync=0;}

#define VTP_SETA(code) VTPflag = code;

#define VTP_SETS(code) VTPflag = code;

#define VTP_ENA(code,val) vtptenable(code,val);

#define VTP_DIS(code,val) vtptdisable(code,val);

#define VTP_ACK(code,val) vtptack(code,val);

#define VTP_CLRS(code) VTPflag = 0;

#define VTP_TTYPE vtpttype

#define VTP_START(val)	 {;}

#define VTP_STOP(val)	 {;}

#define VTP_ENCODE(code) (code)


#endif

