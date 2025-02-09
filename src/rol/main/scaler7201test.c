/* test scaler7201 board using internal tests */

#if defined(VXWORKS) || defined(Linux_vme)


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <jvme.h>
#include "scaler7201.h"

#ifdef VXWORKS
void
scaler7201test(int addr)
#else
int
main(int argc, char *argv[])
#endif
{
  unsigned int i, len, value, buffer[100];
  volatile unsigned int *bufptr;
  int res;
  unsigned int address = atoi(argv[1]);
  unsigned int addr;
  printf("argc=%d, argv[0]=>%s, argv[1]=>%s<\n",argc,argv[0],argv[1]);

address = 0x900000;
  printf("use address 0x%08x\n",address);

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

  res = vmeBusToLocalAdrs(0x39, (char *) address, (char **) &addr);
printf("use addr 0x%08x\n",addr);

  bufptr = (volatile unsigned int *) addr;




  /* initialization */
  scaler7201restore(addr, 0x0);
  scaler7201control(addr, ENABLE_EXT_NEXT);
 


  //scaler7201reset(addr);
  //scaler7201mask(addr, 0xfffffffc); /* disable all channels except 0-1 */
  //scaler7201enablenextlogic(addr);
  //scaler7201nextclock(addr);

/* test 1 */

  //scaler7201control(addr, ENABLE_TEST);
  //for(i=0; i<8; i++) scaler7201testclock(addr);
  //scaler7201control(addr, DISABLE_TEST);

  //scaler7201nextclock(addr);
  printf("Reading fifo1 ...\n");
  while( !(scaler7201status(addr) & FIFO_EMPTY))
  {
    printf("status=%08x\n",scaler7201status(addr));
	for(i=0; i<32; i++)
	{
      value = scaler7201readfifo(addr);
      printf("[%2d] value=%08x\n",i,value);
	}
    printf("\n");
    sleep(1);
  }
  printf("... done.\n");


  exit(0);


/* test 2 */

  scaler7201control(addr, ENABLE_TEST);
  scaler7201control(addr, ENABLE_25MHZ);
  for(i=0; i<100000; i++) {;}
  scaler7201control(addr, DISABLE_25MHZ);
  scaler7201control(addr, DISABLE_TEST);

  scaler7201nextclock(addr);
  printf("Reading fifo2 ...\n");
  len = scaler7201read(addr,buffer);
  for(i=0; i<len; i++) printf("value=%08x\n",buffer[i]);
  printf("... done.\n");

/* test 3 */

  scaler7201control(addr, ENABLE_TEST);
  scaler7201control(addr, ENABLE_25MHZ);
  for(i=0; i<10000; i++) {;}
  scaler7201control(addr, DISABLE_25MHZ);
  scaler7201control(addr, DISABLE_TEST);

  scaler7201nextclock(addr);
  printf("Reading fifo2 ...\n");
  len = scaler7201read(addr,buffer);
  for(i=0; i<len; i++) printf("value=%08x\n",buffer[i]);
  printf("... done.\n");

/* test 4 */

  scaler7201control(addr, ENABLE_TEST);
  scaler7201control(addr, ENABLE_25MHZ);
  for(i=0; i<10000; i++) {;}
  scaler7201control(addr, DISABLE_25MHZ);
  scaler7201control(addr, DISABLE_TEST);

  scaler7201nextclock(addr);
  printf("Reading fifo4 ...\n");
  len = scaler7201read(addr,buffer);
  for(i=0; i<len; i++) printf("value=%08x\n",buffer[i]);
  printf("... done.\n");

/* test 5 */

  scaler7201control(addr, ENABLE_TEST);
  scaler7201control(addr, ENABLE_25MHZ);
  for(i=0; i<10000; i++) {;}
  scaler7201control(addr, DISABLE_25MHZ);
  scaler7201control(addr, DISABLE_TEST);

  scaler7201nextclock(addr);
  printf("Reading fifo5 ...\n");
  len = scaler7201read(addr,buffer);
  for(i=0; i<len; i++) printf("value=%08x\n",buffer[i]);
  printf("... done.\n");

}

#else /* no UNIX version */

int
main()
{
  exit(0);
}

#endif
