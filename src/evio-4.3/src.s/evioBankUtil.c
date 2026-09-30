
/* evioBankUtil.c - evio bank handling routines (Sergey Boyarinov) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "evioBankUtil.h"


//#define DEBUG

/*
  in following functions 'buf' is the buffer filled by evRead, for example:
       status = evRead(handler, buf, MAXBUF);
 */


/* returns pointer to the first data word */
int
evOpenBank(unsigned int *buf, int fragtag, int fragnum, int banktag, int banknum, int banktyp, char *fmt, int *ind_data)
{
  int lev, len1, n1;
  char *ch;
  int nbytes, ind_data1;
  int ind, ind1, lfrag, nw;
  unsigned int *bank_start, *ptr;

  *ind_data = 0;

  if( (ind = evLinkFrag(buf, fragtag, fragnum)) <= 0)
  {
    printf("evOpenbank ERROR: fragment tag=%d num=%d does not exist\n",fragtag,fragnum);
    return(0);
  }

  if(evLinkBank(buf, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data1) > 0)
  {
    printf("evOpenBank ERROR: bank tag=%d num=%d exist already at ind=%d\n",banktag,banknum,ind_data1);
    return(0);
  }

  lev = buf[0] + 1; /*event length*/
  lfrag = buf[ind] + 1; /*fragment length*/

#ifdef DEBUG
  printf("evOpenBank: on entry ind=%d, lev=%d, lfrag=%d\n",ind,lev,lfrag);
#endif

  ind1 = ind+lfrag;
  bank_start = (unsigned int *) &buf[ind1]; /*remember bank header location*/
  ptr = bank_start;

#ifdef DEBUG
  printf("evOpenBank: on entry ind1=%d, lev=%d, lfrag=%d\n",ind1,lev,lfrag);
#endif

  if(banktyp==0xf) /* composite bank */
  {
    len1 = strlen(fmt); /* format length in bytes */
    n1 = (len1+5)/4; /* format length in words */
    *ptr++ = 0; /* bank length: fill be set below */
    *ptr++ = (banktag<<16) + (banktyp<<8) + banknum; /*bank header*/
    /* tagsegment header following by format */
    *ptr++ = (len1<<20) + (0x6<<16) + n1;
    ch = (char *)ptr;
    strncpy(ch,fmt,len1);
    ch[len1]='\0';ch[len1+1]='\4';ch[len1+2]='\4';ch[len1+3]='\4';ch[len1+4]='\4';
    ptr += n1;
    /* 'internal' bank header */
    *ptr++ = 1;  /*set 'internal' bank length to 1 - we have header only*/
    *ptr++ = (0<<16) + (0x0<<8) + 0; /*internal bank header*/
  }
  else /* 'normal' bank with 2-word header */
  {
    *ptr++ = 1;  /*set 'internal' bank length to 1 - we have header only*/
    *ptr++ = (banktag<<16) + (banktyp<<8) + banknum; /*bank header*/
  }

  /* update event, fragment and bank length */
  nw = ptr - bank_start;
  buf[0] += nw; /*update event length*/
  buf[ind] += nw; /*update fragment length*/
  buf[ind1] = nw - 1; /*set bank length */

#ifdef DEBUG
  printf("evOpenBank: on exit bank index=%d, full length: event=%d, frag=%d bank=%d\n",ind1,buf[0]+1,buf[ind]+1,bank_start[0]+1);
#endif

#ifdef DEBUG
  printf("evOpenBank returns 0x%08x, nw=%d, *ind_data=%d\n",ptr,nw,*ind_data);
#endif

  *ind_data = ind1 + (ptr-bank_start);

  return(ind1); /* returns index of the bank */
}


