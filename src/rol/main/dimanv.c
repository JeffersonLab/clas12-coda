
/* dimanv.c - UNIX version only */

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

/* dimanv.c
 *
 * Program to set VME Discriminators CAEN v895
 *
 *   written  by SP  08-March-2007
 *
 *  Sergey Boyarinov Oct 2013: redone for the new dsc2 library
 */

#include "libtcp.h"


#define F_NAME    100       /* length of config. file name */
#define TEXT_STR  50000     /* size of some text strings */

#define PRINT_USAGE \
{ \
  printf("\nUsage:\n"); \
  printf("   dimanv roc_name mon [slot] \n"); \
  printf("   dimanv roc_name set [config_file_name]\n"); \
  printf("      \"roc_name\" - ROC IP name\n"); \
  printf("      \"slot\" - dsc2 slot number (is zero, all boards will be reported)\n"); \
  printf("      \"config_file_name\" - dsc2 config file\n\n"); \
  exit(0); \
}


/*************************************************************************/
/*************************************************************************/
/*  main program                                                         */
/*************************************************************************/
int
main(int argc, char *argv[])
{
  char *mod, *roc;
  int  ii, kk, stt = 0;
  char result[TEXT_STR];      /* string for messages from tcpClientCmd */
  char exename[200];          /* VME discr vxworks exec name */

  /* check input parameters */
  if(argc < 3 || argc > 4) PRINT_USAGE;

  roc = argv[1];
  mod = argv[2];

  if( strcmp(mod,"set") && strcmp(mod,"mon") ) PRINT_USAGE;

  printf("dimanv: mod >%s<, roc >%s<\n",mod,roc);

  if(!strcmp(mod,"mon")) /* mon */
  {
    if(argc==4) sprintf(exename,"dsc2Mon(%s)",argv[3]);
    else sprintf(exename,"dsc2Mon(0)");
  }
  else /* set */
  {
    if(argc==4) sprintf(exename, "dsc2Config(\"%s\")",argv[3]);
    else  sprintf(exename, "dsc2Config(\"\")");
 }

  printf("roc >%s< exename >%s<\n",roc,exename);

  memset(result,0,TEXT_STR);
  tcpClientCmd(roc, exename, result);

  printf("%s", result);

  exit(0);
}

#endif

