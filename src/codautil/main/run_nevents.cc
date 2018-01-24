// run_nevents

//  prints run nevents

//  ejw, 20-may-97


#include <stdlib.h>
#include <string.h>

#include <iostream>

using namespace std;



char *mysql_database = getenv("EXPID");
char *session       = getenv("SESSION");
char *runconfig     = NULL;

#include "codautil.h"

void decode_command_line(int argc, char **argv);


//----------------------------------------------------------------


main(int argc, char **argv){

  int nevents;
  char *config, *conffile, *datafile;

  decode_command_line(argc,argv);
  if(runconfig==NULL) runconfig=(char*)"PROD";


  // get run nevents
  nevents = get_run_nevents(mysql_database, runconfig);
  cout << nevents << endl;

}


//----------------------------------------------------------------


void decode_command_line(int argc, char **argv)
{

  int i=1;
  const char *help="\nusage:\n\n  run_nevents [-s session] [-m mysql_database] \n\n\n";


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
      mysql_database=strdup(argv[i+1]);
      i=i+2;
    }
  }
}


/*---------------------------------------------------------------------*/

