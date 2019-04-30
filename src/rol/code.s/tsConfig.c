#if defined(VXWORKS) || defined(Linux_vme)


/****************************************************************************
 *
 *  tsConfig.c  -  configuration library file for ts board 
 *
 
config file format:

TS_CRATE      <tdcecal1>     <- crate name, usually IP name

TS_ADD_SLAVE 1                                 # for every slave need to be added

TS_FIBER_DELAY_OFFSET 0x80 0xcf                # fiber delay and offsets

TS_BLOCK_LEVEL 1                               # the number of events in readout block

TS_BUFFER_LEVEL 1                              # 0 - pipeline mode, 1 - ROC Lock mode, >=2 - buffered mode

TS_GTP_PRESCALE bit prescale                   # bit: 0-31, prescale: 0-15, actual prescale value is 2^prescale
TS_FP_PRESCALE bit prescale                    # bit: 0-31, prescale: 0-15, actual prescale value is 2^prescale

TS_GTP_INPUT_MASK 0x7FFFFFFF                   # 32-bit mask in HEX
TS_FP_INPUT_MASK 0x7FFFFFFF                    # 32-bit mask in HEX

TS_RANDOM_TRIGGER en prescale                  # en: 0=disabled 1=enabled, prescale: 0-15, nominal rate = 500kHz/2^prescale

TS_HOLDOFF   rule   time  timescale            # rule: 1-4, time: 0-127, timescale: 0-1
                                               # note:
                                                   rule 1 timescale: 0=16ns, 1=480ns  (max time=60,960ns)
                                                   rule 2 timescale: 0=16ns, 1=960ns  (max time=121,920ns)
                                                   rule 3 timescale: 0=32ns, 1=1920ns (max time=243,840ns)
                                                   rule 4 timescale: 0=64ns, 1=3840ns (max time=487,680ns)
                                               # all rules run simultaneously

TS_FIBER_IN 1                                  # fiber number to be used as input

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef Linux_vme
#include "jvme.h"
#endif

#include "tsLib.h"
#include "tsConfig.h"
#include "xxxConfig.h"

#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */

#define MAXSLAVES 100

static int active;

static int is_slave;
static int nslave, slave_list[MAXSLAVES];
static unsigned int delay, offset;
static int block_level;
static int buffer_level;
static int gtp_prescale[32];
static int fp_prescale[32];
static int gtp_input_mask;
static int fp_input_mask;
static int random_enabled;
static int random_prescale;
static int holdoff_rules[4];
static int holdoff_timescale[4];
static int fiber_in;


static char *expid= NULL;

void
tsSetExpid(char *string)
{
  expid = strdup(string);
}

