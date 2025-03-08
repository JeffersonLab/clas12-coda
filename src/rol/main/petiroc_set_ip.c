/* petiroc_set_ip.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"

static int npetiroc;

int main(int argc, char *argv[])
{
  int slot, ip, mac0, mac1;
  printf("\npetiroc_set_ip started ..\n\n");fflush(stdout);

  if(argc==5)
  {
    slot = atoi(argv[1]);
    ip   = strtol(argv[2], NULL, 16);
    mac0 = strtol(argv[3], NULL, 16);
    mac1 = strtol(argv[3], NULL, 16);
  }
  else
  {
    printf("Usage: petiroc_set_ip <slot> <32bit hex ip> <32bit hex mac0> <16bit hex mac1>\n");
    exit(1);
  }

  npetiroc = petirocInit(slot, 1, PETIROC_INIT_REGSOCKET);

  printf("\npetirocinit: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  slot = petirocSlot(0);

  printf("Before update:\n");
  petiroc_read_ip(slot);
  petiroc_program_ip(slot, ip, mac0, mac1);
  printf("After update:\n");
  petiroc_read_ip(slot);

  petirocEnd();

  exit(0);
}

