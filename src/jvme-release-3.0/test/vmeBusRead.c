/*
 * File:
 *    vmeBusRead.c
 *
 * Description:
 *    Perform a VME Bus read cycle at specified VME address
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <byteswap.h>
#include "jvme.h"

char *program_name;

void
usage()
{
  printf("Usage: \n\t");

  printf("%s: <size: 8 / 16 / 32> <address modifier: 0x29 / 0x39 / 0x09> <vme address>\n",
	 program_name);
}

int
main(int argc, char *argv[])
{
  void *localp = NULL;
  uint32_t size = 0, amcode = 0, vme_addr = 0xa80000, val = 0;
  int stat = 0;

  program_name = argv[0];
  if(argc != 4)
    {
      usage();
      exit(-1);
    }

  size  = strtoll(argv[1], NULL, 10);
  /* Check size is 8, 16 or 32 */
  if((size != 8) && (size != 16) && (size != 32))
    {
      fprintf(stderr, "Invalid size (%d)\n", size);
      usage();
      exit(-1);
    }


  amcode = strtoll(argv[2], NULL, 16) & 0xffffffff;
  vme_addr = strtoll(argv[3], NULL, 16) & 0xffffffff;

  vmeSetQuietFlag(0);
  printf("\nDEBUG MESSAGES ON\n");
  stat = vmeOpen();
  if(stat != OK) exit(-1);

  stat = vmeBusToLocalAdrs(amcode, (char*)(unsigned long)vme_addr,
			   (char**)(unsigned long)&localp);
  if(stat != OK) goto ERROR_CLOSE;

  /* Clear any bus errors */
  vmeClearException(0);
  vmeBusLock();
  switch(size)
    {
    case 8:
      val = vmeRead8((uint8_t *)localp);
      break;

    case 16:
      val = vmeRead16((uint16_t *)localp);
      break;

    default:
    case 32:
      val = vmeRead32((uint32_t *)localp);
    }

  /* Check for bus error */
  stat = vmeClearException(0);
  vmeBusUnlock();

  printf("\n\n");

  if(stat != OK)
    printf(" VME Read %d (am=0x%x, vme_address = 0x%x) returned Bus Error\n",
	 size, amcode, vme_addr);
  else
    printf(" VME Read %d (am=0x%x, vme_address = 0x%x) = 0x%x\n",
	   size, amcode, vme_addr, val);

  stat = vmeClose();
  if(stat != OK) exit(-1);

  printf("\n\n");

  exit(0);

 ERROR_CLOSE:
  vmeClose();
  exit(-1);
}

/*
  Local Variables:
  compile-command: "make -k vmeBusRead "
  End:
*/
