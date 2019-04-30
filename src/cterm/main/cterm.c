
/* cterm.c */

#include <stdio.h>

#include "xterm.h"

int ctermlib(int argc, char *argv[]ENVP_ARG);

int
main(int argc, char *argv[])
{
  ctermlib(argc, argv);
}
