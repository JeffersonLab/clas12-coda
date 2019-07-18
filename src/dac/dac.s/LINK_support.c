
/* LINK_support.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#include "rolInt.h"
#include "da.h"
#include "circbuf.h"
#include "libdb.h"
#include "eviofmt.h"
#include "LINK_support.h"


#undef DEBUG

#define CODA_ERROR 1
#define CODA_OK 0


/* external data */

extern char *mysql_host;

extern unsigned int *dataSent; /* see coda_component.c */

int deflt; /* 1 for CODA format, 0 for BOS format (see coda_eb_inc.c) */
#ifdef USE_128
  WORD128 roc_linked;
#else
  unsigned int roc_linked; /* see deb_component.c */
#endif
CIRCBUF *roc_queues[MAX_ROCS]; /* see deb_component.c */
int      roc_queue_ix; /* see deb_component.c */
unsigned int *bufpool[MAX_ROCS][QSIZE]; /* allocated in coda_ebc.c */


static int ending_for_recv;

/* swap big buffer; called by network thread on receiving buffer from ROC */
int
bufferSwap(unsigned int *cbuf, int nlongs)
{
  unsigned int lwd, t1, t2;
  int ii, jj, kk, ix;
  int tlen, blen, dtype, typ, num;
  short shd;
  char cd;
  char *cp;
  short *sp;
  unsigned int *lp;


  ii = 0;

  /* swap buffer header: BBHEAD words */
  lp = (unsigned int *)&cbuf[ii];
  for(jj=0; jj<BBHEAD; jj++)
  {
	lwd = LSWAP(*lp);
	*lp++ = lwd;
  }
  ii += BBHEAD;


#ifdef DEBUG
  printf("\nbufferSwap: buffer header: length=%d words, buffer#=%d, rocid=%d, #events=%d, fd/magic=0x%08x, end=%d\n",
		 cbuf[BBIWORDS],cbuf[BBIBUFNUM],cbuf[BBIROCID],cbuf[BBIEVENTS],cbuf[BBIFD],cbuf[BBIEND]);
#endif


#ifdef DEBUG
  /* print CODA fragments 
  kk = ii;
  while(kk<nlongs)
  {
    lp = (unsigned int *)&cbuf[kk];

    lwd = LSWAP(*lp);
    lp++;
    blen = lwd - 1;
	t1=lwd;
	
	printf("CODA fragment: length = %d, current kk=%d, ",blen+1,kk);
	
    lwd = LSWAP(*lp);
    lp++;
    num = lwd&0xff;
    dtype = (lwd>>8)&0x3f;
    typ = (lwd>>16)&0xff;
	t2=lwd;
	
	printf("2nd word(0x%08x): tag=%d, dtype=%d, num=%d\n",lwd,typ,dtype,num);
	
    kk += 2;

    if(blen == 0) continue;

    if(dtype != DT_BANK)
    {
	  switch(dtswap[dtype])
      {
        case 0:
		  printf("case 0: no swap\n");
	      kk += blen;
	    break;

        case 1:
		  printf("case 1: short swap\n");
	      kk += blen;
	      break;

        case 2:
		  printf("case 2: int swap, deflt=%d\n",deflt);
	      kk += blen;
	      break;

        case 3:
		  printf("case 3: double swap\n");
	      kk += blen;
	      break;

        case 4:
		  printf("case 4: composite swap, blen=%d\n",blen);fflush(stdout);
	      kk += blen;
		  break;

        case 5:
		  printf("case 5: bank of banks swap - do nothing (header swapped already)\n");
		  break;

        default:
		  printf("default: no swap\n");
	      kk += blen;
      }
    }
    else
    {
      printf("DT_BANK: dtype=0x%08x\n",dtype);
    }
  }
  */
#endif


  /* swap CODA fragments */
  while(ii<nlongs)
  {
    lp = (unsigned int *)&cbuf[ii];

    lwd = LSWAP(*lp);    /* Swap the CODA fragment length */
    *lp++ = lwd;
    blen = lwd - 1;
	t1=lwd;
	
#ifdef DEBUG
	printf("bufferSwap: length = %d, current ii=%d, ",blen+1,ii);
#endif
	
    lwd = LSWAP(*lp);    /* Swap the CODA fragment header */
    *lp++ = lwd;
    num = lwd&0xff;
    dtype = (lwd>>8)&0x3f/*0xff*/;
    typ = (lwd>>16)&0xff;
	t2=lwd;
	
#ifdef DEBUG
	printf("bufferSwap: 2nd word(0x%08x): tag=%d, dtype=%d, num=%d\n",lwd,typ,dtype,num);
#endif
	
    ii += 2;

    if(blen == 0) continue; /* nothing to do with empty fragment */

    if(dtype != DT_BANK)
    {
	  switch(dtswap[dtype])
      {
        case 0:
#ifdef DEBUG
		  printf("bufferSwap: case 0: no swap\n");
#endif
	/*
		  printf("ii=%d nlongs=%d 0x%08x 0x%08x)\n",
            ii,nlongs,t1,t2);
		  {
            FILE *fd;
            int iii;
            fd = fopen("/home/boiarino/abc.txt","w");
            for(iii=0; iii<ii; iii++) fprintf(fd,"[%6d] 0x%08x\n",iii,cbuf[iii]);
            fclose(fd);
		  }
		  exit(0);
					*/
	      /* No swap */
	      ii += blen;
	    break;

        case 1:
#ifdef DEBUG
		  printf("bufferSwap: case 1: short swap\n");
#endif
	      /* short swap */
	      sp = (short *)&cbuf[ii];
	      for(jj=0; jj<(blen<<1); jj++)
          {
	        shd = SSWAP(*sp);
	        *sp++ = shd;
	      }
	      ii += blen;
	      break;

        case 2:
#ifdef DEBUG
		  printf("bufferSwap: case 2: int swap, deflt=%d\n",deflt);
#endif
          /* int swap */
          lp = (unsigned int *)&cbuf[ii];
          for(jj=0; jj<blen; jj++)
          {
            lwd = LSWAP(*lp);
            *lp++ = lwd;
          }
	      ii += blen;
	      break;

        case 3:
#ifdef DEBUG
		  printf("bufferSwap: case 3: double swap\n");
#endif
	      /* double swap */
	      lp = (unsigned int *)&cbuf[ii];
	      for(jj=0; jj<blen; jj++)
          {
	        lwd = LSWAP(*lp);
	        *lp++ = lwd;
	      }
	      ii += blen;
	      break;

        case 4:
#ifdef DEBUG
		  printf("bufferSwap: case 4: composite swap, blen=%d\n",blen);fflush(stdout);
#endif

		  /*
lp = (unsigned int *)&cbuf[ii+blen];
printf("befor: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",lp[0],lp[1],lp[2],lp[3],lp[4],lp[5],lp[6],lp[7],lp[8],lp[9]);
		  */
		  lp = (unsigned int *)&cbuf[ii];
          swap_composite_t(lp, 1, NULL);
		  /*
          printf("case 4: composite swap done\n");fflush(stdout);
		  */
		  /*
lp = (unsigned int *)&cbuf[ii+blen];
printf("after: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",lp[0],lp[1],lp[2],lp[3],lp[4],lp[5],lp[6],lp[7],lp[8],lp[9]);
		  */
	      ii += blen;
		  break;

        case 5:
#ifdef DEBUG
		  printf("bufferSwap: case 5: bank of banks swap - do nothing (header swapped already)\n");
#endif
		  break;

        default:
		  printf("bufferSwap: default: no swap\n");
	      /* No swap */
	      ii += blen;
      }
    }
    else
    {
      printf("bufferSwap: DT_BANK: dtype=0x%08x\n",dtype);
    }
  }

  return(0);
}



