/****************************************************************************
 *
 *  flpConfig.c  -  configuration library file for dsc2 board 
 *


config file format:

CRATE      <tdcecal1>     <- crate name, usually IP name

FLP_SLOT  2   <- slot#

FLP_WIDTH  20  40   <- TDC width (ns), TRG width (ns)


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

#include "flpLib.h"
#include "flpConfig.h"
#include "xxxConfig.h"


/* Global variables */
static int active;

static int Nflp = 0;                        /* Number of FLPs in Crate */


#define DEBUG

#define FNLEN     256       /* length of config. file name */
#define STRLEN    256       /* length of str_tmp */
#define ROCLEN    256       /* length of ROC_name */
#define NBOARD     22
#define NCHAN      2


/* Define global variables (NOTE: index in following arrays is SLOT NUMNER !!!) */

static float  v1[NBOARD][NCHAN];
static float  v2[NBOARD][NCHAN];
static float  vled[NBOARD][NCHAN];
static int    period[NBOARD][NCHAN];



/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/



static char *expid = NULL;

void
flpSetExpid(char *string)
{
  expid = strdup(string);
}

void
flpInitGlobals()
{
  int ii, jj;

  for(ii=0; ii<NBOARD; ii++)
  {
    for(jj=0; jj<NCHAN; jj++)
    {
      v1[ii][jj] = 3.7;
      v2[ii][jj] = 3.7;
      vled[ii][jj] = 4.5;
      period[ii][jj] = 125000;
    }
  }

}

