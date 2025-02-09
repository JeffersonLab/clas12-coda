
/* petirocConfig.c */

#ifndef Linux_armv7l

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "petirocConfig.h"
#include "xxxConfig.h"

static int active;
static int npetiroc;
static PETIROC_CONF petiroc[PETIROC_MAX_NUM];

#define SCAN_MSK \
  sscanf (str_tmp, "%*s %d %d %d %d %d %d %d %d   \
                        %d %d %d %d %d %d %d %d", \
          &msk[ 0], &msk[ 1], &msk[ 2], &msk[ 3], \
          &msk[ 4], &msk[ 5], &msk[ 6], &msk[ 7], \
          &msk[ 8], &msk[ 9], &msk[10], &msk[11], \
          &msk[12], &msk[13], &msk[14], &msk[15])

static char *expid = NULL;

void
petirocSetExpid(char *string)
{
  expid = strdup(string);
}

int
petirocConfig(char *fname)
{
  int res;

  npetiroc = petirocGetNpetiroc();
  printf("%s: npetiroc=%d\n", __func__, npetiroc);fflush(stdout);

  //petirocInitGlobals();

  if( (res = petirocReadConfigFile(fname)) < 0 )
  {
    printf("petirocConfig: ERROR: petirocReadConfigFile() returns %d\n",res);fflush(stdout);
    return(res);
  }

  /* download to all boards) */
  petirocDownloadAll();

  return(0);
}

void
petirocInitGlobals()
{
  printf("%s Reached\n", __func__);
  memset(petiroc, 0, sizeof(petiroc));
}

int
petirocReadConfigFile_CheckArgs(int sscan_ret, int req, char *keyword)
{
  if(sscan_ret != req)
  {
    printf("%s: Error in %s arguments: returned %s, expected %d\n",
           __func__, keyword, sscan_ret, req);
    return -1;
  }
  return 0;
}