#define NPROF1 10
#define USLEEP 10000 /* usleep parameter: 1000000 is 1 sec*/


/* called from 'handle_link'; reads one 'big' buffer from 'fd' */
/* returns number of bytes read or -1 if error (i.e. EOF) */

#undef DEBUG

int
LINK_sized_read(int fd, char **buf, hrtime_t tprof[NPROFMAX])
{
  int size;	/* size of incoming packet */
  int cc, jj, llenw, *tmp;
  int rembytes;	/* remaining bytes */
  int n_retries = 0;
  int n_retry2;
  unsigned int netlong;	/* network byte ordered length */
  char *bufferp = 0;
  unsigned int lwd, magic;
  unsigned int *bigbuf;
  int n_ending;

  static int nev;

  /* timing */
  hrtime_t start1, end1, start2, end2, start3, end3;


#ifdef DEBUG
  printf("[%2d] LINK_sized_read reached\n",fd);fflush(stdout);
  printf("[%2d] LINK_sized_read reached\n",fd);fflush(stdout);
  printf("[%2d] LINK_sized_read reached\n",fd);fflush(stdout);
#endif


  /* Wait for all the data requested */
  int recv_flags = MSG_WAITALL;

  n_ending = 0;

  /* read header off socket */
  rembytes = sizeof(netlong);
  bufferp = (char *) &netlong;

#ifdef DEBUG
  printf("[%2d] LINK_sized_read: at the beginning rembytes=%d\n",fd,rembytes);fflush(stdout);
#endif

start1 = start3 = gethrtime();


  while(rembytes)
  {

goto skiipp;
    {
      int nbytes, lbytes;

      nbytes = 65536;
      lbytes=4;
	  /*
      setsockopt(fd, SOL_SOCKET, SO_RCVBUF, 
                 (int *) &nbytes, lbytes); 
	  */
      getsockopt(fd, SOL_SOCKET, SO_RCVBUF, 
                 (int *) &nbytes, &lbytes); 
	  /*
      printf("[%2d] socket buffer size is %d(0x%08x) bytes\n",fd,nbytes,nbytes);
	  */
    }
skiipp:

#ifdef DEBUG
    printf("[%2d] RECV1: bufferp=0x%08x, rembytes=0x%08x, recv_flags=%d\n",
      fd, bufferp, rembytes, recv_flags);fflush(stdout);
    printf("[%2d] 0: rembytes=%d\n",fd,rembytes);
    printf("[%2d] processing 1\n",fd);
#endif
    cc = recv(fd, bufferp, rembytes, /*MSG_DONTWAIT*/recv_flags);
#ifdef DEBUG
    printf("[%2d] processing 2\n",fd);
    printf("[%2d] 1: %d %d\n",fd,rembytes,cc);
#endif
    if(cc == -1)
    {
      if(errno == EWOULDBLOCK)
      {

        if(ending_for_recv)
        {
          n_ending ++;
          printf("[%2d] ending_for_recv=%d n_ending=%d - wait for ROC\n",
            fd,ending_for_recv,n_ending);
          if(n_ending >= 10)
		  {
            printf("[%2d] ROC is not reporting -> force ending\n",fd);
            return(0);
		  }
        }

        /* retry */
		usleep(USLEEP);
        /* we do not know if ROC still alive and will send more data later,
        or ROC is dead ... */
		
        printf("[%2d] LINK_sized_read(): recv would block, retrying ...",fd);
        fflush(stdout);
		
      }
      else
      {
        printf("[%2d] LINK_sized_read() ERROR1\n",fd);
        fflush(stdout);
        if(errno != ECONNRESET)
        {
          perror("read ");
        }
        return(-1);
      }
    }
    else
    {
      /* It is OK for a socket to return with wrong number of bytes. */
      if(cc != rembytes)
      {
        if(cc > rembytes)
        {
          /* read() returned more bytes than requested!?!?!?! */
          /* this can't happen, but appears to anyway */
          printf("[%2d] ERROR: LINK_sized_read(,,%d) = read(,,%d) = %d!?!?!\n",
				 fd,size,rembytes,cc);
          printf("[%2d] ERROR: recv() returned more chars than requested - exit.\n",fd);
          fflush(stdout);
          exit(0);
        }
        else if(cc == 0) /* we are here if ROC closed connection ?! */
        {

          /* let other LINK threads know we are ending, in case if some ROC
          crashed and cannot send buffer with 'End' transition */
          ending_for_recv = 1;

          printf("[%2d] LINK_sized_read(): closed\n",fd);
          fflush(stdout);
          return(0);


        }
      }
#ifdef DEBUG
      printf("[%2d] LINK_sized_read(): recv(,,%d) returned %d\n",fd,rembytes,cc);
      fflush(stdout);
#endif
      /* Always adjust these to get out of the while. */
      bufferp += cc;
      rembytes -= cc;
    }

  } /* first 'recv' loop */


end1 = gethrtime();

  size = (int) ntohl(netlong); /* ntohl() return the argument value
                                  converted from network to host byte order */

  /* read data */
  if(size == 0)
  {
  	/* GHGHGH */
  	unsigned int *p;
  	printf("[%2d] LINK_sized_read(): WARNING: zero length block from ROC\n",fd);
    fflush(stdout);
#ifndef NOALLOC
  	*buf = (char *) calloc(24,1);
    if((*buf) == NULL)
    {
      printf("[%2d] LINK_sized_read(): ERROR1: calloc(%d) returns zero !!!\n",fd,size+6);
      fflush(stdout);
    }
#endif
    p = (unsigned int *) *buf;
    p[BBIWORDS]  = -1;
    p[BBIBUFNUM] = -1;
    p[BBIROCID]  = -1;
    p[BBIEVENTS] = -1; /* setting this to -1 makes handle_link ignore the buffer */
    p[BBHEAD]    = -1;
    p[BBHEAD+1]  = -1;

    return(24);
  }

#ifdef NOALLOC
  if(size > TOTAL_RECEIVE_BUF_SIZE)
  {
    printf("[%2d] LINK_sized_read(): ERROR2: buffer size=%d too big - exit.\n",fd,size+6);
    fflush(stdout);
    exit(0);
  }
#else
  /*printf("[%2d] size=0x%08x\n",fd,size);*/
  *buf = (char *) calloc(size+6,1); /* two bytes extra for null term strings */
  if((*buf) == NULL)
  {
    printf("[%2d] LINK_sized_read(): ERROR2: calloc(%d) returns zero !!!\n",fd,size+6);
    fflush(stdout);
  }
#endif

#ifdef DEBUG
  printf("[%2d] LINK_sized_read(): have %d bytes buffer at 0x%08x\n",fd,size+6,(int)(*buf));
  fflush(stdout);
#endif

  /* Sergey: CHANGE THIS IF BIGBUFS CHANGED IN roc_component.c */
  *((unsigned int *) *buf) = (size >> 2); /* put buffer size in 1st word */
  /*printf("[%2d] 12345: %d (%d 0x%08x)\n",fd,*((unsigned int *) *buf),size,*buf);fflush(stdout);*/
  bufferp = *buf/* + sizeof(size)*/;           /* set pointer to 2nd word */



  rembytes = size;
  n_retry2 = 0;

start2 = gethrtime();

  while(rembytes)
  {

retry1:

	/*
    printf("[%2d] RECV2: bufferp=0x%08x, rembytes=0x%08x, recv_flags=%d\n",
    fd, bufferp, rembytes, recv_flags);fflush(stdout);
	*/


    cc = recv(fd, bufferp, rembytes, recv_flags);



    /*printf("[%2d] 2: %d %d\n",fd,rembytes,cc);*/
    /*
    printf("[%2d] cc=0x%08x\n",fd,cc);fflush(stdout);
    */
    if(cc == -1)
    {
      if(errno == EWOULDBLOCK) goto retry1;
      if(errno != ECONNRESET) perror("read2");
      puts("Error 2.");
      printf("[%2d] LINK_sized_read(): cc = %d, Errno is %d.\n",fd, cc, errno);
      fflush(stdout);
      return(-1);
    }

    if(cc == 0)
    { /* EOF - process died */
      /* GHGHGH */
      printf("[%2d] process died (cc==0) - return\n",fd);
      fflush(stdout);
      return(0);
    }
    else if(cc > rembytes)
    {
      /* read() returned more bytes than requested!?!?!?! */
      /* this can't happen, but appears to anyway */

      printf("[%2d] LINK_sized_read(): returned more bytes than requested!?!?!?!\n",fd);
      printf("[%2d] LINK_sized_read(,,%d) = read(,,%d) = %d!?!?!\n",fd,
              size,rembytes,cc);
      printf("[%2d] LINK_sized_read(): recv() returned more chars than requested - exit.\n",fd);
      fflush(stdout);
      exit(0);
    }
    else if(cc != rembytes)
    {
      /* (cc > 0) && (cc < rembytes) */
      printf("[%2d] LINK_sized_read(): cc=%d != rembytes=%d -> retry ...\n",fd,cc,rembytes);
      fflush(stdout);
      n_retry2++;
    }
#ifdef DEBUG
    printf("[%2d] LINK_sized_read(): recv(,,%d) returned %d\n",fd,rembytes,cc);
    fflush(stdout);
#endif
    /* Always adjust these to get out of the while loop */
    bufferp += cc;
    rembytes -= cc;

  } /*while(rembytes)*/



  /* we received buffer, lets swap it if necessary */
  bigbuf = (unsigned int *) *buf;




  /* chack buffer integrity */
  /*
  bb_check(bigbuf);
  */

  /*
  printf("[%2d] RECV3: %d %d %d %d 0x%08x %d - 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x (%d)\n",fd,
  bigbuf[0],bigbuf[1],bigbuf[2],bigbuf[3],bigbuf[4],bigbuf[5],
  bigbuf[6],bigbuf[7],bigbuf[8],bigbuf[9],bigbuf[10],bigbuf[11],size);
  */
  magic = bigbuf[BBIFD];
  if(magic == 0x01020304)
  {
    llenw = size >> 2;
	/*
    if(llenw>200000)
    {
      printf("[%2d] WARN: llenw=%d, size=%d\n",fd,llenw,size);
      printf("[%2d] RECV4: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x - 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x (%d)\n",fd,
      bigbuf[0],bigbuf[1],bigbuf[2],bigbuf[3],bigbuf[4],bigbuf[5],
      bigbuf[6],bigbuf[7],bigbuf[8],bigbuf[9],bigbuf[10],bigbuf[11],size);
	}
	*/
#ifdef DEBUG
    printf("[%2d] SWAP (0x%08x), llenw=%d\n",fd,magic,llenw);
#endif
    bufferSwap(bigbuf,llenw);
  }
#ifdef DEBUG
  else
  {
    printf("[%2d] DO NOT SWAP (0x%08x)\n",fd,magic);
#ifdef DEBUG
  printf("\n[%2d] buffer header: length=%d words, buffer#=%d, rocid=%d, #events=%d, fd/magic=0x%08x, end=%d\n",fd,
		 bigbuf[BBIWORDS],bigbuf[BBIBUFNUM],bigbuf[BBIROCID],bigbuf[BBIEVENTS],bigbuf[BBIFD],bigbuf[BBIEND]);
#endif
  }
#endif







  /* to simulate delay 
  if(!(nev++%10)) {printf("[%2d] sleep\n",fd);sleep(1);}
  */

  tmp = (int *)(*buf);




  end2 = end3 = gethrtime();
  /*printf("[%2d] 67890: %d\n",fd,tmp[BBIWORDS]);fflush(stdout);*/

  tprof[3] += (end1-start1)/NANOMICRO;
  tprof[4] += (end2-start2)/NANOMICRO;
  tprof[5] += (end3-start3)/NANOMICRO;

  tprof[1] += ((hrtime_t)tmp[BBIEVENTS]); /* the number of events */
  tprof[2] += ((hrtime_t)tmp[BBIWORDS]); /* the number of bytes */
  if(++tprof[0] == NPROF1)
  {
    /*
    printf("[%2d] 1: average buf: %4lld events, %6lld words,",fd,
      tprof[1]/tprof[0],tprof[2]/tprof[0]);
    if(tprof[1] > 0)
    {
      printf("   recv1=%5lld recv2=%5lld tot=%5lld\n",
        tprof[3]/tprof[1],tprof[4]/tprof[1],tprof[5]/tprof[1]);
    }
    else
    {
      printf("\n");
    }
    */
    tprof[0] = 0;
    tprof[1] = 0;
    tprof[2] = 0;
    tprof[3] = 0;
    tprof[4] = 0;
    tprof[5] = 0;
  }




  /*
  if(time3 > 3000000)
  {
    printf("[%2d] %7lld %7lld %7lld microsec (buf %d), rocid=%2d\n",fd,
      time1,time2,time3,tmp[1],tmp[2]);
  }
  */

  /* set appropriate bit letting building thread know we are ready */
#ifdef USE_128
  SetBit128(&roc_linked, tmp[2]);
#else
  roc_linked |= (1<<tmp[2]); /* tmp[2] contains rocid */
#endif

  /*
  printf("[%2d] LINK_sized_read(): set roc_linked for rocid=%d (0x%08x)\n",fd,
    tmp[2],roc_linked);
  fflush(stdout);
  */

  return(size);
}



