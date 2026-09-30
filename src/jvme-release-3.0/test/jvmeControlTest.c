/*
 * File:
 *    jvmeTest.c
 *
 * Description:
 *    Test JLab Vme Driver
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <byteswap.h>
#include "jvme.h"
#include "jvmeVIVO.h"

void
runner()
{
  printf("%s: I'm here\n", __func__);
}

extern int32_t jvmeIntWait(uint32_t vector, uint32_t level, VOIDFUNCPTR routine, uint32_t arg);

int
main(int argc, char *argv[])
{
  uint64_t val64;
  uint32_t vaddr = 0xa80000, val32 = 0, addr_ret = 0;
  u_long laddr = 0;
  int stat = 0;

  stat = vmeOpenDefaultWindows();
  if(stat != OK) exit(-1);

  jvmeIntWait(0xac, 5, runner, 0);

  printf("pause\n"); getchar();

  stat = vmeCloseDefaultWindows();
  if(stat != OK) exit(-1);

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k jvmeControlTest "
  End:
*/
