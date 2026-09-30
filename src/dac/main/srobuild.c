
/* srobuild.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <dirent.h>


#if defined(Linux_armv7l)

int
main()
{
  printf("Not supported on this platform\n");
  exit(0);
}

#else

#include <sys/resource.h>

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

#define MAXEVENTS 1000000

#define BUFFERSIZE 5000000
unsigned int buff[BUFFERSIZE];
unsigned int bufout[BUFFERSIZE];

#define NDATAFILES 256 /*maximum number of data files*/

int
main(int argc, char *argv[])
{
  int ii, len;
  DIR *d;
  struct dirent *dir;
  int status, handler, outhandler;
  int iev, nev, nevents=0, evind[MAXFRAME];
  char dirname[256], updirname[256];
  int ndatafiles;
  char datafiles[NDATAFILES][256];
  char filename[256];
  char foutname[256];
  TIMERL_VAR;
  
  const rlim_t kStackSize = 64L * 1024L * 1024L;   // min stack size = 64 Mb
  struct rlimit rl;
  int result;

  result = getrlimit(RLIMIT_STACK, &rl);
  printf("kStackSize  = 0x%08x\n",kStackSize);
  printf("rl.rlim_cur = 0x%08x, rl.rlim_max = 0x%08x\n",rl.rlim_cur,rl.rlim_max);
  if (result == 0)
  {
    if (rl.rlim_cur < kStackSize)
    {
      rl.rlim_cur = kStackSize;
      result = setrlimit(RLIMIT_STACK, &rl);
      if (result != 0)
      {
        printf("srobuild: ERROR: setrlimit returned result = %d\n", result);
      }
    }
  }
  printf("rl.rlim_cur = 0x%08x\n",rl.rlim_cur);
  
  
  if(argc!=2)
  {
    printf("\nUsage: srobuild <evio dirname>\n\n");
    exit(0);
  }

  strcpy(dirname,argv[1]);
  len = strlen(dirname);
  //printf("dirname >%s<, len=%d\n",dirname,len);
  if(dirname[len-1]=='/') dirname[len-1] = '\0'; /* remove trailing '/' if it is there */
  printf("\nUsing evio dirname >%s<\n",dirname);

  /* open directory */
  d = opendir(dirname);
  if (d == NULL)
  {
    perror("Error opening directory");
    exit(1);
  }

  /* Process each entry inside directory*/
  ndatafiles = 0;
  while ((dir = readdir(d)) != NULL)
  {
    if (dir->d_type == DT_REG) // Check if it's a regular file
    {
      printf("regular file: %s\n", dir->d_name);
      if(ndatafiles<NDATAFILES) strcpy(datafiles[ndatafiles++],dir->d_name);
      else printf("Some data files will be ignored - Increase NDATAFILES (current value is %d) !\n",NDATAFILES);
    }
    else
    {
      printf("something else: %s\n", dir->d_name);
    }
  }

  printf("\n Will process following data files:\n");
  for(ii=0; ii<ndatafiles; ii++)
  {
    printf("   %s/%s\n",dirname,datafiles[ii]);
  }
  printf("\n");


  /****************************************************/
  /*place output file one level up from data directory*/

  strcpy(updirname, dirname);
  len = strlen(updirname);
  for(ii=len-1; ii>0; ii--)
  {
    if(updirname[ii]=='/')
    {
      updirname[ii]='\0';
      break;
    }
  }

  strcpy(filename,datafiles[0]);
  len = strlen(filename);
  for(ii=0; ii<len; ii++)
  {
    if(filename[ii]=='.')
    {
      filename[ii]='\0';
      break;
    }
  }

  sprintf(foutname,"%s/%s.events.evio",updirname,filename);
  status = evOpen(foutname, (char *)"w", &outhandler);
  //printf("status=%d\n", status);
  if (status != 0)
  {
    printf("evOpen error %d - exit\n", status);
    exit(-1);
  }
  else
  {
    printf("Opened output file >%s< for writing\n\n",foutname);
  }


  /*********************************************************/
  /* loop over all input data files in specified directory */

  for(ii=0; ii<ndatafiles; ii++)
  {
    sprintf(filename,"%s/%s",dirname,datafiles[ii]);

    status = evOpen(filename, (char *)"r", &handler);
    //printf("status=%d\n", status);
    if (status != 0)
    {
      printf("evOpen('%s') error %d - exit\n",filename,status);
      exit(-1);
    }
    else
    {
      printf("Opened input data file >%s< for reading\n",filename);
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
      //else
      //{
      //  printf("evRead called\n");fflush(stdout);
      //}

TIMERL_START;
 
      nev = sroEventBuilder(&buff[0],bufout,evind);
      //printf("nev=%d\n",nev);

TIMERL_STOP(1000,0);

      for(iev=0; iev<nev; iev++)
      {
        //printf("evind[%d]=%d\n",iev,evind[iev]);
        status = evWrite(outhandler,(unsigned int *)&bufout[evind[iev]]);
        if(status!=0)
        {
          printf("evWrite error, status=%d\n\n",status);
          exit(0);
        }
        //else
        //{
        //  printf("evWrite called\n");fflush(stdout);
        //}
      }
      nevents += nev;

      sroStatPrint(0);

      if(nevents > MAXEVENTS) break;
    }
    printf("srobuild: nevents=%d\n",nevents);

    sroStatPrint(0);

    status = evClose(handler);
  }



  status = evClose(outhandler);

  exit(0);
}

#endif
