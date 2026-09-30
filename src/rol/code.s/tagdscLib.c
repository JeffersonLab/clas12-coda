
/* tagdscLib.c - library for J.Proffit discriminater/scaler board */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#ifdef Linux_vme

#include "jvme.h"
#include "tagdscLib.h"
#include "xxxConfig.h"
#include "codautil.h"

#define FNLEN     256       /* length of config. file name */
#define STRLEN    256       /* length of str_tmp */
#define ROCLEN    256       /* length of ROC_name */

/*configuration settings*/
static unsigned int tagdsc_width[NTAGDSC];
static unsigned int tagdsc_threshold[NTAGDSC];

/*some locals*/
static int active;
static unsigned long addr_start = 0;
static unsigned long addr_inc = 0x200;
static int nboards = 0;
static unsigned long dscA24[NTAGDSC];
static char *expid = NULL;

void
tagdscSetExpid(char *string)
{
  expid = strdup(string);
}


int
tagdscInit()
{
  int res, ii;
  int tagdscA24Base = 0x00000000;
  unsigned int data;
  unsigned long addr, laddr=0;

  res = vmeBusToLocalAdrs(0x39,(char *)(unsigned long)tagdscA24Base,(char **)&laddr);
  if (res != 0) 
  {
    printf("%s: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",
	     __FUNCTION__,tagdscA24Base);
    return(ERROR);
  }
  else
  {
    //printf("%s: laddr=0x%08x\n",__FUNCTION__,laddr);fflush(stdout);
    addr_start = 0;
    for(ii=0; ii<NTAGDSC; ii++)
    {
      addr = laddr+ii*0x200;
      data = vmeRead32((volatile uint32_t *)addr);
      //printf("%s: [%2d] reading 0x%08x: result = 0x%08x\n",__FUNCTION__,ii,addr,data);
      if(addr_start==0 && data!=0xFFFFFFFF) addr_start = addr;
      if(data!=0xFFFFFFFF)
      {
        if(addr_start==0)
	{
          addr_start = addr;
          nboards = 1;
	}
	else
	{
          nboards ++;
	}
        dscA24[nboards-1] = addr;
        printf("%s: found board %2d ad address 0x%08x (vme address 0x%08x)\n",__FUNCTION__,nboards-1,dscA24[nboards-1],tagdscA24Base+ii*0x200);
      }
    }
    printf("\n%s: found total %d boards\n\n",__FUNCTION__,nboards);
  }

  return(nboards);
}


unsigned int
tagdscReadCSR(int id)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id];
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}

unsigned int
tagdscReadThreshold(int id)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id] + (1<<2);
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}

unsigned int
tagdscWriteThreshold(int id, int threshold)
{
  unsigned long addr;

  addr = dscA24[id] + (1<<2);
  vmeWrite32((volatile uint32_t *)addr, threshold);

  return(0);
}

unsigned int
tagdscReadPulseWidth(int id)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id] + (2<<2);
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}

unsigned int
tagdscWritePulseWidth(int id, int width)
{
  unsigned long addr;

  addr = dscA24[id] + (2<<2);
  vmeWrite32((volatile uint32_t *)addr, width);

  return(0);
}

unsigned int
tagdscLatchGateScalers(int id)
{
  unsigned long addr1, addr2;

  addr1 = dscA24[id] + (3<<2);
  addr2 = dscA24[id] + (4<<2);
  vmeWrite32((volatile uint32_t *)addr1, 0); //latch gate1 scaler
  vmeWrite32((volatile uint32_t *)addr2, 0); //latch gate2 scaler

  return(0);
}

unsigned int
tagdscReadGateScaler1(int id)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id] + (3<<2);
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}

unsigned int
tagdscReadGateScaler2(int id)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id] + (4<<2);
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}

unsigned int
tagdscReadChannelDelay(int id, int chan)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id] + ((40+chan)<<2);
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}

unsigned int
tagdscWriteChannelDelay(int id, int chan, int delay)
{
  unsigned long addr;

  addr = dscA24[id] + ((40+chan)<<2);
  vmeWrite32((volatile uint32_t *)addr, delay);

  return(0);
}

unsigned int
tagdscLatchChannelScalers(int id)
{
  unsigned long addr1, addr2;

  addr1 = dscA24[id] + (20<<2);
  addr2 = dscA24[id] + (30<<2);
  vmeWrite32((volatile uint32_t *)addr1, 0); //latch scalers1
  vmeWrite32((volatile uint32_t *)addr2, 0); //latch scalers2

  return(0);
}

unsigned int
tagdscReadChannelScaler1(int id, int chan)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id] + ((20+chan)<<2);
  vmeWrite32((volatile uint32_t *)addr, 0); //latch scaler
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}