/*if bank was opened by evOpenBank(), but it was no data, evCloseBank() will remove that bank */
int
evCloseBank(unsigned int *buf, int fragtag, int fragnum, int banktag, int banknum, unsigned char *b08)
{
  int lev, len1, len2, n1, nw;
  char *ch;
  int banktyp;
  int nbytes, ind_data;
  int lfrag, ind, ind2, ind3, ind_frag;
  unsigned int *bank_start, *ptr, *dataout;
  uint64_t padding;


  if( (ind_frag = evLinkFrag(buf, fragtag, fragnum)) <= 0)
  {
    printf("evCloseBank ERROR: fragment tag=%d num=%d does not exist\n",fragtag,fragnum);
    return(0);
  }

  lev = buf[0] + 1; /*event length*/
  lfrag = buf[ind_frag] + 1; /*fragment length*/
#ifdef DEBUG
  printf("evCloseBank: on entry ind_frag=%d, ind=%d, nbytes=%d, lev=%d, lfrag=%d\n",ind_frag,ind,nbytes,lev,lfrag);
#endif


  if( (ind = evLinkBank(buf, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data)) <= 0)
  {
    printf("evCloseBank ERROR: evLinkBank(buf,%d,%d,%d,%d) returns %d, cannot close it\n",fragtag,fragnum,banktag,banknum,ind);
    return(0);
  }


  if( ((unsigned char *)&buf[ind_data]) == b08)
  {
#ifdef DEBUG
    printf("\n=========evCloseBank============= &buf[ind_data]=0x%llx, b08=0x%llx\n\n",&buf[ind_data],b08);
    printf("evOpenBank() was called, but it was no data -> remove that bank\n");
#endif
    nw = buf[ind] + 1; /*the number of words written by evOpenBank()*/
    buf[0] -= nw; /*update event length*/
    buf[ind_frag] -= nw; /*update fragment length*/
    
    return(0);
  }






  
  bank_start = (unsigned int *) &buf[ind]; /*remember bank header location*/
#ifdef DEBUG
  printf("evCloseBank: bank_start = 0x%08x, full bank length=%d\n",bank_start,bank_start[0]+1);fflush(stdout);
#endif

  banktyp = (bank_start[1]>>8)&0xFF;

  if(banktyp==0xf) /* composite bank */
  {
    ind2 = ind+2; /* index of the tagsegment (contains format description) */
    len2 = (buf[ind2]&0xffff) + 1; /* tagsegment length */
    ind3 = ind2 + len2; /* index of the internal bank */

    dataout = (unsigned int *) ( ( ((uint64_t)b08+3)/4 ) * 4);
    padding = (uint64_t)dataout - (uint64_t)b08;
    buf[ind3+1] |= (padding&0x3)<<14;

    nw = dataout - bank_start - buf[ind] - 1; /* the number of DATA words */
#ifdef DEBUG
    printf("evCloseBank(composite): length befor: nw=%d ev=%d frag=%d bank=%d int_bank=%d\n",nw,buf[0],buf[ind_frag],buf[ind],buf[ind3]);
#endif

    buf[ind] += nw; /* update bank length */
    buf[ind3] += nw; /* update internal bank length */
    buf[0] += nw; /*update event length*/
    buf[ind_frag] += nw; /*update fragment length*/
  }
  else /* 'normal' bank with 2-word header */
  {
    dataout = (unsigned int *) ( ( ((uint64_t)b08+3)/4 ) * 4); /* ??? */

    nw = dataout - bank_start - buf[ind] - 1; /* the number of DATA words */
#ifdef DEBUG
    printf("evCloseBank(normal): length befor: nw=%d ev=%d frag=%d bank=%d\n",nw,buf[0],buf[ind_frag],buf[ind]);
#endif

    buf[ind] += nw; /* update bank length */
    buf[0] += nw; /*update event length*/
    buf[ind_frag] += nw; /*update fragment length*/
  }


#ifdef DEBUG
  printf("evCloseBank: length after: nw=%d ev=%d frag=%d bank=%d int_bank=%d\n",nw,buf[0],buf[ind_frag],buf[ind],buf[ind3]);
#endif

  return(ind);
}


typedef unsigned int (*index_t)[0xFF];


