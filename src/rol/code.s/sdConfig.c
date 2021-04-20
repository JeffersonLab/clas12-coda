/****************************************************************************
 *
 *  sdConfig.c  -  configuration library file for dsc2 board 
 *


config file format:

CRATE      <adcecal1>     <- crate name, usually IP name

SD_TRIGOUTLOGIC 0  2      <- output trigger type and threshold


*/

#if defined(VXWORKS) || defined(Linux_vme)

#ifdef VXWORKS
#include <vxWorks.h>
#include <vxLib.h>
#include <logLib.h>
#else
#include "jvme.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>

#include "sdLib.h"
#include "sdConfig.h"
#include "xxxConfig.h"


/* Global variables */
static int active;

static int Nsd = 0;                        /* Number of SDs in Crate */


#define DEBUG

#define FNLEN     256       /* length of config. file name */
#define STRLEN    256       /* length of str_tmp */
#define ROCLEN    256       /* length of ROC_name */
#define NBOARD     22
#define NCHAN      2


/* Define global variables (no slot numbers, only one SD in crate !) */

static int TrigoutLogic_type;
static int TrigoutLogic_threshold;

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/



static char *expid = NULL;

void
sdSetExpid(char *string)
{
  expid = strdup(string);
}

void
sdInitGlobals()
{
  TrigoutLogic_type = 0;
  TrigoutLogic_threshold = 2;
}

/* main function, have to be called after sdInit() */
int
sdConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  /* sdInit() must be called by now; get the number of boards from there */
  Nsd = sdGetNsd();

  printf("sdConfig: Nsd=%d\n",Nsd);

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    sdUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    sdInitGlobals();
  }

  /* read config file */
  if( (res = sdReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  sdDownloadAll();

  return(0);
}





/* reading and parsing config file */
/* config file will be selected using following rules:
     1. use 'filename' specified; if name starts from '/' or './', use it,
        otherwise use file from '$CLON_PARMS/sd/' area
     2. if name is "", use file 'hostname'.conf from '$CLON_PARMS/sd/' area
     3. if previous does not exist, use file '$EXPID.conf' from '$CLON_PARMS/sd/' area
 */
int
sdReadConfigFile(char *filename)
{
  FILE   *fd;
  int    ii, jj, ch, kk = 0;
  char   str_tmp[STRLEN], str2[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  int    msk[NCHAN];
  int    args, i1, i2;
  float  f1, f2;
  char fname[FNLEN] = { "" };  /* config file name */
  char *getenv();
  char *clonparms;

  gethostname(host,ROCLEN);  /* obtain our hostname */
  clonparms = getenv("CLON_PARMS");

  if(expid==NULL)
  {
    expid = getenv("EXPID");
    printf("\nNOTE: use EXPID=>%s< from environment\n",expid);
  }
  else
  {
    printf("\nNOTE: use EXPID=>%s< from CODA\n",expid);
  }

  if(strlen(filename)!=0) /* filename specified */
  {
    if ( filename[0]=='/' || (filename[0]=='.' && filename[1]=='/') )
	{
      sprintf(fname, "%s", filename);
	}
    else
	{
      sprintf(fname, "%s/sd/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/sd/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/sd/%s.cnf", clonparms, expid);
      if((fd=fopen(fname,"r")) == NULL)
      {
        printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
        return(-2);
	  }
	}

  }

  printf("\nReadConfigFile: Using configuration file >%s<\n",fname);


  /* Parsing of config file */
  active = 0;
  while ((ch = getc(fd)) != EOF)
  {
    if ( ch == '#' || ch == ' ' || ch == '\t' )
    {
      while (getc(fd) != '\n') {}
    }
    else if( ch == '\n' ) {}
    else
    {
      ungetc(ch,fd);
      fgets(str_tmp, STRLEN, fd);
      sscanf (str_tmp, "%s %s", keyword, ROC_name);
#ifdef DEBUG
      printf("\nfgets returns %s so keyword=%s\n\n",str_tmp,keyword);
#endif

      if(strcmp(keyword,"SD_CRATE") == 0)
      {
	    if(strcmp(ROC_name,host) == 0)
        {
	      printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
          active = 1;
        }
	    else if(strcmp(ROC_name,"all") == 0)
		{
	      printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
          active = 1;
		}
        else
		{
	      printf("\nReadConfigFile: crate = %s  host = %s - disactivated\n",ROC_name,host);
          active = 0;
		}
      }

      else if(active && ((strcmp(keyword,"SD_TRIGOUTLOGIC") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        TrigoutLogic_type = i1;
        TrigoutLogic_threshold = i2;
      }

      else
      {
        ; /* unknown key - do nothing */
		/*
        printf("ReadConfigFile: Unknown Field or Missed Field in\n");
        printf("   %s \n", fname);
        printf("   str_tmp=%s", str_tmp);
        printf("   keyword=%s \n\n", keyword);
        return(-7);
		*/
      }

    }
  }
  fclose(fd);


  printf("sdReadConfigFile !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

#ifdef DEBUG
  printf("\nSD configuration:\n");
  printf("  TrigoutLogic_type=%d\n",TrigoutLogic_type);
  printf("  TrigoutLogic_threshold=%d\n",TrigoutLogic_threshold);
#endif

  return(kk);
}





int
sdDownloadAll()
{
  Nsd = sdGetNsd();
  if(Nsd>0)
  {
    sdSetTrigoutLogic(TrigoutLogic_type, TrigoutLogic_threshold);
  }

  return(0);
}







/* upload setting from all found SDs */
int
sdUploadAll(char *string, int length)
{
  int len1, len2;
  char *str, sss[1024];

  Nsd = sdGetNsd();
  if(Nsd>0)
  {
    sdGetTrigoutLogic(&TrigoutLogic_type, &TrigoutLogic_threshold);

    if(length)
    {
      str = string;
      str[0] = '\0';

      sprintf(sss,"SD_TRIGOUTLOGIC %d %d\n",TrigoutLogic_type, TrigoutLogic_threshold);
      ADD_TO_STRING;

      CLOSE_STRING;
    }
  }
}


int
sdUploadAllPrint()
{
  char str[10000];
  sdUploadAll(str, 10000);
  printf("%s",str);
}



#else /* dummy version*/

void
sdConfig_dummy()
{
  return;
}

#endif
