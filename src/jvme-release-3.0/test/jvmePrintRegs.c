/*
 * File:
 *    jvmePrintRegs.c
 *
 * Description:
 *    Print VME Bridge Regs
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
  if(stat != OK) exit(-1);

  jlabTsi148ReadRegs();


  stat = vmeClose();
  if(stat != OK) exit(-1);

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k jvmePrintRegs "
  End:
*/
