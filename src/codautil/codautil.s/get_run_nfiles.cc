
/* get_run_nfiles.cc - returns the number of data files in the run from database */

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


int
get_run_nfiles(char *mysql_database, char *configname)
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
  query << "SELECT value FROM " << configname << "_option WHERE name='nfile'" << ends;
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
        return(0);
      }
      else
      {
        mysql_free_result(result);
        dbDisconnect(connNum);
        return(atoi(row_out[0]));
      }
	}
  }

  dbDisconnect(connNum);

  return(0);
}