/* main thread function; it calls 'put_cb_data()' to place data
   on the circular buffer for the building thread */

void *
handle_link(DATA_LINK theLink)
{
  int fd;
  int numRead;
  int headerSize;
  char *errMsg;
  unsigned int *buf_long_p;
  int count, i, itmp;
  char *buf;
  char ipaddress[20];

  hrtime_t tprof[NPROFMAX];
  hrtime_t start1, end1, time1=0;
  hrtime_t start2, end2, time2=0;
  hrtime_t start4, end4, time4=0;
  hrtime_t nevtime1=0, nevchun=0, avgsize=0;

  struct sockaddr_in from;
  int len;
  char *address;

#ifdef NOALLOC
  unsigned int *bufptr[QSIZE];

  printf("cleanup pool of buffers for roc=%d (rocid=%d) ..\n",
    theLink->roc_queue->roc,theLink->roc_queue->rocid);
  fflush(stdout);
  for(i=0; i<QSIZE; i++)
  {
    bufptr[i] = bufpool[theLink->roc_queue->roc][i];
    bufptr[i][0] = 0; /* mark buffer as free */
  }
  printf("handle_link .. done.\n");
  fflush(stdout);
#endif


  /* accept socket connection (listen() must be called already) */

acceptagain:


  bzero((char *)&from, sizeof(from));
  len = sizeof (from);

  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("Wait on 'accept(%d,0x%08x,%d)' ..\n",theLink->sock,&from,len);
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

  {
    FILE *fd;
    char fname[80];
    sprintf(fname,"/home/clasrun/ccscans/good_waiting_%s",theLink->name);
    fd = fopen(fname,"w");
	if(fd > 0)
	{
      chmod(fname,777);
      fprintf(fd,"waiting on accept\n");
      fclose(fd);
	}
  }

  fflush(stdout);

  /* NOTE: the original socket theLink->sock remains open for accepting further connections */
  while((theLink->fd = accept(theLink->sock, (struct sockaddr *)&from, &len)) == -1)
  {
    printf("accept: wait for connection\n");
    usleep(USLEEP);
  }



  /* sergey: accept only from specified address (trying to block CC scans and other junk) */
  address = inet_ntoa(from.sin_addr);
  printf("accept() returns Ok, we expected it from >%s<, received from>%s<\n",theLink->name,address);
  if(dacgethostbyname(theLink->name, ipaddress) != 0)
  {
    printf("ERROR in dacgethostbyname() !!!!!\n");
  }
  else if(strcmp(address,ipaddress))
  {
    printf("ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >%s<, goto accept() again\n",address);
    printf("ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >%s<, goto accept() again\n",address);
    printf("ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >%s<, goto accept() again\n",address);
    fflush(stdout);

	{
      FILE *fd;
      char fname[80];
      sprintf(fname,"/home/clasrun/ccscans/bad_%s",theLink->name);
      fd = fopen(fname,"w");
	  if(fd > 0)
	  {
        chmod(fname,777);
        fprintf(fd,"UNAUTORIZED ACCESS FROM >%s<\n",address);
        fclose(fd);
	  }
	}

    close(theLink->fd);
    goto acceptagain;
  }

  {
    FILE *fd;
    char fname[80];
    sprintf(fname,"/home/clasrun/ccscans/good_%s",theLink->name);
    fd = fopen(fname,"w");
	if(fd > 0)
    {
      chmod(fname,0777);
      fprintf(fd,"accepted link from >%s<\n",address);
      fclose(fd);
    }
  }

  fd = theLink->fd;
  printf("connection accepted, fd=%d\n",fd);




  /*************/
  /* main loop */
  while(1)
  {

    start1 = gethrtime();

#ifdef NOALLOC

    /* get free buffer from pool; will wait if nothing is available */
    buf = NULL;
    while(buf == NULL)
    {
#ifdef DO_NOT_PUT
      bufptr[0][0] = 0;
#endif
      for(i=0; i<QSIZE; i++)
      {
        if(bufptr[i][0] == 0) /* means 'free buffer'; it is marked as 'free' in cb_events_get() */
        {
          buf = (char *) bufptr[i];
#ifdef DEBUG
          printf("[%2d] handle_link(): rocid=%d: got free buffer %d at 0x%08x\n",fd,theLink->roc_queue->rocid,i,buf);
          fflush(stdout);
#endif
          break;
        }
      }
      if(buf == NULL)
      {
/*prints all the time
        printf("[%2d] handle_link(): rocid=%d: all buffers are full - wait ..\n",fd,theLink->roc_queue->rocid);
*/
        fflush(stdout);
        usleep(USLEEP);/*sleep(1);*/


		/* wait here instead of sleep ??????????? */


      }
	}

#endif /*#ifdef NOALLOC*/




    /* reading data from roc */
    start4 = gethrtime();
    /*printf("901 %d\n",fd);fflush(stdout);*/
    numRead = LINK_sized_read(fd, &buf, tprof);
    /*printf("902 %d\n",fd);fflush(stdout);*/
    end4 = gethrtime();

#ifdef DEBUG
    printf("[%2d] handle_link(): got %d bytes\n",fd,numRead); fflush(stdout);
    fflush(stdout);
#endif


    /* if 'LINK_sized_read' returned <=0, we exiting */
    if(numRead <= 0)
    {
      printf("[%2d] handle_link(): LINK_sized_read() returns %d\n",fd,numRead);fflush(stdout);
      printf("[%2d] handle_link(): put_cb_data calling ...\n",fd);fflush(stdout);
	  usleep(USLEEP);

      /* sets 'cbp->nevents[icb] = -1;' inside, need it in cb_events_get() call from coda_eb.c */
      put_cb_data(fd, &theLink->roc_queue, (void *) -1);

      printf("[%2d] ===================================\n",fd);fflush(stdout);
      printf("[%2d] ===================================\n",fd);fflush(stdout);
      printf("[%2d] handle_link(): numRead=%d<=0, calling 'put_cb_data(,,-1)', first breaking while() loop\n",fd);fflush(stdout);
      printf("[%2d] ===================================\n",fd);fflush(stdout);
      printf("[%2d] ===================================\n",fd);fflush(stdout);
      break; /* this is the first exit from while(1) loop; will call 'pthread_exit(0)' and return */
    }

    /* count total amount of data words */
    *dataSent += (numRead>>2);

    buf_long_p = (unsigned int *) buf;
#ifdef DEBUG
    printf("[%2d] buffer from >%s< (%08x %08x %08x %08x %08x %08x %08x %08x)\n",fd,
           theLink->name,
           buf_long_p[0],buf_long_p[1],buf_long_p[2],buf_long_p[3],
           buf_long_p[4],buf_long_p[5],buf_long_p[6],buf_long_p[7]);
#endif
    if(buf_long_p[BBIWORDS] == BBHEAD)
    {
      printf("[%2d] handle_link(): WARNING got empty buffer from ROC !\n",fd);
      fflush(stdout);
#ifndef NOALLOC
      free(buf);
#endif
      continue;
    }

    /* Check for test_link data */
    if(buf_long_p[BBIEVENTS] < 0)
    {
      printf("[%2d] WARNING - handle_link discarding buffer. count = %d.\n",fd,
        buf_long_p[BBIEVENTS]);fflush(stdout);
#ifndef NOALLOC
      free(buf);
#endif
      continue;
    }

    /* put buffer to the circular buffer manager */
    start2 = gethrtime();

#ifndef DO_NOT_PUT

#ifdef DEBUG
    {
      unsigned int *bigbuf;
      bigbuf = (unsigned int *) buf;
      printf("[%2d] PUTV3: %d %d %d %d %d %d - 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",fd,
        bigbuf[0],bigbuf[1],bigbuf[2],bigbuf[3],bigbuf[4],bigbuf[5],
        bigbuf[6],bigbuf[7],bigbuf[8],bigbuf[9],bigbuf[10],bigbuf[11]);
    }
#endif
    if(put_cb_data(fd, &theLink->roc_queue, (void *) buf) < 0)
    {
      printf("[%2d] ----------------------------------------------\n",fd);
      printf("[%2d] ----------------------------------------------\n",fd);
      printf("[%2d] handle_link(): put_cb_data returns < 0, second breaking while() loop\n",fd);
      printf("[%2d] ----------------------------------------------\n",fd);
      printf("[%2d] ----------------------------------------------\n",fd);
      fflush(stdout);
      break; /* this is the second exit from while(1) loop; will call 'pthread_exit(0)' and return */
    }
#endif /*#ifndef DO_NOT_PUT*/


    end2 = gethrtime();
    end1 = gethrtime();
    time1 += ((end1-start1)/NANOMICRO);
    time2 += ((end2-start2)/NANOMICRO);
    time4 += ((end4-start4)/NANOMICRO);
    nevchun += ((hrtime_t)buf_long_p[3]); /* the number of events */
    avgsize += ((hrtime_t)buf_long_p[0]);
    /*printf("--- %ld %ld %ld %ld\n",buf_long_p[0],buf_long_p[1],buf_long_p[2],buf_long_p[3]);*/
    if(++nevtime1 == NPROF1)
    {
      /*
      printf("2: average buf: %4lld events, %6lld words,",
        nevchun/nevtime1,avgsize/nevtime1);
      if(nevchun > 0)
      {
        printf("  tot=%5lld recv=%5lld put=%5lld\n",
          time1/nevchun,time4/nevchun,time2/nevchun);
      }
      else
      {
        printf("\n");
      }
      */
      nevtime1 = 0;
      time1 = 0;
      time2 = 0;
      time4 = 0;
      nevchun = 0;
      avgsize = 0;
    }


    /*printf("Checking for exit command theLink->exit=%d\n",theLink->exit);*/
    /* received command to exit */
    if(theLink->exit == 1)
	{
      printf("[%2d] handle_link(): got exit command inside reading loop !!! breaking ..\n",fd); fflush(stdout);
      break;
	}


  }
  /* end of main loop while(1) */
  /*****************************/

  while(theLink->exit != 1)
  {
    printf("[%2d][%s] CHECKING FOR theLink->exit TO BECOME 1, currently it is %d\n",fd,theLink->name,theLink->exit); fflush(stdout);
    sleep(1);
  }

  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);
  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);
  printf("[%2d][%s] handle_link(): got exit command\n",fd,theLink->name); fflush(stdout);
  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);
  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);

  /*printf("handle_link(): thread exit for %9.9s\n",theLink->name); fflush(stdout);*/
  /*NOTE: segm fault printing 'theLink->name', probably pointer is not good any more ??? free'ed in debCloseLink ? */
  printf("[%2d] handle_link(): thread exiting\n",fd); fflush(stdout);


  printf("[%2d] 907\n",fd); fflush(stdout);
  theLink->exit = -1;
  pthread_exit(0);
}





  /*               croctest1    EB5        clon10-daq1
DATA_LINK           argv[1]   argv[2]       argv[3]
debOpenLink(char *fromname, char *toname, char *tohost)
  */

