
//#if defined(VXWORKS) || defined(Linux_vme)

#if defined(Linux)

/****************************************************************************
 *
 *  tipConfig.c  -  configuration library file for TIpcie board 
 *
 
config file format:

TIP_CRATE      <tdcecal1>     <- crate name, usually IP name

TIP_ADD_SLAVE 1                                 # for every slave need to be added

TIP_FIBER_DELAY_OFFSET 0x80 0xcf                # fiber delay and offsets

TIP_SYNC_DELAY_WIDTH 0x52 0x2f                  # sync delay and width (not used ???)

TIP_BLOCK_LEVEL 1                               # the number of events in readout block

TIP_BUFFER_LEVEL 1                              # 0 - pipeline mode, 1 - ROC Lock mode, >=2 - buffered mode

TIP_INPUT_PRESCALE bit prescale                 # bit: 0-5, prescale: 0-15, actual prescale value is 2^prescale

TIP_INPUT_MASK bit1 bit2 bit3 bit4 bit5 bit6    # bits: 0 or 1

TIP_RANDOM_TRIGGER en prescale                  # en: 0=disabled 1=enabled, prescale: 0-15, nominal rate = 500kHz/2^prescale

TIP_HOLDOFF   rule   time  timescale            # rule: 1-4, time: 0-127, timescale: 0-1
                                               # note:
                                                   rule 1 timescale: 0=16ns, 1=480ns  (max time=60,960ns)
                                                   rule 2 timescale: 0=16ns, 1=960ns  (max time=121,920ns)
                                                   rule 3 timescale: 0=32ns, 1=1920ns (max time=243,840ns)
                                                   rule 4 timescale: 0=64ns, 1=3840ns (max time=487,680ns)
                                               # all rules run simultaneously

TIP_FIBER_IN 1                                  # fiber number to be used as input

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*
#ifdef Linux_vme
#include "jvme.h"
#endif
*/

#include "tipConfig.h"
#include "TIpcieLib.h"
#include "xxxConfig.h"

#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */

#define MAXSLAVES 100

static int active;

static int is_slave;
static int nslave, slave_list[MAXSLAVES];
static unsigned int delay, offset;
/*static unsigned int sync_delay, sync_width;*/
static int block_level;
static int buffer_level;
static int input_prescale[6];
static int input_delays[6];
static int input_mask;
static int random_enabled;
static int random_prescale;
static int holdoff_rules[4];
static int holdoff_timescale[4];
/*static int fiber_in;*/


static char *expid = NULL;

void
tipSetExpid(char *string)
{
  expid = strdup(string);
}