int
petirocReadConfigFile(char *filename_in)
{
  FILE   *fd;
  char   filename[FNLEN];
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch;
  char   str_tmp[STRLEN], str2[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  int    args, i1, i2, i3, i4, i5, msk[16];
  long long ll1;
  int    slot, slot1=0, slot2=-1, chan;
  int    asic, asic1=0, asic2=-1;
  unsigned int  ui[6];
  float f1, fmsk[16];
  char *clonparms;
  int do_parsing, error, argc;

  gethostname(host,ROCLEN);  /* obtain our hostname */
  clonparms = getenv("CLON_PARMS");
  printf("CLON_PARMS=>%s< from environment\n",clonparms);fflush(stdout);

  if(expid==NULL)
  {
    expid = getenv("EXPID");
    printf("\nNOTE: use EXPID=>%s< from environment\n",expid);fflush(stdout);
  }
  else
  {
    printf("\nNOTE: use EXPID=>%s< from CODA\n",expid);fflush(stdout);
  }

  strcpy(filename,filename_in); /* copy filename from parameter list to local string */
  do_parsing = 1;

  while(do_parsing)
  {
    if(strlen(filename)!=0) /* filename specified */
    {
      if ( filename[0]=='/' || (filename[0]=='.' && filename[1]=='/') )
      {
        sprintf(fname, "%s", filename);
      }
      else
      {
        sprintf(fname, "%s/petiroc/%s", clonparms, filename);
      }

      if((fd=fopen(fname,"r")) == NULL)
      {
        printf("\n%s: Can't open config file >%s<\n",__func__,fname);fflush(stdout);
        return(-1);
      }
    }
    else if(do_parsing<2) /* filename does not specified */
    {
      sprintf(fname, "%s/petiroc/%s.cnf", clonparms, host);
      if((fd=fopen(fname,"r")) == NULL)
      {
        sprintf(fname, "%s/petiroc/%s.cnf", clonparms, expid);
        if((fd=fopen(fname,"r")) == NULL)
        {
          printf("\n%s: Can't open config file >%s<\n",__func__,fname);fflush(stdout);
          return(-2);
        }
      }
    }
    else
    {
      printf("\n%s: ERROR: since do_parsing=%d (>1), filename must be specified\n",__func__,do_parsing);fflush(stdout);
      return(-1);
    }

    printf("\n%s: Using configuration file >%s<\n",__func__,fname);fflush(stdout);

    /* Parsing of config file */
    active = 0;
    do_parsing = 0; /* will parse only one file specified above, unless it changed during parsing */
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
        /* Start parsing real config inputs */
        if(strcmp(keyword,"PETIROC_CRATE") == 0)
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
          continue;
        }
        
        if(!active)
          continue;
        
        
        if(!strcmp(keyword,"PETIROC_SLOT") || !strcmp(keyword,"PETIROC_SLOTS"))
        {
          sscanf (str_tmp, "%*s %s", str2);
          /*printf("str2=%s\n",str2);*/
          if(isdigit(str2[0]))
          {
            slot1 = atoi(str2);
            slot2 = slot1 + 1;
            if(slot1<0 && slot1>PETIROC_MAX_NUM)
            {
              printf("\nReadConfigFile: Wrong slot number %d\n\n",slot1);
              return(-4);
            }
          }
          else if(!strcmp(str2,"all"))
          {
            slot1 = 0;
            slot2 = PETIROC_MAX_NUM;
          }
          else
          {
            printf("\nReadConfigFile: Wrong slot >%s<, must be 'all' or actual slot number\n\n",str2);
            return(-4);
          }
          /*printf("slot1=%d slot2=%d\n",slot1,slot2);*/
        }
        else if((strcmp(keyword,"PETIROC_W_WIDTH")==0))
        {
          sscanf (str_tmp, "%*s %d %d", &i1);
          for(slot=slot1; slot<slot2; slot++) petiroc[slot].window_width= i1;
        }
        else if((strcmp(keyword,"PETIROC_W_OFFSET")==0))
        {
          sscanf (str_tmp, "%*s %d %d", &i1);
          for(slot=slot1; slot<slot2; slot++) petiroc[slot].window_offset= i1;
        }
        else if(!strcmp(keyword,"PETIROC_ASIC"))
        {
          sscanf (str_tmp, "%*s %s", str2);
          if(isdigit(str2[0]))
          {
            asic1 = atoi(str2);
            asic2 = asic1 + 1;
            if(asic1<0 || asic1>2)
            {
              printf("\nReadConfigFile: Wrong asic number %d\n\n",slot1);
              return(-4);
            }
          }
          else if(!strcmp(str2,"all"))
          {
            asic1 = 0;
            asic2 = 2;
          }
          else
          {
            printf("\nReadConfigFile: Wrong asic >%s<, must be 'all' or actual fiber number\n\n",str2);
            return(-4);
          }
        }
        else if(!strcmp(keyword,"PETIROC_REG_MASK_DISC_CHARGE"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.mask_discri_charge = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_INDAC_0_15"))
        {
          argc = SCAN_MSK;
          if(error = petirocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch0  = msk[0];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch1  = msk[1];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch2  = msk[2];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch3  = msk[3];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch4  = msk[4];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch5  = msk[5];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch6  = msk[6];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch7  = msk[7];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch8  = msk[8];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch9  = msk[9];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch10 = msk[10];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch11 = msk[11];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch12 = msk[12];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch13 = msk[13];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch14 = msk[14];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch15 = msk[15];

            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch0  = (msk[0]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch1  = (msk[1]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch2  = (msk[2]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch3  = (msk[3]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch4  = (msk[4]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch5  = (msk[5]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch6  = (msk[6]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch7  = (msk[7]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch8  = (msk[8]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch9  = (msk[9]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch10 = (msk[10]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch11 = (msk[11]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch12 = (msk[12]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch13 = (msk[13]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch14 = (msk[14]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch15 = (msk[15]>0) ? 1 : 0;
          }
        }
        else if(!strcmp(keyword,"PETIROC_REG_INDAC_16_31"))
        {
          argc = SCAN_MSK;
          if(error = petirocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch16 = msk[0];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch17 = msk[1];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch18 = msk[2];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch19 = msk[3];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch20 = msk[4];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch21 = msk[5];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch22 = msk[6];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch23 = msk[7];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch24 = msk[8];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch25 = msk[9];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch26 = msk[10];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch27 = msk[11];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch28 = msk[12];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch29 = msk[13];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch30 = msk[14];
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch31 = msk[15];

            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch16 = (msk[0]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch17 = (msk[1]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch18 = (msk[2]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch19 = (msk[3]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch20 = (msk[4]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch21 = (msk[5]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch22 = (msk[6]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch23 = (msk[7]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch24 = (msk[8]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch25 = (msk[9]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch26 = (msk[10]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch27 = (msk[11]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch28 = (msk[12]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch29 = (msk[13]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch30 = (msk[14]>0) ? 1 : 0;
            petiroc[slot].chip[asic].SlowControl.inputDAC_en_ch31 = (msk[15]>0) ? 1 : 0;
          }
        }
        else if(!strcmp(keyword,"PETIROC_REG_MASK_DISC_TIME"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.mask_discri_time = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_DAC6B_0_15"))
        {
          argc = SCAN_MSK;
          if(error = petirocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch0  = msk[0];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch1  = msk[1];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch2  = msk[2];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch3  = msk[3];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch4  = msk[4];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch5  = msk[5];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch6  = msk[6];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch7  = msk[7];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch8  = msk[8];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch9  = msk[9];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch10 = msk[10];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch11 = msk[11];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch12 = msk[12];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch13 = msk[13];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch14 = msk[14];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch15 = msk[15];
          }
        }
        else if(!strcmp(keyword,"PETIROC_REG_DAC6B_16_31"))
        {
          argc = SCAN_MSK;
          if(error = petirocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch16 = msk[0];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch17 = msk[1];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch18 = msk[2];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch19 = msk[3];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch20 = msk[4];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch21 = msk[5];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch22 = msk[6];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch23 = msk[7];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch24 = msk[8];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch25 = msk[9];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch26 = msk[10];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch27 = msk[11];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch28 = msk[12];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch29 = msk[13];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch30 = msk[14];
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch31 = msk[15];
          }
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_10B_DAC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_10b_DAC = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_VTH_CHARGE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.vth_discri_charge = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_VTH_TIME"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.vth_time = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_VTH_TIME"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.vth_time = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_ADC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_ADC = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_SEL_STARTB_RAMP_ADC_EXT"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.sel_startb_ramp_ADC_ext = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_USEBCOMP"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.usebcompensation = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_ENBIASDAC_DELAY"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.ENbiasDAC_delay = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_ENBIASRAMP_DELAY"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.ENbiasramp_delay = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_DACDELAY"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.DACdelay = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_DISC_DELAY"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_discri_delay = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_TEMP_SENSOR"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_temp_sensor = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_PA"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_pa = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_DISC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_discri = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_CMD_POLARITY"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.cmd_polarity = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_LATCHDISC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.LatchDiscri = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_6B_DAC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_6b_DAC = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_TDC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_tdc = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_ON_INPUT_DAC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.ON_input_DAC = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_CHARGE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_charge = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_CF"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.Cf = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_SCA"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_sca = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_DISC_CHARGE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_discri_charge = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_DISC_ADC_TIME"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_discri_ADC_time = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_BIAS_DISC_ADC_CHARGE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_bias_discri_ADC_charge = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_DIS_RAZCHN_INT"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.DIS_razchn_int = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_DIS_RAZCHN_EXT"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.DIS_razchn_ext = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_SEL_80M"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.SEL_80M = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_80M"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_80M = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_SLOW_LVDS_REC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_slow_lvds_rec = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_FAST_LVDS_REC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_fast_lvds_rec = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_TRANSMITTER"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_transmitter = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_ON_1MA"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.ON_1mA = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_ON_2MA"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.ON_2mA = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_ON_OTA_MUX"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.ON_ota_mux = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_ON_OTA_PROBE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.ON_ota_probe = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_DIS_TRIG_MUX"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.DIS_trig_mux = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_NOR32_TIME"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_NOR32_time = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_NOR32_CHARGE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_NOR32_charge = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_DIS_TRIGGERS"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.DIS_triggers = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_DOUT_OC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_dout_oc = i1;
        }
        else if(!strcmp(keyword,"PETIROC_REG_EN_TRANSMIT"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            petiroc[slot].chip[asic].SlowControl.EN_transmit = i1;
        }
        else if(!strcmp(keyword,"PETIROC_CTEST_FREQ"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          printf("pulser_freq=%d\n", i1);
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].pulser_freq = i1;
        }
        else if(!strcmp(keyword,"PETIROC_CTEST_DAC"))
        {
          argc = sscanf (str_tmp, "%*s %d %d %d %d", &i1,&i2,&i3,&i4);
          if(error = petirocReadConfigFile_CheckArgs(argc, 4, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          {
            petiroc[slot].pulser_amp[0] = i1;
            petiroc[slot].pulser_amp[1] = i2;
            petiroc[slot].pulser_amp[2] = i3;
            petiroc[slot].pulser_amp[3] = i4;
          }
        }
        else if(!strcmp(keyword,"PETIROC_CTEST_ENABLE"))
        {
          argc = sscanf (str_tmp, "%*s %d %d %d %d", &i1,&i2,&i3,&i4);
          if(error = petirocReadConfigFile_CheckArgs(argc, 4, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          {
            petiroc[slot].pulser_en = i1 ? 0x1 : 0x0;
            petiroc[slot].pulser_en|= i2 ? 0x2 : 0x0;
            petiroc[slot].pulser_en|= i3 ? 0x4 : 0x0;
            petiroc[slot].pulser_en|= i4 ? 0x8 : 0x0;
          }
        }
        else if(!strcmp(keyword,"PETIROC_TDC_AUTO_CAL_EN"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].tdc_auto_cal_en = i1;
        }
        else if(!strcmp(keyword,"PETIROC_TDC_AUTO_CAL_MIN_ENTRIES"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].tdc_auto_cal_min_entries = i1;
        }
        else if(!strcmp(keyword,"PETIROC_TRIG_BUSY_THR"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].trig_busy_thr = i1;
        }
        else if(!strcmp(keyword,"PETIROC_TDC_ENABLE"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X 0x%X", &ui[0],&ui[1]);
          if(error = petirocReadConfigFile_CheckArgs(argc, 2, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(ii=0; ii<2; ii++)
            petiroc[slot].tdc_enable_mask[ii] = ui[ii];
        }
        else if(!strcmp(keyword,"PETIROC_W_WIDTH"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].window_width = i1;
        }
        else if(!strcmp(keyword,"PETIROC_W_OFFSET"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].window_offset = i1;
        }
        else if(!strcmp(keyword,"PETIROC_TRIG_DELAY"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].trig_delay = i1;
        }
        else if(!strcmp(keyword,"PETIROC_CLK_EXT"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = petirocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            petiroc[slot].clk_ext = i1;
        }
        else
        {
          /* unknown key - do nothing */
          printf("petirocReadConfigFile: Unknown Field or Missed Field in\n");
          printf("   %s \n", fname);
          printf("   str_tmp=%s", str_tmp);
          printf("   keyword=%s \n\n", keyword);
//          return(-10);
        }
      }
    } /* end of while */
    fclose(fd);
  }

  return(0);
}

int
petirocDownloadAll()
{
  int slot, ii, jj, kk, ch;
  int asic, mask;

  printf("\n\n%s reached, npetiroc=%d, configuring ..\n", __func__, npetiroc); 
  for(ii=0; ii<npetiroc; ii++)
  {
    slot = petirocSlot(ii);

    petiroc[slot].fw_rev = petiroc_get_fwrev(slot);
    printf("PETIROC slot %d firmware_rev=%08X\n", ii, petiroc[slot].fw_rev);

    petiroc[slot].fw_timestamp = petiroc_get_fwtimestamp(slot);
    printf("PETIROC slot %d timestamp=%08X\n", ii, petiroc[slot].fw_timestamp);

    petiroc_set_clk(slot, petiroc[slot].clk_ext);

    petiroc_cfg_pwr(slot,1,1,0,1,0,0);
      
    petiroc_startb_adc(slot, 1);
    petiroc_raz_chn(slot, 0);
    petiroc_val_evt(slot, 0, 0, 0);
    petiroc_enable(slot, 0);
    petiroc_hold_ext(slot, 0);
    petiroc_start_conv(slot, 0);

    petiroc_set_pulser(
        slot,
        petiroc[slot].pulser_en,
        0xFFFFFFFF,                       // NCycles (0xFFFFFFFF=infinite)
        (float)petiroc[slot].pulser_freq,
        0.5,                              // Duty cycle
        petiroc[slot].pulser_amp
      );


    petiroc_cfg_rst(slot);
    petiroc_slow_control(slot, petiroc[slot].chip);
/*
    printf("***CHIP 0***\n");
    petiroc_print_regs(petiroc[slot].chip[0], 0x3);
    printf("***CHIP 1***\n");
    petiroc_print_regs(petiroc[slot].chip[1], 0x3);
*/


    petiroc_enable(slot, 1);
    petiroc_val_evt(slot, 1, 0, 0);
    petiroc_raz_chn(slot, 1);

    petiroc_trig_setup(slot, 0, 1);  // disable TRIG, enable SYNC
    
    petiroc_set_tdc_enable(
        slot,
        petiroc[slot].tdc_enable_mask
      );

    petiroc_set_readout(
        slot,
        petiroc[slot].window_width,
        petiroc[slot].window_offset,
        petiroc[slot].trig_busy_thr,  // busy threshold
        petiroc[slot].trig_delay      // trig_delay
      );
  }

  // TDC calibration
  printf("petirocDownloadAll: starting TDC calibration -  takes time, be patient ..\n");fflush(stdout);
  for(ii=0; ii<npetiroc; ii++)
  {
    slot = petirocSlot(ii);
    tdc_calibrate_start(
        slot,
        petiroc[slot].tdc_auto_cal_en,
        petiroc[slot].tdc_auto_cal_min_entries
      );
  }

  usleep(10*1000000);

  printf("petirocDownloadAll: stopping TDC calibration\n");fflush(stdout);
  for(ii=0; ii<npetiroc; ii++)
  {
    slot = petirocSlot(ii);
    tdc_calibrate_stop(
        slot,
        petiroc[slot].tdc_auto_cal_en,
        petiroc[slot].tdc_auto_cal_min_entries
      );
  }

  // Clear event buffers
  printf("petirocDownloadAll: clearing event buffers\n");fflush(stdout);
  for(ii=0; ii<npetiroc; ii++)
  {
    slot = petirocSlot(ii);
    petiroc_soft_reset(slot, 1);
    usleep(1000);
    petiroc_soft_reset(slot, 0);
  }

  usleep(100000);

  // Connect event socket
  printf("petirocDownloadAll: connecting event sockets\n");fflush(stdout);
  for(ii=0; ii<npetiroc; ii++)
  {
    slot = petirocSlot(ii);
    petiroc_open_event_socket(slot);
  }

  printf("petirocDownloadAll finished.\n");fflush(stdout);

  return(0);
}

int
petirocUploadAll(char *string, int length)
{
  int slot, i, j, ii, jj, kk, len1, len2, ch, src, mask;
  int asic;
  char *str, sss[1024];
  unsigned int tmp;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];
  unsigned int ival;


  for(ii=0; ii<npetiroc; ii++)
  {
    slot = petirocSlot(ii);

    petiroc_get_clk(slot, &petiroc[slot].clk_ext);

    // Use config local settings instead of readback (write only registers or must be written to read again):
    //petiroc_get_pulser();
    //petiroc_slow_contol(slot, petiroc[slot].chip);

    petiroc_get_tdc_enable(slot, petiroc[slot].tdc_enable_mask);

    petiroc_get_readout(
        slot,
        &petiroc[slot].window_width,
        &petiroc[slot].window_offset,
        &petiroc[slot].trig_busy_thr,  // busy threshold
        &petiroc[slot].trig_delay      // trig_delay
      );
  }

  if(length)
  {
    str = string;
    str[0] = '\0';
    for(ii=0; ii<npetiroc; ii++)
    {
      slot = petirocSlot(ii);

      sprintf(sss,"PETIROC_SLOT %d\n",slot); ADD_TO_STRING;
      sprintf(sss, "PETIROC_W_WIDTH %d\n", petiroc[slot].window_width); ADD_TO_STRING;
      sprintf(sss, "PETIROC_W_OFFSET %d\n", petiroc[slot].window_offset); ADD_TO_STRING;
      sprintf(sss, "PETIROC_TRIG_DELAY %d\n", petiroc[slot].trig_delay); ADD_TO_STRING;
      for(asic=0; asic<2; asic++)
      {
        sprintf(sss, "PETIROC_ASIC %d\n", asic); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_MASK_DISC_CHARGE 0x%08X\n", petiroc[slot].chip[asic].SlowControl.mask_discri_charge); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_INDAC_0_15 %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch0,  petiroc[slot].chip[asic].SlowControl.inputDAC_ch1,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch2,  petiroc[slot].chip[asic].SlowControl.inputDAC_ch3,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch4,  petiroc[slot].chip[asic].SlowControl.inputDAC_ch5,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch6,  petiroc[slot].chip[asic].SlowControl.inputDAC_ch7,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch8,  petiroc[slot].chip[asic].SlowControl.inputDAC_ch9,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch10, petiroc[slot].chip[asic].SlowControl.inputDAC_ch11,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch12, petiroc[slot].chip[asic].SlowControl.inputDAC_ch13,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch14, petiroc[slot].chip[asic].SlowControl.inputDAC_ch15
          ); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_INDAC_16_31 %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch16, petiroc[slot].chip[asic].SlowControl.inputDAC_ch17,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch18, petiroc[slot].chip[asic].SlowControl.inputDAC_ch19,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch20, petiroc[slot].chip[asic].SlowControl.inputDAC_ch21,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch22, petiroc[slot].chip[asic].SlowControl.inputDAC_ch23,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch24, petiroc[slot].chip[asic].SlowControl.inputDAC_ch25,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch26, petiroc[slot].chip[asic].SlowControl.inputDAC_ch27,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch28, petiroc[slot].chip[asic].SlowControl.inputDAC_ch29,
            petiroc[slot].chip[asic].SlowControl.inputDAC_ch30, petiroc[slot].chip[asic].SlowControl.inputDAC_ch31
          ); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_MASK_DISC_TIME %d\n", petiroc[slot].chip[asic].SlowControl.mask_discri_time); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_DAC6B_0_15 %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch0,  petiroc[slot].chip[asic].SlowControl.DAC6b_ch1,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch2,  petiroc[slot].chip[asic].SlowControl.DAC6b_ch3,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch4,  petiroc[slot].chip[asic].SlowControl.DAC6b_ch5,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch6,  petiroc[slot].chip[asic].SlowControl.DAC6b_ch7,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch8,  petiroc[slot].chip[asic].SlowControl.DAC6b_ch9,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch10, petiroc[slot].chip[asic].SlowControl.DAC6b_ch11,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch12, petiroc[slot].chip[asic].SlowControl.DAC6b_ch13,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch14, petiroc[slot].chip[asic].SlowControl.DAC6b_ch15
          ); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_DAC6B_16_31 %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch16, petiroc[slot].chip[asic].SlowControl.DAC6b_ch17,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch18, petiroc[slot].chip[asic].SlowControl.DAC6b_ch19,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch20, petiroc[slot].chip[asic].SlowControl.DAC6b_ch21,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch22, petiroc[slot].chip[asic].SlowControl.DAC6b_ch23,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch24, petiroc[slot].chip[asic].SlowControl.DAC6b_ch25,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch26, petiroc[slot].chip[asic].SlowControl.DAC6b_ch27,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch28, petiroc[slot].chip[asic].SlowControl.DAC6b_ch29,
            petiroc[slot].chip[asic].SlowControl.DAC6b_ch30, petiroc[slot].chip[asic].SlowControl.DAC6b_ch31
          ); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_10B_DAC %d\n", petiroc[slot].chip[asic].SlowControl.EN_10b_DAC); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_VTH_CHARGE %d\n", petiroc[slot].chip[asic].SlowControl.vth_discri_charge); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_VTH_TIME %d\n", petiroc[slot].chip[asic].SlowControl.vth_time); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_ADC %d\n", petiroc[slot].chip[asic].SlowControl.EN_ADC); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_SEL_STARTB_RAMP_ADC_EXT %d\n", petiroc[slot].chip[asic].SlowControl.sel_startb_ramp_ADC_ext); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_USEBCOMP %d\n", petiroc[slot].chip[asic].SlowControl.usebcompensation); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_ENBIASDAC_DELAY %d\n", petiroc[slot].chip[asic].SlowControl.ENbiasDAC_delay); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_ENBIASRAMP_DELAY %d\n", petiroc[slot].chip[asic].SlowControl.ENbiasramp_delay); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_DACDELAY %d\n", petiroc[slot].chip[asic].SlowControl.DACdelay); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_DISC_DELAY %d\n", petiroc[slot].chip[asic].SlowControl.EN_discri_delay); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_TEMP_SENSOR %d\n", petiroc[slot].chip[asic].SlowControl.EN_temp_sensor); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_PA %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_pa); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_DISC %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_discri); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_CMD_POLARITY %d\n", petiroc[slot].chip[asic].SlowControl.cmd_polarity); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_LATCHDISC %d\n", petiroc[slot].chip[asic].SlowControl.LatchDiscri); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_6B_DAC %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_6b_DAC); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_TDC %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_tdc); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_ON_INPUT_DAC %d\n", petiroc[slot].chip[asic].SlowControl.ON_input_DAC); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_CHARGE %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_charge); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_CF %d\n", petiroc[slot].chip[asic].SlowControl.Cf); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_SCA %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_sca); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_DISC_CHARGE %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_discri_charge); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_DISC_ADC_TIME %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_discri_ADC_time); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_BIAS_DISC_ADC_CHARGE %d\n", petiroc[slot].chip[asic].SlowControl.EN_bias_discri_ADC_charge); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_DIS_RAZCHN_INT %d\n", petiroc[slot].chip[asic].SlowControl.DIS_razchn_int); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_DIS_RAZCHN_EXT %d\n", petiroc[slot].chip[asic].SlowControl.DIS_razchn_ext); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_SEL_80M %d\n", petiroc[slot].chip[asic].SlowControl.SEL_80M); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_80M %d\n", petiroc[slot].chip[asic].SlowControl.EN_80M); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_SLOW_LVDS_REC %d\n", petiroc[slot].chip[asic].SlowControl.EN_slow_lvds_rec); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_FAST_LVDS_REC %d\n", petiroc[slot].chip[asic].SlowControl.EN_fast_lvds_rec); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_TRANSMITTER %d\n", petiroc[slot].chip[asic].SlowControl.EN_transmitter); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_ON_1MA %d\n", petiroc[slot].chip[asic].SlowControl.ON_1mA); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_ON_2MA %d\n", petiroc[slot].chip[asic].SlowControl.ON_2mA); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_ON_OTA_MUX %d\n", petiroc[slot].chip[asic].SlowControl.ON_ota_mux); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_ON_OTA_PROBE %d\n", petiroc[slot].chip[asic].SlowControl.ON_ota_probe); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_DIS_TRIG_MUX %d\n", petiroc[slot].chip[asic].SlowControl.DIS_trig_mux); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_NOR32_TIME %d\n", petiroc[slot].chip[asic].SlowControl.EN_NOR32_time); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_NOR32_CHARGE %d\n", petiroc[slot].chip[asic].SlowControl.EN_NOR32_charge); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_DIS_TRIGGERS %d\n", petiroc[slot].chip[asic].SlowControl.DIS_triggers); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_DOUT_OC %d\n", petiroc[slot].chip[asic].SlowControl.EN_dout_oc); ADD_TO_STRING;
        sprintf(sss, "PETIROC_REG_EN_TRANSMIT %d\n", petiroc[slot].chip[asic].SlowControl.EN_transmit); ADD_TO_STRING;
        sprintf(sss, "PETIROC_CTEST_FREQ %d\n", petiroc[slot].pulser_freq); ADD_TO_STRING;
        sprintf(sss, "PETIROC_CTEST_DAC %d %d %d %d\n",
            petiroc[slot].pulser_amp[0], petiroc[slot].pulser_amp[1],
            petiroc[slot].pulser_amp[2], petiroc[slot].pulser_amp[3]
          ); ADD_TO_STRING;
        sprintf(sss, "PETIROC_CTEST_ENABLE %d %d %d %d\n",
            (petiroc[slot].pulser_en>>0) & 0x1, (petiroc[slot].pulser_en>>1) & 0x1,
            (petiroc[slot].pulser_en>>2) & 0x1, (petiroc[slot].pulser_en>>3) & 0x1
          ); ADD_TO_STRING;
        sprintf(sss, "PETIROC_TDC_AUTO_CAL_EN %d\n", petiroc[slot].tdc_auto_cal_en); ADD_TO_STRING;
        sprintf(sss, "PETIROC_TDC_AUTO_CAL_MIN_ENTRIES %d\n", petiroc[slot].tdc_auto_cal_min_entries); ADD_TO_STRING;
        sprintf(sss, "PETIROC_TRIG_BUSY_THR %d\n", petiroc[slot].trig_busy_thr); ADD_TO_STRING;
        sprintf(sss, "PETIROC_TDC_ENABLE 0x%08X 0x%08X\n", petiroc[slot].tdc_enable_mask[0], petiroc[slot].tdc_enable_mask[1]); ADD_TO_STRING;
        sprintf(sss, "PETIROC_CLK_EXT %d\n", petiroc[slot].clk_ext); ADD_TO_STRING;
      }
    }
    CLOSE_STRING;
  }
  return(0);
}

static char str[900001];

int
petirocUploadAllPrint()
{
  petirocUploadAll(str, 900000);
  printf("%s",str);

  return 0;
}

#else /*Linux_armv7l*/

void
petirocConfig_default()
{
}

#endif
