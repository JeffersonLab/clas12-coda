
/* roc_name.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql.h>

#include "codautil.h"

int
main(int argc, char **argv)
{
  char *mysql_database = getenv("EXPID");
  int id = atoi(argv[1]);
  char name[STRLEN];

  get_roc_name(mysql_database, id, name);

  printf("id=%d -> name=%s\n",id,name);

  return(0);
}
