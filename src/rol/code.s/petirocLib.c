
/* petirocLib.c */

#ifndef Linux_armv7l

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include "petirocLib.h"


#ifdef IPC
#include "ipc.h"
#endif

static PETIROC_regs *pPETIROC_regs = (PETIROC_regs *)0x0;

// 0x00000000 = primary/failsafe image, 0x01000100 = run image
//#define PETIROC_FIRMWARE_ADDR           0x01000100
#define PETIROC_FIRMWARE_ADDR           0x00000000

static int npetiroc;
static int devids[PETIROC_MAX_NUM];
static int sockfd_reg[PETIROC_MAX_NUM];
static int sockfd_event[PETIROC_MAX_NUM];

// Intermediate readout buffers to store left TCP socket data for next event blocks
#define PETIROC_EVENT_BUF_LEN   10000

#define PETIROC_HOLD_DELAY      100

static unsigned int petirocEventBufferWork[PETIROC_EVENT_BUF_LEN];
static unsigned int petirocEventBuffer[PETIROC_MAX_NUM][PETIROC_EVENT_BUF_LEN];
static unsigned int petirocEventBufferWrPtr[PETIROC_MAX_NUM];
static unsigned int petirocEventBufferRdPtr[PETIROC_MAX_NUM];
static unsigned int petirocEventBufferSize[PETIROC_MAX_NUM];
static unsigned int petirocEventBufferNBlocks[PETIROC_MAX_NUM];
static int petiroc_sock_type;

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
  int dummy; // to ensure 64bit sized structure
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


void petiroc_write32(int slot, void *addr, int val)
{
	write_struct ws;
  int len;
 
//printf("%s(%d,%08X,%08X) - start\n", __func__, slot, (int)addr, val);
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return;
  }
	ws.len = 16;
	ws.type = 4;
	ws.wrcnt = 1;
	ws.addr = (int)((long)addr);
	ws.flags = 0;
	ws.vals[0] = val;
	len = write(sockfd_reg[slot], &ws, sizeof(ws));
  if(len != sizeof(ws))
  {
    printf("%s: error, closing socket\n", __func__);
    petiroc_close_register_socket(slot);
  }
//printf("%s(%d,%08X,%08X) - end\n", __func__, slot, (int)addr, val);
}

unsigned int petiroc_read32(int slot, void *addr)
{
	read_struct rs;
	read_rsp_struct rs_rsp;
	int len;
	
//printf("%s(%d,%08X) - start\n", __func__, slot, (int)addr);
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return 0;
  }
  rs.dummy = 0;
	rs.len = 12;
	rs.type = 3;
	rs.rdcnt = 1;
	rs.addr = (int)((long)addr);
	rs.flags = 0;
	write(sockfd_reg[slot], &rs, sizeof(rs));
//printf("(reading)");	
	len = read(sockfd_reg[slot], &rs_rsp, sizeof(rs_rsp));
	if(len != sizeof(rs_rsp))
  {
		printf("Error in %s: socket read failed...\n", __FUNCTION__);
    petiroc_close_register_socket(slot);
	}
//printf("%s(%d,%08X)=%08X - end\n", __func__, slot, (int)addr, rs_rsp.data[0]);
	return rs_rsp.data[0];
}

int petiroc_open_socket(char *ip, int port)
{
  struct sockaddr_in serv_addr;
  int sockfd = 0;
	
  printf("petiroc_open_socket start: ip=%s, port=%d\n", ip, port);fflush(stdout);
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf(" %s: Error: Could not create socket \n",__func__);
    return 0;
  }
  memset(&serv_addr, '0', sizeof(serv_addr)); 

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
  {
    printf(" %s: Error: inet_pton error occured\n",__func__);
    return 0;
  } 

  printf("petiroc_open_socket: calling connect() ...\n");fflush(stdout);
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf(" %s: Error : Connect Failed \n",__func__);
    return 0;
  }
  printf("petiroc_open_socket end\n");fflush(stdout);

  return sockfd;
}

int petiroc_check_open_register_socket(int slot)
{
  if(sockfd_reg[slot])
    return OK;

  return ERROR;
}

int petiroc_open_register_socket(int slot, int type)
{
  struct timeval tv;
  int n, val[2];
  char ip[20];

  sprintf(ip, "%s%d", PETIROC_SUBNET, PETIROC_IP_START+slot);

  printf("%s: Connecting to %s (slot=%d, register socket) .. ",__func__,ip, slot);fflush(stdout);
  if(type == PETIROC_INIT_REGSOCKET)
    sockfd_reg[slot] = petiroc_open_socket(ip, PETIROC_REG_SOCKET);
  else //if(type == PETIROC_INIT_SLOWCONSOCKET)
  {
    petiroc_sock_type = type;
    sockfd_reg[slot] = petiroc_open_socket(ip, PETIROC_SLOWCON_SOCKET);
 
    // use timeout on slow controls socket for faster disconnect detection 
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sockfd_reg[slot], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
  }

  if(sockfd_reg[slot])
  {
    printf("Succeeded.\n");fflush(stdout);

    val[0] = 0x12345678; /* Send endian test header */
    val[1] = 0x0;        /* Send dummy msg type to pad 64bit */
    write(sockfd_reg[slot], &val[0], 8);
    
    val[0] = 0;
    n = read(sockfd_reg[slot], &val[0], 4);
    printf("n = %d, val = 0x%08X\n", n, val[0]);fflush(stdout);

    return OK;
  }

  printf("Failed.\n");fflush(stdout);

  return ERROR;
}

int petiroc_open_event_socket(int slot)
{
  char ip[20];

  if(petiroc_sock_type == PETIROC_INIT_SLOWCONSOCKET)
  {
    printf("%s: Skipping event socket (not used by slow controls)\n", __func__);
    return OK;
  }

  sprintf(ip, "%s%d", PETIROC_SUBNET, PETIROC_IP_START+slot);

  printf("%s: Connecting to %s (slot=%d, event socket) .. ",__func__,ip,slot);fflush(stdout);
  sockfd_event[slot] = petiroc_open_socket(ip, PETIROC_EVT_SOCKET);
  
  if(sockfd_event[slot])
  {
    printf("Succeeded.\n");
    return OK;
  }
  printf("Failed.\n");
  return ERROR;
}

void petiroc_close_register_socket(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return;
  }

  close(sockfd_reg[slot]);
  sockfd_reg[slot] = 0;
}

void petiroc_close_event_socket(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_event[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return;
  }

  close(sockfd_event[slot]);
  sockfd_event[slot] = 0;
}