/* tsInit() have to be called before this function */
int  
tsConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  printf("tsConfig reached, fname >%s<\n",fname);

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    tsUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    tsInitGlobals();
  }

  /* read config file */
  if( (res = tsReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  tsDownloadAll();

  return(0);
}

int
tsInitGlobals()
{
  int ii, jj;

  is_slave = 0;
  delay = 0x80;
  offset = 0xcf;
  block_level = 1;
  buffer_level = 1;
  nslave = 0;
  random_enabled = 0;
  random_prescale = 0;
  holdoff_rules[0] = 10;
  holdoff_rules[1] = 1;
  holdoff_rules[2] = 1;
  holdoff_rules[3] = 1;
  holdoff_timescale[0] = 0;
  holdoff_timescale[1] = 0;
  holdoff_timescale[2] = 0;
  holdoff_timescale[3] = 0;
  for(ii=0; ii<MAXSLAVES; ii++) slave_list[ii] = 0;
  for(ii=0; ii<32; ii++) gtp_prescale[ii] = 0;
  for(ii=0; ii<32; ii++) fp_prescale[ii] = 0;
  gtp_input_mask = 0xFFFFFFFF;
  fp_input_mask = 0xFFFFFFFF;
  fiber_in = 1;

  return(0);
}


/**/
int
tsReadConfigFile(char *filename)
{
  FILE   *fd;
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch;
  char   str_tmp[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  char   str2[2];
  int    args, i1, i2, i3, i4, i5, i6, k[32];
  int    slot, chan;
  unsigned int  ui1, ui2;
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
      sprintf(fname, "%s/ts/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/ts/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/ts/%s.cnf", clonparms, expid);
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
      if(strcmp(keyword,"TS_CRATE") == 0)
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

      else if(active && (strcmp(keyword,"TS_ADD_SLAVE")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        slave_list[nslave++] = i1;
      }

      else if(active && (strcmp(keyword,"TS_FIBER_DELAY_OFFSET")==0))
      {
        sscanf (str_tmp, "%*s %x %x", &i1, &i2);
        delay = i1;
        offset = i2;
      }

      else if(active && (strcmp(keyword,"TS_BLOCK_LEVEL")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        block_level = i1;
      }

      else if(active && (strcmp(keyword,"TS_BUFFER_LEVEL")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        buffer_level = i1;
      }

      else if(active && (strcmp(keyword,"TS_GTP_PRESCALE")==0))
      {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        if((i1 < 1) || (i1 > 32))
        {
          printf("\nReadConfigFile: Invalid GTP prescaler inputs selection, %s\n",str_tmp);
        }
        if((i2 < 0) || (i2 > 15))
        {
          printf("\nReadConfigFile: Invalid GTP prescaler value selection, %s\n",str_tmp);
        }
        gtp_prescale[i1-1] = i2;
      }

      else if(active && (strcmp(keyword,"TS_FP_PRESCALE")==0))
      {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        if((i1 < 1) || (i1 > 32))
        {
          printf("\nReadConfigFile: Invalid FP prescaler inputs selection, %s\n",str_tmp);
        }
        if((i2 < 0) || (i2 > 15))
        {
          printf("\nReadConfigFile: Invalid FP prescaler value selection, %s\n",str_tmp);
        }
        fp_prescale[i1-1] = i2;
      }



      else if(active && (strcmp(keyword,"TS_INPUT_MASK")==0))
      {
        sscanf (str_tmp, "%*s 0x%X",&i1);
        fp_input_mask = i1;
        printf("\nReadConfigFile: fp_input_mask = 0x%08x\n",fp_input_mask);
      }
      else if(active && (strcmp(keyword,"TS_FP_INPUT_MASK")==0))
      {
        sscanf (str_tmp, "%*s 0x%X",&i1);
        fp_input_mask = i1;
        printf("\nReadConfigFile: fp_input_mask = 0x%08x\n",fp_input_mask);
      }



      else if(active && (strcmp(keyword,"TS_GTP_INPUT_MASK")==0))
      {
        sscanf(str_tmp, "%*s 0x%X", &i1);
        gtp_input_mask = i1;
        printf("\nReadConfigFile: gtp_input_mask = 0x%08x\n",gtp_input_mask);
      }

      else if(active && (strcmp(keyword,"TS_RANDOM_TRIGGER")==0))
      {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        if((i1 < 0) || (i1 > 1))
        {
          printf("\nReadConfigFile: Invalid random enable option, %s\n",str_tmp);
        }
        if((i2 < 0) || (i2 > 15))
        {
          printf("\nReadConfigFile: Invalid random prescaler value selction, %s\n",str_tmp);
        }
        random_enabled = i1;
        random_prescale = i2;
      }

      else if(active && (strcmp(keyword,"TS_HOLDOFF")==0))
      {
        sscanf (str_tmp, "%*s %d %d %d", &i1, &i2, &i3);
        if((i1 < 1) || (i1 > 4))
        {
          printf("\nReadConfigFile: Invalid holdoff rule selection, %s\n",str_tmp);
        }
        if((i2 < 0) || (i2 > 127))
        {
          printf("\nReadConfigFile: Invalid holdoff time, %s\n",str_tmp);
        }
        if((i3 < 0) || (i3 > 1))
        {
          printf("\nReadConfigFile: Invalid holdoff timescale, %s\n",str_tmp);
        }
        holdoff_rules[i1-1] = i2;
        holdoff_timescale[i1-1] = i3;
      }

      else if(active && (strcmp(keyword,"TS_FIBER_IN")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        fiber_in = i1;
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
tsConfigGetBlockLevel()
{
  printf("block_level = %d\n",block_level);
  return(block_level);
}

int
tsConfigGetBufferLevel()
{
  printf("buffer_level = %d\n",buffer_level);
  return(buffer_level);
}

int
tsDownloadAll()
{
  int ii;

  printf("tsDownloadAll reached\n");

  for(ii=0; ii<32; ii++) tsSetTriggerPrescale(1, ii, gtp_prescale[ii]);
  for(ii=0; ii<32; ii++) tsSetTriggerPrescale(2, ii, fp_prescale[ii]);

  tsSetFPInput(fp_input_mask);
  printf("tsDownloadAll: setting fp_input_mask=0x%08x\n",fp_input_mask);
  fp_input_mask = tsGetFPInput();
  printf("tsDownloadAll: reading back fp_input_mask=0x%08x\n",fp_input_mask);

  tsSetGTPInput(gtp_input_mask);
  printf("tsDownloadAll: setting gtp_input_mask=0x%08x\n",gtp_input_mask);
  gtp_input_mask = tsGetGTPInput();
  printf("tsDownloadAll: reading back gtp_input_mask=0x%08x\n",gtp_input_mask);

  for(ii=0; ii<4; ii++) tsSetTriggerHoldoff(ii+1,holdoff_rules[ii],holdoff_timescale[ii]);
  
  /*tsSetFiberDelay(delay, offset);TS*/

  tsSetInstantBlockLevelChange(1); /* enable immediate block level setting */
  printf("tsDownloadAll: setting block_level = %d\n",block_level);
sleep(1);
  tsSetBlockLevel(block_level);
sleep(1);
  tsSetInstantBlockLevelChange(0); /* disable immediate block level setting */

  printf("tsDownloadAll: setting buffer_level = %d\n",buffer_level);
  tsSetBlockBufferLevel(buffer_level);

  if(!random_enabled)
  {
    tsDisableRandomTrigger();
    tsSetTriggerSource(6);
  }
  else
  {
    tsSetRandomTrigger(1, random_prescale);
    tsSetTriggerSource(5);
  }

  /*tsSetFiberIn_preInit(fiber_in);TS*/

  return(0);
}

void
tsMon(int slot)
{
  tsStatus(1);
}


/* upload setting */
int
tsUploadAll(char *string, int length)
{
  int slot, i, ii, jj, kk, ifiber, len1, len2;
  char *str, sss[4096];
  unsigned int tmp;
  int connectedfibers;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];
  /*
  printf("\ntsUploadAll reached\n");
  */
  nslave = 0;
  /*connectedfibers = tsGetConnectedFiberMask();TS*/
  if(connectedfibers>0)
  {
	for(ifiber=0; ifiber<8; ifiber++)
	{
	  if( connectedfibers & (1<<ifiber) )
	  {
        slave_list[nslave++] = ifiber+1;
	  }
    }
  }
  block_level = tsGetCurrentBlockLevel();
  buffer_level = tsGetBlockBufferLevel();
  /*
  printf("tsUploadAll: block_level=%d, buffer_level=%d\n",block_level,buffer_level);
  */

  fp_input_mask = tsGetFPInput();
  gtp_input_mask = tsGetGTPInput();

  for(ii=0; ii<32; ii++)
  {
    gtp_prescale[ii] = tsGetTriggerPrescale(1, ii);
    fp_prescale[ii] = tsGetTriggerPrescale(2, ii);
  }

  /*random_enabled = tsGetRandomTriggerEnable(1);TS*/
  /*random_prescale = tsGetRandomTriggerSetting(1);TS*/

  /*fiber_in = tsGetSlavePort();TS*/

  if(length)
  {
    str = string;
    str[0] = '\0';

	for(ii=0; ii<nslave; ii++)
	{
      sprintf(sss,"TS_ADD_SLAVE %d\n",slave_list[ii]);
      ADD_TO_STRING;
    }

    sprintf(sss,"TS_BLOCK_LEVEL %d\n",block_level);
    ADD_TO_STRING;

    sprintf(sss,"TS_BUFFER_LEVEL %d\n",buffer_level);
    ADD_TO_STRING;

    sprintf(sss,"TS_FIBER_IN %d\n",fiber_in);
    ADD_TO_STRING;

    for(ii=0; ii<4; ii++)
    {
      tmp = tsGetTriggerHoldoff(ii+1);
      holdoff_rules[ii] = tmp & 0x7F;
      holdoff_timescale[ii] = (tmp>>7)&0x1;
      sprintf(sss,"TS_HOLDOFF %d %d %d\n",ii+1,holdoff_rules[ii],holdoff_timescale[ii]);
      ADD_TO_STRING;
    }

    sprintf(sss,"TS_GTP_INPUT_MASK 0x%08X\n",gtp_input_mask);
    ADD_TO_STRING;

    sprintf(sss,"TS_FP_INPUT_MASK 0x%08x\n",fp_input_mask);
    ADD_TO_STRING;

    for(ii = 0; ii < 32; ii++)
    {
      sprintf(sss,"TS_GTP_PRESCALE %d %d\n",ii+1,gtp_prescale[ii]);
      ADD_TO_STRING;
    }

    for(ii = 0; ii < 32; ii++)
    {
      sprintf(sss,"TS_FP_PRESCALE %d %d\n",ii+1,fp_prescale[ii]);
      ADD_TO_STRING;
    }

    sprintf(sss,"TS_RANDOM_TRIGGER %d %d\n",random_enabled,random_prescale);

    CLOSE_STRING;
  }

}



int
tsUploadAllPrint()
{
  char str[16001];
  tsUploadAll(str, 16000);
  printf("%s",str);
}





#else /* dummy version*/

void
tsConfig_dummy()
{
  return;
}

#endif
