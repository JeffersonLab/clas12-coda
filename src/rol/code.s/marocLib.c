
/* marocLib.c */

#ifndef Linux_armv7l

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include "marocLib.h"

static MAROC_regs *pMAROC_regs = (MAROC_regs *)0x0;
static MAROC_ASIC_regs_t marocASIC[MAROC_MAX_NUM][3];
static MAROC_ASIC_regs_t marocASIC_Rd[MAROC_MAX_NUM][3];
static int marocASIC_num[MAROC_MAX_NUM];

int maroc_ch_to_pixel[64] = {
  59,58,57,56,51,50,49,48,
  43,42,41,40,35,34,33,32,
  27,26,25,24,19,18,17,16,
  11,10, 9, 8, 3, 2, 1, 0,
   4, 5, 6, 7,12,13,14,15,
  20,21,22,23,28,29,30,31,
  36,37,38,39,44,45,46,47,
  52,53,54,55,60,61,62,63
};

static int nmaroc;
static int devids[MAROC_MAX_NUM];
static int sockfd_reg[MAROC_MAX_NUM];
static int sockfd_event[MAROC_MAX_NUM];

// Intermediate readout buffers to store left TCP socket data for next event blocks
#define MAROC_EVENT_BUF_LEN   10000

static int marocEventBufferWork[MAROC_EVENT_BUF_LEN];
static int marocEventBuffer[MAROC_MAX_NUM][MAROC_EVENT_BUF_LEN];
static int marocEventBufferWrPtr[MAROC_MAX_NUM];
static int marocEventBufferRdPtr[MAROC_MAX_NUM];
static int marocEventBufferSize[MAROC_MAX_NUM];
static int marocEventBufferNBlocks[MAROC_MAX_NUM];

/******************************************************/
/** SOCKET functions **********************************/
/******************************************************/

typedef struct
{
  int len;
  int type;
  int wrcnt;
  int addr;
  int flags;
  int vals[1];
} write_struct;

typedef struct
{
  int len;
  int type;
  int rdcnt;
  int addr;
  int flags;
} read_struct;

typedef struct
{
  int len;
  int type;
  int rdcnt;
  int data[1];
} read_rsp_struct;


void maroc_write32(int devid, void *addr, int val)
{
	write_struct ws;
  
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }

	ws.len = 16;
	ws.type = 4;
	ws.wrcnt = 1;
	ws.addr = (int)((long)addr);
	ws.flags = 0;
	ws.vals[0] = val;
	write(sockfd_reg[devid], &ws, sizeof(ws));
}

unsigned int maroc_read32(int devid, void *addr)
{
	read_struct rs;
	read_rsp_struct rs_rsp;
	int len;
	
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return 0;
  }

	rs.len = 12;
	rs.type = 3;
	rs.rdcnt = 1;
	rs.addr = (int)((long)addr);
	rs.flags = 0;
	write(sockfd_reg[devid], &rs, sizeof(rs));
	
	len = read(sockfd_reg[devid], &rs_rsp, sizeof(rs_rsp));
	if(len != sizeof(rs_rsp))
		printf("Error in %s: socket read failed...\n", __FUNCTION__);
	
	return rs_rsp.data[0];
}

void maroc_read32_n(int devid, int n, void *addr, unsigned int *buf)
{
	read_struct rs;
	read_rsp_struct rs_rsp;
	int len, i;
  
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	for(i = 0; i < n; i++)
	{
		rs.len = 12;
		rs.type = 3;
		rs.rdcnt = 1;
		rs.addr = (int)((long)addr);
		rs.flags = 0;
		write(sockfd_reg[devid], &rs, sizeof(rs));
	}
	
	for(i = 0; i < n; i++)
	{
		len = read(sockfd_reg[devid], &rs_rsp, sizeof(rs_rsp));
		if(len != sizeof(rs_rsp))
			printf("Error in %s: socket read failed...\n", __FUNCTION__);
		
		buf[i] = rs_rsp.data[0];
	}
}

int maroc_open_socket(char *ip, int port)
{
  struct sockaddr_in serv_addr;
  int sockfd = 0;
	
  printf("maroc_open_socket 1\n");fflush(stdout);
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf(" %s: Error: Could not create socket \n",__func__);
    return 0;
  }
  memset(&serv_addr, '0', sizeof(serv_addr)); 

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  printf("maroc_open_socket 2\n");fflush(stdout);
  if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
  {
    printf(" %s: Error: inet_pton error occured\n",__func__);
    return 0;
  } 

  printf("maroc_open_socket 3: calling connect() ...\n");fflush(stdout);
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf(" %s: Error : Connect Failed \n",__func__);
    return 0;
  }
  printf("maroc_open_socket 4\n");fflush(stdout);

  return sockfd;
}

int maroc_open_register_socket(int devid)
{
  int n, val;
  char ip[20];

  sprintf(ip, "%s%d", MAROC_SUBNET, MAROC_IP_START+devid);

  printf("%s: Connecting to %s (devid=%d, register socket) .. ",__func__,ip, devid);fflush(stdout);
  sockfd_reg[devid] = maroc_open_socket(ip, MAROC_REG_SOCKET);

  if(sockfd_reg[devid])
  {
    printf("Succeeded.\n");fflush(stdout);

    /* Send endian test header */
    val = 0x12345678;
    write(sockfd_reg[devid], &val, 4);
	
    val = 0;
    n = read(sockfd_reg[devid], &val, 4);
    printf("n = %d, val = 0x%08X\n", n, val);fflush(stdout);

    return OK;
  }

  printf("Failed.\n");fflush(stdout);

  return ERROR;
}

int maroc_open_event_socket(int devid)
{
  char ip[20];

  sprintf(ip, "%s%d", MAROC_SUBNET, MAROC_IP_START+devid);

  printf("%s: Connecting to %s (devid=%d, event socket) .. ",__func__,ip,devid);fflush(stdout);
  sockfd_event[devid] = maroc_open_socket(ip, MAROC_EVT_SOCKET);
  
  if(sockfd_event[devid])
  {
    printf("Succeeded.\n");
    return OK;
  }

  return ERROR;
}

void maroc_close_register_socket(int devid)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }

  close(sockfd_reg[devid]);
  sockfd_reg[devid] = 0;
}

void maroc_close_event_socket(int devid)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_event[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }

  close(sockfd_event[devid]);
  sockfd_event[devid] = 0;
}

int maroc_read_event_socket(int devid, int *buf, int nwords_max)
{
  int nbytes, nwords, result;
  if((devid >= MAROC_MAX_NUM) || !sockfd_event[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  result = ioctl(sockfd_event[devid], FIONREAD, &nbytes);
  if(result < 0)
    return ERROR;

  nwords = nbytes/4;
  if(nwords > nwords_max)
    nwords = nwords_max;

  if(nwords)
    read(sockfd_event[devid], buf, nwords*4);

  return nwords;
}

int marocEventBufferRead(int devid, int *buf, int nwords_max)
{
  int val, nwords = 0;
  while(nwords < nwords_max)
  {
    if(marocEventBufferSize[devid])
    {
      val = marocEventBuffer[devid][marocEventBufferRdPtr[devid]];
      if(nwords < nwords_max)
      {
        *buf++ = val;
        nwords++;
      }
      else
        printf("ERROR: %s - buffer full (nwords_max=%d)\n", __func__, nwords_max);

      marocEventBufferRdPtr[devid] = (marocEventBufferRdPtr[devid]+1) % MAROC_EVENT_BUF_LEN;
      marocEventBufferSize[devid]--;
      if( (val & 0xF8000000) == 0x88000000) // block trailer
      {
        marocEventBufferNBlocks[devid]--;
        break;
      }
    }
    else
    {
      printf("ERROR: %s(devid=%d) no end of event found\n", __func__, devid);
      break;
    }
  }
  return nwords;
}

void marocEventBufferWrite(int devid, int *buf, int nwords)
{
  int i, val;
  for(i=0; i<nwords; i++)
  {
    if(marocEventBufferSize[devid] < MAROC_EVENT_BUF_LEN)
    {
      val = *buf++;
      marocEventBuffer[devid][marocEventBufferWrPtr[devid]] = val;
      marocEventBufferWrPtr[devid] = (marocEventBufferWrPtr[devid]+1) % MAROC_EVENT_BUF_LEN;
      marocEventBufferSize[devid]++;
      if( (val & 0xF8000000) == 0x88000000) // block trailer
        marocEventBufferNBlocks[devid]++;
    }
    else
    {
      printf("ERROR: %s(devid=%d) - ROC event buffer full\n", __func__, devid);
      break;
    }
  }
}

int marocReadBlock(unsigned int *buf, int nwords_max)
{
  int i, devid, nwords=0, result, tries;
  for(i=0;i<nmaroc;i++)
  {
    devid = devids[i];
    tries = 0;

    while(1)
    {
      if(marocEventBufferNBlocks[devid])
      {
        result = marocEventBufferRead(devid, buf, nwords_max-nwords);
        nwords+= result;
        buf+= result;
        break;
      }
      else if(tries < 1000)
      {
        tries++;
        result = maroc_read_event_socket(devid, marocEventBufferWork, MAROC_EVENT_BUF_LEN-marocEventBufferSize[devid]);
        if(result>0)
          marocEventBufferWrite(devid, marocEventBufferWork, result);
        else
          usleep(10);
      }
      else
      {
        printf("ERROR: %s(devid=%d) - failed to receive event block\n", __func__, devid);
        break;
      }
    }
  }
  return nwords;
}

/******************************************************/

int marocGetNmaroc()
{
  return nmaroc;
}

int marocSlot(int n)
{
  return devids[n];
}

int marocInit(int devid_start, int n)
{
  int i, devid;

  nmaroc = 0;
  for(i=0;i<MAROC_MAX_NUM;i++)
  {
    sockfd_reg[i] = 0;
    sockfd_event[i] = 0;
    marocEventBufferWrPtr[i] = 0;
    marocEventBufferRdPtr[i] = 0;
    marocEventBufferSize[i] = 0;
    marocEventBufferNBlocks[i] = 0;
    marocASIC_num[i] = 0;
  }

  for(devid=devid_start; devid<(devid_start+n); devid++)
  {
    printf("\n%s: checking devid=%d ..\n",__func__,devid);fflush(stdout);
    if(maroc_open_register_socket(devid) == OK)
    {
      printf("%s: devid=%d opened !\n",__func__,devid);fflush(stdout);
      maroc_enable_trigger(devid, SD_SRC_SEL_0);
	    maroc_write32(devid, &pMAROC_regs->EvtBuilder.DeviceID, devid);
	    maroc_write32(devid, &pMAROC_regs->EvtBuilder.BlockCfg, 1);				/* 1 event per block */
      maroc_soft_reset(devid);
	    maroc_read32(devid, &pMAROC_regs->EvtBuilder.DeviceID);
      usleep(100);
      
      maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, 0);
      usleep(100);
      maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, 5);
      usleep(100);
//      maroc_enable_trigger(devid, SD_SRC_SEL_INPUT_1);
      maroc_write32(devid, &pMAROC_regs->Sd.OutSrc[0], 2); // coax output0 = OR 0
      maroc_write32(devid, &pMAROC_regs->Sd.OutSrc[1], 3); // coax output1 = OR 1
      
      if(maroc_open_event_socket(devid) != OK)
      {
        maroc_close_register_socket(devid);
      }
      else
      {
        devids[nmaroc++] = devid;
      }
    }
    else
    {
      printf("%s: devid=%d cannot be opened\n",__func__,devid);fflush(stdout);
    }
  }
  printf("\n%s: Found/connect to %d devices\n\n",__func__,nmaroc);fflush(stdout);

  return nmaroc;
}