int petiroc_read_event_socket(int slot, unsigned int *buf, int nwords_max)
{
  int nbytes, nwords, result;

  if((slot >= PETIROC_MAX_NUM) || !sockfd_event[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  result = ioctl(sockfd_event[slot], FIONREAD, &nbytes);
  if(result < 0)
  {
    printf("%s: ERROR in ioctl for slot %d\n", __func__, slot);
    return(ERROR);
  }
//  printf("%s: INFO: slot=%2d: nbytes=%d\n", __func__, slot, nbytes);

  nwords = nbytes/4;
  if(nwords > nwords_max)
  {
    printf("%s: WARN: slot=%2d: nwords=%d > nwords_max=%d\n", __func__, slot, nwords, nwords_max);
    nwords = nwords_max;
  }

  if(nwords)
  {
    result = read(sockfd_event[slot], buf, nwords*4);
//    printf("%s: INFO: slot=%2d: read() returned %d words\n", __func__, slot, result/4);
  }

  return(nwords);
}

int petirocEventBufferRead(int slot, unsigned int *buf, int nwords_max)
{
  unsigned int val, nwords = 0;

  while(nwords < nwords_max)
  {
    if(petirocEventBufferSize[slot])
    {
      val = petirocEventBuffer[slot][petirocEventBufferRdPtr[slot]];
      if(nwords < nwords_max)
      {
        *buf++ = val;
        nwords++;
      }
      else
      {
        printf("ERROR: %s - buffer full (nwords_max=%d)\n", __func__, nwords_max);
      }

      petirocEventBufferRdPtr[slot] = (petirocEventBufferRdPtr[slot]+1) % PETIROC_EVENT_BUF_LEN;
      petirocEventBufferSize[slot]--;
      if( (val & 0xF8000000) == 0x88000000) // block trailer
      {
        petirocEventBufferNBlocks[slot]--;
        break;
      }
    }
    else
    {
      printf("ERROR: %s(slot=%d) no end of event found\n", __func__, slot);
      break;
    }
  }

  return(nwords);
}

void petirocEventBufferWrite(int slot, unsigned int *buf, int nwords)
{
  unsigned int i, val;

  for(i=0; i<nwords; i++)
  {
    if(petirocEventBufferSize[slot] < PETIROC_EVENT_BUF_LEN)
    {
      val = *buf++;
      petirocEventBuffer[slot][petirocEventBufferWrPtr[slot]] = val;
      petirocEventBufferWrPtr[slot] = (petirocEventBufferWrPtr[slot]+1) % PETIROC_EVENT_BUF_LEN;
      petirocEventBufferSize[slot]++;
      if( (val & 0xF8000000) == 0x88000000) // block trailer
        petirocEventBufferNBlocks[slot]++;
    }
    else
    {
      printf("ERROR: %s(slot=%d) - ROC event buffer full\n", __func__, slot);
      break;
    }
  }

  return;
}

int petirocReadBlock(unsigned int *buf, int nwords_max)
{
  int i, slot, nwords=0, result, tries;

  for(i=0;i<npetiroc;i++)
  {
    slot = devids[i];
    tries = 0;

 //   printf("petirocReadBlock: reading slot=%d\n",slot);fflush(stdout);
    while(1)
    {
      if(petirocEventBufferNBlocks[slot])
      {
        result = petirocEventBufferRead(slot, buf, nwords_max-nwords);
        nwords+= result;
        buf+= result;
//	      if(tries>0) printf("2: slot=%d (result=%d, nwords=%d\n",slot,result,nwords);fflush(stdout);
        break;
      }
      else if(tries < 100000) // 100000=> 1sec timeout
      {
//	      printf("1: slot=%d, tries=%d\n",slot,tries);fflush(stdout);
        tries++;
        result = petiroc_read_event_socket(slot, petirocEventBufferWork, PETIROC_EVENT_BUF_LEN-petirocEventBufferSize[slot]);
        if(result == ERROR)
        {
          printf("%s: ERROR - slot %d disconnected\n", __func__, slot);
          break;
        }
        else if(result>0)
        {
//	        printf("3: slot=%d, petirocEventBufferSize=%d\n",slot,petirocEventBufferSize[slot]);fflush(stdout);
          petirocEventBufferWrite(slot, petirocEventBufferWork, result);
	      }
        else
	      {
//	        printf("4: slot=%d\n",slot);fflush(stdout);
          usleep(10);
	      }
      }
      else
      {
        printf("ERROR: %s(slot=%d) - failed to receive event block\n", __func__, slot);
        break;
      }
    }
  }

  return(nwords);
}

/******************************************************/

int petirocGetNpetiroc()
{
  return npetiroc;
}

int petirocSlot(int n)
{
  return devids[n];
}

int petirocInit(int slot_start, int n, int iFlag)
{
  int i, slot;

  npetiroc = 0;
  for(i=0;i<PETIROC_MAX_NUM;i++)
  {
    sockfd_reg[i] = 0;
    sockfd_event[i] = 0;
    petirocEventBufferWrPtr[i] = 0;
    petirocEventBufferRdPtr[i] = 0;
    petirocEventBufferSize[i] = 0;
    petirocEventBufferNBlocks[i] = 0;
  }

  for(slot=slot_start; slot<(slot_start+n); slot++)
  {
    printf("\n%s: checking slot=%d ..\n",__func__,slot);fflush(stdout);
/*
    if(petiroc_open_register_socket(slot) != OK)
    {
      printf("%s: slot=%d cannot be opened\n",__func__,slot);fflush(stdout);
      continue;
    }

    // reboot to run secondary image and clean SEUs
    petiroc_Reboot(slot, 1);
    petiroc_close_register_socket(slot);
*/
    if(petiroc_open_register_socket(slot, iFlag & 0x1) != OK)
    {
      printf("%s: slot=%d cannot be opened\n",__func__,slot);fflush(stdout);
      continue;
    }
    printf("%s: slot=%d opened !\n",__func__,slot);fflush(stdout);
    int boardid      = petiroc_read32(slot, &pPETIROC_regs->Clk.BoardId);
    printf("Board=%08X\n", boardid);
    devids[npetiroc++] = slot;

    petiroc_trig_setup(slot, 0, 1);  // disable TRIG, enable SYNC
  }
  printf("\n%s: Found/connect to %d devices\n\n",__func__,npetiroc);fflush(stdout);


  return npetiroc;
}

void petirocEnable()
{
  int i, slot;
  for(i=0;i<npetiroc;i++)
  {
    slot = devids[i];
    petiroc_trig_setup(slot, 1, 1);  // enable TRIG, enable SYNC
  }
}

void petirocEnd()
{
  int i;

  printf("petirocEnd reached\n");fflush(stdout);
  for(i=0;i<npetiroc;i++)
  {
//    int slot = devids[i];
//    petiroc_trig_setup(slot, 0, 1);  // disable TRIG, enable SYNC

    printf("petirocEnd: checking socket %d\n",i);fflush(stdout);

    if(sockfd_reg[i])
    {
      printf("petirocEnd: closing register socket %d\n",i);fflush(stdout);
      petiroc_close_register_socket(i);
    }

    if(sockfd_event[i])
    {
      printf("petirocEnd: closing event socket %d\n",i);fflush(stdout);
      petiroc_close_event_socket(i);
    }
  }
  printf("petirocEnd done\n");fflush(stdout);
}

int petiroc_startb_adc(int slot, int val)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    rval = (rval & 0xFFFFFFFD) | (val<<1);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval);
  }
  return OK;
}

int petiroc_trig_ext(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval | 0x8);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval);
  }
  return OK;
}

int petiroc_hold_ext(int slot, int val)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    if(val)
      petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval | 0x4);
    else
      petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval & 0xFFFFFFFB);
  }
  return OK;
}

int petiroc_enable(int slot, int enable)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    rval = (rval & 0xFFFFFFFE) | (enable<<0);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval);
  }
  return OK;
}

int petiroc_val_evt(int slot, int sel, int en_dly, int dis_dly)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    rval = (rval & 0x0000FFCF) | (sel<<4) | (en_dly<<24) | (dis_dly<<16);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval);
  }
  return OK;
}

int petiroc_raz_chn(int slot, int sel)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    rval = (rval & 0xFFFFFF3F) | (sel<<6);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval);
  }
  return OK;
}

int petiroc_soft_reset(int slot, int val)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  int v = petiroc_read32(slot, &pPETIROC_regs->Clk.Ctrl);
  if(val)
    v |= 0x01;            /* assert soft reset */
  else
    v &= 0xFFFFFFFE;     /* deassert soft reset */
  petiroc_write32(slot, &pPETIROC_regs->Clk.Ctrl, v);
  return OK;
}

int petiroc_set_tdc_enable(int slot, int en_mask[2])
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_write32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[0], en_mask[0]);
  petiroc_write32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[1], en_mask[1] & 0xFFFF);
  petiroc_write32(slot, &pPETIROC_regs->Tdc.CAL_TdcEn, (en_mask[1] >> 16) & 0xF);
  return OK;
}

int petiroc_get_tdc_enable(int slot, int en_mask[2])
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  en_mask[0] = petiroc_read32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[0]);
  en_mask[1] = petiroc_read32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[1]) & 0xFFFF;
  en_mask[1]|= (petiroc_read32(slot, &pPETIROC_regs->Tdc.CAL_TdcEn) & 0xF) << 16;
  return OK;
}

int petiroc_get_fwrev(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  return petiroc_read32(slot, &pPETIROC_regs->Clk.FirmwareRev);
}

int petiroc_get_fwtimestamp(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  return petiroc_read32(slot, &pPETIROC_regs->Clk.Timestamp);
}

int petiroc_set_readout(int slot, int width, int offset, int busythr, int trigdelay)
{
  int val;
  int delay  = 27; 
  int delayn = 31;
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  if(trigdelay>8191) trigdelay = 8191;
  if(trigdelay<0)    trigdelay = 0;
  trigdelay/= 4;

  if(busythr>256) busythr = 255;
  if(busythr<0)   busythr = 0;

  if(width>8191)  width = 8191;
  if(width<=0)    width = 0;
  width/= 4;

  if(offset>8191) offset = 8191;
  if(offset<0)    offset = 0;
  offset/= 4;

  val = ((delay  & 0x3f)<< 0) | 0x00000080;
  val|= ((delayn & 0x3f)<< 8) | 0x00008000;
  val|= ((delay  & 0x3f)<<16) | 0x00800000;
  val|= ((delayn & 0x3f)<<24) | 0x80000000;
  petiroc_write32(slot, &pPETIROC_regs->Sd.Delay, val);
  petiroc_write32(slot, &pPETIROC_regs->Eb.DeviceID, slot);
  petiroc_write32(slot, &pPETIROC_regs->Eb.WindowWidth, width);
  petiroc_write32(slot, &pPETIROC_regs->Eb.Lookback, offset);
  petiroc_write32(slot, &pPETIROC_regs->Eb.TrigDelay, trigdelay);
  petiroc_write32(slot, &pPETIROC_regs->Eb.TrigBusyThr, busythr);

  return OK;
}

int petiroc_get_readout(int slot, int *width, int *offset, int *busythr, int *trigdelay)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  *width     = petiroc_read32(slot, &pPETIROC_regs->Eb.WindowWidth) * 4;
  *offset    = petiroc_read32(slot, &pPETIROC_regs->Eb.Lookback) * 4;
  *trigdelay = petiroc_read32(slot, &pPETIROC_regs->Eb.TrigDelay) * 4;
  *busythr   = petiroc_read32(slot, &pPETIROC_regs->Eb.TrigBusyThr);

  return OK;
}

