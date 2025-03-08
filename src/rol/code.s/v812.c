#if defined(VXWORKS) || defined(Linux_vme)

/*  v812.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef VXWORKS
#include <vxWorks.h>
#include <logLib.h>
#include <taskLib.h>
#else 
#include <sys/prctl.h>
#include <unistd.h>
#include "jvme.h"
#endif

#include "v812.h"

/* Include definitions */


/* Define external Functions */
#ifdef VXWORKS
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
#endif


volatile struct v812_struct *v812p[21];
static unsigned int n812 = 0;

int
v812Init()
{
  int ii, res, id=0;
  UINT16 code=0, type=0, version=0;
  unsigned long addr;

  printf("\n");
  for(ii=0; ii<V812_NBOARDS; ii++)
  {
    addr = (V812_ADDRSTART + ii*V812_ADDRSTEP) & 0xFFFFFF;

    /* A24 Local address - find translation */
#ifdef VXWORKS
    res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&v812p[id]);
    if (res != 0)
    {
      printf("v812Init: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",(int)addr);
      return(ERROR);
    }
#else
    res = vmeBusToLocalAdrs(0x39,(char *)addr,(char **)&v812p[id]);
    if (res != 0)
    {
      printf("v812Init: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",(int)addr);
      return(ERROR);
    }
#endif

    code = vmeRead16(&v812p[id]->code);
    type = vmeRead16(&v812p[id]->type);
    version = vmeRead16(&v812p[id]->version);

    /*printf("  code=0x%04x, type=0x%04x, version=0x%04x, address = 0x%x\n",code,type,version,(int)v812p[id]);*/
    /*
    printf("Manufacturer = 0x%01x\n",(type>>10)&0x3F);
    printf("Module type = 0x%02x\n",type&0x3FF);
    printf("Module code = 0x%04x\n",code);
    */
    if(type != V812_MODULE_TYPE || code != V812_MODULE_CODE)
    {
      printf("v812Init: did not find v812 at address 0x%x\n",addr);
    }
    else
    {
      printf("v812Init: found v812 at address 0x%x, assign id=%d (slot=%d)\n",addr,id,ii+2);
      id ++;
    }
  }

  n812 = id;
  printf("Found %d v812 board\n\n",n812);

  return(n812);
}



int
v812SetThreshold(int id, int chan, unsigned short threshold)
{
  if(v812p == NULL)
  {
    printf("v812SetThreshold: ERROR, v812 not initialized\n");
    return(ERROR);
  }

  vmeWrite16(&v812p[id]->threshold[chan], threshold&0xFFFF);

  return(0);
}

int
v812SetThresholds(int id, unsigned short thresholds[16])
{
  int chan;

  if(v812p == NULL)
  {
    printf("v812SetThresholds: ERROR, v812 not initialized\n");
    return(ERROR);
  }

  for(chan=0; chan<16; chan++)
  {
    vmeWrite16(&v812p[id]->threshold[chan], thresholds[chan]&0xFFFF);
  }

  return(0);
}

