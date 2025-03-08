
/* marocConfig.c */

#ifndef Linux_armv7l

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "marocLib.h"
#include "marocConfig.h"
#include "xxxConfig.h"

static int active;
static int nmaroc;
static MAROC_CONF maroc[MAROC_MAX_NUM];

#define SCAN_MSK \
  sscanf (str_tmp, "%*s %d %d %d %d %d %d %d %d   \
                        %d %d %d %d %d %d %d %d", \
          &msk[ 0], &msk[ 1], &msk[ 2], &msk[ 3], \
          &msk[ 4], &msk[ 5], &msk[ 6], &msk[ 7], \
          &msk[ 8], &msk[ 9], &msk[10], &msk[11], \
          &msk[12], &msk[13], &msk[14], &msk[15])

static char *expid = NULL;

void
marocSetExpid(char *string)
{
  expid = strdup(string);
}

int
marocConfig(char *fname)
{
  int res;

  nmaroc = marocGetNmaroc();
  printf("%s: nmaroc=%d\n", __func__, nmaroc);fflush(stdout);

  //marocInitGlobals();

  if( (res = marocReadConfigFile(fname)) < 0 )
  {
    printf("marocConfig: ERROR: marocReadConfigFile() returns %d\n",res);fflush(stdout);
    return(res);
  }

  /* download to all boards) */
  marocDownloadAll();

  return(0);
}

void
marocInitGlobals()
{
  printf("%s Reached\n", __func__);
  memset(maroc, 0, sizeof(maroc));
}

