// daq_config

//  gets daq config (trigger.trg file name)

//  ejw, 9-apr-97


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>

using namespace std;



char *msql_database = getenv("EXPID");
char *session       = getenv("SESSION");
char *daqconfig        = NULL;

#include "codautil.h"

void decode_command_line(int argc, char **argv);


//----------------------------------------------------------------

int
main(int argc, char **argv)
{
  char line[200];
  char state[32];
  char fsession[32];
  char *config, *conffile, *datafile;
  char fconfig[32];
  int run;
  int msql_run;
  int stat;

  decode_command_line(argc,argv);

  if(session==NULL) session=(char *)"clasprod";
  get_run_config(msql_database,session,&run,&config,&conffile,&datafile);
  cout << conffile << endl;

  /*
  if(daqconfig==NULL) daqconfig=(char*)"PROD66";
  // try get_run_status
  config = get_daq_config(msql_database, daqconfig);
  cout << config << endl;
  */
}


//----------------------------------------------------------------


void
decode_command_line(int argc, char **argv)
{
  int i=1;
  const char *help="\nusage:\n\n  daq_config [-s session] [-m msql_database]\n\n\n";

  while(i<argc) {
    
    if(strncasecmp(argv[i],"-h",2)==0){
      cout << help;
      exit(EXIT_SUCCESS);
    }
    else if (strncasecmp(argv[i],"-s",2)==0){
      session=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-m",2)==0){
      msql_database=strdup(argv[i+1]);
      i=i+2;
    }
  }
}


//--------------------------------------------------------------------

