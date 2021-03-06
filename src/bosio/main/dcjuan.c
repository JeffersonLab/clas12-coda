
/* dcjuan.c */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
 
#include "bosio.h"

#define NBCS 1000000
#include "bcs.h"


#define RADDEG 57.2957795130823209
#define NWPAWC 10000000 /* Length of the PAWC common block. */
#define LREC 1024       /* Record length in machine words. */
#define EINTERVAL 90000 /* Number of events between error checking */
#define THRESHOLD 0.37  /* How low the ratio of a given 6x6 
                           wire unit with respect to 
						   the reference can be without being 
						   flagged as an error*/

struct {
  float hmemor[NWPAWC];
} pawc_;

 

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

typedef struct section
 {
  int hcounter;
  int WLlist[32][6];
} section;


main(int argc, char **argv)
{
  int nr,sec,strip,nl,ncol,nrow,i,j,l,l1,l2,ichan,nn,iev,iii;
  int crate,slot,connector;
  int ind,ind1,ind2,status,status1,handle,handle1,k,m,rocid;
  int scal,nw,scaler,scaler_old, counter,dccount[7],rccount[7];
  unsigned int hel,str,strob,helicity,strob_old,helicity_old, tgbi1, layer, wire;
  int tmp1 = 1, tmp2 = 2, iret, bit1, bit2, nleading, ntrailing;
  float *bcsfl, rndm_();
  char strr[1000], bankname[5], histname[128];
  static int syn[32], id;
  MTDCHead *mtdchead;
  MTDC *mtdc;
  unsigned int leading[20000], trailing[20000];

  char *HBOOKfile = "dcjuan.his";
  int nwpawc,lun,lrec,istat,icycle,idn,nbins,nbins1,igood,offset;
  float x1,x2,y1,y2,ww,tmpx,tmpy,ttt,ref;
  unsigned short *tlv1, *tlv2, *buf16;

  int lyr,wir;
  float ratio;
  int htotal=0,errors=0,lyrcount=0, sectcount=0;
  section sectlist[6];
  section reference[6];
  section errorlist[6];

  for(sec=0; sec<=5; sec++)
  {
	sectlist[sec].hcounter=0;
	for(wir=0;wir<=31;wir++){
	  for(lyr=0; lyr<=5;lyr++){
		sectlist[sec].WLlist[wir][lyr]=0;
		reference[sec].WLlist[wir][lyr]=0;
		errorlist[sec].WLlist[wir][lyr]=0;
	  }
	}
  }

  printf(" dchist reached  !\n");
 
  if(argc != 2)
  {
    printf("Usage: dchist <fpack_filename>\n");
    exit(1);
  }


  /* BOS initialization */
  bcsfl = (float*)bcs_.iw;
  bosInit(bcs_.iw,NBCS);


  /* HBOOK initialization */
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


  nbins=100;
  x1 = 0.;
  x2 = 100.;
  ww = 0.;
  for(idn=101; idn<=106; idn++)
  {
    sprintf(histname,"DC load REG1 SEC%1d",idn-100);
    hbook1_(&idn,histname,&nbins,&x1,&x2,&ww,strlen(histname));
  }


  nbins=192;
  x1 = 0.;
  x2 = 192.;
  nbins1=36;
  y1 = 0.;
  y2 = 36.;
  ww = 0.;
  for(idn=201; idn<=206; idn++)
  {
    sprintf(histname,"DC load SEC%1d",idn-200);
    hbook2_(&idn,histname,&nbins,&x1,&x2,&nbins1,&y1,&y2,&ww,strlen(histname));
  }

  nbins=32;
  x1 = 0.;
  x2 = 32.;
  nbins1=6;
  y1 = 0.;
  y2 = 6.;
  ww = 0.;
  for(idn=301; idn<=306; idn++)
  {
    sprintf(histname,"Load normalized SEC%1d",idn-300);
    hbook2_(&idn,histname,&nbins,&x1,&x2,&nbins1,&y1,&y2,&ww,strlen(histname));
  }




  sprintf(strr,"OPEN INPUT UNIT=1 FILE='%s' ",argv[1]);
  printf("fparm string: >%s<\n",strr);
  status = fparm_(strr,strlen(strr));

  /*TIME INTERVALS BEGIN*/
  iev = 0;
  for(iii=0; iii<10000000; iii++)
  {
    frbos_(bcs_.iw,&tmp1,"E",&iret,1);
    if(iret != 0) goto roundup;


    if((ind = bosNlink(bcs_.iw,"HEAD",0)) == 0) goto roundup;
    if(bcs_.iw[ind+4] > 9) goto roundup; /* not  physics event */
    if(bcs_.iw[ind+6]>15) goto roundup;
	/*
    printf("HEAD: run number %d, event number %d\n",bcs_.iw[ind+1],bcs_.iw[ind+2]); 
	*/
    iev++;
    if(iev > 75*EINTERVAL) goto roundup;

    if(!(iev%15000)) printf("===== Event no. %d\n",iev);


	/*COPYING TO REFERENCE AFTER FIRST EVENT INTERVAL*/
	if(iev==EINTERVAL){
	  for(sec=0; sec<=5; sec++){
		for(wir=0;wir<=31;wir++){
		  for(lyr=0; lyr<=5;lyr++){
			reference[sec].WLlist[wir][lyr]=sectlist[sec].WLlist[wir][lyr];
			sectlist[sec].WLlist[wir][lyr]=0;
		  }
		}
	  }
	}

	/*COMPARE WITH REFERENCE TO SCAN FOR DROPS/JUMPS AND MARK IN ERRORLIST*/
	if( (!(iev%EINTERVAL)) && iev!=EINTERVAL && iev!=0){

	 
	  for(sec=0; sec<=0/*5*/; sec++){
		for(lyr=0;lyr<=5;lyr++){
		  for(wir=0; wir<=31;wir++){
			printf("count for sector:%d  superlayer:%d  group: %d   hits: %d\n", sec+1, lyr+1, wir+1, sectlist[sec].WLlist[wir][lyr]);
			
		  }
		}
	  }


	  for(sec=0; sec<=5; sec++){
		for(wir=0;wir<=31;wir++){
		  for(lyr=0; lyr<=5;lyr++){

		  

          idn=sec+301;
          tmpx = wir;
          tmpy = lyr;
		  ratio = (float) sectlist[sec].WLlist[wir][lyr]/(float) reference[sec].WLlist[wir][lyr];

          if(reference[sec].WLlist[wir][lyr]<=300) ww = 0.;
          else
          {
            ww = ratio;
            hf2_(&idn,&tmpx,&tmpy,&ww);
		  }

		   	if( ratio <THRESHOLD && reference[sec].WLlist[wir][lyr] > 300){
			   errors++;
			   errorlist[sec].WLlist[wir][lyr] = sectlist[sec].WLlist[wir][lyr];
			  
			}
		   	if( ratio >(2-THRESHOLD) && reference[sec].WLlist[wir][lyr] > 300){
			   errors++;
			   errorlist[sec].WLlist[wir][lyr] = sectlist[sec].WLlist[wir][lyr];
			 
			}
		  }
		}
	  }
	  /*SEND ALARM IF THERE ARE GREATER THAN 4 ERRORS IN TIME INTERVAL*/
	  if(errors>=100){
		printf("WARNING:OVER 100 ERRORS, MAJOR DROPS OF %f PERCENT OR MORE\nIF NOT COINCIDING WTIH BEAM CURRENT DROP BE ALARMED\n", (100*(1-THRESHOLD)));
	  }
	  if(errors>4 && errors<100){

		/*CHECKS TO SEE IF A SUPERLAYER OR SECTION IS ERRORING*/
	    for(sec=0; sec<=5; sec++){

		  sectcount=0;

		  for(lyr=0; lyr<=5;lyr++){

			lyrcount=0;

			for(wir=0;wir<=31;wir++){
			
				if(errorlist[sec].WLlist[wir][lyr]!=0){
				  lyrcount++;
				  sectcount++;
				  }
			  
			}
			/*IF THERE IS A MAJOR DROP IN SUPERLAYER SET VALUES TO -1
			AS TO CIRCUMVENT 6X6 ALARMS*/
			if(lyrcount>4){
			  for(wir=0;wir<=31;wir++){
				errorlist[sec].WLlist[wir][lyr]=-1;
			  }
			  printf("ALARM:DROP IN SUPERLAYER section:%d, superlayer:%d, %d of 32 sections dropped\n",sec+1,lyr+1,lyrcount);
			  sectcount=0;
			}
		  }
		  if(sectcount>4){
			errorlist[sec].hcounter++;
		    printf("WARNING:5 or more 6x6 wire units dropped in section %d\n", sec+1); 
		  }
		}

		/*6x6 ALARMS*/
		 for(sec=0; sec<=5; sec++){
		   for(wir=0;wir<=31;wir++){
			 for(lyr=0; lyr<=5;lyr++){

			   ratio = (float) errorlist[sec].WLlist[wir][lyr]/(float) reference[sec].WLlist[wir][lyr];
			  
			   /*PRINTS ERRORS IN SECTIONS WITH 5 OR MORE DROPS*/
			   if(errorlist[sec].WLlist[wir][lyr]>0 && errorlist[sec].hcounter>0){
			     if(ratio <THRESHOLD){
				   printf("WARNING:DROP in section:%d, wires:%d-%d, layers:%d-%d, ",sec+1,wir*6,(wir+1)*6, lyr*6, (lyr+1)*6);
				   printf("drop of %f percent\n",100*(1- (float) sectlist[sec].WLlist[wir][lyr]/(float) reference[sec].WLlist[wir][lyr]));
				   /*
				   printf("reference is at:%d and current event is at: %d\n", reference[sec].WLlist[wir][lyr], sectlist[sec].WLlist[wir][lyr]);
				   */
				 }

				 if(ratio >(2-THRESHOLD)){ 
			      printf("WARNING:JUMP in section:%d, wires:%d-%d, layers:%d-%d, ",sec+1,wir*6,(wir+1)*6, lyr*6, (lyr+1)*6);
				  printf("increase of %f percent\n",-100*(1- (float) sectlist[sec].WLlist[wir][lyr]/(float) reference[sec].WLlist[wir][lyr]));
				  /*
                  printf("reference is at:%d and current event is at: %d\n", reference[sec].WLlist[wir][lyr], sectlist[sec].WLlist[wir][lyr]);
				  */
				 }

			   }
			 }
		   }
		 }
	  }
	
	  errors=0;
	  
	  for(sec=0; sec<=5; sec++){
		for(wir=0;wir<=31;wir++){
		  for(lyr=0; lyr<=5;lyr++){
			sectlist[sec].WLlist[wir][lyr]=0;
			errorlist[sec].WLlist[wir][lyr]=0;
		  }
		}
	  }
	 
    }


    for(sec=1; sec<=6; sec++)
    {
	  dccount[sec] = 0;

      if((ind1=bosNlink(bcs_.iw,"DC0 ",sec)) > 0)
      {
        unsigned short *b16;
        int crate,slot,channel,edge,data,count,ncol1,nrow1,ii;

        ncol1 = bcs_.iw[ind1-6];
        nrow1 = bcs_.iw[ind1-5];
        nw = nrow1;
        offset = 0;
		/*	    
        printf("sec=%d ncol=%d nrow=%d nw=%d\n",sec,ncol1,nrow1,nw);
	    */
        ww = 1.0;

        /*printf("\n");*/

        b16 = (unsigned short *)&bcs_.iw[ind1];

		/*		
        printf("\nsector %d\n",sec);
	    for(ii=0; ii<nrow1; ii++) printf("[%d] -> 0x%04x 0x%04x)\n",
          ii,b16[ii*2],b16[ii*2+1]);
		*/
        counter = 0;
        for(ii=0; ii<nrow1; ii++)
        {
          layer = (b16[0]>>8)&0xFF;
          wire = b16[0]&0xFF;
		  /*
		  if(sec==1)
		  {
            if(layer>30) printf("[%2d] ==> layer=%d wire=%d (group=%d)\n",ii,layer,wire,(wire-1)/6+1);
		    else printf("[%2d] --> layer=%d wire=%d (group=%d)\n",ii,layer,wire,(wire-1)/6+1);
		  }
		  */
		  htotal++;

          idn=sec+200;
          tmpx = wire;
          tmpy = layer;
          hf2_(&idn,&tmpx,&tmpy,&ww);
		  /*
		  printf("layer=%u wire=%u\n",layer,wire);
		  */
		  if(layer>0&&layer<37&&wire>0&&wire<193)
		  {
		    sectlist[sec-1].hcounter++;
		    sectlist[sec-1].WLlist[(wire-1)/6][(layer-1)/6]++;

            if(layer<13) counter ++;
		  }
          b16+=2;
	    }

        dccount[sec] = counter;
        tmpx = (float)counter;
        idn=sec+100;
        hf1_(&idn,&tmpx,&ww);

	  }

	}


	/*	
{
  int ss,ll,ww;
for(ss=0; ss<=0; ss++){
for(ll=5;ll<=5;ll++){
for(ww=0; ww<=31;ww++){
  printf("count for sector:%d  superlayer:%d  group: %d   hits: %d\n", 
    ss+1, ll+1, ww+1, sectlist[ss].WLlist[ww][ll]);			
}
}
}
}
exit(0);
*/

	/* */

roundup:

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




  for(sec=0; sec<=5; sec++)
  {
	
	printf("section %d has a total of %d hits with a ratio of %f\n", sec+1, sectlist[sec].hcounter, (float) sectlist[sec].hcounter/(float) htotal);
  }  

  
  printf("total number of hits is: %d\n", htotal);







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