/* return index of the bank header; WILL SKIP BANK_OF_BANKS inside BANK_OF_BANKS !!?? */
int
evLinkBank(unsigned int *buf, int fragtag, int fragnum, int banktag, int banknum, int *nbytes, int *ind_data)
{
  int len, nw, tag1, pad1, typ1, num1, len2, pad3, ind_save, ind, ind2, ind3, indexptr;

  *nbytes = 0;
  *ind_data = 0;

#ifdef DEBUG
  printf("evLinkBank: fragtag=0x%08x fragnum=%d banktag=0x%08x banknum=%d\n",fragtag,fragnum,banktag,banknum);
#endif




#if 0
  indexptr = evIndexGet(buf);
  /*if any fragnum and any banknum requested, use indexes*/
  if(indexptr>0 && fragtag<=0xFF && fragnum==-1 /*&& banknum==-1*/)
  {
    index_t index = (index_t)&buf[indexptr];
    index_t length = (index_t)&buf[indexptr+0xFF*0xFF];
    
    *ind_data = index[fragtag][banktag&0xFF];
    *nbytes = length[fragtag][banktag&0xFF];
    //printf("evLinkBank: *ind_data=%d *nbytes=%d (index search)\n",*ind_data,*nbytes);fflush(stdout);
    return(1);
  }
#endif


  
  if( (ind = evLinkFrag(buf, fragtag, fragnum)) <= 0)
  {
#ifdef DEBUG
    printf("evLinkBank: evLinkFrag(buf,%d,%d) returns ind=%d\n",fragtag, fragnum,ind);
#endif
    return(0);
  }

  len = buf[ind]+1; /* fragment length */
  if(len<=2)
  {
#ifdef DEBUG
    printf("evLinkBank: fragment len=%d - return\n",len);
#endif
    return(0);
  }

  ind_save = ind;
  ind += 2;         /* skip fragment header */

#ifdef DEBUG
  printf("evLinkBank: starting while() loop, fragment len=%d, ind=%d\n",len,ind);
#endif

  while((ind-ind_save)<(len/*+1*/)) /*sergey: +1 ??? */
  {
    nw = buf[ind] + 1;
    tag1 = (buf[ind+1]>>16)&0xffff;
    pad1 = (buf[ind+1]>>14)&0x3;
    typ1 = (buf[ind+1]>>8)&0x3f;
    num1 =  buf[ind+1]&0xff;
#ifdef DEBUG
    printf("evLinkBank: considering bank at ind=%5d (ind_save=%5d len=%5d): nw=%d, tag1=0x%04x, pad1=0x%02x, typ1=0x%02x, num1=0x%02x\n",
		   ind,ind_save,len,nw,tag1,pad1,typ1,num1);
#endif

#ifdef DEBUG
    printf("evLinkBank: search ==> %d=%d?  %d=%d?\n",tag1,banktag,num1,banknum);
#endif

    if(tag1==banktag && num1==banknum)
    {
      if(typ1!=0xf)
      {
#ifdef DEBUG
        printf("evLinkBank: >>> found base type bank: header index %d\n",ind);
#endif
        *nbytes = (nw-2)<<2;
        *ind_data = ind+2;
	//printf("evLinkBank: *ind_data=%d *nbytes=%d (search1)\n",*ind_data,*nbytes);fflush(stdout);
        return(ind);
      }
      else
      {
        ind2 = ind+2; /* index of the tagsegment (contains format description) */
        len2 = (buf[ind2]&0xffff) + 1; /* tagsegment length */
        ind3 = ind2 + len2; /* index of the internal bank */
        pad3 = (buf[ind3+1]>>14)&0x3; /* padding from internal bank */
#ifdef DEBUG
	printf("evLinkBank: >>> found composite bank: tag=%d, type=%d, exclusive len=%d (padding from internal bank=%d)\n",((buf[ind2]>>20)&0xfff),((buf[ind2]>>16)&0xf),len2-1,pad3);
        printf("evLinkBank: return composite bank data index %d\n",ind3+2);
#endif
        *nbytes = ((nw-(2+len2+2))<<2)-pad3; /* bank_length - bank_header_length(2) - tagsegment_length(len2) - internal_bank_header_length(2) */
        *ind_data = ind+2+len2+2;
#ifdef DEBUG
	printf("evLinkBank: >>> nbytes=%d\n",*nbytes);
#endif
	//printf("evLinkBank: *ind_data=%d *nbytes=%d (search1)\n",*ind_data,*nbytes);fflush(stdout);
        return(ind);
      }
    }

    ind += nw; /* jump to the next bank */
#ifdef DEBUG
	printf("evLinkBank: >>> going to next bank, ind=%d\n",ind);
#endif
  }
#ifdef DEBUG
  printf("evLinkBank: >>> bank not found, ind=%d\n",ind);
#endif

  return(0);
}