int petiroc_set_blocksize(int slot, int blocksize)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_write32(slot, &pPETIROC_regs->Eb.Blocksize, blocksize);
  return OK;
}

int petiroc_set_blocksize_all(int blocksize)
{
  int i, slot;
  printf("%s(%d)\n", __func__, blocksize);
  for(i=0;i<npetiroc;i++)
  {
    slot = devids[i];
    petiroc_set_blocksize(slot, blocksize);
  }
  return OK;
}


int petiroc_get_fw_timestamp(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  int timestamp    = petiroc_read32(slot, &pPETIROC_regs->Clk.Timestamp);
  return timestamp;
}

int petiroc_get_fw_rev(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  int rev    = petiroc_read32(slot, &pPETIROC_regs->Clk.FirmwareRev);
  return rev;
}

int petiroc_set_clk(int slot, int sel)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  int v = petiroc_read32(slot, &pPETIROC_regs->Clk.Ctrl) & 0xFFFFFFED;
  if(sel) v |= 0x10;
  petiroc_write32(slot, &pPETIROC_regs->Clk.Ctrl, v | 0x2);
  usleep(1000);
  petiroc_write32(slot, &pPETIROC_regs->Clk.Ctrl, v);
  petiroc_read32(slot, &pPETIROC_regs->Clk.Ctrl);
  usleep(100000);
  v = petiroc_read32(slot, &pPETIROC_regs->Clk.Status);
  printf("%s: PLL_LOCKED=%d, SEL=%d\n", __func__, v, sel);
  return OK;
}

int petiroc_get_clk(int slot, int *sel)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  int v = petiroc_read32(slot, &pPETIROC_regs->Clk.Ctrl);
  *sel = (v & 0x10)>>4;
  return OK;
}

int petiroc_start_conv(int slot, int sel)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    rval = (rval & 0xFFFFF9FF) | (sel<<9);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval);
  }
  return OK;
}

int petiroc_force_conv(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(int chip=0; chip<2; chip++)
  {
    unsigned int rval = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);
    rval |= 0x100;
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval);
    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, rval & 0xFFFFFEFF);
  }
  return OK;
}

int petiroc_cfg_pwr(int slot, int en_d, int en_a, int en_adc, int en_dac, int gain, int clk_en)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  unsigned int val;

  val = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);
  val &= 0xFFFFFC0F;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
  usleep(100);
  if(en_d)    val |= 0x10;
  if(en_a)    val |= 0x20;
  if(en_dac)  val |= 0x40;
  if(en_adc)  val |= 0x80;
  if(!clk_en) val |= 0x100;
  if(gain)    val |= 0x200;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
  usleep(100);
  return OK;
}

int petiroc_trig_setup(int slot, int trig, int sync)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  petiroc_write32(slot, &pPETIROC_regs->Sd.LedG, 3);  // LED0: trig
  petiroc_write32(slot, &pPETIROC_regs->Sd.LedY, 4);  // LED1: sync

  petiroc_write32(slot, &pPETIROC_regs->Sd.Trig, trig ? 3 : 0);  // SD_TRIG or 0

  //Assert SYNC before switch to external (effective local software SYNC)
/*
  petiroc_write32(slot, &pPETIROC_regs->Sd.Sync, 1); // SYNC=1
  petiroc_read32(slot, &pPETIROC_regs->Sd.Sync);
*/
  
  petiroc_write32(slot, &pPETIROC_regs->Sd.Sync, 4); // SD SYNC
  petiroc_write32(slot, &pPETIROC_regs->Sd.Busy, 5); // SD BUSY
  petiroc_read32(slot, &pPETIROC_regs->Sd.Sync);
  return OK;
}

int petiroc_clear_scalers(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 1);
  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 0);
  petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);
  return OK;
}

int petiroc_get_scaler(int slot, int ch)
{
  unsigned int ref, val;
  float result;
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 1);
  
  ref = petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[1]); // reference clock (250MHz)
  val = petiroc_read32(slot, &pPETIROC_regs->Tdc.Scalers[ch]);
  result = (float)val;
  if(ref)
  {
    result*= 250.0E6;
    result/=(float)ref;
  }

  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 0);
  petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);
  return (int)result;
}

int petiroc_status_all()
{
  int i, j, slot;
  for(i=0;i<npetiroc;i++)
  {
    slot = devids[i];
    petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 1);
  }
  
  for(j=0;j<npetiroc;j++)
  {
    slot = devids[j];
    printf("slot=%d: ", slot);
    for(int i=0;i<52;i++)
    {
      if(i%10==0) {
        printf(" | ");
      }
      unsigned int reg = petiroc_read32(slot, &pPETIROC_regs->Tdc.Scalers[i]);
      printf(" %d", reg);
    }
    printf("\n");
    fflush(stdout);
  }

  for(i=0;i<npetiroc;i++)
  {
    slot = devids[i];
    petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 0);
  }
  petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);
}

int petiroc_gstatus()
{
  int i;
  for(i=0;i<npetiroc;i++)
    petiroc_status(devids[i]);
}

int petiroc_status(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 1);
  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 0);
  usleep(1000000);
  
  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 1);
  printf("Sysclk = %d\n", petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[0])); fflush(stdout);
  printf("Gclk = %d\n", petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[1])); fflush(stdout);
  printf("Trig = %d\n", petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[3])); fflush(stdout);
  printf("Sync = %d\n", petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[2])); fflush(stdout);
  printf("Busy = %d\n", petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[4])); fflush(stdout);
  printf("PetirocCfg.Ctrl   = %08X\n", petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl)); fflush(stdout);
  printf("PetirocCfg.Status = %08X\n", petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Status)); fflush(stdout);
//  printf("Sd.Status  = %08X\n", petiroc_read32(slot, &pPETIROC_regs->Sd.Status)); fflush(stdout);

  for(int i=0;i<4;i++)
  {
    unsigned int reg = petiroc_read32(slot, &pPETIROC_regs->TempMon.Temp[i]);
    unsigned int res = (reg & 0x7FFF);
    printf("TempMon.Temp[%d]   = %08X res=%d\n", i, reg, res); fflush(stdout);
  }
  for(int i=0;i<52;i++)
  {
    unsigned int reg = petiroc_read32(slot, &pPETIROC_regs->Tdc.Scalers[i]);
    printf("Scaler[%d] = %d\n", i, reg); fflush(stdout);
  }
  petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 0);
  
  petiroc_printmonitor(slot);
  return OK;
}

int petiroc_cfg_rst(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  int val;

  /* set rst_sc low */
  val = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);
  val &= 0xFFFFFFFE;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
  usleep(100);

  /* set rst_sc high */
  val |= 0x00000001;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
  usleep(100);
  return OK;
}

int petiroc_cfg_load(int slot)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  int val;

  val = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);

  /* set load high */
  val |= 0x00000008;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
  usleep(100);

  /* set load low */
  val &= 0xFFFFFFF7;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
  usleep(100);
  return OK;
}

int petiroc_cfg_select(int slot, int sel)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  int val;

  val = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);

  if(sel)
    val |= 0x00000004;
  else
    val &= 0xFFFFFFFB;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
  return OK;
}

int petiroc_clken(int slot, int en)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  
  for(int chip=0; chip<2; chip++)
  {
    int val = petiroc_read32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl);

    if(en)
      val |= 0x0000C000;
    else
      val &= 0xFFFF3FFF;

    petiroc_write32(slot, &pPETIROC_regs->PetirocAdc[chip].Ctrl, val);
  }
  return OK;
}

unsigned int bit_flip(unsigned int val, unsigned int len)
{
  unsigned int result = 0;
  for(int i=0;i<len;i++)
  {
    if(val & (1<<i))
     result |= 1<<(len-1-i);
  }
  return result;
}

int probe(int slot, int ana, int ana_bit, int dig, int dig_bit)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  PETIROC_Regs regs[2];
  PETIROC_Regs result[2];

  memset(&regs, 0, sizeof(regs));
  for(int i=0; i<2; i++)
  { 
    if(ana == 0) regs[i].Probes.out_inpDAC_probe  = (1<<ana_bit);
    if(ana == 1) regs[i].Probes.out_vth_discri    = (1<<ana_bit);
    if(ana == 2) regs[i].Probes.out_time          = (1<<ana_bit);
    if(ana == 3) regs[i].Probes.out_time_dummy    = (1<<ana_bit);
    if(ana == 4) regs[i].Probes.out_ramp_tdc      = (1<<ana_bit);
 
    if(dig == 0) regs[i].Probes.out_discri_charge = (1<<dig_bit);
    if(dig == 1) regs[i].Probes.out_charge        = (1<<dig_bit);
    if(dig == 2) regs[i].Probes.startRampbADC_int = (1<<dig_bit);
    if(dig == 3) regs[i].Probes.holdb             = (1<<dig_bit);
  }

  petiroc_cfg_select(slot, 0);
 
  petiroc_shift_regs(slot, regs, result);

  return OK;
}