/* 'width' in 'ns' */
int
v812SetWidth(int id, unsigned short width)
{
  unsigned short width1, width2;

  /* convert 'ns' to register value */
  if(width<=12)        width1 = 1;
  else if(width==13)  width1 = 50;
  else if(width==14)  width1 = 60;
  else if(width==15)  width1 = 80;
  else if(width==16)  width1 = 85;
  else if(width==17)  width1 = 90;
  else if(width==18)  width1 = 100;
  else if(width==19)  width1 = 110;
  else if(width==20)  width1 = 120;
  else if(width==21)  width1 = 125;
  else if(width==22)  width1 = 130;
  else if(width==23)  width1 = 133;
  else if(width==24)  width1 = 136;
  else if(width==25)  width1 = 140;
  else if(width==26)  width1 = 145;
  else if(width==27)  width1 = 150;
  else if(width==28)  width1 = 153;
  else if(width==29)  width1 = 156;
  else if(width==30)  width1 = 160;
  else if(width==31)  width1 = 163;
  else if(width==32)  width1 = 166;
  else if(width==33)  width1 = 170;
  else if(width==34)  width1 = 173;
  else if(width==35)  width1 = 176;
  else if(width==36)  width1 = 180;
  else if(width==37)  width1 = 182;
  else if(width==38)  width1 = 184;
  else if(width==39)  width1 = 186;
  else if(width==40)  width1 = 188;
  else if(width==41)  width1 = 192;
  else if(width==42)  width1 = 193;
  else if(width==43)  width1 = 195;
  else if(width==44)  width1 = 196;
  else if(width==45)  width1 = 198;
  else if(width==46)  width1 = 199;
  else if(width==47)  width1 = 200;
  else if(width==48)  width1 = 201;
  else if(width==49)  width1 = 202;
  else if(width==50)  width1 = 204;
  else if(width==51)  width1 = 205;
  else if(width==52)  width1 = 206;
  else if(width==53)  width1 = 208;
  else if(width==54)  width1 = 209;
  else if(width==55)  width1 = 210;
  else if(width==56)  width1 = 211;
  else if(width==57)  width1 = 212;
  else if(width==58)  width1 = 213;
  else if(width==59)  width1 = 213;
  else if(width==60)  width1 = 214;
  else if(width==61)  width1 = 215;
  else if(width==62)  width1 = 216;
  else if(width==63)  width1 = 216;
  else if(width==64)  width1 = 217;
  else if(width==65)  width1 = 218;
  else if(width==66)  width1 = 219;
  else if(width==67)  width1 = 220;
  else if(width==68)  width1 = 220;
  else if(width==69)  width1 = 221;
  else if(width==70)  width1 = 221;
  else if(width==71)  width1 = 222;
  else if(width==72)  width1 = 223;
  else if(width==73)  width1 = 223;
  else if(width==74)  width1 = 224;
  else if(width==75)  width1 = 224;
  else if(width==76)  width1 = 225;
  else if(width==77)  width1 = 226;
  else if(width==78)  width1 = 226;
  else if(width==79)  width1 = 227;
  else if(width==80)  width1 = 227;
  else if(width==81)  width1 = 228;
  else if(width==82)  width1 = 229;
  else if(width==83)  width1 = 229;
  else if(width==84)  width1 = 230;
  else if(width==85)  width1 = 230;
  else if(width==86)  width1 = 230;
  else if(width==87)  width1 = 231;
  else if(width==88)  width1 = 231;
  else if(width==89)  width1 = 231;
  else if(width==90)  width1 = 232;
  else if(width==91)  width1 = 232;
  else if(width==92)  width1 = 233;
  else if(width==93)  width1 = 233;
  else if(width==94)  width1 = 233;
  else if(width==95)  width1 = 234;
  else if(width==96)  width1 = 234;
  else if(width==97)  width1 = 235;
  else if(width==98)  width1 = 235;
  else if(width==99)  width1 = 235;
  else if(width==100) width1 = 236;
  else if(width==101) width1 = 236;
  else if(width==102) width1 = 237;
  else if(width==103) width1 = 237;
  else if(width==104) width1 = 237;
  else if(width==105) width1 = 238;
  else if(width==106) width1 = 238;
  else if(width==107) width1 = 239;
  else if(width==108) width1 = 239;
  else if(width==109) width1 = 239;
  else if(width==110) width1 = 240;
  else if(width > 110 && width < 160)
  {
    width1 = 240 + (width-110)/5;
  }
  else if(width==160) width1 = 250;
  else if(width > 160 && width < 203)
  {
    width1 = 250 + (width-160)/9;
  }
  else if(width>=203) width1 = 255;



  if(v812p == NULL)
  {
    printf("v812SetWidth: ERROR, v812 not initialized\n");
    return(ERROR);
  }

  width2 = width1;
  vmeWrite16(&v812p[id]->width1, width1&0xFFFF);
  vmeWrite16(&v812p[id]->width2, width2&0xFFFF);

  return(0);
}

int
v812SetDeadtime(int id, unsigned short deadtime)
{
  unsigned short deadtime2;

  if(v812p == NULL)
  {
    printf("v812SetDeadtime: ERROR, v812 not initialized\n");
    return(ERROR);
  }

  deadtime2 = deadtime;
  vmeWrite16(&v812p[id]->deadtime1, deadtime&0xFFFF);
  vmeWrite16(&v812p[id]->deadtime2, deadtime2&0xFFFF);

  return(0);
}

int
v812SetMajority(int id, unsigned short majority)
{
  if(v812p == NULL)
  {
    printf("v812SetMajority: ERROR, v812 not initialized\n");
    return(ERROR);
  }

  vmeWrite16(&v812p[id]->majority, majority&0xFFFF);

  return(0);
}

int
v812DisableChannels(int id, unsigned short mask)
{
  if(v812p == NULL)
  {
    printf("v812DisableChannels: ERROR, v812 not initialized\n");
    return(ERROR);
  }

  vmeWrite16(&v812p[id]->inhibit, mask&0xFFFF);

  return(0);
}

int
v812TestPulse(int id)
{
  if(v812p == NULL)
  {
    printf("v812TestPulse: ERROR, v812 not initialized\n");
    return(ERROR);
  }

  vmeWrite16(&v812p[id]->test, 1);

  return(0);
}




#else

int
v812_dummy()
{
}

#endif
