/*
   adc1182 LeCroy VME board - Serguei Boiarinov
 
   TIPS for the 1st leadout list constructors:

1. Define addresses:

const ADCADR  = 0xe0390000
const ADCADR1 = 0xe0190000
..........................
 
2. In "Go" routine:

%%
  adc1182reset(ADCADR);
  adc1182reset(ADCADR1);
%%

3. In trigger routine:

%%
  while( adc1182cip(ADCADR) );
  for(i=0; i<len; i++) *rol->dabufp++ = adc1182read(ADCADR, i);

  while( adc1182cip(ADCADR1) );
  for(i=0; i<len; i++) *rol->dabufp++ = adc1182read(ADCADR1, i);
%%

  ....................................

   close event

%%
  adc1182reset(ADCADR);
  adc1182reset(ADCADR1);
%%

4. To create a GATE signal for the adc's output register of TI board
   can be used. Reset bit 0 it in "Go" transition (bit1 always set !)

  tir[1]->tir_oport = 0x2;  

   and then in the begining of trigger routine set and reset bit 0

  tir[1]->tir_oport = 0x3;
  tir[1]->tir_oport = 0x2;

   Bit 0 is used for adc gating in that example. Bit 1 is used
   for another purposes and should be set all time as shown.


   Another way to create GATE: take signal from BISY port of TI board.

   See TI board manual for details.

 */

#if defined(VXWORKS) || defined(Linux_vme)

#include <stdio.h>
#include <string.h>

#ifdef VXWORKS
#include <vxWorks.h>
/*sergey#include "vxCompat.h"*/
#else
#include <stddef.h>
#include <pthread.h>
#include "jvme.h"
#endif


#ifdef VXWORKS
#include <logLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <semLib.h>
#include <vxLib.h>
#else
#include <unistd.h>
#endif

#ifdef VXWORKS
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
#endif

typedef struct LeCroy1182CSR *CSRPtr;
typedef struct LeCroy1182CSR
{
  unsigned empty   : 6; /* D15-D10 - not implemented */
  unsigned test    : 1; /* D9 - Causes a test gate of 500 nsec ... */
  unsigned clear   : 1; /* D8 - Clear module; when a 1 is written, CONV_COMP */
                        /* and EVENT_FULL are cleared and reset. The event   */
                        /* counter and channel counter are both reset to 0.  */
                        /* Resets itself, always reads 0.                    */
  unsigned count   : 4; /* D7-D4 - Event Count (0-15) */
  unsigned full    : 1; /* D3 - Event Buffer not full (EVENT_FULL); when a 0 */
                        /* is read, the Event Count (D7-D4) will be 0        */
                        /* indicating that 16 events have been stored in the */
                        /* memory.                                           */
  unsigned panel   : 1; /* D2 - Rear panel / front panel gate select; when   */
                        /* a 1 is written, front panel is selected. Rear     */
                        /* panel is the CERN Jaux Dataway, if in place.      */
  unsigned cip     : 1; /* D1 - CIP (Conversion In Progress) (read only)     */
  unsigned comp    : 1; /* D0 - CONV_COMP (Conversion complete) (read only)  */
} CSR;


/* base address A23-A16 could be set by switches on a board */

/*---------------------------------------------------------------*/
/*---------------------------- adc1182 --------------------------*/
/*---------------------------------------------------------------*/


static int n1182;
static unsigned int vmeaddress[22];


/* init: just calculate address offset */
int
adc1182init(unsigned int *addr, int nboards)
{
  int ii, res;
  unsigned int laddr;

  n1182 = 0;
  for(ii=0; ii<nboards; ii++)
  {
#ifdef VXWORKS
    res = sysBusToLocalAdrs(0x39,(char *)addr[ii],(char **)&laddr);
    if(res != 0) 
	{
	  printf("adc1182init: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr[ii]);
	  return(ERROR);
	}
#else
    res = vmeBusToLocalAdrs(0x39,(char *)addr[ii],(char **)&laddr);
    if (res != 0) 
	{
	  printf("adc1182init: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr[ii]);
	  return(ERROR);
	}
#endif
    printf("ADC1182 board %d at vme address 0x%08x\n",n1182,(unsigned int)laddr);
    vmeaddress[n1182++] = (unsigned int)laddr;
  }

  return(n1182);
}


 /* reset module at base address addr */
void
adc1182reset(int id)
{
  vmeWrite16((unsigned short *)vmeaddress[id],0x0104);

  return;
}

/* is conversion in progress in module at base address addr ? */
/*       return(2) if conversion in progress (cip)            */
/*       return(0) if not                                     */
unsigned int
adc1182cip(int id)
{
  short ret;

  ret = vmeRead16((unsigned short *)vmeaddress[id]) & 0x0002;

  return(ret);
}

/* is conversion complete in module at base address addr ? */
/*       return(1) if complete                             */
/*       return(0) if not complete                         */
unsigned int
adc1182ready(int id)
{
  short ret;

  ret = vmeRead16((unsigned short *)vmeaddress[id]) & 0x0001;

  return(ret);
}

/* read data from module at base address addr from channel chan */
unsigned int
adc1182read(int id, int chan)
{
  int adrtmp;
  short buf;
  short *address;
  unsigned int ret;

  adrtmp = vmeaddress[id] + 0x0100 + chan*2;

  buf = vmeRead16((unsigned short *)adrtmp);

  ret = (buf & 0x0fff) + (chan<<17);

  return(ret);
}

#else /* no UNIX version */

void
adc1182_dummy()
{
  return;
}

#endif



