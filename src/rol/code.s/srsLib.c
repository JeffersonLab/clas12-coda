/*----------------------------------------------------------------------------*/
/**
 * @mainpage
 * <pre>
 *  Copyright (c) 2015        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Authors: Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Library for readout of SRS
 *
 * </pre>
 *----------------------------------------------------------------------------*/
#define _GNU_SOURCE

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>
#include <errno.h>  
#include <sys/stat.h>  
#include <sys/un.h>  
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>        /* timespec{} for pselect() */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <netdb.h>
#include <sys/stat.h>    /* for S_xxx file mode constants */
#include <sys/uio.h>     /* for iovec{} and readv/writev */
#include <sys/un.h>      /* for Unix domain sockets */
#include <ctype.h>
#include <byteswap.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include "srsLib.h"

#define SOCKET_TIMEOUT 700000 //was 500000

static int srsDebugMode=0; 
static int srsSockFD[MAX_FEC];
int nsrsSockFD=0;
unsigned long long before, after, diff=0;
int srsFrameNumber[MAX_FEC];
int srsID[MAX_FEC];

static int srsSlowControl(char *ip, int port, 
			  unsigned int *req_fields, int nfields,
			  unsigned int *ret_data);
static int srsParseResponse(unsigned int *ibuf, unsigned int *obuf, int nwords, int *nerr);
static int srsPrepareSocket(char *ip_addr,int port_number);
static int srsSendData();
static int srsReceiveData(unsigned int *buffer, int nwords);
static int srsCloseSocket();
static int srsReadFile(char *path, char *ip, int *port, unsigned int *obuffer);
static unsigned long long int rdtsc(void);

/* Mutex to guard SRS read/writes */
pthread_mutex_t     srsMutex = PTHREAD_MUTEX_INITIALIZER;
#define SRSLOCK     if(pthread_mutex_lock(&srsMutex)<0) perror("pthread_mutex_lock");
#define SRSUNLOCK   if(pthread_mutex_unlock(&srsMutex)<0) perror("pthread_mutex_unlock");

int
srsConnect(int *sockfd, char *ip, int port) /* formally createAndBinSocket */
{
  struct sockaddr_in my_addr; 
  struct timeval socket_timeout={0,SOCKET_TIMEOUT}; /* 900 microsecond time out */
  int stat=0;

  *sockfd = socket(AF_INET, SOCK_DGRAM,0);
  if (*sockfd==-1)    
    {
      perror("socket");
      return -1;
    }
  else
    {
      if(srsDebugMode)
	printf("Server : Socket() successful\n");
    }

  stat = setsockopt(*sockfd,
		    SOL_SOCKET,
		    SO_RCVTIMEO,
		    &socket_timeout,
		    sizeof(socket_timeout));

  if(stat!=0)
    {
      perror("setsockopt");
      printf("%s: errno = %d\n",__FUNCTION__,errno);
    }

  int enable = 1;

  if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("SO_REUSEADDR failed");

#ifdef SO_REUSEPORT
  if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
    perror("SO_REUSEPORT failed");
#endif

  memset(&my_addr, 0, sizeof(my_addr));

  my_addr.sin_family      = AF_INET;
  my_addr.sin_port        = htons(port);
  /* my_addr.sin_addr.s_addr = htonl(INADDR_ANY); */
  inet_aton(ip, (struct in_addr *)&my_addr.sin_addr.s_addr);

  stat = bind(*sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr));

  if (stat==-1) 
    {
      perror("bind");
      close(*sockfd);
      return -1;
    }
  else
    {
      if(srsDebugMode)
	printf("Server : bind() successful\n");
    }

  srsSockFD[nsrsSockFD++] = *sockfd;

  return 0;
}

int
srsGetID(int sockfd)
{
  int ifec;
  for(ifec=0; ifec<nsrsSockFD; ifec++)
    if(sockfd==srsSockFD[ifec])
      return ifec;

  return -1;
}