DATA_LINK
debOpenLink(char *fromname, char *toname, char *tohost,  MYSQL *dbsock)
{
  DATA_LINK theLink;
  int i, len, itmp, res, numRows;
  char host[100], hostmp[100], type[100], state[100], chport[100];
  char inhost[100];
  char name[100];
  char tmp[256], tmpp[256], *ch;
  int port = 0;

  MYSQL_RES *result;

  struct hostent *hp, *gethostbyname();
  struct sockaddr_in sin, from;
  int s, slen;

printf("++++++1+ 0x%08x 0x%08x 0x%08x\n",roc_queues[0],roc_queues[1],roc_queues[2]);



  /*******************************************************/
  /* allocate memory for structure 'theLink' and fill it */
  /* will be free'd in debCloseLink()                    */
  /*******************************************************/

  theLink = (DATA_LINK) calloc(sizeof(DATA_LINK_S),1);

  theLink->name = (char *) calloc(strlen(fromname)+1,1);
  strcpy(theLink->name, fromname); /* croctest1 etc */

  /* ROCs will use DB host name to send data to */
  /* 'inhost' will be set to 'links' table to let ROCs know where to send data */
  strncpy(inhost,tohost,99);
  printf("debOpenLink: set inhost to >%s<\n",inhost);
  strncpy(host,inhost,99);
  strncpy(theLink->host,host,99);


  /* construct database table row name */
  strncpy(name,fromname,98);
  len = strlen(name);
  strcpy((char *)&name[len],"->");
  strncpy((char *)&name[len+2],toname,(100-(len+2)));
  theLink->linkname = strdup(name); /* croctest1->EB5 etc */
  printf("debOpenLink: theLink->linkname is >%s<\n",theLink->linkname);

  /* set connection type to TCP */
  strcpy(type,"TCP");

printf("++++++2+ 0x%08x 0x%08x 0x%08x\n",roc_queues[0],roc_queues[1],roc_queues[2]);
  /* ... */

  /* */
  bzero((char *)&sin, sizeof(sin));
  hp = gethostbyname(host);
  if(hp == 0 && (sin.sin_addr.s_addr = inet_addr(host)) == -1)
  {
	printf("debOpenLink: unkown host >%s<\n",host);
	return(NULL);
  }
  if(hp != 0) bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;

  /* create a socket */
  s = socket(AF_INET, SOCK_STREAM, 0); /* tcl: PF_INET !!?? */
  if(s < 0)
  {
    printf("debOpenLink: cannot open socket\n");
    return(NULL);
  }
  else
  {
    theLink->sock = s;
    printf("debOpenLink: listening socket # %d\n",theLink->sock);
  }

  /* if want, set socket options here, but better do not */

  /* bind and listen for server only (EB) */
  if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    printf("debOpenLink: bind failed: host %s port %d\n",
      inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
    close(s);
	return(NULL);
  }

  if(listen(s, 5) < 0)
  {
    printf("debOpenLink: listen failed\n");
    close(s);
	return(NULL);
  }


  /* get the port number */
  len = sizeof(sin);
  if(getsockname (s, (struct sockaddr *) &sin, &len) < 0)
  {
    printf("debOpenLink: getsockname failed\n");
    close(s);
	return(NULL);
  }

  port = ntohs(sin.sin_port);
  printf("debOpenLink: socket is listening: host %s port %d\n",
      inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

printf("++++++4+ 0x%08x 0x%08x 0x%08x\n",roc_queues[0],roc_queues[1],roc_queues[2]);


  /* create 'links' table if it does not exist (database must be opened in calling function) */
  sprintf(tmpp,"SELECT * FROM links");
  if(mysql_query(dbsock, tmpp) != 0)
  {
    /*need to check it !!!*/
    printf("No 'links' table -> we will create it (%s)\n",mysql_error(dbsock));
    sprintf(tmp,"create table links (name char(100) not null, type char(4) not null,host char(30),state char(10),port int)");
    if(mysql_query(dbsock, tmp) != 0)
    {
	  printf("ERROR: cannot create table 'links' (%s)\n",mysql_error(dbsock));
      return(NULL);
    }
    else
    {
      printf("table 'links' created\n");
    }
  }
  else
  {
    MYSQL_RES *res;
    printf("Table 'links' exist\n");

    /*store and free results, otherwise mysql gives error on following mysql_query ..*/
    if(!(res = mysql_store_result (dbsock) ))
    {
      printf("ERROR in mysql_store_result (%s)\n",mysql_error(dbsock));
      return(NULL);
    }
    else
    {
      mysql_free_result(res);
    }
  }


  /* trying to select our link from 'links' table */
  sprintf(tmp,"SELECT name FROM links WHERE name='%s'",name);
  if(mysql_query(dbsock, tmp) != 0)
  {
	printf("debOpenLink: mysql error (%s)\n",mysql_error(dbsock));
    return(NULL);
  }

  /* gets results from previous query */
  /* we assume that numRows=0 if our link does not exist,
     or numRows=1 if it does exist */
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result (%)\n",mysql_error(dbsock));
    return(NULL);
  }
  else
  {
    numRows = mysql_num_rows(result);
    mysql_free_result(result);

    printf("nrow=%d\n",numRows);
    /* insert/update with state='down' */
    if(numRows == 0)
    {
      sprintf(tmp,"INSERT INTO links (name,type,host,port,state) VALUES ('%s','%s','%s',%d,'down')",name,type,host,port);
      printf("!!=1=> >%s<\n",tmp);
    }
    else if(numRows == 1)
    {
      sprintf(tmp,"UPDATE links SET host='%s',type='%s', port=%d, state='down' WHERE name='%s'",host,type,port,name);
      printf("!!=2=> >%s<\n",tmp);
    }
    else
    {
      printf("debOpenLink: ERROR: unknown nrow=%d",numRows);
      return(NULL);
    }

    if(mysql_query(dbsock, tmp) != 0)
    {
	  printf("debOpenLink: ERROR 20-2\n");
      return(NULL);
    }
    else
    {
      printf("Query >%s< succeeded\n",tmp);
    }
  }

  /* set state 'waiting' (MAYBE THIS MUST BE LAST ACTION, AFTER SETTING port etc ???!!!) */
  sprintf(tmp,"UPDATE links SET state='waiting' WHERE name='%s'",name);
  if(mysql_query(dbsock, tmp) != 0)
  {
	printf("debOpenLink: ERROR 22-2\n");
    return(NULL);
  }

  /* database must be closed in calling function */

  /* ================ end of database update =================== */

printf("++++++5+ 0x%08x 0x%08x 0x%08x\n",roc_queues[0],roc_queues[1],roc_queues[2]);

/* cleanup .. */
ending_for_recv = 0;


  theLink->roc_queue = roc_queues[roc_queue_ix++];
  /*theLink->roc_queue->parent = theLink->name;donotneedit???*/
  printf("theLink->name=%s\n",theLink->name);
  theLink->exit = 0;
  printf("LINK_thread_init(): creating thread ..\n"); fflush(stdout);

printf("++++++6+ 0x%08x 0x%08x 0x%08x\n",roc_queues[0],roc_queues[1],roc_queues[2]);

  {
    /*Sergey: better be detached !!??*/
    pthread_attr_t detached_attr;

    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED); /* default is PTHREAD_CREATE_JOINABLE, can use pthread_join() but
                                                                             it seems stuck if thread dies */
    pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);


    /**********************************************************/
    /* start thread which will handle input data from the roc */

    if(pthread_create( &theLink->thread, &detached_attr,
                 (void *(*)(void *)) handle_link, (void *) theLink) != 0)
    {
      printf("LINK_thread_init(): ERROR in thread creating\n"); fflush(stdout);
      perror("pthread_create: ");
      return(NULL);
    }
    printf("LINK_thread_init(): thread is created\n"); fflush(stdout);
  }

  return(theLink);
}