int
evDropBank(unsigned int *buf, int fragtag, int fragnum, int banktag, int banknum)
{
  return(0);
}







int
evOpenFrag(unsigned int *buf, int fragtag, int fragnum)
{
  int ii, ind;
  int fragtyp = 0xe;
  int nw = 0; /*fragment created with length=0*/

  /* if fragment exist, cannot create it */
  if(evLinkFrag(buf, fragtag, fragnum) > 0)
  {
    printf("evOpenFrag(evLinkFrag) ERROR: fragment tag=%d num=%d exist already\n",fragtag,fragnum);
    return(0);
  }

  ind = buf[0] + 1; /* index of the first word after existing event */

  /* create fragment header only */
  buf[ind]   = 1;
  buf[ind+1] = (fragtag&0xffff)<<16;
  buf[ind+1] |= (fragtyp&0x3f)<<8;
  buf[ind+1] |= fragnum&0xff;

  buf[0] += 2; /* increase event length */
#ifdef DEBUG
  printf("evOpenFrag: ind=%d buf[ind]=%d buf[ind+1]=0x%08x buf[0]=%d\n",ind,buf[ind],buf[ind+1],buf[0]);
#endif
  return(ind);
}



/* return index of the fragment header; if fragnum<0, search by fragtag only */
int
evLinkFrag(unsigned int *buf, int fragtag, int fragnum)
{
  int ind, len, nw, tag1, pad1, typ1, num1;

  len = buf[0]+1;  /* event length */
  ind = 2;         /* skip event header */
  while(ind<len)
  {
#ifdef DEBUG
    printf("evLinkFrag: ind=%d len=%d\n",ind,len);fflush(stdout);
#endif
    nw = buf[ind] + 1;
    tag1 = (buf[ind+1]>>16)&0xffff;
    pad1 = (buf[ind+1]>>14)&0x3;
    typ1 = (buf[ind+1]>>8)&0x3f;
    num1 =  buf[ind+1]&0xff;
#ifdef DEBUG
    printf("evLinkFrag: tag1=%d typ1=%d\n",tag1,typ1);fflush(stdout);
#endif

    /*check if it is right fragment*/
    if(typ1==0xe || typ1==0x10)
    {
      if(tag1==fragtag && (num1==fragnum || fragnum<0))
      {
#ifdef DEBUG
        printf("evLinkFrag: right frag, return ind=%d\n",ind);fflush(stdout);
#endif
        return(ind);
      }
    }

    /* jump to the next fragment */
    if(typ1==0xe || typ1==0x10) ind += 2; /* if fragment, jump to the next header, can be another fragment */
    else                        ind += nw;
  }

  return(0);
}









int
evCloseFrag(unsigned int *buf, int fragtag, int fragnum)
{
  int status, ind1, nw;

  /* if input fragment does not exist - return */
  if( (ind1 = evLinkFrag(buf, fragtag, fragnum)) <= 0)
  {
    printf("evCloseFrag ERROR: input fragment tag=%d num=%d does not exist\n",fragtag,fragnum);
    return(-1);
  }

#ifdef DEBUG
  printf("evCloseFrag: input frag ind=%d, header=%d 0x%08x\n",ind1,buf[ind1],buf[ind1+1]);
#endif

  nw = buf[ind1]+1; /*fragment length*/
#ifdef DEBUG
  printf("evCloseFrag: nw=%d (ind1=%d, buf[0]=%d\n",nw,ind1,buf[0]);
#endif
  
  /* if fragment is empty, and it was last fragment in event --> remove it*/
  if(nw==2)
  {
    if( (ind1+1) == buf[0])
    {
#ifdef DEBUG
      printf("evCloseFrag: remove last fragment\n");
#endif
      buf[0] -= 2;
    }
  }
  
  return(0);
}









