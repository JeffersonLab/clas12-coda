/*
 *  scaler7201 Struck VME board - Sergey Boyarinov
 *
 */

#if defined(VXWORKS) || defined(Linux_vme)

#include <stdio.h>

#ifdef VXWORKS
#include <vxWorks.h>
#include <logLib.h>
#endif

#include "scaler7201.h"
/*
#include "ttutils.h"
*/

/* base address A23-A16 could be set by switches on a board */

/*---------------------------------------------------------------*/
/*--------------------------- scaler7201 ------------------------*/
/*---------------------------------------------------------------*/

/* low level functions */

int
scaler7201readfifo(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x100);

  return(*bufptr);
}
void
scaler7201writefifo(int addr, int value)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x10);

  *bufptr = value;
}

int
scaler7201status(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) addr;

  return(*bufptr);
}
void
scaler7201control(int addr, int value)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) addr;

  *bufptr = value;
}

void
scaler7201mask(int addr, int value)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0xc);

  *bufptr = value;
}

void
scaler7201clear(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x20);

  *bufptr = 0x0; /* arbitrary data ! */
}
void
scaler7201nextclock(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x24);

  *bufptr = 0x0; /* arbitrary data ! */
}
void
scaler7201enablenextlogic(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x28);

  *bufptr = 0x0; /* arbitrary data ! */
}
void
scaler7201disablenextlogic(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x2c);

  *bufptr = 0x0; /* arbitrary data ! */
}
void
scaler7201reset(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x60);

  *bufptr = 0x0; /* arbitrary data ! */
}
void
scaler7201testclock(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) (addr + 0x68);

  *bufptr = 0x0; /* arbitrary data ! */
}

/* "high" level functions */

void
scaler7201ledon(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) addr;

  *bufptr = 0x00000001;
}
void
scaler7201ledoff(int addr)
{
  volatile unsigned int *bufptr;
  bufptr = (volatile unsigned int *) addr;

  *bufptr = 0x00000100;
}

int
scaler7201almostread(int addr, int *value)
{
  int *outbuf;

  outbuf = value; /* output buffer have to be allocated in calling function */
  while( !(scaler7201status(addr) & FIFO_ALMOST_EMPTY))
  {
    *outbuf++ = scaler7201readfifo(addr);
  }

  return((int)(outbuf - value));
}

int
scaler7201read(int addr, int *value)
{
  int *outbuf, len;

  outbuf = value; /* output buffer have to be allocated in calling function */
  len = *value;
  while( !(scaler7201status(addr) & FIFO_EMPTY))
  {
    *outbuf++ = scaler7201readfifo(addr);
    len--;
    if(len == 0) break;
  }

  return((int)(outbuf - value));
}




#if 0

/* special readout for eg1 run */

int
scaler7201readHLS(int addr, int *ring, int counter)
{
  int k, dataword, ret;

  /*logMsg("scaler7201readHLS reached, addr=0x%08x\n",(int)addr,0,0,0,0,0);*/
  ret = 0;
  k = 0;
  while( !(scaler7201status(addr) & FIFO_EMPTY))
  {
    dataword = scaler7201readfifo(addr);
    k++;
    if(!(k%16)) /* put nHLS in every 16th channel */
    {
      dataword = ( dataword & 0xff000000 ) + (counter & 0xffffff);
    }
    /* write dataword to the ring buffer */
    if(utRingWriteWord(ring, dataword) < 0)
    {
      /*logMsg("scaler7201readHLS: ERROR: ring0 full\n",0,0,0,0,0,0);*/
      ret = -1;
    }
  }

  return(ret);
}

#endif



int
scaler7201restore(int addr, int mask)
{
  /*logMsg("scaler7201restore reached\n",1,2,3,4,5,6);*/

  scaler7201reset(addr);
  scaler7201mask(addr, mask);
  scaler7201enablenextlogic(addr);
  scaler7201control(addr, ENABLE_MODE2);
  scaler7201control(addr, ENABLE_EXT_DIS);
  /*scaler7201control(addr, ENABLE_EXT_NEXT);calls it separately, at once for all scalers*/
  scaler7201ledon(addr);

  return(0);
}

#else /* no UNIX version */

void
scaler7201_dummy()
{
  return;
}

#endif