int petiroc_slow_control(int slot, PETIROC_Regs regs[2])
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  PETIROC_Regs result[2];
  petiroc_clken(slot, 1);

  petiroc_cfg_select(slot, 1);

  petiroc_shift_regs(slot, regs, result);

  petiroc_cfg_load(slot);

  return OK;
}


int petiroc_shift_regs(int slot, PETIROC_Regs *regs, PETIROC_Regs *result)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  int chip,i, val;

  for(chip=0;chip<2;chip++)
  {
    regs[chip].SlowControl.vth_discri_charge = bit_flip(regs[chip].SlowControl.vth_discri_charge,10);
    regs[chip].SlowControl.vth_time = bit_flip(regs[chip].SlowControl.vth_time,10);
    regs[chip].SlowControl.Cf = bit_flip(regs[chip].SlowControl.Cf,4);
  }

  /* write settings to FPGA shift register */
  for(i = 0; i < 20; i++)
  {
//printf("W i=%2d: %08X %08X\n", i, regs[0].Data[i], regs[1].Data[i]);
    petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.SerData0[i], regs[0].Data[i]);
    petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.SerData1[i], regs[1].Data[i]);

val = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);
  }

//printf("A\n");
  /* do shift register transfer */
  val = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Ctrl);
//printf("B\n");
  val |= 0x00000002;
  petiroc_write32(slot, &pPETIROC_regs->PetirocCfg.Ctrl, val);
//printf("C\n");

  /* check for shift register transfer completion */
  for(i = 10; i > 0; i--)
  {
//printf("i=%d\n", i);
    val = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.Status);
    if(!(val & 0x00000001))
      break;

    if(!i)
      printf("Error in %s: timeout on serial transfer...\n", __FUNCTION__);

    usleep(100);
  }

  /* read back settings from FPGA shift register */
  for(i = 0; i < 20; i++)
  {
    result[0].Data[i] = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.SerData0[i]);
    result[1].Data[i] = petiroc_read32(slot, &pPETIROC_regs->PetirocCfg.SerData1[i]);
//printf("R i=%2d: %08X %08X\n", i, regs[0].Data[i], regs[1].Data[i]);
  }

  for(chip=0;chip<2;chip++)
  {
    result[chip].SlowControl.vth_discri_charge = bit_flip(result[chip].SlowControl.vth_discri_charge,10);
    result[chip].SlowControl.vth_time = bit_flip(result[chip].SlowControl.vth_time,10);
    result[chip].SlowControl.Cf = bit_flip(result[chip].SlowControl.Cf,4);
  }
  return OK;
}

