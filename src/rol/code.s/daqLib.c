
/* daqLib.c - data aquisition-related library */

/*#if defined(VXWORKS) || defined(Linux_vme) || defined(Linux_armv7l)*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef Linux_vme
#include "jvme.h"
#endif


#include "daqLib.h"
#include "xxxConfig.h"


static int report_raw_data;
static int external_vme_readout_interval;

int
daqInit()
{
  return(0);
}

int
daqGetReportRawData()
{
  return(report_raw_data);
}

int
daqGetExternalVmeReadoutInterval()
{
  return(external_vme_readout_interval);
}


/****************************************************************************
 *
 *  configuration functions 
 *
 
config file format:

DAQ_CRATE      <tdcecal1>     <- crate name, usually IP name

DAQ_REPORT_RAW_DATA 0/1          # if 1, tell ROL2 to keep raw data banks

DAQ_EXTERNAL_VME_READOUT_INTERVAL 1   # vme readout interval (seconds, 0 to 10) by DiagGuiServer, 0 means disable readout

DAQ_CRATE end
*/


#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */

#define MAXSLAVES 100

static int active;

static char *expid = NULL;

void
daqSetExpid(char *string)
{
  expid = strdup(string);
}


/* daqInit() have to be called before this function */
int  
daqConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    daqUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    daqInitGlobals();
  }

  /* read config file */
  if( (res = daqReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  daqDownloadAll();

  return(0);
}

int
daqInitGlobals()
{
  int ii, jj;

  report_raw_data = 0;
  external_vme_readout_interval = 1;

  return(0);
}


/**/
int
daqReadConfigFile(char *filename)
{
  FILE   *fd;
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch;
  char   str_tmp[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  char   str2[2];
  int    args, i1, i2, i3;
  int    slot, chan;
  unsigned int  ui1, ui2;
  char *getenv();
  char *clonparms;
  
  gethostname(host,ROCLEN);  /* obtain our hostname */
  clonparms = getenv("CLON_PARMS");
  for(jj=0; jj<strlen(host); jj++)
  {
    if(host[jj] == '.')
    {
      host[jj] = '\0';
      break;
    }
  }

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
      sprintf(fname, "%s/daq/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/daq/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/daq/%s.cnf", clonparms, expid);
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


      /* Start parsing real config inputs */
      if(strcmp(keyword,"DAQ_CRATE") == 0)
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

      else if(active && (strcmp(keyword,"DAQ_REPORT_RAW_DATA")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        report_raw_data = i1;
      }

      else if(active && (strcmp(keyword,"DAQ_EXTERNAL_VME_READOUT_INTERVAL")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        external_vme_readout_interval = i1;
      }

      else
      {
        ; /* unknown key - do nothing */
		/*
        printf("ReadConfigFile: Unknown Field or Missed Field in\n");
        printf("   %s \n", fname);
        printf("   str_tmp=%s", str_tmp);
        printf("   keyword=%s \n\n", keyword);
        return(-10);
		*/
      }

    }
  } /* end of while */

  fclose(fd);

  return(0);
}


int
daqDownloadAll()
{
  int ii;

  /* nothing so far ... */

  return(0);
}

void
daqMon(int slot)
{
  daqUploadAllPrint();
}


int
daqUploadAll(char *string, int length)
{
  int slot, i, ii, jj, kk, ifiber, len1, len2;
  char *str, sss[1024];
  unsigned int tmp;
  int connectedfibers;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];

  if(length)
  {
    str = string;
    str[0] = '\0';

    sprintf(sss,"DAQ_REPORT_RAW_DATA %d\n",report_raw_data);
    ADD_TO_STRING;

    sprintf(sss,"DAQ_EXTERNAL_VME_READOUT_INTERVAL %d\n",external_vme_readout_interval);
    ADD_TO_STRING;

    CLOSE_STRING;
  }

}

int
daqUploadAllPrint()
{
  char str[4097];
  daqUploadAll(str, 4096);
  printf("%s",str);
}





/*
#else

void
daqConfig_dummy()
{
  return;
}

#endif
*/
