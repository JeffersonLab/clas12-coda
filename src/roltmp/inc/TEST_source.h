/******************************************************************************
*
* header file for use with UNIX rols with CODA crl (version 2.0)
*
*                             DJA   March 1996
*
*******************************************************************************/
#ifndef __TEST_ROL__
#define __TEST_ROL__

static int TEST_handlers,TESTflag;
static int TEST_isAsync;
static unsigned long TEST_prescale = 1;
static unsigned long TEST_count = 0;


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
testtenable(int code, int val)
{
  printf("testtenable reached: code=%d val=%d\n",code,val);fflush(stdout);
  TESTflag = 1;
}

static void 
testtdisable(int code, int val)
{
  printf("testtdisable reached\n");fflush(stdout);
  TESTflag = 0;
}

static unsigned long 
testttype(int code)
{
  return(1);
}

static int 
testttest(int code)
{

#ifdef USE_GTP

  unsigned int nev;
  if(TESTflag)
  {
    /*printf("testttest: befor gtpNFIFOEvents()\n");fflush(stdout);*/
    nev = gtpNFIFOEvents();
    /*printf("testttest: after gtpNFIFOEvents()\n");fflush(stdout);*/

    if(nev > 0)
    {
      return(1);
    }
    else
    {
      return(0);
    }
  }
  else
  {
    return(0);
  }

#else

  TEST_count++;
  /*printf("TEST_count=%d\n",TEST_count);*/

  if(TESTflag && ((TEST_count%TEST_prescale) == 0))
  {
    return(1);
  }
  else
  {
    return(0);
  }

#endif

}


/* define CODA readout list specific routines/definitions */


#define TEST_TEST  testttest

#define TEST_INIT { TEST_handlers =0;TEST_isAsync = 0;TESTflag = 0;}

#define TEST_ASYNC(code)  {printf("No Async mode is available for TEST\n"); \
                           TEST_handlers=1; TEST_isAsync = 0;}

#define TEST_SYNC(code)   {TEST_handlers=1; TEST_isAsync = 0;}

#define TEST_SETA(code) TESTflag = code;

#define TEST_SETS(code) TESTflag = code;

#define TEST_ENA(code,val) testtenable(code,val);

#define TEST_DIS(code,val) testtdisable(code,val);

#define TEST_CLRS(code) TESTflag = 0;

#define TEST_TTYPE testttype

#define TEST_START(val)	 {;}

#define TEST_STOP(val)	 {;}

#define TEST_ENCODE(code) (code)


#endif