void petiroc_print_regs(PETIROC_Regs regs, int opt)
{
  int i;

  if(opt&0x1)
  {
    for(i=0;i<20;i++)
      printf("%2d: 0x%08X\n", i, regs.Data[i]);
  }

  if(opt&0x2)
  {
    printf("  mask_discri_charge=%08X\n", regs.SlowControl.mask_discri_charge);
    printf("  inputDAC0    =%d\n", regs.SlowControl.inputDAC_ch0);
    printf("  inputDAC0_en =%d\n", regs.SlowControl.inputDAC_en_ch0);
    printf("  inputDAC1    =%d\n", regs.SlowControl.inputDAC_ch1);
    printf("  inputDAC1_en =%d\n", regs.SlowControl.inputDAC_en_ch1);
    printf("  inputDAC2    =%d\n", regs.SlowControl.inputDAC_ch2);
    printf("  inputDAC2_en =%d\n", regs.SlowControl.inputDAC_en_ch2);
    printf("  inputDAC3    =%d\n", regs.SlowControl.inputDAC_ch3);
    printf("  inputDAC3_en =%d\n", regs.SlowControl.inputDAC_en_ch3);
    printf("  inputDAC4    =%d\n", regs.SlowControl.inputDAC_ch4);
    printf("  inputDAC4_en =%d\n", regs.SlowControl.inputDAC_en_ch4);
    printf("  inputDAC5    =%d\n", regs.SlowControl.inputDAC_ch5);
    printf("  inputDAC5_en =%d\n", regs.SlowControl.inputDAC_en_ch5);
    printf("  inputDAC6    =%d\n", regs.SlowControl.inputDAC_ch6);
    printf("  inputDAC6_en =%d\n", regs.SlowControl.inputDAC_en_ch6);
    printf("  inputDAC7    =%d\n", regs.SlowControl.inputDAC_ch7);
    printf("  inputDAC7_en =%d\n", regs.SlowControl.inputDAC_en_ch7);
    printf("  inputDAC8    =%d\n", regs.SlowControl.inputDAC_ch8);
    printf("  inputDAC8_en =%d\n", regs.SlowControl.inputDAC_en_ch8);
    printf("  inputDAC9    =%d\n", regs.SlowControl.inputDAC_ch9);
    printf("  inputDAC9_en =%d\n", regs.SlowControl.inputDAC_en_ch9);
    printf("  inputDAC10   =%d\n", regs.SlowControl.inputDAC_ch10);
    printf("  inputDAC10_en=%d\n", regs.SlowControl.inputDAC_en_ch10);
    printf("  inputDAC11   =%d\n", regs.SlowControl.inputDAC_ch11);
    printf("  inputDAC11_en=%d\n", regs.SlowControl.inputDAC_en_ch1);
    printf("  inputDAC12   =%d\n", regs.SlowControl.inputDAC_ch12);
    printf("  inputDAC12_en=%d\n", regs.SlowControl.inputDAC_en_ch12);
    printf("  inputDAC13   =%d\n", regs.SlowControl.inputDAC_ch13);
    printf("  inputDAC13_en=%d\n", regs.SlowControl.inputDAC_en_ch13);
    printf("  inputDAC14   =%d\n", regs.SlowControl.inputDAC_ch14);
    printf("  inputDAC14_en=%d\n", regs.SlowControl.inputDAC_en_ch14);
    printf("  inputDAC15   =%d\n", regs.SlowControl.inputDAC_ch15);
    printf("  inputDAC15_en=%d\n", regs.SlowControl.inputDAC_en_ch15);
    printf("  inputDAC16   =%d\n", regs.SlowControl.inputDAC_ch16);
    printf("  inputDAC16_en=%d\n", regs.SlowControl.inputDAC_en_ch16);
    printf("  inputDAC17   =%d\n", regs.SlowControl.inputDAC_ch17);
    printf("  inputDAC17_en=%d\n", regs.SlowControl.inputDAC_en_ch17);
    printf("  inputDAC18   =%d\n", regs.SlowControl.inputDAC_ch18);
    printf("  inputDAC18_en=%d\n", regs.SlowControl.inputDAC_en_ch18);
    printf("  inputDAC19   =%d\n", regs.SlowControl.inputDAC_ch19);
    printf("  inputDAC19_en=%d\n", regs.SlowControl.inputDAC_en_ch19);
    printf("  inputDAC20   =%d\n", regs.SlowControl.inputDAC_ch20);
    printf("  inputDAC20_en=%d\n", regs.SlowControl.inputDAC_en_ch20);
    printf("  inputDAC21   =%d\n", regs.SlowControl.inputDAC_ch21);
    printf("  inputDAC21_en=%d\n", regs.SlowControl.inputDAC_en_ch21);
    printf("  inputDAC22   =%d\n", regs.SlowControl.inputDAC_ch22);
    printf("  inputDAC22_en=%d\n", regs.SlowControl.inputDAC_en_ch22);
    printf("  inputDAC23   =%d\n", regs.SlowControl.inputDAC_ch23);
    printf("  inputDAC23_en=%d\n", regs.SlowControl.inputDAC_en_ch23);
    printf("  inputDAC24   =%d\n", regs.SlowControl.inputDAC_ch24);
    printf("  inputDAC24_en=%d\n", regs.SlowControl.inputDAC_en_ch24);
    printf("  inputDAC25   =%d\n", regs.SlowControl.inputDAC_ch25);
    printf("  inputDAC25_en=%d\n", regs.SlowControl.inputDAC_en_ch25);
    printf("  inputDAC26   =%d\n", regs.SlowControl.inputDAC_ch26);
    printf("  inputDAC26_en=%d\n", regs.SlowControl.inputDAC_en_ch26);
    printf("  inputDAC27   =%d\n", regs.SlowControl.inputDAC_ch27);
    printf("  inputDAC27_en=%d\n", regs.SlowControl.inputDAC_en_ch27);
    printf("  inputDAC28   =%d\n", regs.SlowControl.inputDAC_ch28);
    printf("  inputDAC28_en=%d\n", regs.SlowControl.inputDAC_en_ch28);
    printf("  inputDAC29   =%d\n", regs.SlowControl.inputDAC_ch29);
    printf("  inputDAC29_en=%d\n", regs.SlowControl.inputDAC_en_ch29);
    printf("  inputDAC30   =%d\n", regs.SlowControl.inputDAC_ch30);
    printf("  inputDAC30_en=%d\n", regs.SlowControl.inputDAC_en_ch30);
    printf("  inputDAC31   =%d\n", regs.SlowControl.inputDAC_ch31);
    printf("  inputDAC31_en=%d\n", regs.SlowControl.inputDAC_en_ch31);
    printf("  inputDACdummy=%d\n", regs.SlowControl.inputDACdummy);
    printf("  mask_discri_time=%08X\n", regs.SlowControl.mask_discri_time);

    printf("  DAC6b_ch0 =%d\n", regs.SlowControl.DAC6b_ch0);
    printf("  DAC6b_ch1 =%d\n", regs.SlowControl.DAC6b_ch1);
    printf("  DAC6b_ch2 =%d\n", regs.SlowControl.DAC6b_ch2);
    printf("  DAC6b_ch3 =%d\n", regs.SlowControl.DAC6b_ch3);
    printf("  DAC6b_ch4 =%d\n", regs.SlowControl.DAC6b_ch4);
    printf("  DAC6b_ch5 =%d\n", regs.SlowControl.DAC6b_ch5);
    printf("  DAC6b_ch6 =%d\n", regs.SlowControl.DAC6b_ch6);
    printf("  DAC6b_ch7 =%d\n", regs.SlowControl.DAC6b_ch7);
    printf("  DAC6b_ch8 =%d\n", regs.SlowControl.DAC6b_ch8);
    printf("  DAC6b_ch9 =%d\n", regs.SlowControl.DAC6b_ch9);
    printf("  DAC6b_ch10=%d\n", regs.SlowControl.DAC6b_ch10);
    printf("  DAC6b_ch11=%d\n", regs.SlowControl.DAC6b_ch11);
    printf("  DAC6b_ch12=%d\n", regs.SlowControl.DAC6b_ch12);
    printf("  DAC6b_ch13=%d\n", regs.SlowControl.DAC6b_ch13);
    printf("  DAC6b_ch14=%d\n", regs.SlowControl.DAC6b_ch14);
    printf("  DAC6b_ch15=%d\n", regs.SlowControl.DAC6b_ch15);
    printf("  DAC6b_ch16=%d\n", regs.SlowControl.DAC6b_ch16);
    printf("  DAC6b_ch17=%d\n", regs.SlowControl.DAC6b_ch17);
    printf("  DAC6b_ch18=%d\n", regs.SlowControl.DAC6b_ch18);
    printf("  DAC6b_ch19=%d\n", regs.SlowControl.DAC6b_ch19);
    printf("  DAC6b_ch20=%d\n", regs.SlowControl.DAC6b_ch20);
    printf("  DAC6b_ch21=%d\n", regs.SlowControl.DAC6b_ch21);
    printf("  DAC6b_ch22=%d\n", regs.SlowControl.DAC6b_ch22);
    printf("  DAC6b_ch23=%d\n", regs.SlowControl.DAC6b_ch23);
    printf("  DAC6b_ch24=%d\n", regs.SlowControl.DAC6b_ch24);
    printf("  DAC6b_ch25=%d\n", regs.SlowControl.DAC6b_ch25);
    printf("  DAC6b_ch26=%d\n", regs.SlowControl.DAC6b_ch26);
    printf("  DAC6b_ch27=%d\n", regs.SlowControl.DAC6b_ch27);
    printf("  DAC6b_ch28=%d\n", regs.SlowControl.DAC6b_ch28);
    printf("  DAC6b_ch29=%d\n", regs.SlowControl.DAC6b_ch29);
    printf("  DAC6b_ch30=%d\n", regs.SlowControl.DAC6b_ch30);
    printf("  DAC6b_ch31=%d\n", regs.SlowControl.DAC6b_ch31);

    printf("  EN_10b_DAC=%d\n", regs.SlowControl.EN_10b_DAC);
    printf("  PP_10b_DAC=%d\n", regs.SlowControl.PP_10b_DAC);
    printf("  vth_discri_charge=%d\n", regs.SlowControl.vth_discri_charge);
    printf("  vth_time=%d\n", regs.SlowControl.vth_time);
    printf("  EN_ADC=%d\n", regs.SlowControl.EN_ADC);
    printf("  PP_ADC=%d\n", regs.SlowControl.PP_ADC);
    printf("  sel_startb_ramp_ADC_ext=%d\n", regs.SlowControl.sel_startb_ramp_ADC_ext);
    printf("  usebcompensation=%d\n", regs.SlowControl.usebcompensation);
    printf("  ENbiasDAC_delay=%d\n", regs.SlowControl.ENbiasDAC_delay);
    printf("  PPbiasDAC_delay=%d\n", regs.SlowControl.PPbiasDAC_delay);
    printf("  ENbiasramp_delay=%d\n", regs.SlowControl.ENbiasramp_delay);
    printf("  PPbiasramp_delay=%d\n", regs.SlowControl.PPbiasramp_delay);
    printf("  DACdelay=%d\n", regs.SlowControl.DACdelay);
    printf("  EN_discri_delay=%d\n", regs.SlowControl.EN_discri_delay);
    printf("  PP_discri_delay=%d\n", regs.SlowControl.PP_discri_delay);
    printf("  EN_temp_sensor=%d\n", regs.SlowControl.EN_temp_sensor);
    printf("  PP_temp_sensor=%d\n", regs.SlowControl.PP_temp_sensor);
    printf("  EN_bias_pa=%d\n", regs.SlowControl.EN_bias_pa);
    printf("  PP_bias_pa=%d\n", regs.SlowControl.PP_bias_pa);
    printf("  EN_bias_discri=%d\n", regs.SlowControl.EN_bias_discri);
    printf("  PP_bias_discri=%d\n", regs.SlowControl.PP_bias_discri);
    printf("  cmd_polarity=%d\n", regs.SlowControl.cmd_polarity);
    printf("  LatchDiscri=%d\n", regs.SlowControl.LatchDiscri);
    printf("  EN_bias_6b_DAC=%d\n", regs.SlowControl.EN_bias_6b_DAC);
    printf("  PP_bias_6b_DAC=%d\n", regs.SlowControl.PP_bias_6b_DAC);
    printf("  EN_bias_tdc=%d\n", regs.SlowControl.EN_bias_tdc);
    printf("  PP_bias_tdc=%d\n", regs.SlowControl.PP_bias_tdc);
    printf("  ON_input_DAC=%d\n", regs.SlowControl.ON_input_DAC);
    printf("  EN_bias_charge=%d\n", regs.SlowControl.EN_bias_charge);
    printf("  PP_bias_charge=%d\n", regs.SlowControl.PP_bias_charge);
    printf("  Cf=%d\n", regs.SlowControl.Cf);
    printf("  EN_bias_sca=%d\n", regs.SlowControl.EN_bias_sca);
    printf("  PP_bias_sca=%d\n", regs.SlowControl.PP_bias_sca);
    printf("  EN_bias_discri_charge=%d\n", regs.SlowControl.EN_bias_discri_charge);
    printf("  PP_bias_discri_charge=%d\n", regs.SlowControl.PP_bias_discri_charge);
    printf("  EN_bias_discri_ADC_time=%d\n", regs.SlowControl.EN_bias_discri_ADC_time);
    printf("  PP_bias_discri_ADC_time=%d\n", regs.SlowControl.PP_bias_discri_ADC_time);
    printf("  EN_bias_discri_ADC_charge=%d\n", regs.SlowControl.EN_bias_discri_ADC_charge);
    printf("  PP_bias_discri_ADC_charge=%d\n", regs.SlowControl.PP_bias_discri_ADC_charge);
    printf("  DIS_razchn_int=%d\n", regs.SlowControl.DIS_razchn_int);
    printf("  DIS_razchn_ext=%d\n", regs.SlowControl.DIS_razchn_ext);
    printf("  SEL_80M=%d\n", regs.SlowControl.SEL_80M);
    printf("  EN_80M=%d\n", regs.SlowControl.EN_80M);
    printf("  EN_slow_lvds_rec=%d\n", regs.SlowControl.EN_slow_lvds_rec);
    printf("  PP_slow_lvds_rec=%d\n", regs.SlowControl.PP_slow_lvds_rec);
    printf("  EN_fast_lvds_rec=%d\n", regs.SlowControl.EN_fast_lvds_rec);
    printf("  PP_fast_lvds_rec=%d\n", regs.SlowControl.PP_fast_lvds_rec);
    printf("  EN_transmitter=%d\n", regs.SlowControl.EN_transmitter);
    printf("  PP_transmitter=%d\n", regs.SlowControl.PP_transmitter);
    printf("  ON_1mA=%d\n", regs.SlowControl.ON_1mA);
    printf("  ON_2mA=%d\n", regs.SlowControl.ON_2mA);
    printf("  NC=%d\n", regs.SlowControl.NC);
    printf("  ON_ota_mux=%d\n", regs.SlowControl.ON_ota_mux);
    printf("  ON_ota_probe=%d\n", regs.SlowControl.ON_ota_probe);
    printf("  DIS_trig_mux=%d\n", regs.SlowControl.DIS_trig_mux);
    printf("  EN_NOR32_time=%d\n", regs.SlowControl.EN_NOR32_time);
    printf("  EN_NOR32_charge=%d\n", regs.SlowControl.EN_NOR32_charge);
    printf("  DIS_triggers=%d\n", regs.SlowControl.DIS_triggers);
    printf("  EN_dout_oc=%d\n", regs.SlowControl.EN_dout_oc);
    printf("  EN_transmit=%d\n", regs.SlowControl.EN_transmit);
    printf("\n");
  }
}

