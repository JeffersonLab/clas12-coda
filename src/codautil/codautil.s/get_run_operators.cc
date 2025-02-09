//  get_run_operators
//
//  gets run operators for this session
//


// for posix
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <strstream>

using namespace std;

#include "codautil.h"

// online and coda stuff
extern "C"{
#include "libdb.h"
}

char *
get_run_operators(char *mysql_database, char *session)
{
  MYSQL *dbsock;
  MYSQL_RES *result;
  MYSQL_ROW row_out;
  ostrstream query;
  time_t mytime;
  struct tm *tm_info;
  char tmp[1000], tmp1[100], chdate[30], chtime[30], expert_shift[10], worker_shift[10];
  int ii, jj, len, numRows;
  
  /*hps
  char *user = "hpsshiftbot";
  char *passwd = "88hIMrBzLqwKHQaO";
  */

  /*clas12*/
  const char *user = "shiftbot";
  const char *passwd = "manageshiftdb";


  static char chres[1000];

  chres[0] = '\0';

  time(&mytime);
  tm_info = localtime(&mytime);
  strftime(tmp1, 26, "%Y-%m-%d %H:%M:%S", tm_info);
  /*printf("2: tmp1: >%s<\n", tmp1);*/

  strncpy(chdate,tmp1,10);
  chdate[10] = '\0';
  strcpy(chtime,(char *)&tmp1[11]);
  /*printf("chdate >%s<, chtime >%s<\n",chdate,chtime);*/


  // connect to mysql database
  
  /*hps
  dbsock = dbConnectFull("clasdb", "hpsshift", user, passwd);
  */  

  /*clas12*/
  dbsock = dbConnectFull("clasdb", "shift", user, passwd);


  /* get experiment */
  sprintf(tmp,"SELECT exp FROM acc_sched WHERE shiftdate='%s'",chdate);
  /*printf("query >%s<\n",tmp);*/
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("ERROR in mysql_query >%s<\n",tmp);
    dbDisconnect(dbsock);
    return(NULL);
  }
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result()\n");
    dbDisconnect(dbsock);
    return(NULL);
  }
  else
  {
    numRows = mysql_num_rows(result);
    if(numRows == 1)
    {
      row_out = mysql_fetch_row(result);
      /*printf("experiment >%s<\n",row_out[0]);*/
    }
    else
    {
      /*printf("experiment >%s<\n","none")*/;
    }
    mysql_free_result(result);
  }


  /* check if expert is in 'owl', 'day' or 'eve' */
  ii = atoi(chtime);
  if(ii < 8)       strcpy(expert_shift,"owl");
  else if(ii < 16) strcpy(expert_shift,"day");
  else             strcpy(expert_shift,"eve");


  /* get shift expert */
  sprintf(tmp,"SELECT %s FROM expert WHERE shiftdate='%s'",expert_shift,chdate);
  /*printf("query >%s<\n",tmp);*/
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("ERROR in mysql_query >%s<\n",tmp);
    dbDisconnect(dbsock);
    return(NULL);
  }
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result()\n");
    dbDisconnect(dbsock);
    return(NULL);
  }
  else
  {
    numRows = mysql_num_rows(result);
    strcat(chres," expert: ");
    if(numRows == 1)
    {
      row_out = mysql_fetch_row(result);
      strcat(chres,row_out[0]);
      /*printf("expert >%s<\n",row_out[0]);*/
    }
    else
    {
      strcat(chres,"none");
      /*printf("expert >%s<\n","none");*/
    }
    mysql_free_result(result);
  }


  /* check if worker is in 'owl', 'day' or 'eve' */
  ii = atoi(chtime);
  if(ii < 7)
  {
    strcpy(worker_shift,"owl"); /* but yesterday */
    tm_info->tm_mday --;
    mktime(tm_info); /* Normalise ts */
    strftime(chdate, sizeof(chdate), "%Y-%m-%d", tm_info);
  }
  else if(ii < 15) strcpy(worker_shift,"day");
  else if(ii < 23) strcpy(worker_shift,"eve");
  else             strcpy(worker_shift,"owl");



  /* get shift worker */
  sprintf(tmp,"SELECT %s FROM novice WHERE shiftdate='%s'",worker_shift,chdate);
  /*printf("query >%s<\n",tmp);*/
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("ERROR in mysql_query >%s<\n",tmp);
    dbDisconnect(dbsock);
    return(NULL);
  }
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result()\n");
    dbDisconnect(dbsock);
    return(NULL);
  }
  else
  {
    numRows = mysql_num_rows(result);
    strcat(chres,", worker: ");
    if(numRows == 1)
    {
      row_out = mysql_fetch_row(result);
      strcat(chres,row_out[0]);
      /*printf("worker >%s<\n",row_out[0]);*/
    }
    else
    {
      strcat(chres,"none");
      /*printf("worker >%s<\n","none");*/
    }
    mysql_free_result(result);
  }


  /* disconnect from database */
  dbDisconnect(dbsock);


  printf("get_run_operators >%s<\n",chres);

  return(chres);

}

