
/* get_roc_name.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libdb.h"
#include "codautil.h"

void
get_roc_name(char *mysql_database, int id, char name[STRLEN])
{
  MYSQL *connNum;
  MYSQL_RES *result;
  MYSQL_ROW row_out;
  char query[1024];

  /* connect to mysql database */
  connNum = dbConnect(getenv("MYSQL_HOST"), mysql_database);

  /* form mysql query, execute, then close mysql connection */
  sprintf(query,"SELECT name FROM process WHERE id='%d'",id);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_roc_name: ERROR in mysql_query 1\n");
    dbDisconnect(connNum);
    return;
  }

  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_roc_name: ERROR in mysql_store_result 1\n");
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
    name[0] = '\0';
  }
  else
  {
    strncpy(name,row_out[0],STRLEN);
  }

  mysql_free_result(result);

  dbDisconnect(connNum);

  return;
}

void
get_roc_id(char *mysql_database, char *name, int *id)
{
  MYSQL *connNum;
  MYSQL_RES *result;
  MYSQL_ROW row_out;
  char query[1024];

  *id = 0;

  /* connect to mysql database */
  connNum = dbConnect(getenv("MYSQL_HOST"), mysql_database);

  /* form mysql query, execute, then close mysql connection */
  sprintf(query,"SELECT id FROM process WHERE name='%s'",name);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_roc_id: ERROR in mysql_query 1\n");
    dbDisconnect(connNum);
    return;
  }

  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_roc_id: ERROR in mysql_store_result 1\n");
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
  if(row_out[0]!=NULL)
  {
    *id = atoi(row_out[0]);
  }

  mysql_free_result(result);

  dbDisconnect(connNum);

  return;
}

