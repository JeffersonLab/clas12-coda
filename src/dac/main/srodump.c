

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>


#if defined(Linux_armv7l)

int
main()
{
  printf("Not supported on this platform\n");
  exit(0);
}

#else

/* srodump.c */

#include "evio.h"

#include "rolInt.h"
#include "da.h"
#include "srobuf.h"
#include "sroLib.h"

/*just to resolve references*/
int codaInit(char *confname) {return(0);}
int codaDownload(char *confname) {return(0);}
int codaPrestart() {return(0);}
int codaGo() {return(0);}
int codaEnd() {return(0);}
int codaPause() {return(0);}
int codaExit() {return(0);}

#define BUFFERSIZE 10000000
unsigned int buff[BUFFERSIZE];

int
main(int argc, char *argv[])
{
  int status, handler;
  char filename[256];

  if(argc!=2)
  {
    printf("\nUsage: sroprint <evio filename>\n\n");
    exit(0);
  }

  strcpy(filename,argv[1]);
  printf("\nUsing evio filename >%s<\n",filename);

  status = evOpen(filename, (char *)"r", &handler);
  printf("status=%d\n", status);
  if (status != 0)
  {
    printf("evOpen error %d - exit\n", status);
    exit(-1);
  }
  
  while(1)
  {
    status = evRead(handler, buff, BUFFERSIZE);
    if (status < 0)
    {
      if (status == EOF)
      {
	printf("evRead: end of file - exit\n");
        break;
      }
      else
      {
	printf("evRead error=%d - exit\n", status);
	break;
      }
    }
 
    sroPrintSet(&buff[0]);      
  }
  
  status = evClose(handler);

  exit(0);
}

#endif