void marocEnable()
{
  int i, devid;
  for(i=0;i<nmaroc;i++)
  {
    devid = marocSlot(i);

    maroc_write32(devid, &pMAROC_regs->Sd.SyncSrc, SD_SRC_SEL_1);
	  maroc_read32(devid, &pMAROC_regs->EvtBuilder.DeviceID);
    usleep(100);
    
    maroc_write32(devid, &pMAROC_regs->Sd.SyncSrc, SD_SRC_SEL_0);
	  maroc_read32(devid, &pMAROC_regs->EvtBuilder.DeviceID);
    usleep(100);
 
    maroc_enable_trigger(devid, SD_SRC_SEL_INPUT_1);
	  maroc_read32(devid, &pMAROC_regs->EvtBuilder.DeviceID);
  }
}

void marocEnd()
{
  int i;
  for(i=0;i<MAROC_MAX_NUM;i++)
  {
    if(sockfd_reg[i]) maroc_close_register_socket(i);
    if(sockfd_event[i]) maroc_close_event_socket(i);
  }
}

/*****************************************************************/
/*               Static Register Configuration Interface         */
/*****************************************************************/
int maroc_SetASICNum(int devid, int asic_num)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  marocASIC_num[devid] = asic_num;
  return OK;
}

int maroc_GetASICNum(int devid, int *asic_num)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  *asic_num = marocASIC_num[devid];
  return OK;
}

int maroc_SetLookback(int devid, int lookback)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  // convert lookback from ns to 8ns ticks
  lookback = lookback/8;
  
  if(lookback >= MAROC_EB_LOOKBACK_MAX)
    lookback = MAROC_EB_LOOKBACK_MAX;
  else if(lookback < 0)
    lookback = 0;

  maroc_write32(devid, &pMAROC_regs->EvtBuilder.Lookback, lookback);
  
  return OK;
}

int maroc_GetLookback(int devid, int *lookback)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  *lookback = maroc_read32(devid, &pMAROC_regs->EvtBuilder.Lookback) * 8;
  
  return OK;
}

int maroc_SetWindow(int devid, int window)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  // convert from ns to 8ns ticks
  window = window/8;
  
  if(window >= MAROC_EB_WINDOWWIDTH_MAX)
    window = MAROC_EB_WINDOWWIDTH_MAX;
  else if(window < 0)
    window = 0;

  maroc_write32(devid, &pMAROC_regs->EvtBuilder.WindowWidth, window);
  
  return OK;
}

int maroc_GetWindow(int devid, int *window)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  // convert from 8ns ticks to ns
  *window = maroc_read32(devid, &pMAROC_regs->EvtBuilder.WindowWidth) * 8;
  
  return OK;
}

int maroc_SetCTestAmplitude(int devid, int dac)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  if(dac >= MAROC_CFG_DAC_MAX)
    dac = MAROC_CFG_DAC_MAX;
  else if(dac < 0)
    dac = 0;

  maroc_write32(devid, &pMAROC_regs->Cfg.DACAmplitude, dac<<2);

  return OK;
}

int maroc_GetCTestAmplitude(int devid, int *dac)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  // convert from 8ns ticks to ns
  *dac = maroc_read32(devid, &pMAROC_regs->Cfg.DACAmplitude)>>2;
  
  return OK;
}

int maroc_SetCTestSource(int devid, int src)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  
  maroc_write32(devid, &pMAROC_regs->Sd.CTestSrc, src);
  
  return OK;
}

int maroc_GetCTestSource(int devid, int *src)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  
  *src = maroc_read32(devid, &pMAROC_regs->Sd.CTestSrc);
  
  return OK;
}

int maroc_SetTDCEnableChannelMask(
    int devid,
    int mask0, int mask1,
    int mask2, int mask3,
    int mask4, int mask5
  )
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  mask0 = ~mask0;
  mask1 = ~mask1;
  mask2 = ~mask2;
  mask3 = ~mask3;
  mask4 = ~mask4;
  mask5 = ~mask5;
  
  maroc_write32(devid, &pMAROC_regs->Proc[0].DisableCh[0], (mask0 & 0xFFFF) | ((mask0 & 0xFFFF)<<16));
  maroc_write32(devid, &pMAROC_regs->Proc[0].DisableCh[1], ((mask0 & 0xFFFF0000)>>16) | (mask0 & 0xFFFF0000));
  maroc_write32(devid, &pMAROC_regs->Proc[0].DisableCh[2], (mask1 & 0xFFFF) | ((mask1 & 0xFFFF)<<16));
  maroc_write32(devid, &pMAROC_regs->Proc[0].DisableCh[3], ((mask1 & 0xFFFF0000)>>16) | (mask1 & 0xFFFF0000));
  maroc_write32(devid, &pMAROC_regs->Proc[1].DisableCh[0], (mask2 & 0xFFFF) | ((mask2 & 0xFFFF)<<16));
  maroc_write32(devid, &pMAROC_regs->Proc[1].DisableCh[1], ((mask2 & 0xFFFF0000)>>16) | (mask2 & 0xFFFF0000));
  maroc_write32(devid, &pMAROC_regs->Proc[1].DisableCh[2], (mask3 & 0xFFFF) | ((mask3 & 0xFFFF)<<16));
  maroc_write32(devid, &pMAROC_regs->Proc[1].DisableCh[3], ((mask3 & 0xFFFF0000)>>16) | (mask3 & 0xFFFF0000));
  maroc_write32(devid, &pMAROC_regs->Proc[2].DisableCh[0], (mask4 & 0xFFFF) | ((mask4 & 0xFFFF)<<16));
  maroc_write32(devid, &pMAROC_regs->Proc[2].DisableCh[1], ((mask4 & 0xFFFF0000)>>16) | (mask4 & 0xFFFF0000));
  maroc_write32(devid, &pMAROC_regs->Proc[2].DisableCh[2], (mask5 & 0xFFFF) | ((mask5 & 0xFFFF)<<16));
  maroc_write32(devid, &pMAROC_regs->Proc[2].DisableCh[3], ((mask5 & 0xFFFF0000)>>16) | (mask5 & 0xFFFF0000));
  
  return OK;
}

