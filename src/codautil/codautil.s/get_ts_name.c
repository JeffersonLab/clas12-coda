
/* get_ts_name.c */

/* 4 steps:

 1. get 'config' from 'session'
 2. get all rocs from 'config'
 3. check which roc has type='TS' in process table

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdb.h"

#ifdef VXWORKS
extern char *mystrdup(const char *s);
#endif

char *
get_ts_name(char *mysql_database, char *session)
{
  int i, nrows;
  MYSQL *connNum;
  MYSQL_RES *result;
  MYSQL_ROW row_out;
  char query[1024];
  char *config = 0;
  char *comps[1024];

  /* connect to mysql database */
  connNum = dbConnect(getenv("MYSQL_HOST"), mysql_database);



  /* get config from sessions */
  sprintf(query,"SELECT config FROM sessions WHERE name='%s'",session);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_ts_name: ERROR in mysql_query 1\n");
    dbDisconnect(connNum);
    return(NULL);
  }
  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_ts_name: ERROR in mysql_store_result 1\n");
    dbDisconnect(connNum);
    return(NULL);
  }
  /* get 'row_out' and check it for null */
  row_out = mysql_fetch_row(result);
  if(row_out==NULL)
  {
    mysql_free_result(result);
    dbDisconnect(connNum);
    return(NULL);
  }
#ifdef VXWORKS
  config = mystrdup(row_out[0]);
#else
  config = strdup(row_out[0]);
#endif
  mysql_free_result(result);







  /* get list of rocs from config */

  sprintf(query,"SELECT name FROM %s",(char *)config);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_ts_name: ERROR in mysql_query 2\n");
    dbDisconnect(connNum);
    return(NULL);
  }
  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_ts_name: ERROR in mysql_store_result 2\n");
    dbDisconnect(connNum);
    return(NULL);
  }
  nrows = mysql_num_rows(result);
  if(row_out==NULL)
  {
    mysql_free_result(result);
    dbDisconnect(connNum); 
    return(NULL);
  }

  /*printf("nrows=%d\n",nrows);*/

  for(i=0; i<nrows; i++)
  {
    row_out = mysql_fetch_row(result);
    /*printf(" row[%d] >%s<\n",i,row_out[0]);*/

#ifdef VXWORKS
    comps[i] = mystrdup(row_out[0]);
#else
    comps[i] = strdup(row_out[0]);
#endif
  }

  mysql_free_result(result);





  /* for each component, check if type in 'process' table is TS */

  for(i=0; i<nrows; i++)
  {
    /*printf(" comp[%d] >%s<\n",i,comps[i]);*/
    




    sprintf(query,"SELECT type FROM process WHERE name='%s'",comps[i]);
    if(mysql_query(connNum, query) != 0)
    {
      printf("get_ts_name: ERROR in mysql_query 3\n");
      dbDisconnect(connNum);
      return(NULL);
    }
    if(!(result = mysql_store_result(connNum) ))
    {
      printf("get_ts_name: ERROR in mysql_store_result 3\n");
      dbDisconnect(connNum);
      return(NULL);
    }
    row_out = mysql_fetch_row(result);
    if(row_out==NULL)
    {
      mysql_free_result(result);
      /*printf("no such component in 'process' table\n");*/
      continue;
    }

	/*printf("[%d] type >%s<\n",i,row_out[0]);*/

    if(!strncmp(row_out[0],"TS",2))
	{
      mysql_free_result(result);
      /*printf("get_ts_name returns >%s<\n",comps[i]);*/
      return(comps[i]);
    }


    mysql_free_result(result);
  }




  dbDisconnect(connNum);

  return(NULL);
}


/******************/
/******************/