int
srsStatus(char *ip, int pflag)
{
  const int nsys = 0x10; 
  const int napvapp = 0x10; 
  const int napv = 0x1E; 
  const int npll = 0x4; 
  const int nadccard = 0x7; 
  unsigned int sys[nsys];
  unsigned int apvapp[napvapp];
  unsigned int apv[napv];
  unsigned int pll[npll];
  unsigned int adccard[nadccard];

  memset(sys,0,sizeof(sys));
  memset(apvapp,0,sizeof(apvapp));
  memset(apv,0,sizeof(apv));
  memset(pll,0,sizeof(pll));
  memset(adccard,0,sizeof(adccard));

  if(srsReadBurst(ip, SRS_SYS_PORT, 0, 0, nsys, sys, nsys)<0)
    {
      printf("%s: ERROR reading SYS port\n",__FUNCTION__);
      return -1;
    }
  if(srsReadBurst(ip, SRS_APVAPP_PORT, 0, 0, napvapp, apvapp, napvapp)<0)
    {
      printf("%s: ERROR reading APVAPP port\n",__FUNCTION__);
      return -1;
    }

#ifdef SKIPBROKEN
  if(srsReadBurst(ip, SRS_APV_PORT, SRS_APV_ALLAPV_MASK, 0, napv, apv, napv)<0)
    {
      printf("%s: ERROR reading APV port\n",__FUNCTION__);
      return -1;
    }
  if(srsReadBurst(ip, SRS_APV_PORT, SRS_APV_ALLPLL_MASK, 0, npll, pll, npll)<0)
    {
      printf("%s: ERROR reading APV-PLL port\n",__FUNCTION__);
      return -1;
    }
  if(srsReadBurst(ip, SRS_ADCCARD_PORT, 0, 0, nadccard, adccard, nadccard)<0)
    {
      printf("%s: ERROR reading ADCCARD port\n",__FUNCTION__);
      return -1;
    }
#endif

  printf("\n");
  printf("STATUS for SRS device at %s \n",ip);
  printf("--------------------------------------------------------------------------------\n");
  printf("                              System Summary\n\n");

  printf(" Firmware Version      0x%04x\n",sys[0]&0xffff);

  printf(" FEC IP                %d.%d.%d.%d\n",
	 (sys[3]&0xff000000)>>24, (sys[3]&0xff0000)>>16, 
	 (sys[3]&0xff00)>>8, (sys[3]&0xff)>>0);
  printf(" DAQ IP:PORT           %d.%d.%d.%d:%d\n",
	 (sys[0xa]&0xff000000)>>24, (sys[0xa]&0xff0000)>>16, 
	 (sys[0xa]&0xff00)>>8, (sys[0xa]&0xff)>>0,
	 sys[4]&0xffff);
  printf(" Clock Selection       0x%02x\n",(sys[0xc]&0xff));
  printf("   Status              0x%08x\n",sys[0xd]);
  int ethclk_sel = (sys[0xc]&0x80)>>7;
  int dtcclk_inh = (sys[0xc]&0x1);
  int ethclk_locked = (sys[0xd]&0x2)>>1;
  int dtcclk_locked = (sys[0xd]&0x1);
  printf("   Main Clock Source   ");
  if((ethclk_sel==0) && (dtcclk_inh==0) && (dtcclk_locked==0))
    printf("LOCAL");
  else if((ethclk_sel==0) && (dtcclk_inh==0) && (dtcclk_locked==1))
    printf("DTC");
  else if((ethclk_sel==0) && (dtcclk_inh==1))
    printf("LOCAL");
  else if((ethclk_sel==1) && (ethclk_locked==0))
    printf("LOCAL");
  else if((ethclk_sel==1) && (ethclk_locked==1))
    printf("ETH");
  else
    printf("UNKNOWN");
  printf("\n");
  printf(" Hardware FW Version   0x%04x\n",sys[0xf]&0xffff);

  printf("\n");
  printf(" DTC Config\n");
  printf("   Data over ETH       %s\t",(sys[0xb]&(1<<0))?"Enabled":"Disabled");
  printf("   Flow Control        %s\n",(sys[0xb]&(1<<1))?"Disabled":"Enabled");
  printf("   Padding Type        %d\t",((sys[0xb]&0x300)>>8));
  printf("   Data Channel TrgID  %s\n",(sys[0xb]&(1<<10))?"Enabled":"Disabled");
  printf("   Data Frame TrgID    %s\t",(sys[0xb]&(1<<11))?"Enabled":"Disabled");
  printf("   Trailer word count  %d\n",(sys[0xb]&0xf000)>>12);
  printf("   Padding Byte        0x%02x\t",(sys[0xb]&0xff0000)>>16);
  printf("   Trailer Byte        0x%02x\n",(sys[0xb]&0xff000000)>>24);
  
  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("                         APV Application Summary\n\n");
  printf(" APV Trigger Control\n");
  printf("   Mode                %d\t",apvapp[0]&0xff);
  printf("   TrgBurst            %d\n",apvapp[1]&0xff);
  printf("   Trigger Seq Period  %d\t",apvapp[2]&0xffff);
  printf("   Trigger Delay       %d\n",apvapp[3]&0xffff);
  printf("   Test Pulse Delay    %d\t",apvapp[4]&0xffff);
  printf("   RO Sync             %d\n",apvapp[5]&0xffff);
  printf("   Status              0x%06x\n",apvapp[7]&0xffffff);
  printf("\n");
  
  printf(" Event Build\n");
  printf("   Ch Enable Mask      0x%04x\t",apvapp[8]&0xffff);
  printf("   Capture Window      %d\n",apvapp[9]&0xffff);
  printf("   Mode                %d\t",apvapp[0xa]&0xff);
  printf("   Data Format         %d\n",apvapp[0xb]&0xff);
  printf("   Event Info Data     0x%04x\n",apvapp[0xc]&0xffffffff);
  printf("\n");
  printf(" Readout is %s (0x%x)\n",(apvapp[0xf]&0x1)?"Enabled":"Disabled", apvapp[0xf]);

#ifdef SKIPBROKEN
  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("                             APV Summary (ALL APVs)\n\n");
  printf(" Error Status          0x%02x\t",apv[0]&0xff);
  printf(" Mode                  0x%02x\n",apv[1]&0xff);
  printf(" Latency               0x%02x\t",apv[2]&0xff);
  printf(" Mux Gain              0x%02x\n",apv[3]&0xff);
  printf(" IPRE                  0x%02x\t",apv[0x10]&0xff);
  printf(" IPCASC                0x%02x\n",apv[0x11]&0xff);
  printf(" IPSF                  0x%02x\t",apv[0x12]&0xff);
  printf(" ISHA                  0x%02x\n",apv[0x13]&0xff);
  printf(" ISSF                  0x%02x\t",apv[0x14]&0xff);
  printf(" IPSP                  0x%02x\n",apv[0x15]&0xff);
  printf(" IMUXIN                0x%02x\t",apv[0x16]&0xff);
  printf(" ICAL                  0x%02x\n",apv[0x18]&0xff);
  printf(" VPSP                  0x%02x\t",apv[0x19]&0xff);
  printf(" VFS                   0x%02x\n",apv[0x1a]&0xff);
  printf(" VFP                   0x%02x\t",apv[0x1b]&0xff);
  printf(" CDRV                  0x%02x\n",apv[0x1c]&0xff);
  printf(" CSEL                  0x%02x\n",apv[0x1d]&0xff);
  
  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("                           APV-PLL Summary (ALL PLLs)\n\n");
  printf(" Fine Delay            0x%02x\t",pll[1]&0xff);
  printf(" Trigger Delay         0x%02x\n",pll[3]&0xff);

  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("                            ADC C-Card Summary\n\n");
  printf(" Hybrid Reset Mask     0x%02x\t",adccard[0]&0xff);
  printf(" PowerDown Ch0 Mask    0x%02x\n",adccard[1]&0xff);
  printf(" PowerDown Ch1 Mask    0x%02x\t",adccard[2]&0xff);
  printf(" EQ Level 0 Mask       0x%02x\n",adccard[3]&0xff);
  printf(" EQ Level 1 Mask       0x%02x\t",adccard[4]&0xff);
  printf(" TrgOut Enable Mask    0x%02x\n",adccard[5]&0xff);
  printf(" BCLK Enable Mask      0x%02x\n",adccard[6]&0xff);

#endif /* SKIPBROKEN */

  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("\n\n");

  return 0;
}