int maroc_GetTDCEnableChannelMask(
    int devid,
    int *mask0, int *mask1,
    int *mask2, int *mask3,
    int *mask4, int *mask5
  )
{
  unsigned int val0, val1;
  
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  
  val0 = maroc_read32(devid, &pMAROC_regs->Proc[0].DisableCh[0]);
  val1 = maroc_read32(devid, &pMAROC_regs->Proc[0].DisableCh[1]);
  *mask0 = ((val0>>0) & 0xFFFF)     | ((val0>>16) & 0xFFFF) |
           ((val1<<0) & 0xFFFF0000) | ((val1<<16) & 0xFFFF0000);
  
  val0 = maroc_read32(devid, &pMAROC_regs->Proc[0].DisableCh[2]);
  val1 = maroc_read32(devid, &pMAROC_regs->Proc[0].DisableCh[3]);
  *mask1 = ((val0>>0) & 0xFFFF)     | ((val0>>16) & 0xFFFF) |
           ((val1<<0) & 0xFFFF0000) | ((val1<<16) & 0xFFFF0000);

  val0 = maroc_read32(devid, &pMAROC_regs->Proc[1].DisableCh[0]);
  val1 = maroc_read32(devid, &pMAROC_regs->Proc[1].DisableCh[1]);
  *mask2 = ((val0>>0) & 0xFFFF)     | ((val0>>16) & 0xFFFF) |
           ((val1<<0) & 0xFFFF0000) | ((val1<<16) & 0xFFFF0000);
  
  val0 = maroc_read32(devid, &pMAROC_regs->Proc[1].DisableCh[2]);
  val1 = maroc_read32(devid, &pMAROC_regs->Proc[1].DisableCh[3]);
  *mask3 = ((val0>>0) & 0xFFFF)     | ((val0>>16) & 0xFFFF) |
           ((val1<<0) & 0xFFFF0000) | ((val1<<16) & 0xFFFF0000);

  val0 = maroc_read32(devid, &pMAROC_regs->Proc[2].DisableCh[0]);
  val1 = maroc_read32(devid, &pMAROC_regs->Proc[2].DisableCh[1]);
  *mask4 = ((val0>>0) & 0xFFFF)     | ((val0>>16) & 0xFFFF) |
           ((val1<<0) & 0xFFFF0000) | ((val1<<16) & 0xFFFF0000);
  
  val0 = maroc_read32(devid, &pMAROC_regs->Proc[2].DisableCh[2]);
  val1 = maroc_read32(devid, &pMAROC_regs->Proc[2].DisableCh[3]);
  *mask5 = ((val0>>0) & 0xFFFF)     | ((val0>>16) & 0xFFFF) |
           ((val1<<0) & 0xFFFF0000) | ((val1<<16) & 0xFFFF0000);

  return OK;
}

int maroc_SetDeviceId(int devid, int id)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  maroc_write32(devid, &pMAROC_regs->EvtBuilder.DeviceID, id);
  
  return OK;
}

int maroc_PrintMarocRegs(int devid, int chip, int type)
{
  int i;
  MAROC_ASIC_regs_t *pRegs;
  
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  
  if(chip < 0 || chip > 2)
  {
    printf("%s: ERROR invalid chip specified: %d\n", __func__, chip);
    return ERROR;
  }

  if(type == RICH_MAROC_REGS_WR)
    pRegs = &marocASIC[devid][chip];
  else if(type == RICH_MAROC_REGS_RD)
    pRegs = &marocASIC_Rd[devid][chip];
  else
  {
    printf("%s: ERROR - unknown type specified %d\n", __func__, type);
    return ERROR;
  }

  printf("%s: MAROC Register Dump (Slot %d, Chip %d, %s):\n",
         __func__, devid, chip, (type == RICH_MAROC_REGS_WR) ? "Write" : "Read");
  
  printf("Global0 = 0x%08X \n", pRegs->Global0.val);

  printf("   cmd_fsu           = %d\n", pRegs->Global0.bits.cmd_fsu);
  printf("   cmd_ss            = %d\n", pRegs->Global0.bits.cmd_ss);
  printf("   cmd_fsb           = %d\n", pRegs->Global0.bits.cmd_fsb);
  printf("   swb_buf_250f      = %d\n", pRegs->Global0.bits.swb_buf_250f);
  printf("   swb_buf_500f      = %d\n", pRegs->Global0.bits.swb_buf_500f);
  printf("   swb_buf_1p        = %d\n", pRegs->Global0.bits.swb_buf_1p);
  printf("   swb_buf_2p        = %d\n", pRegs->Global0.bits.swb_buf_2p);
  printf("   ONOFF_ss          = %d\n", pRegs->Global0.bits.ONOFF_ss);
  printf("   sw_ss_300f        = %d\n", pRegs->Global0.bits.sw_ss_300f);
  printf("   sw_ss_600f        = %d\n", pRegs->Global0.bits.sw_ss_600f);
  printf("   sw_ss_1200f       = %d\n", pRegs->Global0.bits.sw_ss_1200f);
  printf("   EN_ADC            = %d\n", pRegs->Global0.bits.EN_ADC);
  printf("   H1H2_choice       = %d\n", pRegs->Global0.bits.H1H2_choice);
  printf("   sw_fsu_20f        = %d\n", pRegs->Global0.bits.sw_fsu_20f);
  printf("   sw_fsu_40f        = %d\n", pRegs->Global0.bits.sw_fsu_40f);
  printf("   sw_fsu_25k        = %d\n", pRegs->Global0.bits.sw_fsu_25k);
  printf("   sw_fsu_50k        = %d\n", pRegs->Global0.bits.sw_fsu_50k);
  printf("   sw_fsu_100k       = %d\n", pRegs->Global0.bits.sw_fsu_100k);
  printf("   sw_fsb1_50k       = %d\n", pRegs->Global0.bits.sw_fsb1_50k);
  printf("   sw_fsb1_100k      = %d\n", pRegs->Global0.bits.sw_fsb1_100k);
  printf("   sw_fsb1_100f      = %d\n", pRegs->Global0.bits.sw_fsb1_100f);
  printf("   sw_fsb1_50f       = %d\n", pRegs->Global0.bits.sw_fsb1_50f);
  printf("   cmd_fsb_fsu       = %d\n", pRegs->Global0.bits.cmd_fsb_fsu);
  printf("   valid_dc_fs       = %d\n", pRegs->Global0.bits.valid_dc_fs);
  printf("   sw_fsb2_50k       = %d\n", pRegs->Global0.bits.sw_fsb2_50k);
  printf("   sw_fsb2_100k      = %d\n", pRegs->Global0.bits.sw_fsb2_100k);
  printf("   sw_fsb2_100f      = %d\n", pRegs->Global0.bits.sw_fsb2_100f);
  printf("   sw_fsb2_50f       = %d\n", pRegs->Global0.bits.sw_fsb2_50f);
  printf("   valid_dc_fsb2     = %d\n", pRegs->Global0.bits.valid_dc_fsb2);
  printf("   ENb_tristate      = %d\n", pRegs->Global0.bits.ENb_tristate);
  printf("   polar_discri      = %d\n", pRegs->Global0.bits.polar_discri);
  printf("   inv_discriADC     = %d\n", pRegs->Global0.bits.inv_discriADC);

  printf("Global1 = 0x%08X\n", pRegs->Global1.val);
  printf("   d1_d2             = %d\n", pRegs->Global1.bits.d1_d2);
  printf("   cmd_CK_mux        = %d\n", pRegs->Global1.bits.cmd_CK_mux);
  printf("   ONOFF_otabg       = %d\n", pRegs->Global1.bits.ONOFF_otabg);
  printf("   ONOFF_dac         = %d\n", pRegs->Global1.bits.ONOFF_dac);
  printf("   small_dac         = %d\n", pRegs->Global1.bits.small_dac);
  printf("   enb_outADC        = %d\n", pRegs->Global1.bits.enb_outADC);
  printf("   inv_startCmptGray = %d\n", pRegs->Global1.bits.inv_startCmptGray);
  printf("   ramp_8bit         = %d\n", pRegs->Global1.bits.ramp_8bit);
  printf("   ramp_10bit        = %d\n", pRegs->Global1.bits.ramp_10bit);
  printf("DAC = 0x%08X\n", pRegs->DAC.val);
  printf("   DAC0              = %d\n", pRegs->DAC.bits.DAC0);
  printf("   DAC1              = %d\n", pRegs->DAC.bits.DAC1);
    
  printf("Channels:\n");
  printf("%7s%7s%7s%7s%7s\n", "CH", "Gain", "Sum", "CTest", "MaskOr");
  for(i = 0; i < 64; i++)
  {
    if(i & 0x1)
      printf("%7d%7d%7d%7d%7d\n", i,
        pRegs->CH[i>>1].bits.Gain0, pRegs->CH[i>>1].bits.Sum0,
        pRegs->CH[i>>1].bits.CTest0, pRegs->CH[i>>1].bits.MaskOr0);
    else
      printf("%7d%7d%7d%7d%7d\n", i,
        pRegs->CH[i>>1].bits.Gain1, pRegs->CH[i>>1].bits.Sum1,
        pRegs->CH[i>>1].bits.CTest1, pRegs->CH[i>>1].bits.MaskOr1);
  }
  printf("\n");
  
  return OK;
}

