
/* echist.c */
 
#include <stdio.h>
#include <math.h>
 
#include "bosio.h"

#define NBCS 700000
#include "bcs.h"

#define RADDEG 57.2957795130823209
#define NWPAWC 10000000 /* Length of the PAWC common block. */
#define LREC 1024       /* Record length in machine words. */

struct {
  float hmemor[NWPAWC];
} pawc_;


#define MIN(a,b)          ( (a) < (b) ? (a) : (b) )

typedef struct LeCroy1877Head
{
  unsigned slot    :  5;
  unsigned empty0  :  3;
  unsigned empty1  :  8;
  unsigned empty   :  5;
  unsigned count   : 11;
} MTDCHead;

typedef struct LeCroy1877
{
  unsigned slot    :  5;
  unsigned type    :  3;
  unsigned channel :  7;
  unsigned edge    :  1;
  unsigned data    : 16;
} MTDC;



main(int argc, char **argv)
{
  int nr,sec,layer,strip,wire,nl,ncol,nrow,i,j,l,l1,l2,ichan,nn,iev;
  int ind,ind1,ind2,status,status1,handle,handle1,k,m,rocid;
  int scal,nw,scaler,scaler_old;
  unsigned int hel,str,strob,helicity,strob_old,helicity_old, tgbi1;
  int tmp1 = 1, tmp2 = 2, iret, bit1, bit2, leading[2000], trailing[2000], nleading, ntrailing;
  float *bcsfl, rndm_();
  char strr[1000], bankname[5], histname[128];
  static int syn[32], id;
  MTDCHead *mtdchead;
  MTDC *mtdc;

  char *HBOOKfile = "test.his";
  int nwpawc,lun,lrec,istat,icycle,idn,nbins,nbins1,igood,offset;
  float x1,x2,y1,y2,ww,tmpx,tmpy,ttt,ref;
  unsigned short *tlv1, *tlv2, *buf16;

  printf(" echist reached !\n");

  if(argc != 2)
  {
    printf("Usage: dchist <fpack_filename>\n");
    exit(1);
  }



  bcsfl = (float*)bcs_.iw;
  bosInit(bcs_.iw,NBCS);


  nwpawc = NWPAWC;
  hlimit_(&nwpawc);
  lun = 11;
  lrec = LREC;
  hropen_(&lun,"NTUPEL",HBOOKfile,"N",&lrec,&istat,
     strlen("NTUPEL"),strlen(HBOOKfile),1);
  if(istat)
  {
    printf("\aError: cannot open RZ file %s for writing.\n", HBOOKfile);
    exit(0);
  }
  else
  {
    printf("RZ file >%s< opened for writing, istat = %d\n\n", HBOOKfile, istat);
  }



  nbins=1000;
  x1 = 0.;
  x2 = 1000.;
  ww = 0.;

  for(k=1; k<=6; k++)
  {
    for(j=1; j<=6; j++)
    {
      for(i=1; i<=36; i++)
      {
        sprintf(histname,"TDC: strip=%2d layer=%2d",i,j);
        idn = k*1000+j*100+i;
        hbook1_(&idn,histname,&nbins,&x1,&x2,&ww,strlen(histname));
        sprintf(histname,"ADC: strip=%2d layer=%2d",i,j);
        idn = 10000 + k*1000+j*100+i;
        hbook1_(&idn,histname,&nbins,&x1,&x2,&ww,strlen(histname));
      }
    }
  }









  /*
  nbins=100;
  x1 = 0.;
  x2 = 10000.;
  nbins1=100;
  y1 = 0.;
  y2 = 10000.;
  ww = 0.;
  idn=3;
  sprintf(histname,"Reference: channel1 vs channel2",idn);
  hbook2_(&idn,histname,&nbins,&x1,&x2,&nbins1,&y1,&y2,&ww,strlen(histname));
  idn=4;
  sprintf(histname,"Reference: channel1 vs channel2 (sync)",idn);
  hbook2_(&idn,histname,&nbins,&x1,&x2,&nbins1,&y1,&y2,&ww,strlen(histname));
  */




  sprintf(strr,"OPEN INPUT UNIT=1 FILE='%s' ",argv[1]);
  printf("fparm string: >%s<\n",strr);
  status = fparm_(strr,strlen(strr));
  for(iev=0; iev<1000000; iev++)
  {
    frbos_(bcs_.iw,&tmp1,"E",&iret,1);

    if(!(iev%1000)) printf("===== Event no. %d\n",iev);



    for(k=1; k<=6; k++)
    {
    if((ind1=bosNlink(bcs_.iw,"EC  ",k)) > 0)
    {
      unsigned int *fb, *fbend;
      unsigned short *fb16;
      int crate,slot,channel,edge,data,irow,ncol1,nrow1,tdc,adc;

      ncol1 = bcs_.iw[ind1-6];
      nrow1 = bcs_.iw[ind1-5];
      nw = nrow1;
      offset = 0;
	  /*
      printf("ncol=%d nrow=%d nw=%d\n",ncol1,nrow1,nw);
	  */
      ww = 1.0;

      /*printf("\n");*/

      fb16 = (unsigned short *)&bcs_.iw[ind1];
      for(irow=0; irow<nrow1; irow++)
      {
        layer = (fb16[0]>>8)&0xFF;
        strip = (fb16[0])&0xFF;
        tdc = fb16[1];
		adc = fb16[2];
		/*
        printf("layer=%d, strip=%d tdc=%d adc=%d\n",layer,strip,tdc,adc);
		*/
        if(layer>0&&layer<7&&strip>0&&strip<37)
        {
          tmpx = tdc;
          idn = k*1000+layer*100+strip;
          hf1_(&idn,&tmpx,&ww);
          tmpx = adc;
          idn = 10000 + k*1000+layer*100+strip;
          hf1_(&idn,&tmpx,&ww);
		}
        else printf("ERROR: sec=%d layer=%d strip=%d\n",k,layer,strip);

        fb16 += 3;
	  }
	}
	}





	/* */

    if(iret == -1 || iret > 0)
    {
      printf(" End-of-File flag, iret =%d\n",iret);
      goto a111111;
    }
    else if(iret < 0)
    {
      printf(" Error1, iret =%d\n",iret);
      goto a111111;
    }
    if(iret != 0)
    {
      printf(" Error2, iret =%d\n",iret);
    }
    bdrop_(bcs_.iw,"E",1);
    bgarb_(bcs_.iw);
  }
a111111:

  fparm_("CLOSE",5);

  /* closing HBOOK file */
  idn = 0;
  hrout_(&idn,&icycle," ",1);
  /*hprint_(&idn);*/
  hrend_("NTUPEL", 6);



  exit(0);

  /* end of v1190 test */

}

















