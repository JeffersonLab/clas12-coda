
/* gtptest1.c - gets gtp configuration string using tcpClientCmd */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


#if defined(VXWORKS) || defined(Linux_armv7l)

int
main()
{
  exit(0);
}

#else

#include "libtcp.h"


#define F_NAME    100       /* length of config. file name */
#define TEXT_STR  50000     /* size of some text strings */

#define PRINT_USAGE \
{ \
  printf("\nUsage:\n"); \
  printf("   gtptest1 roc_name\n"); \
  exit(0); \
}


/*************************************************************************/
/*************************************************************************/
/*  main program                                                         */
/*************************************************************************/
int
main(int argc, char *argv[])
{
  char *roc;
  int  ii, kk, stt = 0;
  char result[TEXT_STR];      /* string for messages from tcpClientCmd */
  char exename[200];          /* VME discr vxworks exec name */

  /* check input parameters */
  if(argc < 2 || argc > 2) PRINT_USAGE;

  roc = argv[1];

  sprintf(exename,"gtpUploadAllPrint()");

  printf("gtptest1: roc >%s< exename >%s<\n",roc,exename);

  memset(result,0,TEXT_STR);
  tcpClientCmd(roc, exename, result);

  printf("%s", result);

  exit(0);
}

#endif

