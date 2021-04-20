/******************************************************************************
*
* header file for use with UNIX rols with CODA crl (version 2.0)
*
*                             DJA   March 1996
*
*******************************************************************************/
#ifndef __DTM_ROL__
#define __DTM_ROL__

static int DTM_handlers,DTMflag;
static int DTM_isAsync;
static unsigned long DTM_prescale = 1;
static unsigned long DTM_count = 0;

#include <time.h>

/*
struct vme_ts {
    unsigned long csr;       
    unsigned long trig;
    unsigned long roc;
    unsigned long sync;
    unsigned long test;
    unsigned long state;
    unsigned long blank_1;           
    unsigned long blank_2;          
    unsigned long prescale[8];
    unsigned long timer[5];
    unsigned long blank_3;          
    unsigned long blank_4;          
    unsigned long blank_5;           
    unsigned long sc_as;
    unsigned long scale_0a;
    unsigned long scale_1a;
    unsigned long blank_6;          
    unsigned long blank_7;          
    unsigned long scale_0b;
    unsigned long scale_1b;
  };

struct vme_tir {
    unsigned short tir_csr;
    unsigned short tir_vec;
    unsigned short tir_dat;
    unsigned short tir_oport;
    unsigned short tir_iport;
  };


struct vme_ts  *ts;
struct vme_tir *tir[2];

long *tsmem;
long ts_memory[4096];
long preset_count;

long *vme2_ir;
*/


#ifdef USE_GTP
unsigned int gtpNFIFOEvents();
#endif

/*----------------------------------------------------------------------------*/
 /* test_trigLib.c -- Dummy trigger routines for UNIX based ROLs

 File : test_trigLib.h

 Routines:
	   void testtenable();        enable trigger
	   void testtdisable();       disable trigger
	   char testttype();          read trigger type
	   int  testttest();          test for trigger (POLL)
------------------------------------------------------------------------------*/


static void 
dtmtenable(int code, int val)
{
  printf("dtmtenable reached: code=%d val=%d\n",code,val);fflush(stdout);
  DTMflag = 1;
}

static void 
dtmtdisable(int code, int val)
{
  printf("dtmtdisable reached\n");fflush(stdout);
  DTMflag = 0;
}

static unsigned long 
dtmttype(int code)
{
  return(1);
}

static int 
dtmttest(int code)
{
  return(0);
}


/* define CODA readout list specific routines/definitions */


#define DTM_TEST dtmttest

#define DTM_INIT { DTM_handlers =0;DTM_isAsync = 0;DTMflag = 0;}

#define DTM_ASYNC(code)  {printf("No Async mode is available for DTM\n"); DTM_handlers=1; DTM_isAsync = 0;}

#define DTM_SYNC(code)   {DTM_handlers=1; DTM_isAsync = 0;}

#define DTM_SETA(code) DTMflag = code;

#define DTM_SETS(code) DTMflag = code;

#define DTM_ENA(code,val) dtmtenable(code,val);

#define DTM_DIS(code,val) dtmtdisable(code,val);

#define DTM_CLRS(code) DTMflag = 0;

#define DTM_TTYPE dtmttype

#define DTM_START(val)	 {;}

#define DTM_STOP(val)	 {;}

#define DTM_ENCODE(code) (code)


#endif