int maroc_SetMarocReg(int devid, int chip, int reg, int channel, int val)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  
  if(chip < 0 || chip > 2)
  {
    printf("%s: ERROR invalid chip specified: %d\n", __func__, chip);
    return ERROR;
  }

  if(channel < 0 || channel > 64)
  {
    printf("%s: ERROR invalid channel specified: %d\n", __func__, channel);
    return ERROR;
  }

  switch(reg)
  {
    case MAROC_REG_GLOBAL0:
      marocASIC[devid][chip].Global0.val = val;
      break;
    case MAROC_REG_GLOBAL1:
      marocASIC[devid][chip].Global1.val = val;
      break;
    case MAROC_REG_CMD_FSU:
      marocASIC[devid][chip].Global0.bits.cmd_fsu = val;
      break;
    case MAROC_REG_CMD_SS:
      marocASIC[devid][chip].Global0.bits.cmd_ss = val;
      break;
    case MAROC_REG_CMD_FSB:
      marocASIC[devid][chip].Global0.bits.cmd_fsb = val;
      break;
    case MAROC_REG_SWB_BUF_250F:
      marocASIC[devid][chip].Global0.bits.swb_buf_250f = val;
      break;
    case MAROC_REG_SWB_BUF_500F:
      marocASIC[devid][chip].Global0.bits.swb_buf_500f = val;
      break;
    case MAROC_REG_SWB_BUF_1P:
      marocASIC[devid][chip].Global0.bits.swb_buf_1p = val;
      break;
    case MAROC_REG_SWB_BUF_2P:
      marocASIC[devid][chip].Global0.bits.swb_buf_2p = val;
      break;
    case MAROC_REG_ONOFF_SS:
      marocASIC[devid][chip].Global0.bits.ONOFF_ss = val;
      break;
    case MAROC_REG_SW_SS_300F:
      marocASIC[devid][chip].Global0.bits.sw_ss_300f = val;
      break;
    case MAROC_REG_SW_SS_600F:
      marocASIC[devid][chip].Global0.bits.sw_ss_600f = val;
      break;
    case MAROC_REG_SW_SS1200F:
      marocASIC[devid][chip].Global0.bits.sw_ss_1200f = val;
      break;
    case MAROC_REG_EN_ADC:
      marocASIC[devid][chip].Global0.bits.EN_ADC = val;
      break;
    case MAROC_REG_H1H2_CHOICE:
      marocASIC[devid][chip].Global0.bits.H1H2_choice = val;
      break;
    case MAROC_REG_SW_FSU_20F:
      marocASIC[devid][chip].Global0.bits.sw_fsu_20f = val;
      break;
    case MAROC_REG_SW_FSU_40F:
      marocASIC[devid][chip].Global0.bits.sw_fsu_40f = val;
      break;
    case MAROC_REG_SW_FSU_25K:
      marocASIC[devid][chip].Global0.bits.sw_fsu_25k = val;
      break;
    case MAROC_REG_SW_FSU_50K:
      marocASIC[devid][chip].Global0.bits.sw_fsu_50k = val;
      break;
    case MAROC_REG_SW_FSU_100K:
      marocASIC[devid][chip].Global0.bits.sw_fsu_100k = val;
      break;
    case MAROC_REG_SW_FSB1_50K:
      marocASIC[devid][chip].Global0.bits.sw_fsb1_50k = val;
      break;
    case MAROC_REG_SW_FSB1_100K:
      marocASIC[devid][chip].Global0.bits.sw_fsb1_100k = val;
      break;
    case MAROC_REG_SW_FSB1_100F:
      marocASIC[devid][chip].Global0.bits.sw_fsb1_100f = val;
      break;
    case MAROC_REG_SW_FSB1_50F:
      marocASIC[devid][chip].Global0.bits.sw_fsb1_50f = val;
      break;
    case MAROC_REG_CMD_FSB_FSU:
      marocASIC[devid][chip].Global0.bits.cmd_fsb_fsu = val;
      break;
    case MAROC_REG_VALID_DC_FS:
      marocASIC[devid][chip].Global0.bits.valid_dc_fs = val;
      break;
    case MAROC_REG_SW_FSB2_50K:
      marocASIC[devid][chip].Global0.bits.sw_fsb2_50k = val;
      break;
    case MAROC_REG_SW_FSB2_100K:
      marocASIC[devid][chip].Global0.bits.sw_fsb2_100k = val;
      break;
    case MAROC_REG_SW_FSB2_100F:
      marocASIC[devid][chip].Global0.bits.sw_fsb2_100f = val;
      break;
    case MAROC_REG_SW_FSB2_50F:
      marocASIC[devid][chip].Global0.bits.sw_fsb2_50f = val;
      break;
    case MAROC_REG_VALID_DC_FSB2:
      marocASIC[devid][chip].Global0.bits.valid_dc_fsb2 = val;
      break;
    case MAROC_REG_ENB_TRISTATE:
      marocASIC[devid][chip].Global0.bits.ENb_tristate = val;
      break;
    case MAROC_REG_POLAR_DISCRI:
      marocASIC[devid][chip].Global0.bits.polar_discri = val;
      break;
    case MAROC_REG_INV_DISCRIADC:
      marocASIC[devid][chip].Global0.bits.inv_discriADC = val;
      break;
    case MAROC_REG_D1_D2:
      marocASIC[devid][chip].Global1.bits.d1_d2 = val;
      break;
    case MAROC_REG_CMD_CK_MUX:
      marocASIC[devid][chip].Global1.bits.cmd_CK_mux = val;
      break;
    case MAROC_REG_ONOFF_OTABG:
      marocASIC[devid][chip].Global1.bits.ONOFF_otabg = val;
      break;
    case MAROC_REG_ONOFF_DAC:
      marocASIC[devid][chip].Global1.bits.ONOFF_dac = val;
      break;
    case MAROC_REG_SMALL_DAC:
      marocASIC[devid][chip].Global1.bits.small_dac = val;
      break;
    case MAROC_REG_ENB_OUTADC:
      marocASIC[devid][chip].Global1.bits.enb_outADC = val;
      break;
    case MAROC_REG_INV_STARTCMPTGRAY:
      marocASIC[devid][chip].Global1.bits.inv_startCmptGray = val;
      break;
    case MAROC_REG_RAMP_8BIT:
      marocASIC[devid][chip].Global1.bits.ramp_8bit = val;
      break;
    case MAROC_REG_RAMP_10BIT:
      marocASIC[devid][chip].Global1.bits.ramp_10bit = val;
      break;
    case MAROC_REG_DAC0:
      marocASIC[devid][chip].DAC.bits.DAC0 = val;
      break;
    case MAROC_REG_DAC1:
      marocASIC[devid][chip].DAC.bits.DAC1 = val;
      break;
    case MAROC_REG_GAIN:
      if(!(channel & 0x1))
        marocASIC[devid][chip].CH[channel>>1].bits.Gain0 = val;
      else
        marocASIC[devid][chip].CH[channel>>1].bits.Gain1 = val;
      break;
    case MAROC_REG_SUM:
      if(!(channel & 0x1))
        marocASIC[devid][chip].CH[channel>>1].bits.Sum0 = val;
      else
        marocASIC[devid][chip].CH[channel>>1].bits.Sum1 = val;
      break;
    case MAROC_REG_CTEST:
      if(!(channel & 0x1))
        marocASIC[devid][chip].CH[channel>>1].bits.CTest0 = val;
      else
        marocASIC[devid][chip].CH[channel>>1].bits.CTest1 = val;
      break;
    case MAROC_REG_MASKOR:
      if(!(channel & 0x1))
        marocASIC[devid][chip].CH[channel>>1].bits.MaskOr0 = val;
      else
        marocASIC[devid][chip].CH[channel>>1].bits.MaskOr1 = val;
      break;
    default:
      printf("%s: ERROR invalid register specified: %d\n", __func__, reg);
      return ERROR;
  }
  return OK;
}