int
evCopyFrag(unsigned int *buf, int fragtag, int fragnum, unsigned int *bufout)
{
  int status, ind1, ind2, nw, num, ii;

  /* if input fragment does not exist - return */
  if( (ind1 = evLinkFrag(buf, fragtag, fragnum)) <= 0)
  {
    printf("evCopyFrag ERROR: input fragment tag=%d num=%d does not exist\n",fragtag,fragnum);
    return(-1);
  }

#ifdef DEBUG
  printf("evCopyFrag: input frag ind=%d, header=%d 0x%08x\n",ind1,buf[ind1],buf[ind1+1]);
#endif
  if(fragnum<0)
  {
    num = buf[ind1+1]&0xff;
  }
  else
  {
    num = fragnum;
  }
#ifdef DEBUG
  printf("evCopyFrag: input fragnum=%d\n",num);fflush(stdout);
#endif
  /* if output fragment exist, cannot create it */
  if( evLinkFrag(bufout, fragtag, num) > 0)
  {
    printf("evCopyFrag ERROR: output fragment tag=%d num=%d exist already\n",fragtag,num);fflush(stdout);
    return(-1);
  }
#ifdef DEBUG
  printf("evCopyFrag: 111\n");fflush(stdout);
#endif  
  ind2 = bufout[0] + 1; /* index of the first word after existing event */

  nw = buf[ind1]+1;
#ifdef DEBUG
  printf("evCopyFrag: ind2=%d nw=%d\n",ind2,nw);fflush(stdout);
#endif
  memcpy(&bufout[ind2],&buf[ind1],nw<<2);
  bufout[0] += nw; /* increase event length */
#ifdef DEBUG
  printf("evCopyFrag: bufout[0]=%d\n",bufout[0]);fflush(stdout);
#endif

  return(0);
}

#if 0
int
evCopyBank(unsigned int *buf, int fragtag, int fragnum, int banktag, int banknum, unsigned int *bufout)
{
  int status, ind1, ind2, nw, num, ii, nbytes, ind_data;

  /* if input bank does not exist - return */
  if( (ind1 = evLinkBank(buf, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data)) <= 0)
  {
    printf("evCopyBank ERROR: input bank fragtag=%d fragnum=%d banktag=%d banknum=%d does not exist\n",fragtag,fragnum,banktag,banknum);
    return(-1);
  }

#ifdef DEBUG
  printf("evCopyBank: input bank ind=%d, header=%d 0x%08x\n",ind1,buf[ind1],buf[ind1+1]);
#endif
  if(fragnum<0)
  {
    num = buf[ind1+1]&0xff;
  }
  else
  {
    num = fragnum;
  }
#ifdef DEBUG
  printf("evCopyBank: input fragnum=%d\n",num);
#endif

  /* if output fragment exist, cannot create it */
  if( evLinkFrag(bufout, fragtag, num) > 0)
  {
    printf("evCopyBank ERROR: output fragment tag=%d num=%d exist already\n",fragtag,num);
    return(-1);
  }

  ind2 = bufout[0] + 1; /* index of the first word after existing event */

  nw = buf[ind1]+1;
  memcpy(&bufout[ind2],&buf[ind1],nw<<2);
  bufout[0] += nw; /* increase event length */

  return(0);
}
#endif
















int
evDropFrag(unsigned int *buf, int fragtag, int fragnum)
{
  return(0);
}






int
evOpenEvent(unsigned int *buf, int eventtag)
{
  int ii, ind;
  int eventtyp = 0x10;
  int eventnum = 0xCC;

  /* create event header */
  ind = 0;
  buf[ind]   = 1;
  buf[ind+1] = (eventtag&0xffff)<<16;
  buf[ind+1] |= (eventtyp&0x3f)<<8;
  buf[ind+1] |= eventnum&0xff;

#ifdef DEBUG
  printf("evOpenEvent: ind=%d buf[ind]=%d buf[ind+1]=0x%08x buf[0]=%d\n",ind,buf[ind],buf[ind+1],buf[0]);
#endif
  return(ind);
}


int
evGarbageCollection(unsigned int *buf)
{
  return(0);
}



//#define DEBUG


const int magic[4] = {0x00010203,0x04050607,0x08090a0b,0x0c0d0e0f};
  