int petiroc_set_pulser(int slot, int mask, int ncycles, float freq, float duty, int amp[4])
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  unsigned int period, lowcycles, ctrl = mask & 0xF;
  unsigned int dac_word, i, j;

  printf("%s: slot=%d, mask=0x%01X, ncycles=0x%08X, freq=%f, duty=%f, amp=%d,%d,%d,%d\n",
    __func__, slot, mask, ncycles, freq, duty, amp[0], amp[1], amp[2], amp[3]);

  period = 33.333E6 / freq;
  lowcycles = period * duty;

  petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x480); // syncn rstn
  petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x080); // syncn
  for(i=0; i<4; i++)
  {
    dac_word = amp[i] | (i<<17) | (1<<20);

    petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x480); // syncn rstn
    petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x400); //       rstn
    for(j=0; j<24; j++)
    {
petiroc_read32(slot, &pPETIROC_regs->Pulser.Ctrl);
      if(dac_word & 0x800000)
      {
        petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x700);
        petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x500);
      }
      else
      {
        petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x600);
        petiroc_write32(slot, &pPETIROC_regs->Pulser.Ctrl, ctrl | 0x400);
      }
      dac_word = dac_word<<1;
    }
  }

  petiroc_write32(slot, &pPETIROC_regs->Pulser.Period, 33.333E6 / freq);
  petiroc_write32(slot, &pPETIROC_regs->Pulser.LowCycles, duty * 33.333E6 / freq);
  petiroc_write32(slot, &pPETIROC_regs->Pulser.NCycles, ncycles);
  petiroc_write32(slot, &pPETIROC_regs->Pulser.Start, 0);
  //petiroc_write32(slot, &pPETIROC_regs->Pulser.Status, );
  return OK;
}

int tdc_calibrate_start(int slot, int auto_cal_en, int min_entries)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_write32(slot, &pPETIROC_regs->Tdc.NumEntriesMin, 100000);
  petiroc_write32(slot, &pPETIROC_regs->Tdc.Ctrl, 0x6);  // Calibration & pulser enabled 
  petiroc_write32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[0], 0);
  petiroc_write32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[1], 0);
  petiroc_write32(slot, &pPETIROC_regs->Tdc.CAL_TdcEn, 0);
  return OK;
}

int tdc_calibrate_stop(int slot, int auto_cal_en, int min_entries)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  if(auto_cal_en)
    petiroc_write32(slot, &pPETIROC_regs->Tdc.Ctrl, 0x3);
  else
    petiroc_write32(slot, &pPETIROC_regs->Tdc.Ctrl, 0x1);

  petiroc_write32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[0], 0xFFFFFFFF);
  petiroc_write32(slot, &pPETIROC_regs->Tdc.TOF_TdcEn[1], 0xFFFF);
  petiroc_write32(slot, &pPETIROC_regs->Tdc.CAL_TdcEn, 0xf);
  return OK;
}

int petiroc_flash_SelectSpi(int slot, int sel)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  if(sel)
    petiroc_write32(slot, &pPETIROC_regs->Clk.SpiCtrl, 0x200);
  else
    petiroc_write32(slot, &pPETIROC_regs->Clk.SpiCtrl, 0x100);
  return OK;
}

int petiroc_flash_TransferSpi(int slot, unsigned char wr_data, unsigned char *rd_data)
{
  static int no_read_cnt = 0;
  int i;
  unsigned int val=0;
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  petiroc_write32(slot, &pPETIROC_regs->Clk.SpiCtrl, wr_data | 0x400);

  if(rd_data)
  {
    for(i = 1000; i>0; i--)
    {
      val = petiroc_read32(slot, &pPETIROC_regs->Clk.SpiStatus);
      if(val & 0x800)
      {
        *rd_data = (val & 0xFF);
        break;
      }
    }
    if(i<=0)
      printf("%s: ERROR: Timeout!!!\n", __func__);
  }
  return OK;
}

int petiroc_flash_GetId(int slot, unsigned char *rsp)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_flash_SelectSpi(slot, 1);
  petiroc_flash_TransferSpi(slot, PETIROC_FLASH_CMD_GETID, NULL);
  petiroc_flash_TransferSpi(slot, 0xFF, &rsp[0]);
  petiroc_flash_TransferSpi(slot, 0xFF, &rsp[1]);
  petiroc_flash_TransferSpi(slot, 0xFF, &rsp[2]);
  petiroc_flash_SelectSpi(slot, 0);
  return OK;
}

int petiroc_flash_GetStatus(int slot, unsigned char *status)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_flash_SelectSpi(slot, 1);
  petiroc_flash_TransferSpi(slot, PETIROC_FLASH_CMD_GETSTATUS, NULL);
  petiroc_flash_TransferSpi(slot, 0xFF, status);
  petiroc_flash_SelectSpi(slot, 0);

  return OK;
}

int petiroc_flash_Cmd(int slot, unsigned char cmd)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_flash_SelectSpi(slot, 1);
  petiroc_flash_TransferSpi(slot, cmd, NULL);
  petiroc_flash_SelectSpi(slot, 0);

  return OK;
}

int petiroc_flash_CmdAddr(int slot, unsigned char cmd, unsigned int addr)
{
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_flash_SelectSpi(slot, 1);
  petiroc_flash_TransferSpi(slot, cmd, NULL);
  petiroc_flash_TransferSpi(slot, (addr>>24)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>>16)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 8)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 0)&0xFF, NULL);
  petiroc_flash_SelectSpi(slot, 0);

  return OK;
}

int petiroc_flash_CmdAddrData(int slot, unsigned char cmd, unsigned int addr, unsigned char *data, int len)
{
  int i;
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  petiroc_flash_SelectSpi(slot, 1);
  petiroc_flash_TransferSpi(slot, cmd, NULL);
  petiroc_flash_TransferSpi(slot, (addr>>24)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>>16)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 8)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 0)&0xFF, NULL);

  for(i=0;i<len;i++)
    petiroc_flash_TransferSpi(slot, data[i], NULL);

  petiroc_flash_SelectSpi(slot, 0);

  return OK;
}

int petiroc_flash_IsValid(int slot)
{
  unsigned char rsp[3];
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  petiroc_flash_SelectSpi(slot, 0);
  petiroc_flash_GetId(slot, rsp);
  petiroc_flash_GetId(slot, rsp);

  if( (rsp[0] == PETIROC_SPI_MFG_WINBOND) &&
      (rsp[1] == (PETIROC_SPI_DEVID_W25Q256JVIQ>>8)) &&
      (rsp[2] == (PETIROC_SPI_DEVID_W25Q256JVIQ&0xFF)) )
    return OK;

  printf("%s: ERROR mfg=%02X, devid=%02X%02X\n", __func__, (unsigned int)rsp[0], (unsigned int)rsp[1], (unsigned int)rsp[2]);
  return ERROR;
}

int petiroc_flash_GFirmwareUpdate(char *filename)
{
  FILE *f;
  int i, n, slot;
  unsigned int addr = PETIROC_FIRMWARE_ADDR, page_size = 256, erase_first = 1;
  unsigned char buf[256], status;

  for(n=0; n<npetiroc; n++)
  {
    slot = devids[n];
    if(petiroc_flash_IsValid(slot) != OK)
    {
      printf("%s: ERROR - invalid flash on slot %d\n", __func__, slot);
      return ERROR;
    }
  }

  f = fopen(filename, "rb");
  if(!f)
  {
    printf("%s: ERROR: invalid file %s\n", __func__, filename);
    return ERROR;
  }

  for(n=0; n<npetiroc; n++)
  {
    slot = devids[n];
    petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
    petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_EN);
  }

  memset(buf, 0xff, page_size);
  while(fread(buf, 1, page_size, f) > 0)
  {
    // Erase when at a new 64k block boundary
    if(!(addr % 65536) || erase_first)
    {
      erase_first = 0;
      printf("%s: Erasing sector @ 0x%08X\n", __func__, addr);
      fflush(stdout);

      // Issue block erase to all
      for(n=0; n<npetiroc; n++)
      {
        slot = devids[n];
        petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
        petiroc_flash_CmdAddr(slot, PETIROC_FLASH_CMD_ERASE64K, addr);
      }

      // Wait for block erase to complete
      for(n=0; n<npetiroc; n++)
      {
        slot = devids[n];

        i = 0;
        while(1)
        {
          petiroc_flash_GetStatus(slot, &status);
          if(!(status & 0x1))
            break;
          usleep(16000);
          if(i == 60+6) /* 1000ms maximum sector erase time */
          {
            printf("%s: ERROR: failed to erase flash on slot=%d, address=%d\n", __func__, slot, addr);
            break;
          }
          i++;
        }
      }
    }

    // Program 256 byte page
    for(n=0; n<npetiroc; n++)
    {
      slot = devids[n];
      petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
      petiroc_flash_CmdAddrData(slot, PETIROC_FLASH_CMD_WRPAGE, addr, buf, 256);
    }

    // Wait for page program to complete
    for(n=0; n<npetiroc; n++)
    {
      slot = devids[n];

      i = 0;
      while(1)
      {
        petiroc_flash_GetStatus(slot, &status);
        if(!(status & 0x1))
          break;
        if(i == 300) /* 3ms maximum page program time  */
        {
          printf("%s: ERROR: failed to program flash page on slot=%d, address=%d\n", __func__, slot, addr);
          break;
        }
        i++;
      }
    }
    memset(buf, 0xff, 256);
    addr+= 256;
  }
  
  for(n=0; n<npetiroc; n++)
  {
    slot = devids[n];
    petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
    petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_DIS);
  }
  return OK;
}