int maroc_GetMarocReg(int devid, int chip, int reg, int channel, int *val)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  
  if(chip < 0 || chip > 2)
  {
    printf("%s: ERROR invalid chip specified: %d\n", __func__, chip);
    return ERROR;
  }

  if(channel < 0 || channel > 64)
  {
    printf("%s: ERROR invalid channel specified: %d\n", __func__, channel);
    return ERROR;
  }

  switch(reg)
  {
    case MAROC_REG_GLOBAL0:
      *val = marocASIC_Rd[devid][chip].Global0.val;
      break;
    case MAROC_REG_GLOBAL1:
      *val = marocASIC_Rd[devid][chip].Global1.val;
      break;
    case MAROC_REG_CMD_FSU:
      *val = marocASIC_Rd[devid][chip].Global0.bits.cmd_fsu;
      break;
    case MAROC_REG_CMD_SS:
      *val = marocASIC_Rd[devid][chip].Global0.bits.cmd_ss;
      break;
    case MAROC_REG_CMD_FSB:
      *val = marocASIC_Rd[devid][chip].Global0.bits.cmd_fsb;
      break;
    case MAROC_REG_SWB_BUF_250F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.swb_buf_250f;
      break;
    case MAROC_REG_SWB_BUF_500F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.swb_buf_500f;
      break;
    case MAROC_REG_SWB_BUF_1P:
      *val = marocASIC_Rd[devid][chip].Global0.bits.swb_buf_1p;
      break;
    case MAROC_REG_SWB_BUF_2P:
      *val = marocASIC_Rd[devid][chip].Global0.bits.swb_buf_2p;
      break;
    case MAROC_REG_ONOFF_SS:
      *val = marocASIC_Rd[devid][chip].Global0.bits.ONOFF_ss;
      break;
    case MAROC_REG_SW_SS_300F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_ss_300f;
      break;
    case MAROC_REG_SW_SS_600F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_ss_600f;
      break;
    case MAROC_REG_SW_SS1200F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_ss_1200f;
      break;
    case MAROC_REG_EN_ADC:
      *val = marocASIC_Rd[devid][chip].Global0.bits.EN_ADC;
      break;
    case MAROC_REG_H1H2_CHOICE:
      *val = marocASIC_Rd[devid][chip].Global0.bits.H1H2_choice;
      break;
    case MAROC_REG_SW_FSU_20F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsu_20f;
      break;
    case MAROC_REG_SW_FSU_40F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsu_40f;
      break;
    case MAROC_REG_SW_FSU_25K:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsu_25k;
      break;
    case MAROC_REG_SW_FSU_50K:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsu_50k;
      break;
    case MAROC_REG_SW_FSU_100K:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsu_100k;
      break;
    case MAROC_REG_SW_FSB1_50K:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb1_50k;
      break;
    case MAROC_REG_SW_FSB1_100K:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb1_100k;
      break;
    case MAROC_REG_SW_FSB1_100F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb1_100f;
      break;
    case MAROC_REG_SW_FSB1_50F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb1_50f;
      break;
    case MAROC_REG_CMD_FSB_FSU:
      *val = marocASIC_Rd[devid][chip].Global0.bits.cmd_fsb_fsu;
      break;
    case MAROC_REG_VALID_DC_FS:
      *val = marocASIC_Rd[devid][chip].Global0.bits.valid_dc_fs;
      break;
    case MAROC_REG_SW_FSB2_50K:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb2_50k;
      break;
    case MAROC_REG_SW_FSB2_100K:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb2_100k;
      break;
    case MAROC_REG_SW_FSB2_100F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb2_100f;
      break;
    case MAROC_REG_SW_FSB2_50F:
      *val = marocASIC_Rd[devid][chip].Global0.bits.sw_fsb2_50f;
      break;
    case MAROC_REG_VALID_DC_FSB2:
      *val = marocASIC_Rd[devid][chip].Global0.bits.valid_dc_fsb2;
      break;
    case MAROC_REG_ENB_TRISTATE:
      *val = marocASIC_Rd[devid][chip].Global0.bits.ENb_tristate;
      break;
    case MAROC_REG_POLAR_DISCRI:
      *val = marocASIC_Rd[devid][chip].Global0.bits.polar_discri;
      break;
    case MAROC_REG_INV_DISCRIADC:
      *val = marocASIC_Rd[devid][chip].Global0.bits.inv_discriADC;
      break;
    case MAROC_REG_D1_D2:
      *val = marocASIC_Rd[devid][chip].Global1.bits.d1_d2;
      break;
    case MAROC_REG_CMD_CK_MUX:
      *val = marocASIC_Rd[devid][chip].Global1.bits.cmd_CK_mux;
      break;
    case MAROC_REG_ONOFF_OTABG:
      *val = marocASIC_Rd[devid][chip].Global1.bits.ONOFF_otabg;
      break;
    case MAROC_REG_ONOFF_DAC:
      *val = marocASIC_Rd[devid][chip].Global1.bits.ONOFF_dac;
      break;
    case MAROC_REG_SMALL_DAC:
      *val = marocASIC_Rd[devid][chip].Global1.bits.small_dac;
      break;
    case MAROC_REG_ENB_OUTADC:
      *val = marocASIC_Rd[devid][chip].Global1.bits.enb_outADC;
      break;
    case MAROC_REG_INV_STARTCMPTGRAY:
      *val = marocASIC_Rd[devid][chip].Global1.bits.inv_startCmptGray;
      break;
    case MAROC_REG_RAMP_8BIT:
      *val = marocASIC_Rd[devid][chip].Global1.bits.ramp_8bit;
      break;
    case MAROC_REG_RAMP_10BIT:
      *val = marocASIC_Rd[devid][chip].Global1.bits.ramp_10bit;
      break;
    case MAROC_REG_DAC0:
      *val = marocASIC_Rd[devid][chip].DAC.bits.DAC0;
      break;
    case MAROC_REG_DAC1:
      *val = marocASIC_Rd[devid][chip].DAC.bits.DAC1;
      break;
    case MAROC_REG_GAIN:
      if(!(channel & 0x1))
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.Gain0;
      else
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.Gain1;
      break;
    case MAROC_REG_SUM:
      if(!(channel & 0x1))
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.Sum0;
      else
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.Sum1;
      break;
    case MAROC_REG_CTEST:
      if(!(channel & 0x1))
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.CTest0;
      else
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.CTest1;
      break;
    case MAROC_REG_MASKOR:
      if(!(channel & 0x1))
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.MaskOr0;
      else
        *val = marocASIC_Rd[devid][chip].CH[channel>>1].bits.MaskOr1;
      break;
    default:
      printf("%s: ERROR invalid register specified: %d\n", __func__, reg);
      return ERROR;
  }
  return OK;
}

int maroc_UpdateMarocRegs(int devid)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }
  
  if(marocASIC_num[devid] == MAROC_ASIC_TYPE_2MAROC)
  {
    maroc_ShiftMarocRegs(devid, &marocASIC[devid][2], &marocASIC_Rd[devid][2]);
    maroc_ShiftMarocRegs(devid, &marocASIC[devid][0], &marocASIC_Rd[devid][0]);
  }
  else if(marocASIC_num[devid] == MAROC_ASIC_TYPE_3MAROC)
  {
    maroc_ShiftMarocRegs(devid, &marocASIC[devid][2], &marocASIC_Rd[devid][2]);
    maroc_ShiftMarocRegs(devid, &marocASIC[devid][1], &marocASIC_Rd[devid][1]);
    maroc_ShiftMarocRegs(devid, &marocASIC[devid][0], &marocASIC_Rd[devid][0]);
  }
  else
  {
    printf("%s: ERROR - unknown type specified %d for devid=%d\n", __func__, marocASIC_num[devid], devid);
    return ERROR;
  }
  return OK;
}

int maroc_ShiftMarocRegs(int devid, MAROC_ASIC_regs_t *regs_in, MAROC_ASIC_regs_t *regs_out)
{
  int i, val;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return ERROR;
  }

  // Write 1 set of MAROC registers into shift register
  maroc_write32(devid, &pMAROC_regs->Cfg.Regs.Global0.val, regs_in->Global0.val);
  maroc_write32(devid, &pMAROC_regs->Cfg.Regs.Global1.val, regs_in->Global1.val);
  maroc_write32(devid, &pMAROC_regs->Cfg.Regs.DAC.val, regs_in->DAC.val);

  for(i = 0; i < 32; i++)
    maroc_write32(devid, &pMAROC_regs->Cfg.Regs.CH[i].val, regs_in->CH[i].val);

  // Perform shift operation for 1 set of MAROC registers
  val = maroc_read32(devid, &pMAROC_regs->Cfg.SerCtrl);
  val |= 0x00000002;
  maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, val);

  // Check for shift register transfer completion
  for(i = 10; i > 0; i--)
  {
    val = maroc_read32(devid, &pMAROC_regs->Cfg.SerStatus);
    usleep(1000);
    
    if(!(val & 0x00000001))
      break;
    
    if(!i)
    {
      printf("%s: MAROC devid %d - timeout on serial transfer\n", __func__, devid);
      return ERROR;
    }
    usleep(100);
  }
  regs_out->Global0.val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.Global0.val);
  regs_out->Global1.val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.Global1.val);
  regs_out->DAC.val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.DAC.val);

  for(i = 0; i < 32; i++)
    regs_out->CH[i].val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.CH[i].val);
  
  return OK;
}




















#if 0

void maroc_clear_regs(int devid)
{
	int val;
  
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }

	/* set rst_sc low */
	val = maroc_read32(devid, &pMAROC_regs->Cfg.SerCtrl);
	val &= 0xFFFFFFFE;
	maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, val);

	/* set rst_sc high */
	val |= 0x00000001;
	maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, val);
}

MAROC_Regs maroc_shift_regs(int devid, MAROC_Regs regs)
{
	MAROC_Regs result;
	int i, val;
  
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return regs;
  }
	
	/* write settings to FPGA shift register */
	maroc_write32(devid, &pMAROC_regs->Cfg.Regs.Global0.val, regs.Global0.val);
	maroc_write32(devid, &pMAROC_regs->Cfg.Regs.Global1.val, regs.Global1.val);
	maroc_write32(devid, &pMAROC_regs->Cfg.Regs.DAC.val,     regs.DAC.val);
	
	for(i = 0; i < 32; i++)
		maroc_write32(devid, &pMAROC_regs->Cfg.Regs.CH[i].val, regs.CH[i].val);
	
	/* do shift register transfer */
	val = maroc_read32(devid, &pMAROC_regs->Cfg.SerCtrl);
	val |= 0x00000002;
	maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, val);
	
	/* check for shift register transfer completion */
	for(i = 10; i > 0; i--)
	{
		val = maroc_read32(devid, &pMAROC_regs->Cfg.SerStatus);
		if(!(val & 0x00000001))
			break;
		
		if(!i)
			printf("Error in %s: timeout on serial transfer...\n", __FUNCTION__);

		usleep(100);
	}

	/* read back settings from FPGA shift register */
	result.Global0.val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.Global0.val);
	result.Global1.val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.Global1.val);
	result.DAC.val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.DAC.val);
	
	for(i = 0; i < 32; i++)
		result.CH[i].val = maroc_read32(devid, &pMAROC_regs->Cfg.Regs.CH[i].val);
	
	return result;
}

