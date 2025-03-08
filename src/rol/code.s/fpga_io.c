
/* fpga_io.c */

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
#include "fpga_io.h"

int sockfd_reg = 0;
int sockfd_event = 0;

FPGA_regs *pFPGA_regs = (FPGA_regs *)0x0;

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

void
fpga_init()
{
  int val, i;
  unsigned int *addr = (unsigned int *)0;

  val = fpga_read32(&pFPGA_regs->Clk.BoardId);
  printf("%s: BoardId = 0x%08X\n", __func__, val);
}

void
fpga_write32(void *addr, int val)
{
  write_struct ws;

  if(sockfd_reg<=0)
  {
    return;
  }

//printf("%s(0x%08X,0x%08X)", __func__, (int)((long)addr), val);
//fflush(stdout);

  ws.len = 16;
  ws.type = 4;
  ws.wrcnt = 1;
  ws.addr = (int)((long)addr);
  ws.flags = 0;
  ws.vals[0] = val;
  write(sockfd_reg, &ws, sizeof(ws));
//printf(" done.\n");
}

unsigned int
fpga_read32(void *addr)
{
  read_struct rs;
  read_rsp_struct rs_rsp;
  int len;

  if(sockfd_reg<=0)
  {
    printf("Error in %s: sockfd_reg<=0 ...\n", __FUNCTION__);
    return(0);
  }

  //printf("%s(0x%08X)=", __func__, (int)((long)addr));	

  rs.len = 12;
  rs.type = 3;
  rs.rdcnt = 1;
  rs.addr = (int)((long)addr);
  rs.flags = 0;
  write(sockfd_reg, &rs, sizeof(rs));
	
  len = read(sockfd_reg, &rs_rsp, sizeof(rs_rsp));
  if(len != sizeof(rs_rsp))
  {
    printf("Error in %s: socket read failed ...\n", __FUNCTION__);
  }
  //printf("0x%08X\n", rs_rsp.data[0]);
	
  return(rs_rsp.data[0]);
}


void
fpga_read32_n(int n, void *addr, unsigned int *buf)
{
  read_struct rs;
  read_rsp_struct rs_rsp;
  int len, i;

  if(sockfd_reg<=0)
  {
    return;
  }
	
  for(i = 0; i < n; i++)
  {
    rs.len = 12;
    rs.type = 3;
    rs.rdcnt = 1;
    rs.addr = (int)((long)addr);
    rs.flags = 0;
    write(sockfd_reg, &rs, sizeof(rs));
  }
	
  for(i = 0; i < n; i++)
  {
    len = read(sockfd_reg, &rs_rsp, sizeof(rs_rsp));
    if(len != sizeof(rs_rsp))
    {
      printf("Error in %s: socket read failed...\n", __FUNCTION__);
    }		
    buf[i] = rs_rsp.data[0];
  }
}

int
open_socket(int port)
{
	struct sockaddr_in serv_addr;
	int sockfd = 0;
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Error : Could not create socket \n");
		exit(1);
	}
	memset(&serv_addr, '0', sizeof(serv_addr)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if(inet_pton(AF_INET, FPGA_IP_ADDR, &serv_addr.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		exit(1);
	} 

	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
		//exit(1); sergey: let it return 0
	}
	return sockfd;
}

int open_register_socket()
{
	int n, val;
	
	sockfd_reg = open_socket(FPGA_PORT);
        if(sockfd_reg<=0) return(-1);

	/* Send endian test header */
	val = 0x12345678;
	write(sockfd_reg, &val, 4);
	
	val = 0;
	n = read(sockfd_reg, &val, 4);
	printf("open_register_socket: n = %d, val = 0x%08X\n", n, val);
}

int open_event_socket()
{
  sockfd_event = open_socket(6103);
  if(sockfd_event<=0) return(-1);
}

void close_register_socket()
{
  if(sockfd_reg>0)
  {
    close(sockfd_reg);
    sockfd_reg = 0;
  }
}

void close_event_socket()
{
  if(sockfd_event>0)
  {
    close(sockfd_event);
    sockfd_event = 0;
  }
}

int read_event_socket(int *buf, int nwords_max)
{
  int nbytes, nwords, result;

  if(sockfd_event<=0)
  {
    printf("read_event_socket: ERROR: sockfd_event=%d\n",sockfd_event);
    return(0);
  }

  result = ioctl(sockfd_event, FIONREAD, &nbytes);
  if(result < 0)
  {
    printf("read_event_socket: error in ioctl()\n");
    return(0);
  }

  nwords = nbytes/4;
  if(nwords > nwords_max) nwords = nwords_max;

  if(nwords) read(sockfd_event, buf, nwords*4);

  return(nwords);
}

