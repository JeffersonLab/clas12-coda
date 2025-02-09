
/* evio_fadc250hist.c */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG


#define ROCID 90
#define NSAMPS4PED 10


#define MAX(a,b)          ( (a) > (b) ? (a) : (b) )
 
#define NWPAWC 10000000 /* Length of the PAWC common block. */
#define LREC 1024       /* Record length in machine words. */

struct {
  float hmemor[NWPAWC];
} pawc_;


#define MAXBUF 1000000
unsigned int buf[MAXBUF];

#define SWAP32(x) ( (((x) >> 24) & 0x000000FF) | \
                    (((x) >> 8)  & 0x0000FF00) | \
                    (((x) << 8)  & 0x00FF0000) | \
                    (((x) << 24) & 0xFF000000) )

#define PRINT_BUFFER \
  b08 = start; \
  while(b08<end) \
  { \
    GET32(tmp); \
    printf("== 0x%08x\n",tmp); \
  } \
  b08 = start


#define GET8(ret_val) \
  ret_val = *b08++

#define GET16(ret_val) \
  b16 = (unsigned short *)b08; \
  ret_val = *b16; \
  b08+=2

#define GET32(ret_val) \
  b32 = (unsigned int *)b08; \
  ret_val = *b32; \
  b08+=4

#define GET64(ret_val) \
  b64 = (unsigned long long *)b08; \
  ret_val = *b64; \
  b08+=8