/* main function, have to be called after flpInit() */
int
flpConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  /* flpInit() must be called by now; get the number of boards from there */
  Nflp = flpGetNflp();

  printf("flpConfig: Nflp=%d\n",Nflp);

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    flpUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    flpInitGlobals();
  }

  /* read config file */
  if( (res = flpReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  flpDownloadAll();

  return(0);
}





/* reading and parsing config file */
/* config file will be selected using following rules:
     1. use 'filename' specified; if name starts from '/' or './', use it,
        otherwise use file from '$CLON_PARMS/flp/' area
     2. if name is "", use file 'hostname'.conf from '$CLON_PARMS/flp/' area
     3. if previous does not exist, use file '$EXPID.conf' from '$CLON_PARMS/flp/' area
 */
int
flpReadConfigFile(char *filename)
{
  FILE   *fd;
  int    ii, jj, ch, kk = 0;
  char   str_tmp[STRLEN], str2[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  int    msk[NCHAN];
  int    args, i1, i2;
  float  f1, f2;
  int    slot, slot1, slot2, chan;
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
      sprintf(fname, "%s/flp/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/flp/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/flp/%s.cnf", clonparms, expid);
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

      if(strcmp(keyword,"FLP_CRATE") == 0)
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

      else if(active && (strcmp(keyword,"FLP_SLOT")==0))
      {
        kk++;
        sscanf (str_tmp, "%*s %s", str2);
        /*printf("str2=%s\n",str2);*/
        if(isdigit(str2[0]))
        {
          slot1 = atoi(str2);
          slot2 = slot1 + 1;
          if(slot1<0 && slot1>21)
          {
            printf("\nReadConfigFile: Wrong slot number %d\n\n",slot1);
            return(-4);
          }
        }
        else if(!strcmp(str2,"all"))
        {
          slot1 = 0;
          slot2 = NBOARD;
        }
        else
        {
          printf("\nReadConfigFile: Wrong slot >%s<, must be 'all' or actual slot number\n\n",str2);
          return(-4);
        }
        printf("\nReadConfigFile: will be using slot1=%d, slot2=%d\n",slot1,slot2);
	  }






      else if(active && ((strcmp(keyword,"FLP_V1") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %f %f", &f1, &f2);
        for(slot=slot1; slot<slot2; slot++)
        {
          v1[slot][0] = f1;
          v1[slot][1] = f2;
        }
      }

      else if(active && ((strcmp(keyword,"FLP_V2") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %f %f", &f1, &f2);
        for(slot=slot1; slot<slot2; slot++)
        {
          v2[slot][0] = f1;
          v2[slot][1] = f2;
        }
      }

      else if(active && ((strcmp(keyword,"FLP_VLED") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %f %f", &f1, &f2);
        for(slot=slot1; slot<slot2; slot++)
        {
          vled[slot][0] = f1;
          vled[slot][1] = f2;
        }
      }

      else if(active && ((strcmp(keyword,"FLP_PERIOD") == 0) && (kk >= 0)))
	  {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        for(slot=slot1; slot<slot2; slot++)
        {
          period[slot][0] = i1;
          period[slot][1] = i2;
        }
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


  printf("flpReadConfigFile !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

#ifdef DEBUG
  Nflp = flpGetNflp();
  for(ii=0; ii<Nflp; ii++)
  {
    slot = flpSlot(ii);
    printf("\nslot=%2d\n",slot);
	printf("       V1: %f V    %f V\n",v1[slot][0],v1[slot][1]);
	printf("       V2: %f V    %f V\n",v2[slot][0],v2[slot][1]);
	printf("     VLED: %f V    %f V\n",vled[slot][0],vled[slot][1]);
	printf("   PERIOD: %d ns    %d ns\n",period[slot][0],period[slot][1]);
  }
#endif

  return(kk);
}





int
flpDownloadAll()
{
  int slot, chan, kk;
  UINT16 selMask;

  Nflp = flpGetNflp();
  for(kk=0; kk<Nflp; kk++)
  {
    slot = flpSlot(kk);

    for(chan=0; chan<NCHAN; chan++)
    {
      flpSetOutputVoltages(chan, v1[slot][chan], v2[slot][chan], vled[slot][chan]);

      period[slot][chan] /= 20; /* convert to 20ns units */
      flpSetPulserPeriod(chan, period[slot][chan]);
    }
  }

  return(0);
}







/* upload setting from all found FLPs */
int
flpUploadAll(char *string, int length)
{
  int slot, i, ii, jj, kk, len1, len2;
  char *str, sss[1024];
  unsigned int tmp, val[NCHAN], val1[NCHAN];
  unsigned short sval[NCHAN];
  float f1, f2, f3;

  Nflp = flpGetNflp();
  for(kk=0; kk<Nflp; kk++)
  {
    slot = flpSlot(kk);

    for(jj=0; jj<NCHAN; jj++)
    {
      v1[slot][jj]=0.0;
      v2[slot][jj]=0.0;
      vled[slot][jj]=0.0;
      flpGetOutputVoltages(jj, (float *)&v1[slot][jj], (float *)&v2[slot][jj], (float *)&vled[slot][jj]);
#ifdef DEBUG
	  printf("UPLOAD V: slot %d, jj=%d: %f %f %f\n",slot,jj,v1[slot][jj],v2[slot][jj],vled[slot][jj]);
#endif
	  
	  period[slot][jj]=0;
      flpGetPulserPeriod(jj, (int *)&period[slot][jj]);
      period[slot][jj] *= 20; /* convert to ns */

	}

  }

  if(length)
  {
    str = string;
    str[0] = '\0';

    for(kk=0; kk<Nflp; kk++)
    {
      slot = flpSlot(kk);
      sprintf(sss,"FLP_SLOT %d\n",slot);
      ADD_TO_STRING;

      sprintf(sss,"FLP_V1 %f %f\n",v1[slot][0], v1[slot][1]);
      ADD_TO_STRING;

      sprintf(sss,"FLP_V2 %f %f\n",v2[slot][0], v2[slot][1]);
      ADD_TO_STRING;

      sprintf(sss,"FLP_VLED %f %f\n",vled[slot][0], vled[slot][1]);
      ADD_TO_STRING;

      sprintf(sss,"FLP_PERIOD %d %d\n",period[slot][0], period[slot][1]);
      ADD_TO_STRING;
	}

    CLOSE_STRING;
  }

}


int
flpUploadAllPrint()
{
  char str[10000];
  flpUploadAll(str, 10000);
  printf("%s",str);
}



#else /* dummy version*/

void
flpConfig_dummy()
{
  return;
}

#endif