static int srsContinueReadoutUntilEmpty=0;

int 
srsReadBlock(int sockfd, volatile unsigned int* buf_in, int nwrds, int blocklevel, int *frameCnt)
{
  int trigNum=0, n=0;
  struct sockaddr_in cli_addr;  
  socklen_t slen=sizeof(cli_addr);  
  unsigned int *ptr; 
  unsigned int rawdata=0;
  int nwords=0;
  unsigned long long total=0;
  int l_frameCnt=0;
  int id=0;
  
  id=srsGetID(sockfd);
  if(id==-1)
    {
      printf("%s: ERROR: SRS socket file descriptor (%d) not initialized\n",
	     __FUNCTION__,sockfd);
      return -1;
    }

  srsFrameNumber[id]=0;

  /* SRSLOCK; */
  ptr = (unsigned int*)buf_in; 

  while(1)
    {
      if(srsDebugMode)
	printf("%s: call recvfrom  buf = 0x%lx  nwrds = %d\n",
	       __FUNCTION__,(unsigned long)ptr,nwrds);

      before = rdtsc();
      n = recvfrom(sockfd, (void *)ptr, nwrds*sizeof(unsigned int), 
		   0, (struct sockaddr*)&cli_addr, &slen); 
      after = rdtsc();

      diff = after-before;
      total += diff;
      if(srsDebugMode)
	  printf("%s: recvfrom returned %d    time = %lld\n",__FUNCTION__,n,diff);
      
      if (n == -1)
	{ 
	  if((errno == EAGAIN) && (srsContinueReadoutUntilEmpty == 0))
	    printf("%s(%d): timeout\n",__FUNCTION__,id);
	  else if (srsContinueReadoutUntilEmpty == 0)
	    {
	      perror("recvfrom");
	      printf("%s(%d): ERROR\n",__FUNCTION__,id);
	    }

	  if(frameCnt!=NULL)
	    *frameCnt = l_frameCnt++;

	    /* SRSUNLOCK; */
	  return -999; /*nwords; sergey: force rol to ignore 'incomplete' events */
	}

      if (n>=4)
	{
	  l_frameCnt++;

	  if(srsDebugMode)
	    printf("Received packet from %s:%d of length %i\n",
		   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), n);


	  /* Save the data at the beginning of the frame */
	  rawdata = bswap_32(*ptr);

	  /* Bump the pointer and number of words by the return value */
	  ptr    += n/4;
	  nwords += n/4;
	  
	  if(srsDebugMode)
	    printf("%d  0x%x\n",nwords,bswap_32(rawdata));

	  /* Check if we've gotten the event-trailer (in its own frame) */
	  if(rawdata == 0xfafafafa) /* event-trailer frame */
	    {
	      if(srsDebugMode)
		printf("--- Received event trailer --- \n");
	      
	      /* Reset frame number for next trigger */
	      srsFrameNumber[id]=0;
	      /* Bump the trigger number */
	      trigNum++;

	      if(srsContinueReadoutUntilEmpty==0)
		{
		  if(trigNum==blocklevel) /* Quit... */
		    break;
		}
	    }
	  else
	    {
	      if((rawdata&0xff) != srsFrameNumber[id])
		{
		  printf("%s(%d): ERROR: Expected frame number %x.  Received %x.\n",
			 __FUNCTION__,id,srsFrameNumber[id], rawdata&0xff);
		}
	      if(srsDebugMode)
		{
		  printf("%s(%d): Timestamp = 0x%06x\n",
			 __FUNCTION__,id,rawdata>>8);
		}
	      srsFrameNumber[id]++;
	    }
	}
    }

  if(frameCnt!=NULL)
    *frameCnt = l_frameCnt++;

  if(srsDebugMode)
    if(frameCnt!=NULL)
      printf("%s: frameCnt = %d\n",__FUNCTION__,*frameCnt);

  /* SRSUNLOCK; */
  return nwords;
}

