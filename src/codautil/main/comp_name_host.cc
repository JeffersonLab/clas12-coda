// comp_name_host

//  gets name and host for specified partial component name

#include <stdlib.h>
#include <string.h>

#include <iostream>

using namespace std;



char *mysql_database = getenv("EXPID");
char *session        = getenv("SESSION");
char *partial_name   = "EB";

extern "C"{
  int get_comp_name_host(char *mysql_database, char *session, char *name, char **compname, char **comphost);
}
void decode_command_line(int argc, char **argv);


//----------------------------------------------------------------

int
main(int argc, char **argv)
{
  int run;
  char *compname;
  char *comphost;

  decode_command_line(argc,argv);
  if(session==NULL) session=(char *)"clasprod";

  //printf(">%s<\n",partial_name);

  // get
  get_comp_name_host(mysql_database, session, partial_name, &compname, &comphost);
  cout << compname << " " << comphost << endl;

}


//----------------------------------------------------------------


void decode_command_line(int argc, char **argv)
{

  int i=1;
  char *help=(char *)"\nusage:\n\n  comp_name_host [-n partial_name] [-s session] [-m mysql_database] \n\n\n";


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
    else if (strncasecmp(argv[i],"-n",2)==0){
      partial_name=strdup(argv[i+1]);
      i=i+2;
    }
  }
}


/*---------------------------------------------------------------------*/

