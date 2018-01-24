
/* get_daq_config.cc - returns confFile from database */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
#include <strstream>

#include "codautil.h"

// online and coda stuff
extern "C"{
#include "libdb.h"
}

#if 0
void
get_daq_config(char *configname, char *conffile, int lname)
{
  MYSQL *dbsock;
  char tmp[1000];
  char tmpp[1000];

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);

  sprintf(tmp,"SELECT value FROM %s_option WHERE name='confFile'",configname);
  if(dbGetStr(dbsock, tmp, tmpp)==CODA_ERROR)
  {
    printf("cannot get 'confFile' from table >%s_option<\n",configname);
    return;
  }
  else
  {
    strncpy(conffile,tmpp,lname);
    printf("got conffile >%s<\n",conffile);
  }

  /* disconnect from database */
  dbDisconnect(dbsock);
}
#endif

char *
get_daq_config(char *mysql_database, char *configname)
{
  MYSQL *connNum;
  MYSQL_RES *result;
  MYSQL_ROW row_out;
  int numRows;
  ostrstream query;
  static char chres[1000];

  strcpy(chres,"unknown");

  // connect to mysql database
  connNum = dbConnect(getenv("MYSQL_HOST"), mysql_database);
  
  // form mysql query, execute, then close mysql connection
  query << "SELECT value FROM " << configname << "_option WHERE name='confFile'" << ends;
  mysql_query(connNum,query.str());

  result = mysql_store_result(connNum);
  if(result)
  {
    numRows = mysql_num_rows(result);
    if(numRows == 1)
	{
      row_out = mysql_fetch_row(result);
      // get run status 
      if(row_out[0] == NULL)
      {
        mysql_free_result(result);
      }
      else
      {
        strcpy(chres,row_out[0]);
        mysql_free_result(result);
      }
	}
  }

  dbDisconnect(connNum);

  return(chres);
}