int
srsCheckAndClearBuffers(int *sockfds, int nfds, volatile unsigned int* buf_in,
			int nwrds, int blocklevel, int *frameCnt)
{
  int ifd=0, checkData=1, nret=0, stat=0;
  struct timespec poll_timeout = {0, 50000}; /* 50 microsecond timeout / poll */
  struct pollfd poll_set[16];

  while(checkData)
    {
      /* Setup the polling variables with the socket file descriptors */
      for(ifd = 0; ifd < nfds; ifd++)
	{
	  poll_set[ifd].fd     = sockfds[ifd];
	  poll_set[ifd].events = POLLIN;
	}

      /* Ignore the rest */
      for(ifd = nfds; ifd < 16; ifd++)
	{ 
	  poll_set[ifd].fd     = -1;
	}

      /* Poll the sockets */
      stat = ppoll(poll_set, nfds, &poll_timeout, NULL);
      if(stat < 0)
	{
	  /* ppoll error */
	  perror("poll");
	  printf("%s: ERROR: ppoll returned %d",
		 __FUNCTION__, stat);
	  return -1;
	}
      else if (stat == 0)
	{
	  /* No Data... Buffers clean */
	  checkData = 0;
	}
      else
	{
	  checkData = 1;
	}

      if(stat>0) /* At least one fd has data available */
	{
	  /* Set readblock to keep going, instead of stopping at the trailer */
	  srsContinueReadoutUntilEmpty=1;

	  for(ifd = 0; ifd < nfds; ifd++)
	    {
	      if(poll_set[ifd].revents & POLLIN)
		{
		  /* Go do it */
		  nret = srsReadBlock(poll_set[ifd].fd, buf_in, nwrds, blocklevel, frameCnt);
	      
		  if(nret>0)
		    {
		      printf("%s: WARN: %d words cleared from socket %d\n",
			     __FUNCTION__, nret, poll_set[ifd].fd);
		    }
		}
	    }

	  /* Set readblock to stop at the trailer */
	  srsContinueReadoutUntilEmpty=0;

	}
    }
  
  return nret;
}

int
srsSetDAQIP(char *ip, char *daq_ip, int port)
{
  int stat=0;
  struct in_addr sa;
  const int nregs=2;
  unsigned int reg[nregs];
  unsigned int data[nregs];
  
  if(inet_pton(AF_INET, daq_ip, &sa)!=1)
  {
    perror("inet_pton");
    return -1;
  }
  else
  {
    printf("inet_pton successfully converted daq ip >%s< into network address structure\n",daq_ip);
  }

  reg[0]  = 0x4;
  data[0] = port;

  reg[1]  = 0xa;
  data[1] = bswap_32(sa.s_addr);

  printf("======================= srsSetDAQIP: befor srsWritePairs()\n");
  stat = srsWritePairs(ip, SRS_SYS_PORT, 0, reg, nregs, data,nregs);
  printf("======================= srsSetDAQIP: after srsWritePairs()\n");

  return stat;
}

int
srsSetDTCClk(char *ip, int dtcclk_inh, int dtctrg_inh, 
	    int dtc_swapports, int dtc_swaplanes, int dtctrg_invert)
{
  int stat=0;
  const int nregs=1;
  unsigned int reg[nregs];
  unsigned int data[nregs];

  reg[0]  = 0xc;
  data[0] = (dtcclk_inh) | (dtctrg_inh<<1) | 
    (dtc_swapports<<2) | (dtc_swaplanes<<3) |
    (dtctrg_invert<<4) ;
		     
  if(srsDebugMode)
    printf("%s: data = 0x%08x\n",__FUNCTION__,data[0]);

  stat = srsWritePairs(ip, SRS_SYS_PORT, 0, reg, nregs, data,nregs);

  return stat;
}

int
srsSetDTCC(char *ip, int dataOverEth, int noFlowCtrl, int paddingType,
	   int trgIDEnable, int trgIDAll, int trailerCnt, int paddingByte, int trailerByte)
{
  int stat=0;
  const int nregs=1;
  unsigned int reg[nregs];
  unsigned int data[nregs];

  dataOverEth = dataOverEth?1:0;
  noFlowCtrl  = noFlowCtrl?1:0;
  trgIDEnable = trgIDEnable?1:0;
  trgIDAll    = trgIDAll?1:0;
  
  if((paddingType<0) || (paddingType>2))
    {
      printf("%s: Invalid paddingType (%d)\n",
	     __FUNCTION__,paddingType);
      return -1;
    }
  
  if((trailerCnt<0) || (trailerCnt>0xf))
    {
      printf("%s: Invalid trailerCnt (%d)\n",
	     __FUNCTION__,trailerCnt);
      return -1;
    }

  if((paddingByte<0) || (paddingByte>0xff))
    {
      printf("%s: Invalid paddingByte (%d)\n",
	     __FUNCTION__,paddingByte);
      return -1;
    }

  if((trailerByte<0) || (trailerByte>0xff))
    {
      printf("%s: Invalid trailerByte (%d)\n",
	     __FUNCTION__,trailerByte);
      return -1;
    }

  reg[0]  = 0xb;
  data[0] = dataOverEth | (noFlowCtrl<<1) | (paddingType<<8) |
    (trgIDEnable<<10) | (trgIDAll<<11) | (trailerCnt<<12) |
    (paddingByte<<16) | (trailerByte<<24) ;
		     
  if(srsDebugMode)
    printf("%s: data = 0x%08x\n",__FUNCTION__,data[0]);

  stat = srsWritePairs(ip, SRS_SYS_PORT, 0, reg, nregs, data,nregs);

  return stat;
}

int
srsConfigADC(char *ip,
	     int reset_mask, int ch0_down_mask, int ch1_down_mask,
	     int eq_level0_mask, int eq_level1_mask, int trgout_enable_mask,
	     int bclk_enable_mask)
{
  int stat;
  const int nregs=7;
  unsigned int data[nregs];

  memset(data,0,sizeof(data));

  data[0] = reset_mask;
  data[1] = ch0_down_mask;
  data[2] = ch1_down_mask;
  data[3] = eq_level0_mask;
  data[4] = eq_level1_mask;
  data[5] = trgout_enable_mask;
  data[6] = bclk_enable_mask;

  stat = srsWriteBurst(ip, SRS_ADCCARD_PORT, 
		       0, 0, nregs,
		       data, nregs);
    
  return stat;
}