void maroc_print_regs(MAROC_Regs regs)
{
	int i;
	
	printf("Global0 = 0x%08X\n", regs.Global0.val);
	printf("   cmd_fsu           = %d\n", regs.Global0.bits.cmd_fsu);
	printf("   cmd_ss            = %d\n", regs.Global0.bits.cmd_ss);
	printf("   cmd_fsb           = %d\n", regs.Global0.bits.cmd_fsb);
	printf("   swb_buf_250f      = %d\n", regs.Global0.bits.swb_buf_250f);
	printf("   swb_buf_500f      = %d\n", regs.Global0.bits.swb_buf_500f);
	printf("   swb_buf_1p        = %d\n", regs.Global0.bits.swb_buf_1p);
	printf("   swb_buf_2p        = %d\n", regs.Global0.bits.swb_buf_2p);
	printf("   ONOFF_ss          = %d\n", regs.Global0.bits.ONOFF_ss);
	printf("   sw_ss_300f        = %d\n", regs.Global0.bits.sw_ss_300f);
	printf("   sw_ss_600f        = %d\n", regs.Global0.bits.sw_ss_600f);
	printf("   sw_ss_1200f       = %d\n", regs.Global0.bits.sw_ss_1200f);
	printf("   EN_ADC            = %d\n", regs.Global0.bits.EN_ADC);
	printf("   H1H2_choice       = %d\n", regs.Global0.bits.H1H2_choice);
	printf("   sw_fsu_20f        = %d\n", regs.Global0.bits.sw_fsu_20f);
	printf("   sw_fsu_40f        = %d\n", regs.Global0.bits.sw_fsu_40f);
	printf("   sw_fsu_25k        = %d\n", regs.Global0.bits.sw_fsu_25k);
	printf("   sw_fsu_50k        = %d\n", regs.Global0.bits.sw_fsu_50k);
	printf("   sw_fsu_100k       = %d\n", regs.Global0.bits.sw_fsu_100k);
	printf("   sw_fsb1_50k       = %d\n", regs.Global0.bits.sw_fsb1_50k);
	printf("   sw_fsb1_100k      = %d\n", regs.Global0.bits.sw_fsb1_100k);
	printf("   sw_fsb1_100f      = %d\n", regs.Global0.bits.sw_fsb1_100f);
	printf("   sw_fsb1_50f       = %d\n", regs.Global0.bits.sw_fsb1_50f);
	printf("   cmd_fsb_fsu       = %d\n", regs.Global0.bits.cmd_fsb_fsu);
	printf("   valid_dc_fs       = %d\n", regs.Global0.bits.valid_dc_fs);
	printf("   sw_fsb2_50k       = %d\n", regs.Global0.bits.sw_fsb2_50k);
	printf("   sw_fsb2_100k      = %d\n", regs.Global0.bits.sw_fsb2_100k);
	printf("   sw_fsb2_100f      = %d\n", regs.Global0.bits.sw_fsb2_100f);
	printf("   sw_fsb2_50f       = %d\n", regs.Global0.bits.sw_fsb2_50f);
	printf("   valid_dc_fsb2     = %d\n", regs.Global0.bits.valid_dc_fsb2);
	printf("   ENb_tristate      = %d\n", regs.Global0.bits.ENb_tristate);
	printf("   polar_discri      = %d\n", regs.Global0.bits.polar_discri);
	printf("   inv_discriADC     = %d\n", regs.Global0.bits.inv_discriADC);

	printf("Global1 = 0x%08X\n", regs.Global1.val);
	printf("   d1_d2             = %d\n", regs.Global1.bits.d1_d2);
	printf("   cmd_CK_mux        = %d\n", regs.Global1.bits.cmd_CK_mux);
	printf("   ONOFF_otabg       = %d\n", regs.Global1.bits.ONOFF_otabg);
	printf("   ONOFF_dac         = %d\n", regs.Global1.bits.ONOFF_dac);
	printf("   small_dac         = %d\n", regs.Global1.bits.small_dac);
	printf("   enb_outADC        = %d\n", regs.Global1.bits.enb_outADC);
	printf("   inv_startCmptGray = %d\n", regs.Global1.bits.inv_startCmptGray);
	printf("   ramp_8bit         = %d\n", regs.Global1.bits.ramp_8bit);
	printf("   ramp_10bit        = %d\n", regs.Global1.bits.ramp_10bit);

	printf("DAC = 0x%08X\n", regs.DAC.val);
	printf("   DAC0              = %d\n", regs.DAC.bits.DAC0);
	printf("   DAC1              = %d\n", regs.DAC.bits.DAC1);

	printf("Channels:\n");
	printf("%7s%7s%7s%7s%7s\n", "CH", "Gain", "Sum", "CTest", "MaskOr");
	for(i = 0; i < 64; i++)
	{
		if(i & 0x1)
			printf("%7d%7d%7d%7d%7d\n", i,
					 regs.CH[i>>1].bits.Gain0, regs.CH[i>>1].bits.Sum0,
					 regs.CH[i>>1].bits.CTest0, regs.CH[i>>1].bits.MaskOr0);
		else
			printf("%7d%7d%7d%7d%7d\n", i,
					 regs.CH[i>>1].bits.Gain1, regs.CH[i>>1].bits.Sum1,
					 regs.CH[i>>1].bits.CTest1, regs.CH[i>>1].bits.MaskOr1);
	}
	printf("\n");
}


void maroc_init_regs(MAROC_Regs *regs, int thr, int gains[64])
{
	int i;

	memset(regs, 0, sizeof(MAROC_Regs));

	regs->Global0.bits.cmd_fsu = 1;
	regs->Global0.bits.cmd_ss = 1;
	regs->Global0.bits.cmd_fsb = 1;
	regs->Global0.bits.swb_buf_250f = 0;
	regs->Global0.bits.swb_buf_500f = 0;
	regs->Global0.bits.swb_buf_1p = 0;
	regs->Global0.bits.swb_buf_2p = 0;
	regs->Global0.bits.ONOFF_ss = 1;
	regs->Global0.bits.sw_ss_300f = 1;
	regs->Global0.bits.sw_ss_600f = 1;
	regs->Global0.bits.sw_ss_1200f = 0;
	regs->Global0.bits.EN_ADC = 1;	// enable ADC
//regs->Global0.bits.EN_ADC = 0;	// disable ADC
	regs->Global0.bits.H1H2_choice = 0;
	regs->Global0.bits.sw_fsu_20f = 1;
	regs->Global0.bits.sw_fsu_40f = 1;
	regs->Global0.bits.sw_fsu_25k = 0;
	regs->Global0.bits.sw_fsu_50k = 0;
	regs->Global0.bits.sw_fsu_100k = 0;
	regs->Global0.bits.sw_fsb1_50k = 0;
	regs->Global0.bits.sw_fsb1_100k = 0;
	regs->Global0.bits.sw_fsb1_100f = 1;
	regs->Global0.bits.sw_fsb1_50f = 1;
	regs->Global0.bits.cmd_fsb_fsu = 0;
	regs->Global0.bits.valid_dc_fs = 1;
	regs->Global0.bits.sw_fsb2_50k = 0;
	regs->Global0.bits.sw_fsb2_100k = 0;
	regs->Global0.bits.sw_fsb2_100f = 0;
	regs->Global0.bits.sw_fsb2_50f = 1;
	regs->Global0.bits.valid_dc_fsb2 = 0;
	regs->Global0.bits.ENb_tristate = 1;
	regs->Global0.bits.polar_discri = 0;
	regs->Global0.bits.inv_discriADC = 0;
	regs->Global1.bits.d1_d2 = 0;
	regs->Global1.bits.cmd_CK_mux = 0;
	regs->Global1.bits.ONOFF_otabg = 0;
	regs->Global1.bits.ONOFF_dac = 0;
	regs->Global1.bits.small_dac = 0; /* 0=2.3mV/DAC LSB, 1=1.1mV/DAC LSB */
	regs->Global1.bits.enb_outADC = 0;
//	regs->Global1.bits.enb_outADC = 1;
	regs->Global1.bits.inv_startCmptGray = 0;
	regs->Global1.bits.ramp_8bit = 0;
	regs->Global1.bits.ramp_10bit = 0;
//	regs->DAC.bits.DAC0 = 300; /* with small_dac = 0,  pedestal < ~200, signal ~200 to ~500, 500fC/pulse injected */
	regs->DAC.bits.DAC0 = thr; /* with small_dac = 0,  pedestal < ~200, signal ~200 to ~500, 500fC/pulse injected */
	regs->DAC.bits.DAC1 = 0;


	for(i = 0; i < 64; i++)
	{
		int ctest = (i==0) ? 1 : 0;
		
		if(!(i & 0x1))
		{
			regs->CH[i>>1].bits.Gain0 = gains[i]; /* Gain 64 = unity */
			regs->CH[i>>1].bits.Sum0 = 0;
			regs->CH[i>>1].bits.CTest0 = ctest;
			regs->CH[i>>1].bits.MaskOr0 = 0;  /* 2=disable or1, enable or0 */
		}
		else
		{
			regs->CH[i>>1].bits.Gain1 = gains[i]; /* Gain 64 = unity */
			regs->CH[i>>1].bits.Sum1 = 0;
			regs->CH[i>>1].bits.CTest1 = ctest;
			regs->CH[i>>1].bits.MaskOr1 = 0;  /* 2=disable or1, enable or0 */
		}
	}
}
/*****************************************************************/
/*****************************************************************/
/*****************************************************************/

/*****************************************************************/
/*          RICH Dynamic Register Configuration Interface        */
/*****************************************************************/

void maroc_clear_dynregs(int devid)
{
	int val;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }

	/* set rst_r low */
	val = maroc_read32(devid, &pMAROC_regs->Cfg.SerCtrl);
	val &= 0xFFFFFFFB;
	maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, val);

	/* set rst_r high */
	val |= 0x00000004;
	maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, val);
}