/* tipInit() have to be called before this function */
int
tipConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  printf("tipConfig reached, fname >%s<\n",fname);

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    tipUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    tipInitGlobals();
  }

  /* read config file */
  if( (res = tipReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  tipDownloadAll();

  return(0);
}

int
tipInitGlobals()
{
  int ii, jj;

  is_slave = 0;
  delay = 0x80;
  offset = 0xcf;
  /*
  sync_delay = 0x52;
  sync_width = 0x2f;
  */
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
  for(ii=0; ii<6; ii++) input_prescale[ii] = 0;
  for(ii=0; ii<6; ii++) input_delays[ii] = 0;
  input_mask = 0x3f;
  /*fiber_in = 1;*/

  return(0);
}


/**/
int
tipReadConfigFile(char *filename)
{
  FILE   *fd;
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch;
  char   str_tmp[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  char   str2[2];
  int    args, i1, i2, i3, i4, i5, i6;
  int    slot, chan;
  unsigned int  ui1, ui2;
  char *getenv();
  char *clonparms;
  
  gethostname(host,ROCLEN);  /* obtain our hostname */
  for(jj=0; jj<strlen(host); jj++)
  {
    if(host[jj] == '.')
    {
      host[jj] = '\0';
      break;
    }
  }

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
      sprintf(fname, "%s/tip/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\nReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/tip/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/tip/%s.cnf", clonparms, expid);
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
      if(strcmp(keyword,"TIP_CRATE") == 0)
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

      else if(active && (strcmp(keyword,"TIP_ADD_SLAVE")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        slave_list[nslave++] = i1;
      }

      else if(active && (strcmp(keyword,"TIP_FIBER_DELAY_OFFSET")==0))
      {
        sscanf (str_tmp, "%*s %x %x", &i1, &i2);
        delay = i1;
        offset = i2;
      }
	  /*
      else if(active && (strcmp(keyword,"TIP_SYNC_DELAY_WIDTH")==0))
      {
        sscanf (str_tmp, "%*s %x %x", &i1, &i2);
        sync_delay = i1;
        sync_width = i2;
      }
	  */
      else if(active && (strcmp(keyword,"TIP_BLOCK_LEVEL")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        block_level = i1;
      }

      else if(active && (strcmp(keyword,"TIP_BUFFER_LEVEL")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        buffer_level = i1;
      }

      else if(active && (strcmp(keyword,"TIP_INPUT_PRESCALE")==0))
      {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        if((i1 < 1) || (i1 > 6))
        {
          printf("\nReadConfigFile: Invalid prescaler inputs selection, %s\n",str_tmp);
        }
        if((i2 < 0) || (i2 > 15))
        {
          printf("\nReadConfigFile: Invalid prescaler value selection, %s\n",str_tmp);
        }
        input_prescale[i1-1] = i2;
      }

      else if(active && (strcmp(keyword, "TIP_INPUT_DELAY")==0))
      {
        sscanf (str_tmp, "%*s %d %d", &i1, &i2);
        if((i1 < 1) || (i1 > 6))
        {
          printf("\nReadConfigFile: Invalid ts inputs selection, %s\n",str_tmp);
        }
        if((i2 < 0) || (i2 > 511))
        {
          printf("\nReadConfigFile: Invalid ts input delay, %s\n",str_tmp);
        }
        input_delays[i1-1] = i2;
      }

      else if(active && (strcmp(keyword,"TIP_INPUT_MASK")==0))
      {
        sscanf (str_tmp, "%*s %d %d %d %d %d %d",&i1,&i2,&i3,&i4,&i5,&i6);
 		printf("INPUT MASK = %d %d %d %d %d %d\n",i1,i2,i3,i4,i5,i6);
        if( (i1<0)||(i1>1)||(i2<0)||(i2>1)||(i3<0)||(i3>1)||(i4<0)||(i4>1)||(i5<0)||(i5>1)||(i6<0)||(i6>1) )
        {
          printf("\nReadConfigFile: Invalid input mask selection, %s\n",str_tmp);
        }
        input_mask = i1+(i2<<1)+(i3<<2)+(i4<<3)+(i5<<4)+(i6<<5);
		printf("INPUT MASK = 0x%08x\n",input_mask);
      }

      else if(active && (strcmp(keyword,"TIP_RANDOM_TRIGGER")==0))
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

      else if(active && (strcmp(keyword,"TIP_HOLDOFF")==0))
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
	  /*
      else if(active && (strcmp(keyword,"TIP_FIBER_IN")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        fiber_in = i1;
      }
	  */
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
tipConfigGetBlockLevel()
{
  printf("block_level = %d\n",block_level);
  return(block_level);
}

int
tipDownloadAll()
{
  int ii;

  /*
  tipIntDisable();
  tipDisableVXSSignals();
  */

  printf("tipDownloadAll reached\n");

  /*for(ii=0; ii<nslave; ii++) tipAddSlave(slave_list[ii]); done automatically in ROL1 */
  for(ii=0; ii<6; ii++) tipSetInputPrescale(ii+1,input_prescale[ii]);
  for(ii=0; ii<6; ii++) tipSetTSInputDelay(ii+1,input_delays[ii]);

  tipDisableTSInput(TIP_TSINPUT_ALL);
  tipEnableTSInput(input_mask);

  for(ii=0; ii<4; ii++) tipSetTriggerHoldoff(ii+1,holdoff_rules[ii],holdoff_timescale[ii]);

#if 1
  tipSetFiberDelay(delay, offset);
#endif

  /*
  tipSetSyncDelayWidth(sync_delay, sync_width, 0);
  */
  tipSetInstantBlockLevelChange(1); /* enable immediate block level setting */
  printf("tipDownloadAll: setting block_level = %d\n",block_level);
  tipSetBlockLevel(block_level);
  tipSetInstantBlockLevelChange(0); /* disable immediate block level setting */

  printf("tipDownloadAll: setting buffer_level = %d\n",buffer_level);
  tipSetBlockBufferLevel(buffer_level);

  if(!random_enabled)
  {
    tipDisableRandomTrigger();
    tipSetTriggerSource(TIP_TRIGGER_TSINPUTS);
  }
  else
  {
    tipSetRandomTrigger(1, random_prescale);
    tipSetTriggerSource(TIP_TRIGGER_RANDOM);
  }

  /*tipSetFiberIn_preInit(fiber_in);*/

  return(0);
}

void
tipMon(int slot)
{
  tipStatus(1);
}


/*
static int is_slave;
static nslave, slave_list[MAXSLAVES];
static unsigned int delay, offset;
static int block_level;
static int buffer_level;
static int input_prescale[6];
*/

/* upload setting */
int
tipUploadAll(char *string, int length)
{
  int slot, i, ii, jj, kk, ifiber, len1, len2;
  char *str, sss[1024];
  unsigned int tmp;
  int connectedfibers;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];
  /*
  printf("\ntipUploadAll reached\n");
  */
  nslave = 0;
  connectedfibers = tipGetConnectedFiberMask();
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
  block_level = tipGetCurrentBlockLevel();
  buffer_level = tipGetBlockBufferLevel();
  /*
  printf("tipUploadAll: block_level=%d, buffer_level=%d\n",block_level,buffer_level);
  */

  input_mask = tipGetTSInputMask();

  for(ii = 0; ii < 6; ii++)
  {
    input_prescale[ii] = tipGetInputPrescale(ii+1);
    input_delays[ii] = tipGetTSInputDelay(ii+1);
  }

  random_enabled = tipGetRandomTriggerEnable(1);
  random_prescale = tipGetRandomTriggerSetting(1);

  /*DO NOT USE tipGetFiberDelay() UNTIL IT RETURNS RIGHT VALUES, OTHERWISE CALLING tipUploadAll() WILL SCRUDUP FOLLOWING tipDownloadAll() !!!!!!!!
  delay = tipGetFiberDelay();
  sync_delay = tipGetSyncDelay();
  */

  /*fiber_in = tipGetSlavePort();*/

  if(length)
  {
    str = string;
    str[0] = '\0';

	for(ii=0; ii<nslave; ii++)
	{
      sprintf(sss,"TIP_ADD_SLAVE %d\n",slave_list[ii]);
      ADD_TO_STRING;
    }

    sprintf(sss,"TIP_BLOCK_LEVEL %d\n",block_level);
    ADD_TO_STRING;

    sprintf(sss,"TIP_BUFFER_LEVEL %d\n",buffer_level);
    ADD_TO_STRING;
	/*
    sprintf(sss,"TIP_FIBER_IN %d\n",fiber_in);
    ADD_TO_STRING;
	*/
    for(ii=0; ii<4; ii++)
    {
      tmp = tipGetTriggerHoldoff(ii+1);
      holdoff_rules[ii] = tmp & 0x7F;
      holdoff_timescale[ii] = (tmp>>7)&0x1;
      sprintf(sss,"TIP_HOLDOFF %d %d %d\n",ii+1,holdoff_rules[ii],holdoff_timescale[ii]);
      ADD_TO_STRING;
    }

    sprintf(sss,"TIP_INPUT_MASK %d %d %d %d %d %d\n",
      (input_mask>>0)&1,(input_mask>>1)&1,(input_mask>>2)&1,
      (input_mask>>3)&1,(input_mask>>4)&1,(input_mask>>5)&1);
    ADD_TO_STRING;

    for(ii = 0; ii < 6; ii++)
    {
      sprintf(sss,"TIP_INPUT_DELAY %d %d\n",ii+1,input_delays[ii]);
      ADD_TO_STRING;
    }


    for(ii = 0; ii < 6; ii++)
    {
      sprintf(sss,"TIP_INPUT_PRESCALE %d %d\n",ii+1,input_prescale[ii]);
      ADD_TO_STRING;
    }

    sprintf(sss,"TIP_RANDOM_TRIGGER %d %d\n",random_enabled,random_prescale);
    ADD_TO_STRING;

	
    sprintf(sss,"TIP_FIBER_DELAY_OFFSET %d %d\n",delay,0);
    ADD_TO_STRING;
	/*
    sprintf(sss,"TIP_SYNC_DELAY_WIDTH %d %d\n",sync_delay,0);
    ADD_TO_STRING;
	*/

    CLOSE_STRING;
  }

}



int
tipUploadAllPrint()
{
  char str[16001];
  tipUploadAll(str, 16000);
  printf("%s",str);
}





#else /* dummy version*/

void
tipConfig_dummy()
{
  return;
}

#endif