int
srsSetApvTriggerControl(char *ip,
			int mode, int trgburst, int freq,
			int trgdelay, int tpdelay, int rosync)
{
  int stat=0;
  const int nregs=6;
  unsigned int data[nregs];

  memset(data,0,sizeof(data));

  data[0] = mode;
  data[1] = trgburst;
  data[2] = freq;
  data[3] = trgdelay;
  data[4] = tpdelay;
  data[5] = rosync;

  stat = srsWriteBurst(ip, SRS_APVAPP_PORT, 
		       0, 0, nregs,
		       data, nregs);

  return stat;
}

int
srsSetEventBuild(char *ip,
		 int chEnable, int dataLength, int mode, 
		 int eventInfoType, unsigned int eventInfoData)
{
  int stat=0;
  const int nregs=5;
  unsigned int data[nregs];

  memset(data,0,sizeof(data));

  data[0] = chEnable;
  data[1] = dataLength;
  data[2] = mode;
  data[3] = eventInfoType;
  data[4] = eventInfoData;

  stat = srsWriteBurst(ip, SRS_APVAPP_PORT, 
		       0, 8, nregs,
		       data, nregs);

  return stat;
}

int
srsTrigEnable(char *ip)
{
  int stat=0;
  const int nregs=1;
  unsigned int data[nregs];

  memset(data,0,sizeof(data));

  data[0] = 1;

  stat = srsWriteBurst(ip, SRS_APVAPP_PORT, 
		       0, 0xf, nregs,
		       data, nregs);

  return stat;
}

int
srsTrigDisable(char *ip)
{
  int stat=0;
  const int nregs=1;
  unsigned int data[nregs];

  memset(data,0,sizeof(data));

  data[0] = 0;

  stat = srsWriteBurst(ip, SRS_APVAPP_PORT, 
		       0, 0xf, nregs,
		       data, nregs);

  return stat;
}

int
srsAPVReset(char *ip)
{
  int stat=0;
  const int nregs=1;
  unsigned int data[nregs];

  memset(data,0,sizeof(data));

  data[0] = 1;

  stat = srsWriteBurst(ip, SRS_APVAPP_PORT, 
		       0, 0xffffffff, nregs,
		       data, nregs);

  return stat;
}

int
srsAPVConfig(char *ip, int channel_mask, int device_mask,
	     int mode, int latency, int mux_gain, 
	     int ipre, int ipcasc, int ipsf, 
	     int isha, int issf, int ipsp, 
	     int imuxin, int ical, int vsps,
	     int vfs, int vfp, int cdrv, int csel)
{
  int stat=0;
  const int nregs=16;
  unsigned int regs[nregs];
  unsigned int data[nregs];
  unsigned int subaddr=0;

  if((device_mask<1) || (device_mask>3))
    {
      printf("%s: ERROR: Invalid device_mask (%d)\n",
	     __FUNCTION__,device_mask);
      return -1;
    }

  memset(data,0,sizeof(data));
  memset(regs,0,sizeof(regs));

  subaddr = ((channel_mask&0xff)<<8) | (device_mask&0x3);

  regs[0]  = 0x1;  data[0]  = mode;
  regs[1]  = 0x2;  data[1]  = latency;
  regs[2]  = 0x3;  data[2]  = mux_gain;
  regs[3]  = 0x10; data[3]  = ipre;
  regs[4]  = 0x11; data[4]  = ipcasc;
  regs[5]  = 0x12; data[5]  = ipsf;
  regs[6]  = 0x13; data[6]  = isha;
  regs[7]  = 0x14; data[7]  = issf;
  regs[8]  = 0x15; data[8]  = ipsp;
  regs[9]  = 0x16; data[9]  = imuxin;
  regs[10] = 0x18; data[10] = ical;
  regs[11] = 0x19; data[11] = vsps;
  regs[12] = 0x1A; data[12] = vfs;
  regs[13] = 0x1B; data[13] = vfp;
  regs[14] = 0x1C; data[14] = cdrv;
  regs[15] = 0x1D; data[15] = csel;

  stat = srsWritePairs(ip, SRS_APV_PORT, subaddr, regs, nregs, data, nregs);

  return stat;
}

int
srsPLLConfig(char *ip, int channel_mask,
	     int fine_delay, int trg_delay)
{
  int stat=0;
  const int nregs=0x2;
  unsigned int regs[nregs];
  unsigned int data[nregs];
  unsigned int subaddr=0;

  memset(data,0,sizeof(data));
  memset(regs,0,sizeof(regs));

  subaddr = ((channel_mask&0xff)<<8);

  regs[0]  = 0x1;  data[0]  = fine_delay;
  regs[1]  = 0x3;  data[1]  = trg_delay;

  stat = srsWritePairs(ip, SRS_APV_PORT, subaddr, regs, nregs, data, nregs);

  return stat;
}

void
srsSetDebugMode(int enable)
{
  if(enable)
    srsDebugMode=1;
  else
    srsDebugMode=0;
}