int
evNlink(unsigned int *buf, int frag, int tag, int num, int *nbytes)
{
  int ii, len, nw, tag1, pad1, typ1, num1, len2, pad3, ind;
  int right_frag = 0;


#ifdef DEBUG
  printf("0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		 buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
  printf("%d %d %d %d %d %d\n",
		 buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
#endif

  len = buf[0]+1;
  ii = 2;
  while(ii<len)
  {
    nw = buf[ii] + 1;
    tag1 = (buf[ii+1]>>16)&0xffff;
    pad1 = (buf[ii+1]>>14)&0x3;
    typ1 = (buf[ii+1]>>8)&0x3f;
    num1 =  buf[ii+1]&0xff;
#ifdef DEBUG
    printf("[%5d] nw=%d, tag1=0x%04x, pad1=0x%02x, typ1=0x%02x, num1=0x%02x\n",ii,nw,tag1,pad1,typ1,num1);
#endif
    /*check if it is right fragment*/
    if(typ1==0xe || typ1==0x10)
    {
      if(tag1==frag)
      {
#ifdef DEBUG
        printf("right frag\n");
#endif
        right_frag = 1;
      }
      else
      {
#ifdef DEBUG
        printf("wrong frag\n");
#endif
        right_frag = 0;
      }
    }

#ifdef DEBUG
    printf("search ==> %d=1?  %d=%d?  %d=%d?\n",right_frag,tag1,tag,num1,num);
#endif
    if(typ1!=0xe && typ1!=0x10) /*assumes there are no bank-of-banks inside fragment, will redo later*/
    {
      if( right_frag==1 && tag1==tag && num1==num )
      {
        if(typ1!=0xf)
        {
#ifdef DEBUG
          printf("return primitive bank data index %d\n",ii+2);
#endif
          *nbytes = (nw-2)<<2;
          return(ii+2);
        }
        else
        {
          len2 = (buf[ii+2]&0xffff) + 1; /* tagsegment length (tagsegment contains format description) */
          ind = ii + len2+2; /* internal bank */
          pad3 = (buf[ind+1]>>14)&0x3; /* padding from internal bank */
#ifdef DEBUG
	  printf(">>> found composite bank: tag=%d, type=%d, exclusive len=%d (padding from internal bank=%d)\n",((buf[ii+2]>>20)&0xfff),((buf[ii+2]>>16)&0xf),len2-1,pad3);
          printf("return composite bank data index %d\n",ii+2+len2+2);
#endif
          *nbytes = ((nw-(2+len2+2))<<2)-pad3;
#ifdef DEBUG
	  printf(">>> nbytes=%d\n",*nbytes);
#endif
          return(ii+2+len2+2);
        }
      }
    }

    if(typ1==0xe || typ1==0x10) ii += 2; /* bank of banks */
    else ii += nw;
  }

  return(0);
}

int
main(int argc, char **argv)
{
  FILE *fd = NULL;
  char fname[1024];
  int handler, status, ifpga;
  unsigned long long *b64;
  unsigned int *b32;
  unsigned short *b16, iw16;
  unsigned char *b08;
  int trig,chan,fpga,apv,hybrid;
  int i1;
  float f1,f2;

  int nr,sec,layer,strip,nl,ncol,nrow,i,j,ii,jj,kk,l,l1,l2,ichan,nn,iev,nbytes,ind1;
  char title[128];
  char *HBOOKfile = "fadc250hist.his";
  int nwpawc,lun,lrec,istat,icycle,idn,nbins,nbins1,igood,offset;
  float x1,x2,y1,y2,ww,tmpx,tmpy,ttt,ref;

  if(argc != 2)
  {
    printf("Usage: evio_fadc250hist <evio_filename>\n");
    exit(1);
  }

  nwpawc = NWPAWC;
  hlimit_(&nwpawc);
  lun = 11;
  lrec = LREC;
  hropen_(&lun,"NTUPEL",HBOOKfile,"N",&lrec,&istat,strlen("NTUPEL"),strlen(HBOOKfile),1);
  if(istat)
  {
    printf("\aError: cannot open RZ file %s for writing.\n", HBOOKfile);
    exit(0);
  }
  else
  {
    printf("RZ file >%s< opened for writing, istat = %d\n\n", HBOOKfile, istat);
  }




  nbins=120;
  x1 = 0.;
  x2 = 120.;
  ww = 0.;
  for(ii=1; ii<=21; ii++)
  {
    for(jj=0; jj<=15; jj++)
    {
      idn = ii*100+jj;
      sprintf(title,"rawdata sl%d ch%d",ii,jj);
      hbook1_(&idn,title,&nbins,&x1,&x2,&ww,strlen(title));
    }
  }

  nbins=400;
  x1 = 0.;
  x2 = 400.;
  ww = 0.;
  for(ii=1; ii<=21; ii++)
  {
    for(jj=0; jj<=15; jj++)
    {
      idn = 10000 + ii*100+jj;
      sprintf(title,"peds sl%d ch%d",ii,jj);
      hbook1_(&idn,title,&nbins,&x1,&x2,&ww,strlen(title));
    }
  }

  
  nbins=4100;
  x1 = -1000.;
  x2 = 40000.;
  ww = 0.;
  for(ii=1; ii<=21; ii++)
  {
    for(jj=0; jj<=15; jj++)
    {
      idn = 20000+ii*100+jj;
      sprintf(title,"spectra sl%d ch%d",ii,jj);
      hbook1_(&idn,title,&nbins,&x1,&x2,&ww,strlen(title));
    }
  }

  idn = 25000;
  sprintf(title,"2d moller",jj);
  nbins = 100;
  nbins1 = 100;
  x1 = 0.;
  x2 = 1000.;
  y1 = 0.;
  y2 = 1000.;
  ww = 0.;
  hbook2_(&idn,title,&nbins,&x1,&x2,&nbins1,&y1,&y2,&ww,strlen(title));


  nbins = 200;
  nbins1 = 200;
  x1 = -1000.;
  x2 = 10000.;
  y1 = -1000.;
  y2 = 10000.;
  ww = 0.;
  for(ii=0; ii<12; ii++)
  {
    idn = 26000+ii;
    sprintf(title,"2d for group %2d",ii);
    hbook2_(&idn,title,&nbins,&x1,&x2,&nbins1,&y1,&y2,&ww,strlen(title));
  }


  nbins=100;
  x1 = 0.;
  x2 = 100.;
  ww = 0.;
  for(ii=1; ii<=21; ii++)
  {
    for(jj=0; jj<=15; jj++)
    {
      idn = 30000+ii*100+jj;
      sprintf(title,"pulse time sl%d ch%d",ii,jj);
      hbook1_(&idn,title,&nbins,&x1,&x2,&ww,strlen(title));
    }
  }


  /*
  nbins=80;
  nbins1=80;
  x1 = 0.;
  x2 = 2000.;
  y1 = 0.;
  y2 = 8000.;
  ww = 0.;
  for(idn=1001; idn<=1036; idn++)
  {
    hbook2_(&idn,"YvxX",&nbins,&x1,&x2,&nbins1,&y1,&y2,&ww,4);
  }
  */


  nbins=600;
  x1 = 0./*4000.*/;
  x2 = 6000./*5000.*/;
  ww = 0.;






  status = evOpen(argv[1],"r",&handler);
  if(status < 0)
  {
    printf("evOpen error %d - exit\n",status);
    exit(0);
  }

  for(iev=1; iev<4000000; iev++)
  {

    if(!(iev%10000)) printf("\n\n\nEvent %d\n\n",iev);

    status = evRead(handler, buf, MAXBUF);
    if(status < 0)
    {
      if(status==EOF) printf("end of file after %d events - exit\n",iev);
      else printf("evRead error=%d after %d events - exit\n",status,iev);
      break;
    }

    if(iev < 3) continue; /*skip first 2 events*/

	
    if((ind1 = evNlink(buf, ROCID, 0xe101, 0, &nbytes)) > 0)
    {
      unsigned char *end;
      unsigned long long time;
      int crate,slot,trig,nchan,chan,nsamples,notvalid,edge,data,count,ncol1,nrow1;
      int oldslot = 100;
      int ndata0[22], data0[21][8];
      float sum, ped, integral, ww16, wmax;
      int moller1, moller2;
      int ba[12][2];

#ifdef DEBUG
      printf("ind1=%d, nbytes=%d\n",ind1,nbytes);
#endif
      b08 = (unsigned char *) &buf[ind1];
      end = b08 + nbytes;
#ifdef DEBUG
      printf("ind1=%d, nbytes=%d (from 0x%08x to 0x%08x)\n",ind1,nbytes,b08,end);
#endif
      moller1 = moller2 = 0;
      for(ii=0; ii<12; ii++) ba[ii][0]=ba[ii][1]=0;
      while(b08<end)
      {
#ifdef DEBUG
        printf("begin while: b08=0x%08x\n",b08);
#endif
        GET8(slot);
        GET32(trig);
        GET64(time);
        GET32(nchan);
#ifdef DEBUG
        printf("slot=%d, trig=%d, time=%lld nchan=%d\n",slot,trig,time,nchan);
#endif
        for(jj=0; jj<nchan; jj++)
	{
          GET8(chan);
          /*chan++;*/
          GET32(nsamples);
#ifdef DEBUG
          printf("  chan=%d, nsamples=%d\n",chan,nsamples);
#endif
          sum = 0.0;
          integral = 0.0;
          wmax = 0.0;
          for(kk=0; kk<nsamples; kk++)
	  {
	    GET16(iw16);
            ww16 = (float)iw16;

            /*rawdata*/
            tmpx = kk+0.5;
	    idn = slot*100+chan;
	    hf1_(&idn,&tmpx,&ww16);

            if(kk<NSAMPS4PED)
	    {
              sum = sum + ww16;
	    }
            else if(kk==NSAMPS4PED)
	    {
              ped = sum / (float)NSAMPS4PED;
              //printf("ped=%f\n",ped);

              /*pedestal*/
              idn = 10000 + slot*100+chan;
              tmpx = ww16;
              ww = 1.;
	      hf1_(&idn,&tmpx,&ww);
	    }
            else if(kk>15 && kk<40)
	    {
              wmax = MAX(wmax,(ww16 - ped));
              integral = integral + (ww16 - ped);
              //printf("[kk=%2d] integral=%7.2f wmax=%7.2f ...\n",kk,integral,wmax);
	    }
	  }

	  //if(wmax>10.)
	  {
            if(slot==3 && chan==14) moller1 = integral;
            if(slot==3 && chan==15) moller2 = integral;

            if     (slot==3 && chan==13) ba[0][0] = integral;
            else if(slot==3 && chan==12) ba[1][0] = integral;
            else if(slot==3 && chan==11) ba[2][0] = integral;
            else if(slot==3 && chan==10) ba[3][0] = integral;
            else if(slot==3 && chan==9) ba[4][0] = integral;
            else if(slot==3 && chan==8) ba[5][0] = integral;
            else if(slot==3 && chan==7) ba[6][0] = integral;
            else if(slot==3 && chan==6) ba[7][0] = integral;
            else if(slot==3 && chan==5) ba[8][0] = integral;
            else if(slot==3 && chan==4) ba[9][0] = integral;
            else if(slot==3 && chan==3) ba[10][0] = integral;
            else if(slot==3 && chan==2) ba[11][0] = integral;

            if     (slot==3 && chan==1) ba[0][1] = integral;
            else if(slot==4 && chan==2) ba[1][1] = integral;
            else if(slot==4 && chan==0) ba[2][1] = integral;
            else if(slot==4 && chan==1) ba[3][1] = integral;
            else if(slot==3 && chan==0) ba[4][1] = integral;
            else if(slot==4 && chan==3) ba[5][1] = integral;
            else if(slot==4 && chan==4) ba[6][1] = integral;
            else if(slot==4 && chan==5) ba[7][1] = integral;
            else if(slot==4 && chan==6) ba[8][1] = integral;
            else if(slot==4 && chan==7) ba[9][1] = integral;
            else if(slot==4 && chan==8) ba[10][1] = integral;
            else if(slot==4 && chan==9) ba[11][1] = integral;

            //printf("integral=%f\n",integral);

            /*spectra*/
            tmpx = integral;
	    ww = 1.;
	    idn = 20000+slot*100+chan;
	    hf1_(&idn,&tmpx,&ww);

	    if(moller1>0 && moller2>0)
	    {
              idn = 25000;
              tmpx = (float)moller1;
              tmpy = (float)moller2;
 	      ww = 1.;
             hf2_(&idn,&tmpx,&tmpy,&ww);
	    }
	  }



	}
#ifdef DEBUG
        printf("end loop: b08=0x%08x\n",b08);
#endif
      }

      for(ii=0;ii<12;ii++)
      {
        if(ba[ii][0]>500 && ba[ii][1]>500)
	{
          //printf("[%2d] %6d %6d\n",ii,ba[ii][0],ba[ii][1]);
          idn = 26000+ii;
          tmpx = (float)ba[ii][0];
          tmpy = (float)ba[ii][1];
	  ww   = 1.;
          hf2_(&idn,&tmpx,&tmpy,&ww);
	}
      }

    }


	

#if 0
    if((ind1 = evNlink(buf, ROCID, 0xe103, 0, &nbytes)) > 0)
    {
      unsigned short pulse_time;
      unsigned int pulse_integral;
      unsigned char *end;
      unsigned long long time;
      int crate,slot,trig,nchan,chan,npulses,notvalid,edge,data,count,ncol1,nrow1;
      int oldslot = 100;
      int ndata0[22], data0[21][8];
      int sum, channel;


      b08 = (unsigned char *) &buf[ind1];
      b16 = (unsigned short *) &buf[ind1];
      b32 = (unsigned int *) &buf[ind1];

      end = b08 + nbytes;
#ifdef DEBUG
      printf("ind1=%d, nbytes=%d (from 0x%08x to 0x%08x)\n",ind1,nbytes,b32,end);
#endif
      while(b08<end)
      {
#ifdef DEBUG
        printf("begin while: b08=0x%08x\n",b08);
#endif
        b08 = (unsigned char *)b32;
        slot = *b08 ++;

        b32 = (unsigned int *)b08;
        trig = *b32++;

        b64 = (unsigned long long *)b32;
        time = *b64++;

        b32 = (unsigned int *)b64;
        nchan = *b32++;
#ifdef DEBUG
        printf("slot=%d, trig=%d, time=%lld nchan=%d\n",slot,trig,time,nchan);
#endif
        for(jj=0; jj<nchan; jj++)
	{
          b08 = (unsigned char *)b32;
          chan = (*b08 ++) /*+ 1*/;
      
          b32 = (unsigned int *)b08;
          npulses = *b32++;
#ifdef DEBUG
          printf("  chan=%d, npulses=%d\n",chan,npulses);
#endif
          for(kk=0; kk<npulses; kk++)
	  {
            b16 = (unsigned short *)b32;
            pulse_time = (*b16++)>>6;
            b32 = (unsigned int *)b16;
            pulse_integral = *b32++;
#ifdef DEBUG
            printf(" b32=0x%08x:  pulse_time=%d pulse_integral=%d\n",b32,pulse_time,pulse_integral);
#endif			
            tmpx = pulse_integral;
	    ww   = 1.;
	    idn  = 20000+slot*100+chan;
	    hf1_(&idn,&tmpx,&ww);

            tmpx = pulse_time;
	    ww   = 1.;
	    idn  = 30000+slot*100+chan;
	    hf1_(&idn,&tmpx,&ww);	
	  }
	}
        b08 = (unsigned char *)b32;
#ifdef DEBUG
        printf("end loop: b08=0x%08x\n",b08);
#endif
      }
    }
#endif


  }

  printf("evClose after %d events\n",iev);
  evClose(handler);

  /* closing HBOOK file */
  idn = 0;
  hrout_(&idn,&icycle," ",1);
  /*hprint_(&idn);*/
  hrend_("NTUPEL", 6);

  exit(0);
}

















