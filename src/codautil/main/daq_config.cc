// daq_config

//  gets daq config

//  ejw, 9-apr-97


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>

using namespace std;



char *msql_database = getenv("EXPID");
char *session       = getenv("SESSION");
char *runconfig        = NULL;

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

  if(runconfig==NULL) runconfig=(char*)"PROD";

  // try get_run_status
  config = get_daq_config(msql_database,runconfig);
  cout << config << endl;
}


//----------------------------------------------------------------


void
decode_command_line(int argc, char **argv)
{
  int i=1;
  const char *help="\nusage:\n\n  run_status [-s session] [-m msql_database]\n\n\n";

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

