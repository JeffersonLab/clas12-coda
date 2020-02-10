
/* get_comp_name_host.c */

/* 4 steps:

 1. get 'config' from 'session'
 2. get all components from 'config'
 3. check which component name starts from ''
 4. get component name and host from 'process' table

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libdb.h"
#include "codautil.h"

#ifdef VXWORKS
extern char *mystrdup(const char *s);
#endif

int
get_comp_name_host(char *mysql_database, char *session, char *name, char **compname, char **comphost)
{
  int i, id, nrows;
  MYSQL *connNum;
  MYSQL_RES *result;
  MYSQL_ROW row_out;
  char query[1024];
  char *config = 0;
  char *comps[1024];

  *compname = NULL;
  *comphost = NULL;

  /* connect to mysql database */
  connNum = dbConnect(getenv("MYSQL_HOST"), mysql_database);



  /* get config from sessions */
  sprintf(query,"SELECT config FROM sessions WHERE name='%s'",session);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_comp_name_host: ERROR in mysql_query 1\n");
    dbDisconnect(connNum);
    return(-1);
  }
  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_comp_name_host: ERROR in mysql_store_result 1\n");
    dbDisconnect(connNum);
    return(-1);
  }
  /* get 'row_out' and check it for null */
  row_out = mysql_fetch_row(result);
  if(row_out==NULL)
  {
    mysql_free_result(result);
    dbDisconnect(connNum);
    return(-1);
  }
#ifdef VXWORKS
  config = mystrdup(row_out[0]);
#else
  config = strdup(row_out[0]);
#endif
  mysql_free_result(result);







  /* get list of components from config */

  sprintf(query,"SELECT name FROM %s",(char *)config);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_comp_name_host: ERROR in mysql_query 2\n");
    dbDisconnect(connNum);
    return(-1);
  }
  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_comp_name_host: ERROR in mysql_store_result 2\n");
    dbDisconnect(connNum);
    return(-1);
  }
  nrows = mysql_num_rows(result);
  if(row_out==NULL)
  {
    mysql_free_result(result);
    dbDisconnect(connNum); 
    return(-1);
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





  /* for each component, check if component name starts from 'name' */

  id = -1;
  for(i=0; i<nrows; i++)
  {
    /*printf(" comp[%d] >%s<\n",i,comps[i]);*/

    if(!strncmp(comps[i],name,strlen(name)))
	{
      id = i;
      break;
	}
  }

  if(id<0)
  {
    printf("get_comp_name_host: component with name started with >%s< not found\n",name);
    return(0);
  }




  /* for found component, get hostname from 'process' table */

  sprintf(query,"SELECT host FROM process WHERE name='%s'",comps[id]);
  if(mysql_query(connNum, query) != 0)
  {
    printf("get_comp_name_host: ERROR in mysql_query 3\n");
    dbDisconnect(connNum);
    return(-1);
  }
  if(!(result = mysql_store_result(connNum) ))
  {
    printf("get_comp_name_host: ERROR in mysql_store_result 3\n");
    dbDisconnect(connNum);
    return(-1);
  }
  row_out = mysql_fetch_row(result);
  if(row_out==NULL)
  {
    mysql_free_result(result);
    printf("get_comp_name_host: ERROR: no such component in 'process' table\n");
    return(0);
  }

  /*printf("[%d] host >%s<\n",i,row_out[0]);*/
#ifdef VXWORKS
  *compname = mystrdup(comps[id]);
  *comphost = mystrdup(row_out[0]);
#else
  *compname = strdup(comps[id]);
  *comphost = strdup(row_out[0]);
#endif

  mysql_free_result(result);

  dbDisconnect(connNum);

  return(1);
}


/******************/
/******************/