/*scan event and fill index array(s); place index array(s) in the end of 'buf' after last bank, and returns index of that array*/
int
evIndexSet(unsigned int *buf)
{
  int index[0xFF][0xFF] = {0}; /*data index [fragment#][banktag#]*/
  int length[0xFF][0xFF] = {0}; /*data length in bytes [fragment#][banktag#]*/
  int ii, len, nw, tag1, pad1, typ1, num1, len2, pad3, ind, num, nbytes, banktag;
  int fragtag = -1;
  int fragindex, bankindex, dataindex;

#ifdef DEBUG
  printf("\n\nevIndex reached\n");
  printf("evIndex: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		 buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
  printf("evIndex: %d %d %d %d %d %d\n",
		 buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
#endif

  len = buf[0]+1;

  /*
  if(buf[len]==magic[0]&&buf[len+1]==magic[1]&&buf[len+2]==magic[2]&&buf[len+3]==magic[3])
  {
#ifdef DEBUG
    printf("evIndex: index already exist - do nothing\n");
#endif
    return(len+4);
  }
  */

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
    
    if(typ1==0xe || typ1==0x10) /*fragment (bank of banks)*/
    {
      fragtag = tag1;
      fragindex = ii;
#ifdef DEBUG
      printf("fragindex=%d\n",fragindex);
#endif
    }
    else if(fragtag!=-1)  /*we've seen fragment already, and this is not fragment (we assume there are no bank-of-banks inside fragment)*/
    {
      if(typ1!=0xf) /*non-composite bank*/
      {
        banktag = tag1 & 0xFF; /*remove 0xe100*/
	bankindex = ii;
	dataindex = ii+2;
        nbytes = (nw-2)<<2;
#ifdef DEBUG
	printf("-> non-composite bank\n");
#endif
      }
      else /*composite bank*/
      {
        len2 = (buf[ii+2]&0xffff) + 1; /* tagsegment length (tagsegment contains format description) */
        ind = ii + len2+2; /* internal bank */
        pad3 = (buf[ind+1]>>14)&0x3; /* padding from internal bank */
#ifdef DEBUG
	printf(">>> found composite bank: tag=%d, type=%d, exclusive len=%d (padding from internal bank=%d)\n",((buf[ii+2]>>20)&0xfff),((buf[ii+2]>>16)&0xf),len2-1,pad3);
#endif
        banktag = tag1 & 0xFF; /*remove 0xe100*/
	bankindex = ii;
	dataindex = ii+2+len2+2;
        nbytes = ((nw-(2+len2+2))<<2)-pad3;
#ifdef DEBUG
	printf("-> composite bank\n");
#endif
      }

      if(fragtag<0 || banktag<0)
      {
	printf("evIndex error: fragtag=%d, banktag=%d --> exit\n",fragtag,banktag);
	exit(0);
      }
      if(fragtag<0xFF && banktag<0xFF)
      {
        index[fragtag][banktag] = dataindex;
        length[fragtag][banktag] = nbytes;
#ifdef DEBUG
        printf("---> frag=%3d, bank=0x%02x, fragindex=%d, bankindex=%d, dataindex=%d, nbytes=%d\n",fragtag,banktag,fragindex,bankindex,dataindex,nbytes);
#endif
      }
#ifdef DEBUG
      else
      {
	printf("---> frag=%3d, bank=0x%08x -> ignore\n",fragtag,banktag);
      }
#endif

    }

    if(typ1==0xe || typ1==0x10) ii += 2; /* bank of banks */
    else ii += nw;
  }


#ifdef DEBUG
  printf("=====> first empty index=%6d (len=%6d)\n\n",ii,len);
#endif

  /*write index array(s) after the last bank*/
#ifdef DEBUG
  printf("evIndex: creating new index at %d\n",len+4);
#endif
  buf[len  ] = magic[0];
  buf[len+1] = magic[1];
  buf[len+2] = magic[2];
  buf[len+3] = magic[3];
  memcpy(&buf[len+4],&index[0][0],0xFF*0xFF*4);
  memcpy(&buf[len+4+0xFF*0xFF],&length[0][0],0xFF*0xFF*4);
  
  return(len+4);
}

int
evIndexGet(unsigned int *buf)
{
  int len;
  len = buf[0]+1;
  return(len+4);
}