int
debCloseLink(DATA_LINK theLink, MYSQL *dbsock)
{
  void *status;
  char tmp[1000];
  int exittimeout = 5;

  /* */
  if(theLink == NULL)
  {
    printf("debCloseLink: theLink=NULL -> return\n");
    return(CODA_OK);
  }
  printf("debCloseLink: theLink=0x%08x -> closing\n",theLink);

  theLink->exit = 1; /* tells thread to exit */
  /* give it a time to exit */
  while((theLink->exit!=-1) && (exittimeout>0))
  {
    printf("debCloseLink: waiting for link thread to exit, exittimeout=%d\n",exittimeout);
    sleep(1);
    exittimeout --;
  }

  printf("debCloseLink reached, fd=%d sock=%d exit=%d (exittimeout=%d)\n",
		   theLink->fd,theLink->sock,theLink->exit,exittimeout);
  fflush(stdout);

  /* shutdown socket connection */
  printf("11: shutdown fd=%d\n",theLink->fd);fflush(stdout);
  if(shutdown(theLink->fd, SHUT_RDWR)==0) /*SHUT_RD,SHUT_WR,SHUT_RDWR*/
  {
	printf("12\n");fflush(stdout);
    printf("debCloseLink: socket fd=%d sock=%d connection closed\n",theLink->fd,theLink->sock);

    printf("903 %d\n",theLink->fd); fflush(stdout);
    close(theLink->fd);
    printf("904 %d\n",theLink->fd); fflush(stdout);
    close(theLink->sock);
    printf("905 %d\n",theLink->fd); fflush(stdout);
  }
  else
  {
	printf("13\n");fflush(stdout);
    printf("debCloseLink: ERROR in socket fd=%d sock=%d connection closing\n",
          theLink->fd,theLink->sock);
    exit(0);
  }
  printf("906\n"); fflush(stdout);

  /* shut down a connection by telling the other end to shutdown (database must be opened in calling function) */
  /* set state 'down' */
  sprintf(tmp,"UPDATE links SET state='down' WHERE name='%s'",theLink->linkname);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("debCloseLink: ERROR in database query {UPDATE ..}\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("debCloseLink: link is down\n");
  }

  /* database must be closed in calling function */

  /* ================ end of database update =================== */



  /* cancel thread if still exists */
  if(theLink->exit!=-1)
  {
    pthread_t thread = theLink->thread;
    printf("debCloseLink: canceling thread .\n");
    pthread_cancel(thread);
    printf("debCloseLink: canceling thread ..\n");
    /*pthread_join(thread,&status); stuck here if one of the ROCs crashed */
    printf("debCloseLink: canceling thread !\n");
  }

  theLink->thread = 0;
  theLink->exit = 0;

  /* SHOULD DO FOLLOWING ONLY IF handle_thread() IS DONE !!! (AND PREVIOUS AS WELL ??) */

  /* release memory */
  printf("debCloseLink: free memory\n");
  cfree((char *) theLink->name);
  /*cfree((char *) theLink->parent);donotneedit???*/


  /* sergey: probably error, after following call 'put_cb_data(fd, &theLink->roc_queue, (void *) -1)'
     from 'handle_link()' will fail since  'theLink' does not exist any more; this is probably why there
     is check 'if(cbp <(CIRCBUF *)100000)' inside put_cb_data ... */
  /* probably 'handle_link()' must set some flag when done, and we'll wait for that flag here ... */
  cfree((char *) theLink);
  printf("debCloseLink: done.\n");
  
  return(CODA_OK);
}