int
marocReadConfigFile_CheckArgs(int sscan_ret, int req, char *keyword)
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
marocReadConfigFile(char *filename_in)
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
        sprintf(fname, "%s/maroc/%s", clonparms, filename);
      }

      if((fd=fopen(fname,"r")) == NULL)
      {
        printf("\n%s: Can't open config file >%s<\n",__func__,fname);fflush(stdout);
        return(-1);
      }
    }
    else if(do_parsing<2) /* filename does not specified */
    {
      sprintf(fname, "%s/maroc/%s.cnf", clonparms, host);
      if((fd=fopen(fname,"r")) == NULL)
      {
        sprintf(fname, "%s/maroc/%s.cnf", clonparms, expid);
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
        if(strcmp(keyword,"MAROC_CRATE") == 0)
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
        
        
        if(!strcmp(keyword,"MAROC_SLOT") || !strcmp(keyword,"MAROC_SLOTS"))
        {
          sscanf (str_tmp, "%*s %s", str2);
          /*printf("str2=%s\n",str2);*/
          if(isdigit(str2[0]))
          {
            slot1 = atoi(str2);
            slot2 = slot1 + 1;
            if(slot1<0 && slot1>MAROC_MAX_NUM)
            {
              printf("\nReadConfigFile: Wrong slot number %d\n\n",slot1);
              return(-4);
            }
          }
          else if(!strcmp(str2,"all"))
          {
            slot1 = 0;
            slot2 = MAROC_MAX_NUM;
          }
          else
          {
            printf("\nReadConfigFile: Wrong slot >%s<, must be 'all' or actual slot number\n\n",str2);
            return(-4);
          }
          /*printf("slot1=%d slot2=%d\n",slot1,slot2);*/
        }
        else if((strcmp(keyword,"MAROC_W_WIDTH")==0))
        {
          sscanf (str_tmp, "%*s %d %d", &i1);
          for(slot=slot1; slot<slot2; slot++) maroc[slot].window_width= i1;
        }
        else if((strcmp(keyword,"MAROC_W_OFFSET")==0))
        {
          sscanf (str_tmp, "%*s %d %d", &i1);
          for(slot=slot1; slot<slot2; slot++) maroc[slot].window_offset= i1;
        }
        else if(!strcmp(keyword,"MAROC_ASIC"))
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
            asic2 = 3;
          }
          else
          {
            printf("\nReadConfigFile: Wrong asic >%s<, must be 'all' or actual fiber number\n\n",str2);
            return(-4);
          }
        }
        else if(!strcmp(keyword,"MAROC_ASIC_NUM"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            maroc[slot].asic_num = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_CMD_FSU"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.cmd_fsu = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_CMD_SS"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.cmd_ss = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_CMD_FSB"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.cmd_fsb = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SWB_BUF_250F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.swb_buf_250f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SWB_BUF_500F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.swb_buf_500f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SWB_BUF_1P"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.swb_buf_1p = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SWB_BUF_2P"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.swb_buf_2p = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_ONOFF_SS"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.ONOFF_ss = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_SS_300F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_ss_300f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_SS_600F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_ss_600f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_SS1200F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_ss_1200f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_EN_ADC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.EN_ADC = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_H1H2_CHOICE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.H1H2_choice = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSU_20F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsu_20f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSU_40F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsu_40f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSU_25K"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsu_25k = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSU_50K"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsu_50k = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSU_100K"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsu_100k = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB1_50K"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb1_50k = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB1_100K"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb1_100k = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB1_100F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb1_100f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB1_50F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb1_50f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_CMD_FSB_FSU"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.cmd_fsb_fsu = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_VALID_DC_FS"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.valid_dc_fs = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB2_50K"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb2_50k = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB2_100K"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb2_100k = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB2_100F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb2_100f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SW_FSB2_50F"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.sw_fsb2_50f = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_VALID_DC_FSB2"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.valid_dc_fsb2 = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_ENB_TRISTATE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.ENb_tristate = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_POLAR_DISCRI"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.polar_discri = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_INV_DISCRIADC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.bits.inv_discriADC = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_D1_D2"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.d1_d2 = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_CMD_CK_MUX"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.cmd_CK_mux = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_ONOFF_OTABG"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.ONOFF_otabg = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_ONOFF_DAC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.ONOFF_dac = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_SMALL_DAC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.small_dac = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_ENB_OUTADC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.enb_outADC = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_INV_STARTCMPTGRAY"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.inv_startCmptGray = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_RAMP_8BIT"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.ramp_8bit = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_RAMP_10BIT"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.bits.ramp_10bit = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_GLOBAL0"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X", &ui[0]);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global0.val = ui[0];
        }
        else if(!strcmp(keyword,"MAROC_REG_GLOBAL1"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X", &ui[0]);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].Global1.val = ui[0];
        }
        else if(!strcmp(keyword,"MAROC_REG_DAC0"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].DAC0 = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_DAC1"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
            maroc[slot].chip[asic].DAC1 = i1;
        }
        else if(!strcmp(keyword,"MAROC_REG_GAIN_0_15"))
        {
          argc = SCAN_MSK;
          if(error = marocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          for(ch=0; ch<16; ch++)
            maroc[slot].chip[asic].Gain[0+ch] = msk[ch];
        }
        else if(!strcmp(keyword,"MAROC_REG_GAIN_16_31"))
        {
          argc = SCAN_MSK;
          if(error = marocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          for(ch=0; ch<16; ch++)
            maroc[slot].chip[asic].Gain[16+ch] = msk[ch];
        }
        else if(!strcmp(keyword,"MAROC_REG_GAIN_32_47"))
        {
          argc = SCAN_MSK;
          if(error = marocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          for(ch=0; ch<16; ch++)
            maroc[slot].chip[asic].Gain[32+ch] = msk[ch];
        }
        else if(!strcmp(keyword,"MAROC_REG_GAIN_48_63"))
        {
          argc = SCAN_MSK;
          if(error = marocReadConfigFile_CheckArgs(argc, 16, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          for(ch=0; ch<16; ch++)
            maroc[slot].chip[asic].Gain[48+ch] = msk[ch];
        }
        else if(!strcmp(keyword,"MAROC_REG_SUM"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X 0x%X", &ui[0], &ui[1]);
          if(error = marocReadConfigFile_CheckArgs(argc, 2, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            maroc[slot].chip[asic].Sum[0] = ui[0];
            maroc[slot].chip[asic].Sum[1] = ui[1];
          }
        }
        else if(!strcmp(keyword,"MAROC_REG_CTEST"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X 0x%X", &ui[0], &ui[1]);
          if(error = marocReadConfigFile_CheckArgs(argc, 2, keyword)) return error;

          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            maroc[slot].chip[asic].CTest[0] = ui[0];
            maroc[slot].chip[asic].CTest[1] = ui[1];
          }
        }
        else if(!strcmp(keyword,"MAROC_REG_MASKOR"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X 0x%X", &ui[0], &ui[1]);
          if(error = marocReadConfigFile_CheckArgs(argc, 2, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            maroc[slot].chip[asic].MaskOr[0] = ui[0];
            maroc[slot].chip[asic].MaskOr[1] = ui[1];
          }
        }        
        else if(!strcmp(keyword,"MAROC_TRIGGER_OR0"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X 0x%X", &ui[0], &ui[1]);
          if(error = marocReadConfigFile_CheckArgs(argc, 2, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            maroc[slot].chip[asic].TriggerOr0[0] = ui[0];
            maroc[slot].chip[asic].TriggerOr0[1] = ui[1];
          }
        }
        else if(!strcmp(keyword,"MAROC_TRIGGER_OR1"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X 0x%X", &ui[0], &ui[1]);
          if(error = marocReadConfigFile_CheckArgs(argc, 2, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(asic=asic1; asic<asic2; asic++)
          {
            maroc[slot].chip[asic].TriggerOr1[0] = ui[0];
            maroc[slot].chip[asic].TriggerOr1[1] = ui[1];
          }
        }
        else if(!strcmp(keyword,"MAROC_CTEST_DAC"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            maroc[slot].ctest_dac = i1;
        }
        else if(!strcmp(keyword,"MAROC_CTEST_ENABLE"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            maroc[slot].ctest_enable = i1;
        }
        else if(!strcmp(keyword,"MAROC_TDC_ENABLE"))
        {
          argc = sscanf (str_tmp, "%*s 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X",
                         &ui[0],&ui[1],&ui[2],&ui[3],&ui[4],&ui[5]);
          if(error = marocReadConfigFile_CheckArgs(argc, 6, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
          for(ii=0; ii<6; ii++)
            maroc[slot].tdc_enable_mask[ii] = ui[ii];
        }
        else if(!strcmp(keyword,"MAROC_W_WIDTH"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            maroc[slot].window_width = i1;
        }
        else if(!strcmp(keyword,"MAROC_W_OFFSET"))
        {
          argc = sscanf (str_tmp, "%*s %d", &i1);
          if(error = marocReadConfigFile_CheckArgs(argc, 1, keyword)) return error;
          
          for(slot=slot1; slot<slot2; slot++)
            maroc[slot].window_offset = i1;
        }
        else
        {
          ; /* unknown key - do nothing */
          /*
          printf("marocReadConfigFile: Unknown Field or Missed Field in\n");
          printf("   %s \n", fname);
          printf("   str_tmp=%s", str_tmp);
          printf("   keyword=%s \n\n", keyword);
          return(-10);
          */
        }
      }
    } /* end of while */
    fclose(fd);
  }

  return(0);
}

int
marocDownloadAll()
{
  int slot, ii, jj, kk, ch;
  int asic, mask;

  printf("\n\n%s reached, nmaroc=%d\n", __func__, nmaroc); 
  for(ii=0; ii<nmaroc; ii++)
  {
    slot = marocSlot(ii);
          
    maroc_SetASICNum(slot, maroc[slot].asic_num);
    
    for(asic=0; asic<3; asic++)
    {
      maroc_SetMarocReg(slot, asic, MAROC_REG_GLOBAL0,            0, maroc[slot].chip[asic].Global0.val);
      maroc_SetMarocReg(slot, asic, MAROC_REG_GLOBAL1,            0, maroc[slot].chip[asic].Global1.val);

      maroc_SetMarocReg(slot, asic, MAROC_REG_DAC0,               0, maroc[slot].chip[asic].DAC0);
      maroc_SetMarocReg(slot, asic, MAROC_REG_DAC1,               0, maroc[slot].chip[asic].DAC1);

      maroc_setmask_fpga_or0(slot,
          maroc[slot].chip[0].TriggerOr0[0], maroc[slot].chip[0].TriggerOr0[1], 
          maroc[slot].chip[1].TriggerOr0[0], maroc[slot].chip[1].TriggerOr0[1], 
          maroc[slot].chip[2].TriggerOr0[0], maroc[slot].chip[2].TriggerOr0[1]
        ); 
   
      maroc_setmask_fpga_or1(slot,
          maroc[slot].chip[0].TriggerOr1[0], maroc[slot].chip[0].TriggerOr1[1], 
          maroc[slot].chip[1].TriggerOr1[0], maroc[slot].chip[1].TriggerOr1[1], 
          maroc[slot].chip[2].TriggerOr1[0], maroc[slot].chip[2].TriggerOr1[1]
        ); 
 
      for(ch=0; ch<64; ch++)
      {
        int sum, ctest, maskor, gain;
      
        gain = maroc[slot].chip[asic].Gain[ch];
      
        if(ch < 32)
        {
          sum = (maroc[slot].chip[asic].Sum[0] & (1<<ch)) ? 1 : 0;
          ctest = (maroc[slot].chip[asic].CTest[0] & (1<<ch)) ? 1 : 0;
          maskor = (maroc[slot].chip[asic].MaskOr[0] & (1<<ch)) ? 1 : 0;
        }
        else
        {
          sum = (maroc[slot].chip[asic].Sum[1] & (1<<(ch-32))) ? 1 : 0;
          ctest = (maroc[slot].chip[asic].CTest[1] & (1<<(ch-32))) ? 1 : 0;
          maskor = (maroc[slot].chip[asic].MaskOr[1] & (1<<(ch-32))) ? 1 : 0;
        }
      
        maroc_SetMarocReg(slot, asic, MAROC_REG_GAIN,            ch, gain);
        maroc_SetMarocReg(slot, asic, MAROC_REG_SUM,             ch, sum);
        maroc_SetMarocReg(slot, asic, MAROC_REG_CTEST,           ch, ctest);
        maroc_SetMarocReg(slot, asic, MAROC_REG_MASKOR,          ch, maskor);
      }
    }

    maroc_UpdateMarocRegs(slot);
  
    maroc_SetCTestAmplitude(slot, maroc[slot].ctest_dac);
  
    maroc_SetTDCEnableChannelMask(slot,
                                  maroc[slot].tdc_enable_mask[0],
                                  maroc[slot].tdc_enable_mask[1],
                                  maroc[slot].tdc_enable_mask[2],
                                  maroc[slot].tdc_enable_mask[3],
                                  maroc[slot].tdc_enable_mask[4],
                                  maroc[slot].tdc_enable_mask[5]
                                );
  
    maroc_SetLookback(slot, maroc[slot].window_offset);
    maroc_SetWindow(slot, maroc[slot].window_width);
  }
  return(0);
}

int
marocUploadAll(char *string, int length)
{
  int slot, i, j, jj, kk, len1, len2, ch, src, mask;
  int asic;
  char *str, sss[1024];
  unsigned int tmp;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];
  unsigned int ival;

  for(kk=0; kk<nmaroc; kk++)
  {
    slot = marocSlot(kk);    

    maroc_GetASICNum(slot, &maroc[slot].asic_num);
 
    maroc_UpdateMarocRegs(slot);

    for(asic=0; asic<3; asic++)
    {
      maroc_GetMarocReg(slot, asic, MAROC_REG_GLOBAL0,            0, &maroc[slot].chip[asic].Global0.val);
      maroc_GetMarocReg(slot, asic, MAROC_REG_GLOBAL1,            0, &maroc[slot].chip[asic].Global1.val);
      maroc_GetMarocReg(slot, asic, MAROC_REG_DAC0,               0, &maroc[slot].chip[asic].DAC0);
      maroc_GetMarocReg(slot, asic, MAROC_REG_DAC1,               0, &maroc[slot].chip[asic].DAC1);

      
      maroc[slot].chip[asic].Sum[0] = 0;
      maroc[slot].chip[asic].Sum[1] = 0;
      maroc[slot].chip[asic].CTest[0] = 0;
      maroc[slot].chip[asic].CTest[1] = 0;
      maroc[slot].chip[asic].MaskOr[0] = 0;
      maroc[slot].chip[asic].MaskOr[1] = 0;
      maroc_getmask_fpga_or0(slot, &maroc[slot].chip[asic].TriggerOr0[0], &maroc[slot].chip[asic].TriggerOr0[1]);
      maroc_getmask_fpga_or1(slot, &maroc[slot].chip[asic].TriggerOr1[0], &maroc[slot].chip[asic].TriggerOr1[1]);

      for(ch=0; ch<64; ch++)
      {
        int gain, sum, ctest, maskor;
        
        maroc_GetMarocReg(slot, asic, MAROC_REG_GAIN,            ch, &gain);
        maroc_GetMarocReg(slot, asic, MAROC_REG_SUM,             ch, &sum);
        maroc_GetMarocReg(slot, asic, MAROC_REG_CTEST,           ch, &ctest);
        maroc_GetMarocReg(slot, asic, MAROC_REG_MASKOR,          ch, &maskor);
        
        maroc[slot].chip[asic].Gain[ch] = gain;

        if(ch < 32)
        {
          if(sum)    maroc[slot].chip[asic].Sum[0] |= 1<<ch;
          if(ctest)  maroc[slot].chip[asic].CTest[0] |= 1<<ch;
          if(maskor) maroc[slot].chip[asic].MaskOr[0] |= 1<<ch;
        }
        else
        {
          if(sum)    maroc[slot].chip[asic].Sum[1] |= 1<<(ch-32);
          if(ctest)  maroc[slot].chip[asic].CTest[1] |= 1<<(ch-32);
          if(maskor) maroc[slot].chip[asic].MaskOr[1] |= 1<<(ch-32);
        }
      }
    } 
    
    maroc_GetCTestAmplitude(slot, &maroc[slot].ctest_dac);
    
    maroc_GetCTestSource(slot, &src);
    if(src == SD_SRC_SEL_0)
      maroc[slot].ctest_enable = 0;
    else
      maroc[slot].ctest_enable = 1;
    
    maroc_GetTDCEnableChannelMask(slot,
                                    &maroc[slot].tdc_enable_mask[0],
                                    &maroc[slot].tdc_enable_mask[1],
                                    &maroc[slot].tdc_enable_mask[2],
                                    &maroc[slot].tdc_enable_mask[3],
                                    &maroc[slot].tdc_enable_mask[4],
                                    &maroc[slot].tdc_enable_mask[5]
                                  );
    
    maroc_GetLookback(slot, &maroc[slot].window_offset);
    maroc_GetWindow(slot, &maroc[slot].window_width);
  }
  
  if(length)
  {
    str = string;
    str[0] = '\0';
    for(kk=0; kk<nmaroc; kk++)
    {
      slot = marocSlot(kk);

      sprintf(sss,"MAROC_SLOT %d\n",slot); ADD_TO_STRING;
      sprintf(sss, "MAROC_ASIC_NUM %d\n", maroc[slot].asic_num); ADD_TO_STRING;
     
      for(asic=0; asic<3; asic++)
      {
        sprintf(sss, "MAROC_ASIC %d\n", asic); ADD_TO_STRING;
        sprintf(sss, "MAROC_REG_GLOBAL0 0x%08X\n",       maroc[slot].chip[asic].Global0.val); ADD_TO_STRING;
        sprintf(sss, "MAROC_REG_GLOBAL1 0x%08X\n",       maroc[slot].chip[asic].Global1.val); ADD_TO_STRING;
        sprintf(sss, "MAROC_REG_DAC0 %d\n",              maroc[slot].chip[asic].DAC0); ADD_TO_STRING;
        sprintf(sss, "MAROC_REG_DAC1 %d\n",              maroc[slot].chip[asic].DAC1); ADD_TO_STRING;
        
     
        ADD_TO_STRING;
        sprintf(sss, "MAROC_REG_GAIN_0_15 "); ADD_TO_STRING;
        for(ch=0; ch<16; ch++) { sprintf(sss, "%d%c", maroc[slot].chip[asic].Gain[0+ch], (ch==15)?'\n':' '); ADD_TO_STRING;}
          
        sprintf(sss, "MAROC_REG_GAIN_16_31 "); ADD_TO_STRING;
        for(ch=0; ch<16; ch++) { sprintf(sss, "%d%c", maroc[slot].chip[asic].Gain[16+ch], (ch==15)?'\n':' '); ADD_TO_STRING;}

        sprintf(sss, "MAROC_REG_GAIN_32_47 "); ADD_TO_STRING;
        for(ch=0; ch<16; ch++) { sprintf(sss, "%d%c", maroc[slot].chip[asic].Gain[32+ch], (ch==15)?'\n':' '); ADD_TO_STRING;}

        sprintf(sss, "MAROC_REG_GAIN_48_63 "); ADD_TO_STRING;
        for(ch=0; ch<16; ch++) { sprintf(sss, "%d%c", maroc[slot].chip[asic].Gain[48+ch], (ch==15)?'\n':' '); ADD_TO_STRING;}
        
        sprintf(sss, "MAROC_REG_SUM 0x%08X 0x%08X\n",
            maroc[slot].chip[asic].Sum[0],
            maroc[slot].chip[asic].Sum[1]
          ); ADD_TO_STRING;

        sprintf(sss, "MAROC_REG_CTEST 0x%08X 0x%08X\n",
            maroc[slot].chip[asic].CTest[0],
            maroc[slot].chip[asic].CTest[1]
          ); ADD_TO_STRING;

        sprintf(sss, "MAROC_REG_MASKOR 0x%08X 0x%08X\n",
            maroc[slot].chip[asic].MaskOr[0],
            maroc[slot].chip[asic].MaskOr[1]
          ); ADD_TO_STRING;
        
        sprintf(sss, "MAROC_REG_TRIGGEROR0 0x%08X 0x%08X\n",
            maroc[slot].chip[asic].TriggerOr0[0],
            maroc[slot].chip[asic].TriggerOr0[1]
          ); ADD_TO_STRING;
        
        sprintf(sss, "MAROC_REG_TRIGGEROR1 0x%08X 0x%08X\n",
            maroc[slot].chip[asic].TriggerOr1[0],
            maroc[slot].chip[asic].TriggerOr1[1]
          ); ADD_TO_STRING;
      }

      sprintf(sss, "MAROC_CTEST_DAC %d\n", maroc[slot].ctest_dac); ADD_TO_STRING;

      sprintf(sss, "MAROC_CTEST_ENABLE %d\n", maroc[slot].ctest_enable); ADD_TO_STRING;

      sprintf(sss, "MAROC_TDC_ENABLE 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\n",
          maroc[slot].tdc_enable_mask[0],
          maroc[slot].tdc_enable_mask[1],
          maroc[slot].tdc_enable_mask[2],
          maroc[slot].tdc_enable_mask[3],
          maroc[slot].tdc_enable_mask[4],
          maroc[slot].tdc_enable_mask[5]
      ); ADD_TO_STRING;

      sprintf(sss, "MAROC_W_WIDTH %d\n", maroc[slot].window_width); ADD_TO_STRING;
      sprintf(sss, "MAROC_W_OFFSET %d\n", maroc[slot].window_offset); ADD_TO_STRING;
    }
    CLOSE_STRING;
  }
  return(0);
}

static char str[500001];

int
marocUploadAllPrint()
{
  marocUploadAll(str, 500000);
  printf("%s",str);

  return 0;
}

#else /*Linux_armv7l*/

void
marocConfig_default()
{
}

#endif