int
srsReadList(char *ip, int port, 
	    unsigned int sub_addr, unsigned int *list, int nlist,
	    unsigned int *data, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *read_command;

  switch(port)
    {
    case SRS_SYS_PORT:
      maxregs = 16;
      break;

    case SRS_APVAPP_PORT:
      maxregs = 32;
      break;

    case SRS_ADCCARD_PORT:
      maxregs = 7;
      break;
      
    default:
      maxregs=0xff; // FIXME: should be more reasonable.
    }

  read_command = (unsigned int *)malloc((maxregs+4)*sizeof(unsigned int));
  if(read_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  read_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  read_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  read_command[wCnt++] = bswap_32(0xBBAAffff);  /* Read List */
  read_command[wCnt++] = bswap_32(0x11223344);  /* CMD INFO (dont care) */
  for(iword=0; iword<nlist; iword++) /* Add in dummy fields */
    {
      read_command[wCnt++] = bswap_32(list[iword]);
    }

  nwords = srsSlowControl(ip, port, 
			  read_command, wCnt,
			  data);

  if(read_command)
    free(read_command);

  return nwords;
}

int
srsReadBurst(char *ip, int port, 
	     unsigned int sub_addr, unsigned int first_addr, int nregs,
	     unsigned int *data, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *read_command;

  switch(port)
    {
    case SRS_SYS_PORT:
      maxregs = 16;
      break;

    case SRS_APVAPP_PORT:
      maxregs = 32;
      break;

    case SRS_ADCCARD_PORT:
      maxregs = 7;
      break;
      
    default:
      /* printf("%s: Reading All registers from port %d not supported\n", */
      /* 	     __FUNCTION__,port); */
      maxregs=0xff;
    }

  if(nregs>maxregs)
    {
      printf("%s: WARN: nregs (%d) > maxregs (%d). Using %d.\n",
	     __FUNCTION__, nregs, maxregs, maxregs);
      nregs = maxregs;
    }
  
  read_command = (unsigned int *)malloc((nregs+3)*sizeof(unsigned int));
  if(read_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  read_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  read_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  read_command[wCnt++] = bswap_32(0xBBBBffff);  /* Read Burst */
  read_command[wCnt++] = bswap_32(first_addr);  /* CMD INFO (first address to read) */
  for(iword=0; iword<nregs-1; iword++) /* Add in dummy fields */
    {
      read_command[wCnt++] = bswap_32(first_addr+iword+1);
    }

  nwords = srsSlowControl(ip, port, 
			  read_command, wCnt,
			  data);

  if(read_command)
    free(read_command);

  return nwords;
}

int
srsWriteBurst(char *ip, int port, 
	      unsigned int sub_addr, unsigned int first_addr, int nregs,
	      unsigned int *wdata, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *write_command;

  switch(port)
    {
    case SRS_SYS_PORT:
      maxregs = 16;
      break;

    case SRS_APVAPP_PORT:
      maxregs = 32;
      break;

    case SRS_ADCCARD_PORT:
      maxregs = 7;
      break;
      
    default:
      /* printf("%s: Reading All registers from port %d not supported\n", */
      /* 	     __FUNCTION__,port); */
      nregs=0xff;
    }

  if(nregs>maxregs)
    {
      printf("%s: WARN: nregs (%d) > maxregs (%d). Using %d.\n",
	     __FUNCTION__, nregs, maxregs, maxregs);
      nregs = maxregs;
    }
  
  write_command = (unsigned int *)malloc((nregs+4)*sizeof(unsigned int));
  if(write_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  write_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  write_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  write_command[wCnt++] = bswap_32(0xAABBffff);  /* Write Burst */
  write_command[wCnt++] = bswap_32(first_addr);  /* CMD INFO (first address to write) */

  for(iword=0; iword<nregs; iword++) /* Add in dummy fields */
    {
      write_command[wCnt++] = bswap_32(wdata[iword]);
    }

  nwords = srsSlowControl(ip, port, 
			  write_command, wCnt,
			  NULL);

  if(write_command)
    free(write_command);

  return nwords;
}

int
srsWritePairs(char *ip, int port, 
	      unsigned int sub_addr, unsigned int *addr, int nregs,
	      unsigned int *wdata, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *write_command;

  switch(port)
    {
    case SRS_SYS_PORT:
      maxregs = 16;
      break;

    case SRS_APVAPP_PORT:
      maxregs = 32;
      break;

    case SRS_ADCCARD_PORT:
      maxregs = 7;
      break;
      
    default:
      /* printf("%s: Reading All registers from port %d not supported\n", */
      /* 	     __FUNCTION__,port); */
      maxregs=0xff;
    }

  if(nregs>maxregs)
    {
      printf("%s: WARN: nregs (%d) > maxregs (%d). Using %d.\n",
	     __FUNCTION__, nregs, maxregs, maxregs);
      nregs = maxregs;
    }
  
  if(srsDebugMode)
    printf("%s: ip = %s:%d sub_addr = 0x%x  nregs = %d\n",
	   __FUNCTION__,ip,port,sub_addr,nregs);

  write_command = (unsigned int *)malloc((2*nregs+4)*sizeof(unsigned int));
  if(write_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  write_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  write_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  write_command[wCnt++] = bswap_32(0xAAAAffff);  /* Write Pairs */
  write_command[wCnt++] = bswap_32(0x11223344);  /* CMD INFO (dont care) */
  for(iword=0; iword<nregs; iword++) /* Add in dummy fields */
    {
      write_command[wCnt++] = bswap_32(addr[iword]);
      write_command[wCnt++] = bswap_32(wdata[iword]);
    }

  nwords = srsSlowControl(ip, port, 
			  write_command, wCnt,
			  NULL);

  if(write_command)
    free(write_command);

  return nwords;
}

int
srsExecConfigFile(char *filename)
{
  char ip[100];
  int port = 0;
  unsigned int *buf;
  int nwords=0;

  buf = (unsigned int *)malloc(50*sizeof(unsigned int));

  nwords = srsReadFile(filename, (char *)&ip, (int *)&port, buf);

  srsSlowControl(ip, port,
		 buf, nwords,
		 NULL);

  free(buf);
  return 0;
}

static int
srsSlowControl(char *ip, int port, 
	       unsigned int *req_fields, int nfields,
	       unsigned int *ret_data)
{
  unsigned int *response;
  int iword, nwords=0, maxwords=0, nerr=0;

  maxwords      = (2*(nfields-4))+4;
  response      = (unsigned int *)malloc(maxwords*sizeof(unsigned int));

  if(srsDebugMode)
    {
      printf("%s: Writing to port %d  ip %s nfields = %d\n",
	     __FUNCTION__,port, ip,nfields);
      for(iword=0; iword<nfields; iword++)
	printf("%2d: 0x%08x\n",iword,bswap_32(req_fields[iword]));
    }


  SRSLOCK;
  if(srsPrepareSocket(ip, port)<0)
    {
      printf("%s(%s:%d): Unable to open/prepare socket\n",__FUNCTION__,ip,port);
      srsCloseSocket();

      if(response)
	free(response);
      SRSUNLOCK;
      return -1;
    }

  if(srsSendData(req_fields,nfields)<0)
    {
      printf("%s(%s:%d): Unable to send data\n",__FUNCTION__,ip,port);
      srsCloseSocket();

      if(response)
	free(response);
      SRSUNLOCK;
      return -1;
    }
  
  nwords = srsReceiveData(response, maxwords);
  if(nwords<0)
    {
      printf("%s(%s:%d): Failed to receive data\n",__FUNCTION__,ip,port);
      srsCloseSocket();

      if(response)
	free(response);
      SRSUNLOCK;
      return -1;
    }

  if(srsDebugMode)
    {
      printf("%s: Response:\n",__FUNCTION__);
      for(iword=0; iword<nwords; iword++)
	printf("%2d: 0x%08x\n",iword,bswap_32(response[iword]));

      printf("\n");
    }

  srsCloseSocket();

  nwords = srsParseResponse(response, ret_data, nwords, &nerr);

  SRSUNLOCK;

  if(nerr!=0)
    {
      printf("%s(%s:%d): %d Errors reported in response.\n",
	     __FUNCTION__,ip,port,nerr);
    }

  usleep(100000);

  if(response)
    free(response);

  return nwords;
}

static int
srsDecodeErrorReply(unsigned int error_reply, int pflag)
{
  int rval=0;
  unsigned int err_swapped = bswap_32(error_reply);

  if(err_swapped & 0xF80D007F)
    {
      rval=-1;
      if(pflag)
	{
	  printf("%s: Error Reply 0x%08x :\n",__FUNCTION__,err_swapped);
	  if(err_swapped & (1<<31))
	    printf("\tDestination Port Unavailable\n");

	  if(err_swapped & (1<<30))
	    printf("\tIllegal Source Port\n");

	  if(err_swapped & (1<<29))
	    printf("\tBuffer Full\n");

	  if(err_swapped & (1<<28))
	    printf("\tIllegal Length (incomplete 32bit word)\n");

	  if(err_swapped & (1<<27))
	    printf("\tIllegal Length (< 4 words)\n");

	  if(err_swapped & (1<<26))
	    printf("\tReply ID Error\n");

	  if(err_swapped & (1<<19))
	    printf("\tCommand Unrecognized\n");

	  if(err_swapped & (1<<18))
	    printf("\tIllformed Command\n");

	  if(err_swapped & (1<<16))
	    printf("\tChecksum Error\n");

	  if(err_swapped & (0x7F))
	    printf("\tI2C Error\n");
	}

    }
  else if(err_swapped!=0)
    {
      rval = -1;
      printf("\tUnrecognized Error Code\n");
    }


  return rval;
}

static int
srsParseResponse(unsigned int *ibuf, unsigned int *obuf, int nwords, int *err)
{
  int iword=0, dCnt=0, nerr=0;

  /* First word */
  if((bswap_32(ibuf[0]) & (1<<31)) != 0)
    {
      printf("%s: 0x%08x: Invalid Reply Identifier\n",
	     __FUNCTION__,ibuf[0]);
    }

  for(iword=4; iword<nwords; iword+=2)
    {
      if(srsDecodeErrorReply(ibuf[iword],1)==-1) /* Error condition */
	nerr++;

      if(obuf)
	obuf[dCnt++] = bswap_32(ibuf[iword+1]);
    }
  
  if(nerr>0)
    printf("%s: %d Error(s) found in reply\n",__FUNCTION__,nerr);

  *err = nerr;

  return dCnt;
}


/* UDP Send/Receive commands */
int g_sockfd;
struct sockaddr_in g_servaddr;


static struct addrinfo *
host_serv(const char *host, const char *serv, int family, int socktype) 
{
  struct addrinfo hints, *res;

  memset(&hints,0,sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME; // returns canonical name
  hints.ai_family = family; // AF_UNSPEC AF_INET AF_INET6 etc...
  hints.ai_socktype = socktype; // 0 SOCK_STREAM SOCK_DGRAM

  if (getaddrinfo(host,serv,&hints,&res) != 0) 
    {
      perror("getaddrinfo");
      return NULL;
    }
  
  return (res);    
}

static char* 
sock_ntop_host(const struct sockaddr *sa, socklen_t salen) 
{
  static char str[128];		/* Unix domain is largest */
  
  switch (sa->sa_family) 
    {
    case AF_INET: 
      {
	struct sockaddr_in	*sin = (struct sockaddr_in *) sa;
	
	if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
	  return(NULL);
	return(str);
      }

    default:
      {
	snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
		 sa->sa_family, salen);
	return(str);
      }
  }
  return (NULL);
}

//********************** create a socket ******************************************
static int
srsPrepareSocket(char *ip_addr,int port_number) 
{
  int rval=0;
  struct addrinfo *ai;
  char *h;
  struct timeval socket_timeout={0,SOCKET_TIMEOUT}; /* 900 microsecond time out */
  int stat, enable=1;

  g_sockfd = socket(AF_INET,SOCK_DGRAM,0);  
  if(g_sockfd<0)
    {
      perror("socket");
      return -1;
    }

  stat = setsockopt(g_sockfd,
		    SOL_SOCKET,
		    SO_RCVTIMEO,
		    &socket_timeout,sizeof(socket_timeout));
  if(stat!=0)
    {
      perror("setsockopt SO_RECVTIMEO");
      rval=-1;
    }

  stat =setsockopt(g_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if(stat!=0)
    {
      perror("setsockopt SO_REUSEADDR");
      rval=-1;
    }

#ifdef SO_REUSEPORT
  stat =setsockopt(g_sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
  if(stat!=0)
    {
      perror("setsockopt SO_REUSEPORT");
      rval=-1;
    }
#endif

  memset(&g_servaddr,0,sizeof(g_servaddr));
  g_servaddr.sin_family = AF_INET;

  //
  // set the source port
  //
  g_servaddr.sin_port = htons(6007);
  /* g_servaddr.sin_addr.s_addr = htonl(INADDR_ANY); */
  stat = bind(g_sockfd,(struct sockaddr *)& g_servaddr, sizeof(g_servaddr));
  if (stat==-1) 
    {
      perror("bind");
      return -1;
    }
  else
    {
      if(srsDebugMode)
	printf("%s: DEBUG: bind() successful\n",__FUNCTION__);
    }

  //
  // set the destination port
  //
  g_servaddr.sin_port = htons(port_number);
  ai = host_serv(ip_addr,NULL,AF_INET,SOCK_DGRAM);
  if(ai==NULL)
    {
      printf("%s: host_serv returned NULL\n",__FUNCTION__);
      return -1;
    }

  h = sock_ntop_host(ai->ai_addr,ai->ai_addrlen);
  stat = inet_pton(AF_INET,(ai->ai_canonname ? h : ai->ai_canonname),&g_servaddr.sin_addr);
  if(stat==0)
    {
      printf("%s: Invalid network address (%s)\n",
	     __FUNCTION__,(ai->ai_canonname ? h : ai->ai_canonname));
    }
  else if(stat<0)
    {
      perror("inet_pton");
      rval = -1;
    }

  freeaddrinfo(ai);  

  return rval;
}

//
// Start to send data
//
static int
srsSendData(unsigned int *buffer, int nwords) 
{
  int dSent=0;
  
  dSent = sendto(g_sockfd,buffer,(nwords<<2),0,
		 (struct sockaddr *)&g_servaddr,sizeof(g_servaddr));

  if ( dSent == -1) 
    {
      perror("sendto");
      return -1;
    }
  return (dSent>>2);
}

static int
srsReceiveData(unsigned int *buffer, int nwords) 
{
  int n;
  int dCnt;
  struct sockaddr_in g_cliaddr;
  socklen_t g_len = sizeof(g_cliaddr);

  n = recvfrom(g_sockfd,(void *)buffer,nwords*sizeof(unsigned int),
	       0,(struct sockaddr *) &g_cliaddr,&g_len);

  if(n<0)
    {
      perror("recvfrom");
      printf("%s: ERROR: Timeout (%d)\n",__FUNCTION__, n);
      return -1;
    }

  dCnt = n>>2;

  if(srsDebugMode)
    {
      int i;
      printf("%s: Received %d bytes\n",__FUNCTION__,n);
      for(i=0; i<dCnt; i++)
	{
	  printf("%4d: %08x  %08x\n",i,buffer[i],bswap_32(buffer[i]));
	}
	  
      printf("\n");
    }

  return dCnt;
}

static void 
charToHex(char *buffer,int *buf) 
{
  int i;
  int num = 0, tot = 0, base = 1;

  for (i=7;i>=0;i--)
    {
      buffer[i] = toupper(buffer[i]);
      if (buffer[i] < '0' || buffer[i] > '9')
	{
	  if (buffer[i] > 'F') buffer[i] = 'F';
	  num = buffer[i] - 'A';
	  num += 10;
	}
      else 
	num = buffer[i] - '0';
      
      tot += num*base;
      base *= 16;
    }
  *buf = tot;
}

static int
srsCloseSocket()
{
  if(srsDebugMode)
    printf("%s: Close Socket %d\n",
	   __FUNCTION__, g_sockfd);

  if(g_sockfd)
    {
      if(close(g_sockfd)!=0)
	{
	  perror("close");
	  return -1;
	}
    }

  return 0;
}

static int
srsReadFile(char *path, char *ip, int *port, unsigned int *obuffer)
{
  FILE *pFile;
  int *buf;
  char buffer[100];
  int nwords=0;
  
  buf = (int*)obuffer;
  memset(buffer,0,100);
  pFile = fopen (path , "r");
  if (pFile == NULL) 
    {
      perror("fopen");
      printf("%s: Error opening file %s",__FUNCTION__,path);
      return -1;
    } 
  else 
    {
      //
      // IP
      //
      fgets (buffer,100,pFile);
      strncpy(ip,buffer,100);
      //
      // PORT
      //
      fgets (buffer,100,pFile);
      *port = atoi(buffer);
      //
      // INFO
      //
      while (fgets (buffer,100,pFile) != NULL) 
	{
	  if (buffer[0] == '#') continue;
	  charToHex(buffer,buf);
	  *buf = htonl(*buf);
	  buf++;
	  nwords++;
	}
      fclose (pFile);
    }

  return nwords;
}

#ifndef Linux_armv7l
static unsigned long long int 
rdtsc(void)
{
  /*    unsigned long long int x; */
  unsigned a, d;
   
  __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

  return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}
#endif
