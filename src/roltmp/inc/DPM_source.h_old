/******************************************************************************
*
* header file for use with UNIX rols with CODA crl (version 2.0)
*
*                             DJA   March 1996
*
*******************************************************************************/
#ifndef __DPM_ROL__
#define __DPM_ROL__

static int DPM_handlers,DPMflag;
static int DPM_isAsync;
static unsigned long DPM_prescale = 1;
static unsigned long DPM_count = 0;

static int              myfd;
static unsigned int *   map;

#include<time.h>

static unsigned int pollCount;

static time_t lastTime;
static time_t trigCount;
static time_t ackTrig;
static time_t lastCount;
static unsigned int     lastSize;

#define MEM_MAP_BASE 0xA0010000
#define MEM_MAP_SIZE 0x00002000

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
dpmtenable(int code, int val)
{
  printf("dpmtenable reached: code=%d val=%d\n",code,val);fflush(stdout);
  DPMflag = 1;
}

static void 
dpmtdisable(int code, int val)
{
  printf("dpmtdisable reached\n");fflush(stdout);
  DPMflag = 0;
}

static unsigned long 
dpmttype(int code)
{
  return(1);
}

static int 
dpmttest(int code)
{

   if ( pollCount < 10 ) {
      printf("Startup Polling\n");
      usleep(1000);
   }
   pollCount++;

   time_t currTime;
   time(&currTime);
   if ( currTime != lastTime )
   {
      printf("Poll. Trig Count %i, Rate %i, Size=%i, Bw=%i Bps\n",trigCount,(trigCount-lastCount),lastSize,((trigCount-lastCount)*lastSize));
      lastTime  = currTime;
      lastCount = trigCount;
   }

   char c;
   if ( read(myfd,&c,1) > 0 ) {
      trigCount++;
      return(1);
   }
   else return(0);
}


/* define CODA readout list specific routines/definitions */


#define DPM_TEST dpmttest

#define DPM_INIT { DPM_handlers =0;DPM_isAsync = 0;DPMflag = 0;}

#define DPM_ASYNC(code)  {printf("No Async mode is available for DPM\n"); \
                           DPM_handlers=1; DPM_isAsync = 0;}

#define DPM_SYNC(code)   {DPM_handlers=1; DPM_isAsync = 0;}

#define DPM_SETA(code) DPMflag = code;

#define DPM_SETS(code) DPMflag = code;

#define DPM_ENA(code,val) dpmtenable(code,val);

#define DPM_DIS(code,val) dpmtdisable(code,val);

#define DPM_CLRS(code) DPMflag = 0;

#define DPM_TTYPE dpmttype

#define DPM_START(val)	 {;}

#define DPM_STOP(val)	 {;}

#define DPM_ENCODE(code) (code)


#endif

