//  get_run_status
//
//  gets run status for this session
//


// for posix
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__

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


char *
get_run_status(char *mysql_database, char *session)
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
  query << "select log_name from sessions where name='" << session << "'" << ends;
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
        dbDisconnect(connNum);
      }
      else
      {
        strcpy(chres,row_out[0]);
        mysql_free_result(result);
        dbDisconnect(connNum);
      }
	}
  }

  dbDisconnect(connNum);

  return(chres);
}



//--------------------------------------------------------------------------
