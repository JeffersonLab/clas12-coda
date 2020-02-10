// run_comp_name_host

//  gets component name and host

#include <stdlib.h>
#include <string.h>

#include "codautil.h"

#include <iostream>

using namespace std;

char *msql_database = getenv("EXPID");
char *session       = getenv("SESSION");
char *name          = (char *)NULL;

void decode_command_line(int argc, char **argv);
static char *help = (char *)"\nusage:\n\n  run_comp_name_host name [-s session] [-m msql_database] \n\n\n";


//----------------------------------------------------------------

int
main(int argc, char **argv)
{
  int run;
  char *compname;
  char *comphost;

  decode_command_line(argc,argv);
  if(session==NULL) session=(char *)"clasprod";
  if(name==NULL)
  {
    printf("have to specify component name (or first few letters of name)\n");
    exit(0);
  }

  // get run status
  printf("Looking for component with name starting from '%s'\n",name);fflush(stdout);

  get_comp_name_host(msql_database, session, name, &compname, &comphost);
  cout << "name=" << compname << ", host="<< comphost << endl;
  
  exit(0);
}


//----------------------------------------------------------------


void decode_command_line(int argc, char **argv)
{

  int i=1;


  while(i<argc) {
    
    if(strncasecmp(argv[i],"-h",2)==0)
    {
      cout << help;
      exit(EXIT_SUCCESS);
    }
    else if (strncasecmp(argv[i],"-s",2)==0)
    {
      session=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-m",2)==0)
    {
      msql_database=strdup(argv[i+1]);
      i=i+2;
    }
    else
    {
      name=strdup(argv[i]);
      i++;
	}
  }
}


/*---------------------------------------------------------------------*/

