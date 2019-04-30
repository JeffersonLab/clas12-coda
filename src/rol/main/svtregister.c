#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vscmLib.h"

#ifdef Linux_vme

#include "jvme.h"

int
main(int argc, char *argv[])
{
  int board_id, slot, connector;
  int ii, jj, nvscm1;

  if (argc != 3) {
    printf("usage: svtregister <slot#> <connector#>\n");
    exit(EXIT_FAILURE);
  }
  else {
    slot = atoi(argv[1]);
    connector = atoi(argv[2]);
    printf("Arguments: slot=%02d connector=%1d\n", slot, connector);
  }

  if (connector < 1 || connector > 2) {
    printf("Invalid HFCB connector #. Must be 1 or 2.\n");
    exit(EXIT_FAILURE);
  }

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

  /* initialize VSCM board(s) */
  nvscm1 = vscmInit((unsigned int)(3<<19), (1<<19), 21, 1);
  /* Make sure Chip IDs for slot are set */
  fssrSetChipID(slot, connector, 1, 2, 3, 4);
  
  if (connector == 1)
     jj = 0;
  else
     jj = 4;

  for (ii = (0 + jj); ii < (4 + jj); ii++) {
    fssrChipIDTest(slot, ii);
    printf("========================================\n");
    if (!fssrRegisterTest(slot, ii)) {
      printf("========================================\n");
      fssrDiffLineTest(slot, ii);
      if (ii != ( 3 + jj)) printf("\n");
    }
    else if (ii != (3 + jj))
      printf("\n");
  }

  // Do a Master Reset after test is done
  fssrMasterReset(slot);

  exit(EXIT_SUCCESS);
}
#else

int
main()
{
  return(0);
}

#endif
