
/* get_run_statistics.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdb.h"

#ifdef VXWORKS
extern char *mystrdup(const char *s);
#endif


#define NVALS 3
static char *db_field[3] = {"nfile","nevent","ndata"};


void
get_run_statistics(char *mysql_database, char *session, int *run, int maxvals, unsigned int *vals)
{
  MYSQL *connNum;
  MYSQL_RES *result;
  MYSQL_ROW row_out;
  char query[1024];
  char *config;
  int nvals, iii=0;

  *run=0;
  *config = 0;

  nvals = NVALS;
  if(maxvals<nvals) nvals=maxvals;

  /* connect to mysql database */
  connNum = dbConnect(getenv("MYSQL_HOST"), mysql_database);

  /* form mysql query, execute, then close mysql connection */
  sprintf(query,"SELECT runNumber,config FROM sessions WHERE name='%s'",session);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_run_statistics: ERROR in mysql_query 1\n");
    dbDisconnect(connNum);
    return;
  }

  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_run_statistics: ERROR in mysql_store_result 1\n");
    dbDisconnect(connNum);
    return;
  }

  /* get 'row_out' and check it for null */
  row_out = mysql_fetch_row(result);
  if(row_out==NULL)
  {
    mysql_free_result(result);
    dbDisconnect(connNum);
 
    return;
  }

  /* run number */ 
  if(row_out[0]==NULL)
  {
    *run=0;
  }
  else
  {
    *run=atoi(row_out[0]);
  }

  /* config */
#ifdef VXWORKS
  config = mystrdup(row_out[1]);
#else
  config = strdup(row_out[1]);
#endif
  mysql_free_result(result);








  /* get statistics */

  for(iii=0; iii<nvals; iii++)
  {
    sprintf(query,"SELECT value FROM %s_option WHERE name='%s'",(char *)config,db_field[iii]);
    if(mysql_query(connNum, query) != 0)
    {
      printf("get_run_statistics: ERROR in mysql_query 2\n");
      dbDisconnect(connNum);
      return;
    }
    if(!(result = mysql_store_result(connNum) ))
    {
      printf("get_run_statistics: ERROR in mysql_store_result 2\n");
      dbDisconnect(connNum);
      return;
    }
    row_out = mysql_fetch_row(result);
    if(row_out==NULL)
    {
      mysql_free_result(result);
      dbDisconnect(connNum); 
      return;
    }

    vals[iii] = atoi(row_out[0]);

    mysql_free_result(result);
  }




  dbDisconnect(connNum);

  return;
}


/******************/
/******************/