int petiroc_flash_FirmwareVerify(int slot, char *filename)
{
  FILE *f;
  int i, len, addr = PETIROC_FIRMWARE_ADDR;
  unsigned char buf[256], val;

  if(petiroc_flash_IsValid(slot) != OK)
    return ERROR;

  f = fopen(filename, "rb");
  if(!f)
  {
    printf("%s: ERROR: invalid file %s\n", __func__, filename);
    return ERROR;
  }

  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_EN);

  petiroc_flash_SelectSpi(slot, 1);
  petiroc_flash_TransferSpi(slot, PETIROC_FLASH_CMD_RD, NULL);  // continuous array read
  petiroc_flash_TransferSpi(slot, (addr>>24)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>>16)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 8)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 0)&0xFF, NULL);

  while((len = fread(buf, 1, 256, f)) > 0)
  {
    for(i=0; i<len; i++)
    {
      petiroc_flash_TransferSpi(slot, 0xFF, &val);
      if(buf[i] != val)
      {
        petiroc_flash_SelectSpi(slot, 0);
        petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
        petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_DIS);
        fclose(f);
        printf("%s: ERROR: slot=%d, failed verify at address 0x%08X[%02X,%02X]\n", __func__, slot, addr, (unsigned int)buf[i], (unsigned int)val);
        return ERROR;
      }
    }
    addr+= 256;
    if(!(addr & 0xFFFF))
    {
      printf(".");
      fflush(stdout);
    }
  }
  petiroc_flash_SelectSpi(slot, 0);
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_DIS);
  fclose(f);

  return OK;
}

int petiroc_flash_GFirmwareVerify(char *filename)
{
  FILE *f;
  int i, n, slot;
  unsigned int addr = PETIROC_FIRMWARE_ADDR, page_size = 256, erase_first = 1;
  unsigned char buf[256], status;

  for(n=0; n<npetiroc; n++)
  {
    slot = devids[n];
    printf("%s: Verifying slot=%d ", __func__, slot);
    fflush(stdout);
    petiroc_flash_FirmwareVerify(slot, filename);
  }
  return OK;
}

int petiroc_read_ip(int slot)
{
  unsigned char buf[256], val;
  unsigned int addr = 0x1FF0000;

  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_EN);

  petiroc_flash_SelectSpi(slot, 1);
  petiroc_flash_TransferSpi(slot, PETIROC_FLASH_CMD_RD, NULL);  // continuous array read
  petiroc_flash_TransferSpi(slot, (addr>>24)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>>16)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 8)&0xFF, NULL);
  petiroc_flash_TransferSpi(slot, (addr>> 0)&0xFF, NULL);

  printf("CFG:");
  for(int i=0; i<15; i++)
  {
    petiroc_flash_TransferSpi(slot, 0xFF, &val);
    printf(" %02X", val);
  }
  printf("\n");
  return OK;
}

int petiroc_program_ip(int slot, unsigned int ip, unsigned int mac0, unsigned int mac1)
{
  unsigned char buf[256], status;
  unsigned int addr = 0x1FF0000;
  int i;
  
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  memset(buf, 0xFF, sizeof(buf));
  buf[0] = 0xA5;
  // IP address
  buf[1]  = (ip>>24) & 0xFF;
  buf[2]  = (ip>>16) & 0xFF;
  buf[3]  = (ip>> 8) & 0xFF;
  buf[4]  = (ip>> 0) & 0xFF;
  // IP mask
  buf[1]  = 0;
  buf[2]  = 255;
  buf[3]  = 255;
  buf[4]  = 255;
  // Gateway address
  buf[5]  = 0;
  buf[6]  = (ip>>16) & 0xFF;
  buf[7]  = (ip>> 8) & 0xFF;
  buf[8]  = (ip>> 0) & 0xFF;
  // MAC address
  buf[9]  = (mac0>> 0) & 0xFF;
  buf[10] = (mac0>> 8) & 0xFF;
  buf[11] = (mac0>>16) & 0xFF;
  buf[12] = (mac0>>24) & 0xFF;
  buf[13] = (mac1>> 0) & 0xFF;
  buf[14] = (mac1>> 8) & 0xFF;
  
  if(petiroc_flash_IsValid(slot) != OK)
  {
    printf("%s: ERROR - invalid flash on slot %d\n", __func__, slot);
    return ERROR;
  }
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_EN);
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
  petiroc_flash_CmdAddr(slot, PETIROC_FLASH_CMD_ERASE64K, addr);

  i = 0;
  while(1)
  {
    petiroc_flash_GetStatus(slot, &status);
    if(!(status & 0x1))
      break;
    usleep(16000);
    if(i == 60+6) /* 1000ms maximum sector erase time */
    {
      printf("%s: ERROR: failed to erase flash on slot=%d, address=%d\n", __func__, slot, addr);
      break;
    }
    i++;
  }

  // Program 256 byte page
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
  petiroc_flash_CmdAddrData(slot, PETIROC_FLASH_CMD_WRPAGE, addr, buf, 256);

  // Wait for page program to complete
  i = 0;
  while(1)
  {
    petiroc_flash_GetStatus(slot, &status);
    if(!(status & 0x1))
      break;
    if(i == 300) /* 3ms maximum page program time  */
    {
      printf("%s: ERROR: failed to program flash page on slot=%d, address=%d\n", __func__, slot, addr);
      break;
    }
    i++;
  }

  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_WREN);
  petiroc_flash_Cmd(slot, PETIROC_FLASH_CMD_4BYTE_DIS);

  return OK;
}


int petiroc_set_idelay(
    int slot,
    int delay_trig1_p, int delay_trig1_n,
    int delay_sync_p, int delay_sync_n
  )
{
  int val;
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  val = ((delay_trig1_p & 0x3f)<< 0) | 0x00000080;
  val|= ((delay_trig1_n & 0x3f)<< 8) | 0x00008000;
  val|= ((delay_sync_p  & 0x3f)<<16) | 0x00800000;
  val|= ((delay_sync_n  & 0x3f)<<24) | 0x80000000;
  petiroc_write32(slot, &pPETIROC_regs->Sd.Delay, val);
  return OK;
}

int petiroc_get_idelayerr(int slot)
{
  int val;
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }
  val = petiroc_read32(slot, &pPETIROC_regs->Sd.ErrStatus) & 0x3;
  petiroc_write32(slot, &pPETIROC_regs->Sd.ErrCtrl, 0x3);
  printf("%s(%d) returns %d\n", __func__, slot, val);
  return val;
}

int petiroc_Reboot(int slot, int image)
{
  unsigned int ctrl = 0x80000000;
  /* Note mask: 0xFFFFFF00 intended to start boot 256 bytes before real image since lower 8bits of boot address may be unknown per Xilinx spec in 32bit SPI mode */
  unsigned int addr = PETIROC_FIRMWARE_ADDR & 0xFFFFFF00;

  ctrl |= image ? (addr>>8) : 0;

  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  printf("%s: Rebooting PETIROC Slot %d\n", __func__, slot);

  petiroc_write32(slot, &pPETIROC_regs->Testing.FpgaRebootCtrl, ctrl);
}

