/*
 * File:
 *    jvmePrintDma.c
 *
 * Description:
 *    Print DMA regs to std out
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <byteswap.h>
#include "jvme.h"

int
main(int argc, char *argv[])
{
  int stat = 0;

  stat = vmeOpen();
  /*sergey
  jvmeTsi148ReadDMARegs();
  jvmeTsi148ReadMasterWindowRegs();
  */
  jvmePrintDmaRegs();
  

  stat = vmeClose();
  printf("  stat = %d\n", stat);

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k jvmePrintDma "
  End:
*/