unsigned int
tagdscReadChannelScaler2(int id, int chan)
{
  unsigned long addr;
  unsigned int data;

  addr = dscA24[id] + ((30+chan)<<2);
  vmeWrite32((volatile uint32_t *)addr, 0); //latch scaler
  data = vmeRead32((volatile uint32_t *)addr);

  return(data);
}




/*************************/
/*configuration functions*/

int
tagdscConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  printf("tagdscConfig: nboards=%d\n",nboards);

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    tagdscUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    tagdscInitGlobals();
  }
  printf("1\n");fflush(stdout);

  /* read config file */
  if( (res = tagdscReadConfigFile(fname)) < 0 ) return(res);

  printf("2\n");fflush(stdout);
  /* download to all boards */
  tagdscDownloadAll();

  return(0);
}

void
tagdscInitGlobals()
{
  int ii, jj;

  for(ii=0; ii<NTAGDSC; ii++)
  {
    tagdsc_width[ii] = 100;
    tagdsc_threshold[ii] = 50;
  }
}




int
tagdscUploadAll(char *string, int length)
{
  int kk, len1, len2;
  char *str, sss[1024];

  for(kk=0; kk<nboards; kk++)
  {
    tagdsc_width[kk]     = tagdscReadPulseWidth(kk);
    tagdsc_threshold[kk] = tagdscReadThreshold(kk);
  }

  if(length)
  {
    str = string;
    str[0] = '\0';

    for(kk=0; kk<nboards; kk++)
    {
      sprintf(sss,"TAGDSC_BOARD %d\n",kk);
      ADD_TO_STRING;

      sprintf(sss,"TAGDSC_WIDTH %d\n",tagdsc_width[kk]);
      ADD_TO_STRING;

      sprintf(sss,"TAGDSC_THRESHOLD %d\n",tagdsc_threshold[kk]);
      ADD_TO_STRING;
    }

    CLOSE_STRING;
  }

}


int
tagdscUploadAllPrint()
{
  char str[16001];
  tagdscUploadAll(str, 16000);
  printf("%s",str);
}




int
tagdscReadConfigFile(char *filename)
{
  FILE   *fd;
  int    ii, jj, ch, kk = 0;
  char   str_tmp[STRLEN], str2[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  int    msk[NTAGDSCCHAN];
  int    args, i1, i2;
  float  f1;
  int    board, board1, board2, chan;
  char fname[FNLEN] = { "" };  /* config file name */
  char *getenv();
  char *clonparms;

  get_hostname(host,ROCLEN);  /* obtain our hostname */
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
      sprintf(fname, "%s/tagdsc/%s", clonparms, filename);
    }

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/tagdsc/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/tagdsc/%s.cnf", clonparms, expid);
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

      if(strcmp(keyword,"TAGDSC_CRATE") == 0)
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

      else if(active && (strcmp(keyword,"TAGDSC_BOARD")==0))
      {
	//printf("11\n");fflush(stdout);
        kk++;
        sscanf (str_tmp, "%*s %s", str2);
        /*printf("str2=%s\n",str2);*/
        if(isdigit(str2[0]))
        {
          board1 = atoi(str2);
          board2 = board1 + 1;
          if(board1<0 && board1>=NTAGDSC)
          {
            printf("\nReadConfigFile: Wrong board number %d\n\n",board1);
            return(-4);
          }
        }
        else if(!strcmp(str2,"all"))
        {
	  //printf("12\n");fflush(stdout);
          board1 = 0;
          board2 = NTAGDSC;
        }
        else
        {
	  //printf("13\n");fflush(stdout);
          printf("\nReadConfigFile: Wrong board >%s<, must be 'all' or actual board number\n\n",str2);
          return(-4);
        }
        /*printf("board1=%d board2=%d\n",board1,board2);*/
      }





      else if(active && ((strcmp(keyword,"TAGDSC_WIDTH") == 0) && (kk >= 0)))
      {
	//printf("14: board1=%d board2=%d i1=%d\n",board1,board2,i1);fflush(stdout);
        sscanf (str_tmp, "%*s %d", &i1);
        for(board=board1; board<board2; board++) tagdsc_width[board] = i1;
      }


      else if(active && ((strcmp(keyword,"TAGDSC_THRESHOLD") == 0) && (kk >= 0)))
      {
	//printf("15\n");fflush(stdout);
        sscanf (str_tmp, "%*s %d", &i1);
        for(board=board1; board<board2; board++) tagdsc_threshold[board] = i1;
      }


      else
      {
	//printf("19\n");fflush(stdout);
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

  printf("tagdscReadConfigFile !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);

  return(kk);
}


int
tagdscDownloadAll()
{
  int kk;

  for(kk=0; kk<nboards; kk++)
  {
    tagdscWritePulseWidth(kk, tagdsc_width[kk]);
    tagdscWriteThreshold(kk, tagdsc_threshold[kk]);
  }

  return(0);
}











#else

void
tagdscLib_dummy()
{
  return;
}

#endif