int petiroc_readmonitor(int slot, petiroc_monitor_t *mon)
{
  unsigned int val[32];
  int addr;
  float v;

  memset(mon, 0, sizeof(petiroc_monitor_t));
 
  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  for(addr = 0; addr < 0x20; addr++)
  {
    petiroc_write32(slot, &pPETIROC_regs->Testing.XAdcCtrl, 0x01000000 | (addr<<16));
    val[addr] = petiroc_read32(slot, &pPETIROC_regs->Testing.XAdcStatus);
  }
  mon->sem.heartbeat = petiroc_read32(slot, &pPETIROC_regs->Testing.HeartBeatCnt);
  mon->sem.seu_cnt = petiroc_read32(slot, &pPETIROC_regs->Testing.CorrectionCnt);

  // Temperature registers/scaling
  v = (float)(val[0] & 0xFFFF) * 503.975f / 65536.0f - 273.15f;
  v = 1000.0f * v;
  mon->temps.fpga = (int)v;

  for(int i=0;i<4;i++)
  {
    unsigned int reg = petiroc_read32(slot, &pPETIROC_regs->TempMon.Temp[i]);
    unsigned int res = (reg & 0x7FFF);
    mon->temps.sipm[i] = res;
  }

  // Votlage registers/scaling  
  v = (float)(val[16] & 0xFFFF) * 1.0f / 65536.0f;
  v = 1000.0f * (v + (v / 442.0f) * 4700.0f);
  mon->voltages.pcb_lv1 = (int)v;

  v = (float)(val[17] & 0xFFFF) * 1.0f / 65536.0f;
  v = 1000.0f * (v + (v / 442.0f) * 4700.0f);
  mon->voltages.pcb_2_5v = (int)v;

  v = (float)(val[19] & 0xFFFF) * 1.0f / 65536.0f;
  v = 1000.0f * (v + (v / 442.0f) * 4700.0f);
  mon->voltages.fpga_mgt_1v = (int)v;
  
  v = (float)(val[24] & 0xFFFF) * 1.0f / 65536.0f;
  v = 1000.0f * (v + (v / 442.0f) * 4700.0f);
  mon->voltages.pcb_lv2 = (int)v;

  v = (float)(val[25] & 0xFFFF) * 1.0f / 65536.0f;
  v = 1000.0f * (v + (v / 442.0f) * 4700.0f);
  mon->voltages.pcb_3_3v = (int)v;

  v = (float)(val[26] & 0xFFFF) * 1.0f / 65536.0f;
  v = 1000.0f * (v + (v / 442.0f) * 4700.0f);
  mon->voltages.pcb_3_3va = (int)v;
  
  v = (double)(val[27] & 0xFFFF) * 1.0 / 65536.0;
  v = 1000.0f * (v + (v / 442.0f) * 4700.0f);
  mon->voltages.fpga_mgt_1_2v = (int)v;

  v = (float)(val[1] & 0xFFFF) * 3.0f / 65.536f;
  mon->voltages.fpga_vccint_1v = (int)v;

  v = (float)(val[2] & 0xFFFF) * 3.0f / 65.536f;
  mon->voltages.fpga_vccaux_1_8v = (int)v;
  
  return OK;
}

int petiroc_printmonitor(int slot)
{
  petiroc_monitor_t mon;

  if((slot >= PETIROC_MAX_NUM) || !sockfd_reg[slot])
  {
    printf("%s: ERROR invalid slot %d\n", __func__, slot);
    return ERROR;
  }

  petiroc_readmonitor(slot, &mon);

  printf("%s(slot=%d):\n", __func__, slot);
  printf("  Temperatures:\n");
  printf("    FPGA          %.3fC\n", (float)mon.temps.fpga / 1000.0f);
  printf("    SiPM0         %d\n", mon.temps.sipm[0]);
  printf("    SiPM1         %d\n", mon.temps.sipm[1]);
  printf("    SiPM2         %d\n", mon.temps.sipm[2]);
  printf("    SiPM3         %d\n", mon.temps.sipm[3]);
  printf("  Voltages:\n");
  printf("    LV1           %.3fV\n", (float)mon.voltages.pcb_lv1 / 1000.0f);
  printf("    LV2           %.3fV\n", (float)mon.voltages.pcb_lv2 / 1000.0f);
  printf("    +3.3VA        %.3fV\n", (float)mon.voltages.pcb_3_3va / 1000.0f);
  printf("    +3.3V         %.3fV\n", (float)mon.voltages.pcb_3_3v / 1000.0f);
  printf("    +2.5V         %.3fV\n", (float)mon.voltages.pcb_2_5v / 1000.0f);
  printf("    VccInt(+1.0V) %.3fV\n", (float)mon.voltages.fpga_vccint_1v / 1000.0f);
  printf("    VccAux(+1.8V) %.3fV\n", (float)mon.voltages.fpga_vccaux_1_8v / 1000.0f);
  printf("    Mgt(+1.0V)    %.3fV\n", (float)mon.voltages.fpga_mgt_1v / 1000.0f);
  printf("    Mgt(+1.2V)    %.3fV\n", (float)mon.voltages.fpga_mgt_1_2v / 1000.0f);
  
  printf("  SEM Monitor:\n");
  printf("    Heartbeat     %d\n", mon.sem.heartbeat);
  printf("    Errors        %d\n", mon.sem.seu_cnt);

  return OK;
}


#ifdef IPC
int petiroc_sendscalers(char *host)
{
  petiroc_monitor_t mon;
  char name[100];
  float fdata[10];
  int idata[52];
  int i, j, slot, val;
/*
  int trig[15], sync[15];
  int trig_err[15], sync_err[15];
  static int delay = 0;
  static int delay_cnt = 0;

  delay_cnt++;
  if(delay_cnt > 3)
  {
    delay_cnt = 0;
    delay = (delay+1) % 59;
    printf("*** Delay = %d ***\n", delay);
  }
*/
  for(j=0;j<npetiroc;j++)
  {
    slot = devids[j];

    if(petiroc_check_open_register_socket(slot) == OK)
    {
      petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 1);
      for(int i=0;i<52;i++)
        idata[i] = petiroc_read32(slot, &pPETIROC_regs->Tdc.Scalers[i]);
printf("sync[%d] = %d\n", slot, petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[2]));
printf("trig[%d] = %d\n", slot, petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[3]));
/*
      trig[slot] = petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[3]);
      sync[slot] = petiroc_read32(slot, &pPETIROC_regs->Sd.Scalers[2]);
      trig_err[slot] = (petiroc_read32(slot, &pPETIROC_regs->Sd.ErrStatus)>>0) & 0x1;
      sync_err[slot] = (petiroc_read32(slot, &pPETIROC_regs->Sd.Sync)>>1) & 0x1;

      if(trig_err[slot] || sync_err[slot])
        petiroc_write32(slot, &pPETIROC_regs->Sd.ErrCtrl, 0x3);

      if(!delay_cnt)
      {
        int delayn = delay + 5;
        val = ((delay  & 0x3f)<< 0) | 0x00000080;
        val|= ((delayn & 0x3f)<< 8) | 0x00008000;
        val|= ((delay  & 0x3f)<<16) | 0x00800000;
        val|= ((delayn & 0x3f)<<24) | 0x80000000;
        petiroc_write32(slot, &pPETIROC_regs->Sd.Delay, val);
      }
*/
      petiroc_write32(slot, &pPETIROC_regs->Sd.Latch, 0);

      sprintf(name, "%s_PETIROC_SLOT%d_TDC_SCALERS", host, slot);
      epics_json_msg_send(name, "int", 52, idata);

      petiroc_readmonitor(slot, &mon);

      fdata[0] = mon.temps.fpga / 1000.0f;
      fdata[1] = mon.temps.sipm[0];
      fdata[2] = mon.temps.sipm[1];
      fdata[3] = mon.temps.sipm[2];
      fdata[4] = mon.temps.sipm[3];
      sprintf(name, "%s_PETIROC_SLOT%d_TEMPS", host, slot);
      epics_json_msg_send(name, "float", 5, fdata);
      
      fdata[0] = mon.voltages.pcb_lv1;
      fdata[1] = mon.voltages.pcb_lv2;
      fdata[2] = mon.voltages.pcb_3_3va;
      fdata[3] = mon.voltages.pcb_3_3v;
      fdata[4] = mon.voltages.pcb_2_5v;
      fdata[5] = mon.voltages.fpga_vccint_1v;
      fdata[6] = mon.voltages.fpga_vccaux_1_8v;
      fdata[7] = mon.voltages.fpga_mgt_1v;
      fdata[8] = mon.voltages.fpga_mgt_1_2v;
      sprintf(name, "%s_PETIROC_SLOT%d_VOLTS", host, slot);
      epics_json_msg_send(name, "float", 9, fdata);
    }
    else
      petiroc_open_register_socket(slot, petiroc_sock_type);
  }
/*
  printf("trig:");
  for(j=0;j<npetiroc;j++)
  {
    slot = devids[j];
    printf(" %d", trig[slot]);
  }
  printf("\n");

  printf("terr:");
  for(j=0;j<npetiroc;j++)
  {
    slot = devids[j];
    printf(" %d", trig_err[slot]);
  }
  printf("\n");

  printf("sync:");
  for(j=0;j<npetiroc;j++)
  {
    slot = devids[j];
    printf(" %d", sync[slot]);
  }
  printf("\n");

  printf("serr:");
  for(j=0;j<npetiroc;j++)
  {
    slot = devids[j];
    printf(" %d", sync_err[slot]);
  }
  printf("\n\n");
*/
  return OK;
}
#endif


#else

void
petirocLib_default()
{
}

#endif