void maroc_shift_dynregs(int devid, MAROC_DyRegs wr, MAROC_DyRegs *rd1, MAROC_DyRegs *rd2, MAROC_DyRegs *rd3)
{
	int i, val;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	/* write settings to FPGA shift register */
	maroc_write32(devid, &pMAROC_regs->Cfg.DyRegs_WrAll.Ch0_31_Hold1, wr.Ch0_31_Hold1);
	maroc_write32(devid, &pMAROC_regs->Cfg.DyRegs_WrAll.Ch32_63_Hold1, wr.Ch32_63_Hold1);
	maroc_write32(devid, &pMAROC_regs->Cfg.DyRegs_WrAll.Ch0_31_Hold2, wr.Ch0_31_Hold2);
	maroc_write32(devid, &pMAROC_regs->Cfg.DyRegs_WrAll.Ch32_63_Hold2, wr.Ch32_63_Hold2);

	/* do shift register transfer */
	val = maroc_read32(devid, &pMAROC_regs->Cfg.SerCtrl);
	val |= 0x00000008;
	maroc_write32(devid, &pMAROC_regs->Cfg.SerCtrl, val);
	
	/* check for shift register transfer completion */
	for(i = 10; i > 0; i--)
	{
		val = maroc_read32(devid, &pMAROC_regs->Cfg.SerStatus);
		if(!(val & 0x00000002))
			break;
		
		if(!i)
			printf("Error in %s: timeout on serial transfer...\n", __FUNCTION__);

		usleep(100);
	}

	/* read back settings from FPGA shift register */
	if(rd1)
	{
		rd1->Ch0_31_Hold1 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[0].Ch0_31_Hold1);
		rd1->Ch32_63_Hold1 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[0].Ch32_63_Hold1);
		rd1->Ch0_31_Hold2 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[0].Ch0_31_Hold2);
		rd1->Ch32_63_Hold2 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[0].Ch32_63_Hold2);
	}

	if(rd2)
	{
		rd2->Ch0_31_Hold1 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[1].Ch0_31_Hold1);
		rd2->Ch32_63_Hold1 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[1].Ch32_63_Hold1);
		rd2->Ch0_31_Hold2 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[1].Ch0_31_Hold2);
		rd2->Ch32_63_Hold2 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[1].Ch32_63_Hold2);
	}

	if(rd3)
	{
		rd3->Ch0_31_Hold1 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[2].Ch0_31_Hold1);
		rd3->Ch32_63_Hold1 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[2].Ch32_63_Hold1);
		rd3->Ch0_31_Hold2 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[2].Ch0_31_Hold2);
		rd3->Ch32_63_Hold2 = maroc_read32(devid, &pMAROC_regs->Cfg.DyRegs_Rd[2].Ch32_63_Hold2);
	}
}

void maroc_print_dynregs(MAROC_DyRegs regs)
{
	int i;
	
	printf("Channels:\n");
	printf("%7s%7s%7s\n", "CH", "Hold1", "Hold2");
	for(i = 0; i < 64; i++)
	{
		if(i < 32)
			printf("%7d%7d%7d\n", i,
				(regs.Ch0_31_Hold1>>i) & 0x1,
				(regs.Ch0_31_Hold2>>i) & 0x1);
		else
			printf("%7d%7d%7d\n", i,
				(regs.Ch32_63_Hold1>>(i-32)) & 0x1,
				(regs.Ch32_63_Hold2>>(i-32)) & 0x1);
	}
	printf("\n");
}

void maroc_init_dynregs(MAROC_DyRegs *regs)
{
	int i;

	memset(regs, 0, sizeof(MAROC_DyRegs));

	regs->Ch0_31_Hold1 = 0x00000001;
	regs->Ch32_63_Hold1 = 0x00000000;
	regs->Ch0_31_Hold2 = 0x00000000;
	regs->Ch32_63_Hold2 = 0x00000000;
}

#endif

float maroc_print_scaler(char *name, unsigned int scaler, float ref)
{
  float rate = (float)scaler / ref;
	printf("%-10s %9u %9fHz\n", name, scaler, rate);
  return rate;
}

void maroc_get_scalers(int devid, unsigned int scalers[192], int normalize)
{
	float ref, fval;
	unsigned int val;
	int i, j;
	char buf[100];
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }

	/* halt scaler counting */
	maroc_write32(devid, &pMAROC_regs->Sd.ScalerLatch, 0x1);

	/* read scalers */
	for(j = 0; j < 64; j++)
  for(i = 0; i < 3; i++)
    scalers[i*64+j] = maroc_read32(devid, &pMAROC_regs->Proc[i].Scalers[j]);

  val = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_Input[0]); printf("Input0: %d\n", val);
  val = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_Input[1]); printf("Input1: %d\n", val);
  val = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_Trig); printf("Trig: %d\n", val);
	val = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_GClk125); printf("Gclk125: %d\n", val);
  val = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_Output[0]); printf("Output0: %d\n", val);
  val = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_Output[1]); printf("Outut1: %d\n", val);
/*
  unsigned int or0[3], or1[3];
  for(i=0;i<3;i++)
  {
    or0[i] = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_Or0[i]);
    or1[i] = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_Or1[i]);
    printf("Or0[%d]=%9d, Or1[%d]=%9d\n",i,or0[i],i,or1[i]);
  }
*/
  if(normalize)
  {
  	/* read reference time */
	  val = maroc_read32(devid, &pMAROC_regs->Sd.Scaler_GClk125);
  	if(!val)
	  {
		  printf("Error in %s: reference time invalid - scaler normalization incorrect\n", __FUNCTION__);
  		val = 1;
	  }
  	ref = 125.0E6 / (float)val;

  	for(j = 0; j < 64; j++)
    for(i = 0; i < 3; i++)
      scalers[i*64+j] = scalers[i*64+j] * ref;
  }

	/* resets scalers */
	maroc_write32(devid, &pMAROC_regs->Sd.ScalerLatch, 0x2);

	/* enable scaler counting */
	maroc_write32(devid, &pMAROC_regs->Sd.ScalerLatch, 0x0);

}

void maroc_dump_scalers(int devid)
{
  unsigned int scalers[192];
  int i, j;

  maroc_get_scalers(devid, scalers, 1);

	for(j = 0; j < 64; j++)
	{
		printf("CH%2d", j);

		for(i = 0; i < 3; i++)
			printf(" [%10u Hz]", scalers[i*64+j]);

		printf("\n");
	}
}

void maroc_set_pulser(int devid, float freq, float duty, int count)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
  if(freq < 0.0)
	{
		printf("Error in %s: freq invalid, setting to 1Hz", __FUNCTION__);
		freq = 0.0;
	}

	if((duty <= 0.0) || (duty >= 1.0))
	{
		printf("Error in %s: duty invalid, setting to 50%", __FUNCTION__);
		duty = 0.5;
	}

	maroc_write32(devid, &pMAROC_regs->Sd.PulserStart, 0);

  if(freq)	
  	freq = 125000000 / freq;
  else
    freq = 0;
	maroc_write32(devid, &pMAROC_regs->Sd.PulserPeriod, (int)freq);

	duty = freq * (1.0 - duty);
	maroc_write32(devid, &pMAROC_regs->Sd.PulserLowCycles, (int)duty);

	maroc_write32(devid, &pMAROC_regs->Sd.PulserNCycles, count);
	maroc_write32(devid, &pMAROC_regs->Sd.PulserStart, 1);
}

/* Currently feature does not exist in firmware
void maroc_set_tdc_deadtime(int devid, int ticks_8ns)
{
	int i;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	for(i = 0; i < 3; i++)
		maroc_write32(devid, &pMAROC_regs->Proc[i].DeadCycles, ticks_8ns);
}
*/

void maroc_enable_all_tdc_channels(int devid)
{
	int i;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	for(i = 0; i < 3; i++)
	{
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[0], 0x00000000);
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[1], 0x00000000);
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[2], 0x00000000);
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[3], 0x00000000);
	}
}

void maroc_disable_all_tdc_channels(int devid)
{
	int i;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	for(i = 0; i < 3; i++)
	{
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[0], 0xFFFFFFFF);
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[1], 0xFFFFFFFF);
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[2], 0xFFFFFFFF);
		maroc_write32(devid, &pMAROC_regs->Proc[i].DisableCh[3], 0xFFFFFFFF);
	}
}

void maroc_enable_tdc_channel(int devid, int ch)
{
	int i;
	int proc_idx, proc_reg, proc_bit;
	int val;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	if((ch < 0) || (ch > 191))
	{
		printf("Error in %s: invalid channel %d\n", __FUNCTION__, ch);
		return;
	}

	proc_idx = ch / 64;
	proc_reg = (ch % 64) / 16;
	proc_bit = (ch % 16);

	val = maroc_read32(devid, &pMAROC_regs->Proc[proc_idx].DisableCh[proc_reg]);
	val = val & ~(0x00010001<<proc_bit);
	maroc_write32(devid, &pMAROC_regs->Proc[proc_idx].DisableCh[proc_reg], val);
}

void maroc_disable_tdc_channel(int devid, int ch)
{
	int i;
	int proc_idx, proc_reg, proc_bit;
	int val;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	if((ch < 0) || (ch > 191))
	{
		printf("Error in %s: invalid channel %d\n", __FUNCTION__, ch);
		return;
	}

	proc_idx = ch / 64;
	proc_reg = (ch % 64) / 16;
	proc_bit = (ch % 16);

	val = maroc_read32(devid, &pMAROC_regs->Proc[proc_idx].DisableCh[proc_reg]);
	val = val | (0x00010001<<proc_bit);
	maroc_write32(devid, &pMAROC_regs->Proc[proc_idx].DisableCh[proc_reg], val);
}

void maroc_setup_readout(int devid, int lookback, int window)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	maroc_write32(devid, &pMAROC_regs->EvtBuilder.Lookback, lookback);		/* lookback*8ns from trigger time */
	maroc_write32(devid, &pMAROC_regs->EvtBuilder.WindowWidth, window);	/* capture window*8ns hits after lookback time */
	maroc_write32(devid, &pMAROC_regs->EvtBuilder.BlockCfg, 1);				/* 1 event per block */
	maroc_write32(devid, &pMAROC_regs->EvtBuilder.DeviceID, 0x01);			/* setup a dummy device ID (5bit) written in each event for module id - needs to be expanded */
}

void maroc_soft_trig(int devid)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }

  printf("maroc_soft_trig called for devid=%d ...\n",devid);fflush(stdout);
  maroc_enable_trigger(devid, SD_SRC_SEL_1);
  maroc_enable_trigger(devid, SD_SRC_SEL_0);
  printf("... maroc_soft_trig done\n");fflush(stdout);
}

void maroc_soft_reset(int devid)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	maroc_write32(devid, &pMAROC_regs->Clk.Ctrl, 0x01);						/* assert soft reset */
	maroc_write32(devid, &pMAROC_regs->Clk.Ctrl, 0x00);						/* deassert soft reset */
}

void maroc_fifo_status(int devid)
{
	int wordCnt, eventCnt;
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	
	wordCnt = maroc_read32(devid, &pMAROC_regs->EvtBuilder.FifoWordCnt);
	eventCnt = maroc_read32(devid, &pMAROC_regs->EvtBuilder.FifoEventCnt);
	
	printf("FIFO Event Status: WCNT=%9d ECNT=%9d\n", wordCnt, eventCnt);
}

int maroc_fifo_nwords(int devid)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return 0;
  }
	return maroc_read32(devid, &pMAROC_regs->EvtBuilder.FifoWordCnt);
}

void maroc_fifo_read(int devid, int *buf, int nwords)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	while(nwords--)
		*buf++ = maroc_read32(devid, &pMAROC_regs->EvtBuilder.FifoData);
}

int maroc_process_buf(int *buf, int nwords, FILE *f, int print)
{
	static int tag = 15;
	static int tag_idx = 0;
  char str[500];
	int word;
  int nevents = 0;
	
	while(nwords--)
	{
		word = *buf++;
		
		if(f) fprintf(f, "0x%08X", word);
		if(print) printf("0x%08X", word);
		
		if(word & 0x80000000)
		{
			tag = (word>>27) & 0xF;
			tag_idx = 0;
		}
		else
			tag_idx++;
	
    str[0] = 0;	
		switch(tag)
		{
			case 0:	// block header
        sprintf(str, " [BLOCKHEADER] SLOT=%d, BLOCKNUM=%d, BLOCKSIZE=%d\n", (word>>22)&0x1F, (word>>8)&0x3FF, (word>>0)&0xFF);
				break;
				
			case 1:	// block trailer
				sprintf(str, " [BLOCKTRAILER] SLOT=%d, WORDCNT=%d\n", (word>>22)&0x1F, (word>>0)&0x3FFFFF);
				break;

			case 2:	// event header
        nevents++;
				sprintf(str, " [EVENTHEADER] TRIGGERNUM=%d, DEVID=%d\n", (word>>0)&0x3FFFFF, (word>>22)&0x1F);
				break;
				
			case 3:	// trigger time
				if(tag_idx == 0)
					sprintf(str, " [TIMESTAMP 0] TIME=%d\n", (word>>0)&0xFFFFFF);
				else if(tag_idx == 1)
					sprintf(str, " [TIMESTAMP 1] TIME=%d\n", (word>>0)&0xFFFFFF);
				break;

			case 8:	// TDC hit
				sprintf(str, " [TDC HIT] EDGE=%d, CH=%d, TIME=%d\n", (word>>26)&0x1,(word>>16)&0xFF, (word>>0)&0xFFFF);
				break;

      case 9: // ADC Hit
        if(tag_idx == 0)
          sprintf(str, " [ADC DATA %d] MAROC_ID=%d, RESOLUTION=%dbit, HOLD1_DLY=%d, HOLD2_DLY=%d\n", tag_idx, (word>>0)&0x3, (word>>4)&0xF, (word>>8)&0xFF, (word>>16)&0xFF);
        else
          sprintf(str, " [ADC DATA %d] Ch%2d ADC=%4d, Ch%2d ADC=%4d\n", tag_idx, 2*(tag_idx-1)+0, (word>>0)&0xFFF,2*(tag_idx-1)+1, (word>>16)&0xFFF);
        break;

			case 14:	// data not valid
				sprintf(str, " [DNV]\n");
				break;
				
			case 15:	// filler word
				sprintf(str, " [FILLER]\n");
				break;
				
			default:	// unknown
				sprintf(str, " [UNKNOWN]\n");
				break;
		}
    if(str[0])
    {
    	if(f) fprintf(f, "%s", str);
	  	if(print) printf("%s", str);
    }
	}
  return nevents;
}

void maroc_setmask_fpga_or0(int devid, int m0_0, int m0_1, int m1_0, int m1_1, int m2_0, int m2_1)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	maroc_write32(devid, &pMAROC_regs->Proc[0].HitOrMask0, m0_0);
	maroc_write32(devid, &pMAROC_regs->Proc[0].HitOrMask1, m0_1);
	maroc_write32(devid, &pMAROC_regs->Proc[1].HitOrMask0, m1_0);
	maroc_write32(devid, &pMAROC_regs->Proc[1].HitOrMask1, m1_1);
	maroc_write32(devid, &pMAROC_regs->Proc[2].HitOrMask0, m2_0);
	maroc_write32(devid, &pMAROC_regs->Proc[2].HitOrMask1, m2_1);
}

void maroc_setmask_fpga_or1(int devid, int m0_0, int m0_1, int m1_0, int m1_1, int m2_0, int m2_1)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	maroc_write32(devid, &pMAROC_regs->Proc[0].HitOrMask2, m0_0);
	maroc_write32(devid, &pMAROC_regs->Proc[0].HitOrMask3, m0_1);
	maroc_write32(devid, &pMAROC_regs->Proc[1].HitOrMask2, m1_0);
	maroc_write32(devid, &pMAROC_regs->Proc[1].HitOrMask3, m1_1);
	maroc_write32(devid, &pMAROC_regs->Proc[2].HitOrMask2, m2_0);
	maroc_write32(devid, &pMAROC_regs->Proc[2].HitOrMask3, m2_1);
}

void maroc_getmask_fpga_or0(int devid, int *m0_0, int *m0_1, int *m1_0, int *m1_1, int *m2_0, int *m2_1)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	*m0_0 = maroc_read32(devid, &pMAROC_regs->Proc[0].HitOrMask0);
	*m0_1 = maroc_read32(devid, &pMAROC_regs->Proc[0].HitOrMask1);
	*m1_0 = maroc_read32(devid, &pMAROC_regs->Proc[1].HitOrMask0);
	*m1_1 = maroc_read32(devid, &pMAROC_regs->Proc[1].HitOrMask1);
	*m2_0 = maroc_read32(devid, &pMAROC_regs->Proc[2].HitOrMask0);
	*m2_1 = maroc_read32(devid, &pMAROC_regs->Proc[2].HitOrMask1);
}

void maroc_getmask_fpga_or1(int devid, int *m0_0, int *m0_1, int *m1_0, int *m1_1, int *m2_0, int *m2_1)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	*m0_0 = maroc_read32(devid, &pMAROC_regs->Proc[0].HitOrMask2);
	*m0_1 = maroc_read32(devid, &pMAROC_regs->Proc[0].HitOrMask3);
	*m1_0 = maroc_read32(devid, &pMAROC_regs->Proc[1].HitOrMask2);
	*m1_1 = maroc_read32(devid, &pMAROC_regs->Proc[1].HitOrMask3);
	*m2_0 = maroc_read32(devid, &pMAROC_regs->Proc[2].HitOrMask2);
	*m2_1 = maroc_read32(devid, &pMAROC_regs->Proc[2].HitOrMask3);
}

/*****************************************************************/
/*****************************************************************/
/*****************************************************************/

void SetupMAROC_ADC(int devid, unsigned char h1, unsigned char h2, int resolution, int maroc_mask)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
	maroc_write32(devid, &pMAROC_regs->Adc.Hold1Delay, h1);
	maroc_write32(devid, &pMAROC_regs->Adc.Hold2Delay, h2);
	maroc_write32(devid, &pMAROC_regs->Adc.AdcCtrl, (resolution<<4) | (maroc_mask<<1) | 0x1);
	maroc_write32(devid, &pMAROC_regs->Adc.AdcCtrl, (resolution<<4) | (maroc_mask<<1));
}

void maroc_fifo_reset(int devid)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
  maroc_write32(devid, &pMAROC_regs->Clk.Ctrl, 0x01);            /* assert soft reset */
  maroc_write32(devid, &pMAROC_regs->Clk.Ctrl, 0x00);            /* deassert soft reset */
}

void maroc_enable_trigger(int devid, int source)
{
  if((devid >= MAROC_MAX_NUM) || !sockfd_reg[devid])
  {
    printf("%s: ERROR invalid devid %d\n", __func__, devid);
    return;
  }
  maroc_write32(devid, &pMAROC_regs->Sd.TrigSrc, source);
}

#else /*Linux_armv7l*/

void
marocLib_default()
{
}

#endif
