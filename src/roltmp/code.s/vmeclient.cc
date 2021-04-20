
/* vmeclient.cc */

#if !defined(NIOS) && !defined(VXWORKS)


#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "arpa/inet.h" // htonl

#include "vmeclient.h"
#include "libtcp.h"
#include "libdb.h"

#define MAX_DWORD_TRANSFER	160

static void
alarmHandler(int sig)
{
  printf("alarmHandler reached\n");fflush(stdout);
  /*signal(sig, alarmHandler);*/
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


VMEClient::VMEClient()
{
  //memset(&m_HallBTriggerRegs, 0, sizeof(m_HallBTriggerRegs));
  m_pDbgDlg = NULL;
  m_bConnected = FALSE;
  m_bDebugAll = FALSE;
  sFd = 0;
}


VMEClient::~VMEClient()
{
}


int VMEClient::ReadFromSocket(unsigned char *buf, int length)
{
  int len, lll;

  //printf("ReadFromSocket INFO: trying to read...\n");
  len = read(sFd,(void *)buf, length);
  //printf("ReadFromSocket INFO: len=%d length=%d\n",len,length);

  while(len<length)
  {
    //printf("ReadFromSocket INFO: trying to read the rest ...\n");
    lll = read(sFd,(void *)&buf[len], length-len);
    //printf("ReadFromSocket INFO: ... received %d (expected %d)\n",lll,length-len);
    len += lll;
  }

  return(len);
}


BOOL VMEClient::VMERead16(unsigned int addr, unsigned short *val, BOOL bDebug, BOOL bIgnoreReady)
{
	if(!bIgnoreReady && !m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}

	Cmd_Read16 *pCmd_Read16;
	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_Read16)-4);
	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_READ16);
	pCmd_Read16 = (Cmd_Read16 *)OutgoingMsg.Msg;
	pCmd_Read16->Address = htonl(addr);
	if(write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_Read16)) == -1)
	{
	  DebugMsg("ERROR: remote VMERead16 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}

	if( (ReadFromSocket((unsigned char *)&IncomingMsg.Length, 4) == 4) &&
		(ntohl(IncomingMsg.Length) <= sizeof(RemoteMsgStruct)) &&
		((unsigned long)ReadFromSocket((unsigned char *)&IncomingMsg.BoardType, ntohl(IncomingMsg.Length)) == ntohl(IncomingMsg.Length)) )
	{
	  IncomingMsg.Length = ntohl(IncomingMsg.Length);
	  IncomingMsg.BoardType = ntohl(IncomingMsg.BoardType);
	  IncomingMsg.MsgType = ntohl(IncomingMsg.MsgType);
	  if(IncomingMsg.MsgType == CMD_RSP(REMOTE_CMD_READ16))
	  {
		unsigned short *p = (unsigned short *)IncomingMsg.Msg;
		*val = ntohs(*p);
	
		if(bDebug || (m_pDbgDlg /*&& ((DebugDlg *)m_pDbgDlg)->m_chDebugAll.GetCheck()*/))
	   	{
		  printf("VMERead16 SUCCESS: Read 0x%04X <= 0x%08X\n", (unsigned int)*val, addr);
		  //DebugMsg(str);
		}
		return TRUE;
	  }
    }
	DebugMsg("ERROR: remote VMERead16 failed");
	DisconnectVMEOnError();
	return FALSE;
}


BOOL VMEClient::VMEWrite16(unsigned int addr, unsigned short val, BOOL bDebug, BOOL bIgnoreReady)
{
	if(!bIgnoreReady && !m_bConnected)
	{
	  DebugMsg("ERROR: VME not initialized");
	  return FALSE;
	}
	Cmd_Write16 *pCmd_Write16;
	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_Write16)-4);
	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_WRITE16);
	pCmd_Write16 = (Cmd_Write16 *)OutgoingMsg.Msg;
	pCmd_Write16->Address = htonl(addr);
	pCmd_Write16->Value = htons(val);
	if(write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_Write16)) == -1)
	{
	  DebugMsg("ERROR: remote VMEWrite16 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}
	if(bDebug || (m_pDbgDlg /*&& ((DebugDlg *)m_pDbgDlg)->m_chDebugAll.GetCheck()*/))
	{
	  printf("VMEWrite16 SUCCESS: Write 0x%04X => 0x%08X\n", (unsigned int)val, addr);
	  //DebugMsg(str);
	}
	return TRUE;
}


BOOL VMEClient::VMEBlkRead16(unsigned int addr, unsigned short count, unsigned short *vals)
{
	if(!m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}
	Cmd_BlkRead16 *pCmd_BlkRead16;
	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_BlkRead16)-4);
	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_BLKREAD16);
	pCmd_BlkRead16 = (Cmd_BlkRead16 *)OutgoingMsg.Msg;
	pCmd_BlkRead16->Address = htonl(addr);
	pCmd_BlkRead16->Count = htons(count);
	if(write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_BlkRead16)) == -1)
	{
	  DebugMsg("ERROR: remote VMERead16 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}

	if( (ReadFromSocket((unsigned char *)&IncomingMsg.Length, 4) == 4) &&
		(ntohl(IncomingMsg.Length) <= sizeof(RemoteMsgStruct)) &&
		((unsigned long)ReadFromSocket((unsigned char *)&IncomingMsg.BoardType, ntohl(IncomingMsg.Length)) == ntohl(IncomingMsg.Length)) )
	{
	  IncomingMsg.Length = ntohl(IncomingMsg.Length);
	  IncomingMsg.BoardType = ntohl(IncomingMsg.BoardType);
	  IncomingMsg.MsgType = ntohl(IncomingMsg.MsgType);
	  if(IncomingMsg.MsgType == CMD_RSP(REMOTE_CMD_BLKREAD16))
	  {
		unsigned short *p = (unsigned short *)IncomingMsg.Msg;
		for(int i = 0; i < count; i++)
		{
		  *vals++ = ntohs(*p);
		  p++;
		}
		return TRUE;
	  }
	}
	DebugMsg("ERROR: remote VMEBLKRead16 failed");
	DisconnectVMEOnError();
	return FALSE;
}



#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))


BOOL VMEClient::VMERead32(unsigned int addr, unsigned int *val, BOOL bDebug, BOOL bIgnoreReady)
{
  int res, len1, len2;

	if(!bIgnoreReady && !m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}
	/*
	printf("\nVMERead32 INFO: addr=0x%08x\n",addr);
	*/
	Cmd_Read32 *pCmd_Read32;
	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_Read32)-4);
	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_READ32);
	pCmd_Read32 = (Cmd_Read32 *)OutgoingMsg.Msg;
	pCmd_Read32->Address = htonl(addr);
	if( (res = write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_Read32))) == -1)
	{
	  DebugMsg("ERROR: remote VMERead32 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}
	/*
    else
	{
      printf("VMERead32 INFO: res(write)=%d\n",res);
	}
	*/

    len1 = ReadFromSocket((unsigned char *)&IncomingMsg.Length, 4);
    len2 = ReadFromSocket((unsigned char *)&IncomingMsg.BoardType, ntohl(IncomingMsg.Length));
	/*
    printf("VMERead32 INFO: len1=%d len2=%d\n",len1,len2);
	printf("VMERead32 INFO: incoming len=%d boardtype=%d msgtype=%d msg=%d\n",
		   ntohl(IncomingMsg.Length),ntohl(IncomingMsg.BoardType),ntohl(IncomingMsg.MsgType), ntohl(*((unsigned int *)IncomingMsg.Msg)) );
	if(len2==20)
	{
      unsigned int stri[4];
      char *strc = (char *)stri;
      stri[0] = LSWAP(ntohl(IncomingMsg.Length));
      stri[1] = LSWAP(ntohl(IncomingMsg.BoardType));
      stri[2] = LSWAP(ntohl(IncomingMsg.MsgType));
      stri[3] = LSWAP(ntohl(*((unsigned int *)IncomingMsg.Msg)));
      printf(">%16.16s<\n",strc);
	}
	*/
	if( len1 == 4 &&
		(ntohl(IncomingMsg.Length) <= sizeof(RemoteMsgStruct)) &&
		((unsigned long)len2 == ntohl(IncomingMsg.Length)) )
	{
	  IncomingMsg.Length = ntohl(IncomingMsg.Length);
	  IncomingMsg.BoardType = ntohl(IncomingMsg.BoardType);
	  IncomingMsg.MsgType = ntohl(IncomingMsg.MsgType);


	  if(IncomingMsg.MsgType == CMD_RSP(REMOTE_CMD_READ32))
	  {
	    unsigned int *p = (unsigned int *)IncomingMsg.Msg;
	    *val = ntohl(*p);
	
	    if(bDebug /* || (m_pDbgDlg && ((DebugDlg *)m_pDbgDlg)->m_chDebugAll.GetCheck())*/)
	    {
	  	  //printf("VMERead32 SUCCESS: Read 0x%08X <= 0x%08X\n",*val,addr);
	      //DebugMsg(str);
		}
		return TRUE;
	  }
	}
	printf("VMERead32 ERROR: addr=0x%08x res(read)=%d\n",addr,res);
	DebugMsg("ERROR: remote VMERead32 failed");
	DisconnectVMEOnError();
	return FALSE;
}


BOOL VMEClient::VMEWrite32(unsigned int addr, unsigned int val, BOOL bDebug, BOOL bIgnoreReady)
{
	if(!bIgnoreReady && !m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}
	Cmd_Write32 *pCmd_Write32;
	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_Write32)-4);
	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_WRITE32);
	pCmd_Write32 = (Cmd_Write32 *)OutgoingMsg.Msg;
	pCmd_Write32->Address = htonl(addr);
	pCmd_Write32->Value = htonl(val);

	if(write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_Write32)) == -1)
	{
	  DebugMsg("ERROR: remote VMEWrite32 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}
	if(bDebug || (m_pDbgDlg /*&& ((DebugDlg *)m_pDbgDlg)->m_chDebugAll.GetCheck()*/))
	{
	  //printf("VMEWrite32 SUCCESS: Write 0x%08X => 0x%08X\n", val, addr);
	  //DebugMsg(str);
	}
	return TRUE;
}


BOOL VMEClient::VMEBlkRead32(unsigned int addr, unsigned int count, unsigned int *vals, unsigned int NoAddressInc)
{
	if(!m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}
	if(count > MAX_DWORD_TRANSFER)
	{
		DebugMsg("ERROR: VMEBlkRead32 - Too large...");
		return FALSE;
	}

	Cmd_BlkRead32 *pCmd_BlkRead32;
	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_BlkRead32)-4);
	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_BLKREAD32);
	pCmd_BlkRead32 = (Cmd_BlkRead32 *)OutgoingMsg.Msg;
	pCmd_BlkRead32->Address = htonl(addr);
	pCmd_BlkRead32->Count = htonl(count);
	pCmd_BlkRead32->NoAddressInc = NoAddressInc;
	if(write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_BlkRead32)) == -1)
	{
	  DebugMsg("ERROR: remote VMEBlkRead32 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}

	if( (ReadFromSocket((unsigned char *)&IncomingMsg.Length, 4) == 4) &&
		(ntohl(IncomingMsg.Length) <= sizeof(RemoteMsgStruct)) &&
		((unsigned long)ReadFromSocket((unsigned char *)&IncomingMsg.BoardType, ntohl(IncomingMsg.Length)) == ntohl(IncomingMsg.Length)) )
	{
	  IncomingMsg.Length = ntohl(IncomingMsg.Length);
	  IncomingMsg.BoardType = ntohl(IncomingMsg.BoardType);
	  IncomingMsg.MsgType = ntohl(IncomingMsg.MsgType);
	  if(IncomingMsg.MsgType == CMD_RSP(REMOTE_CMD_BLKREAD32))
	  {
		unsigned int *p = (unsigned int *)IncomingMsg.Msg;
		for(unsigned int i = 0; i < count; i++)
		{
		  *vals++ = ntohl(*p);
		  p++;
		}
		return TRUE;
	  }
	}
	DebugMsg("ERROR: remote VMEBLKRead32 failed");
	DisconnectVMEOnError();
	return FALSE;
}





















/*
BOOL VMEClient::VMEGTPReset()
{
	if(!m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}

	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_Empty)-4);
	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_RESET);

	if(write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_Empty)) == -1)
	{
	  DebugMsg("ERROR: remote VMEWrite32 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}

	return TRUE;
}


BOOL VMEClient::VMEGTPEventCount(unsigned int *val)
{
  int res, len1, len2;

	if(!m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}

	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_Empty)-4);

	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_GETNUMEVT);


	if( (res = write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_Empty))) == -1)
	{
	  DebugMsg("ERROR: remote VMERead32 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}


    len1 = ReadFromSocket((unsigned char *)&IncomingMsg.Length, 4);
    len2 = ReadFromSocket((unsigned char *)&IncomingMsg.BoardType, ntohl(IncomingMsg.Length));
	if( len1 == 4 &&
		(ntohl(IncomingMsg.Length) <= sizeof(RemoteMsgStruct)) &&
		((unsigned long)len2 == ntohl(IncomingMsg.Length)) )
	{
	  IncomingMsg.Length = ntohl(IncomingMsg.Length);
	  IncomingMsg.BoardType = ntohl(IncomingMsg.BoardType);
	  IncomingMsg.MsgType = ntohl(IncomingMsg.MsgType);


	  if(IncomingMsg.MsgType == CMD_RSP(REMOTE_CMD_GETNUMEVT))
	  {
	    unsigned int *p = (unsigned int *)IncomingMsg.Msg;
	    *val = ntohl(*p);
	
		return TRUE;
	  }
	}
	printf("VMERead32 ERROR: res(read)=%d\n",res);
	DebugMsg("ERROR: remote VMERead32 failed");
	DisconnectVMEOnError();
	return FALSE;
}


BOOL VMEClient::VMEGTPGetEvent(unsigned int *evnum, unsigned int *count, unsigned int *vals)
{
  int res, len1, len2;

	if(!m_bConnected)
	{
		DebugMsg("ERROR: VME not initialized");
		return FALSE;
	}

	OutgoingMsg.Length = htonl(REMOTE_MSG_SIZE(Cmd_Empty)-4);

	OutgoingMsg.BoardType = htonl(BOARD_TYPE_CLASTRIG);
	OutgoingMsg.MsgType = htonl(REMOTE_CMD_GETEVENT);


	if( (res = write(sFd,(void *)&OutgoingMsg, REMOTE_MSG_SIZE(Cmd_Empty))) == -1)
	{
	  DebugMsg("ERROR: remote VMERead32 send() failed");
	  DisconnectVMEOnError();
	  return FALSE;
	}


    len1 = ReadFromSocket((unsigned char *)&IncomingMsg.Length, 4);
    len2 = ReadFromSocket((unsigned char *)&IncomingMsg.BoardType, ntohl(IncomingMsg.Length));
	if( len1 == 4 &&
		(ntohl(IncomingMsg.Length) <= sizeof(RemoteMsgStruct)) &&
		((unsigned long)len2 == ntohl(IncomingMsg.Length)) )
	{
	  IncomingMsg.Length = ntohl(IncomingMsg.Length);
	  IncomingMsg.BoardType = ntohl(IncomingMsg.BoardType);
	  IncomingMsg.MsgType = ntohl(IncomingMsg.MsgType);

	  if(IncomingMsg.MsgType == CMD_RSP(REMOTE_CMD_GETEVENT))
	  {
	    unsigned int *p = (unsigned int *)IncomingMsg.Msg;
	    *evnum = ntohl(*p);
		p++;
	    *count = ntohl(*p);
		p++;
		for(int i=0; i<(*count); i++)
		{
		  vals[i] = ntohl(*p);
		  p++;
		}
	
		return TRUE;
	  }
	}
	printf("VMERead32 ERROR: res(read)=%d\n",res);
	DebugMsg("ERROR: remote VMERead32 failed");
	DisconnectVMEOnError();
	return FALSE;
}
*/
































BOOL VMEClient::ConnectVME(char *targetname, int port)
{
  struct hostent *hptr;
  struct sockaddr_in serverAddr; /* server's socket address */ 
  int sockAddrSize; /* size of socket address structure */ 
  int portnum = 0;
  char hostname[128];
  MYSQL *dbsock;
  char tmp[1000], *mysqlhost;

  printf("VMEClient::ConnectVME: targetname=>%s< \n",targetname);fflush(stdout);

  m_bConnected = FALSE;

  /* create client's socket */ 
  if((sFd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror ("socket"); 
    return (FALSE); 
  } 

  /* set socket parameters */
  {
    int ret;
    struct timeval tv;
    tv.tv_sec = 10;  /* timeout in seconds */
    tv.tv_usec = 0;  /* not init'ing this can cause strange errors */
    ret = setsockopt(sFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
    if(ret!=0) printf("ERROR from setsockopt()\n");
  }

  printf("00\n");fflush(stdout);

  /* bind not required - port number is dynamic */ 
  /* build server socket address */ 
  sockAddrSize = sizeof (struct sockaddr_in);
  memset ((char *) &serverAddr, 0, sockAddrSize);
  serverAddr.sin_family = AF_INET; 

  if(port==0)
  {
    printf("11\n");fflush(stdout);
    mysqlhost = getenv("MYSQL_HOST");
    printf("11 >%s<\n",mysqlhost);fflush(stdout);

    /* get port and host from DB; if no record in DB for <target name> - exit */
    dbsock = dbConnect(mysqlhost, "daq");

    printf("22\n");fflush(stdout);

    /* use 'port' field */
    sprintf(tmp,"SELECT Trigger_tcp FROM Ports WHERE name='%s'",targetname);
    /*printf("DB select: >%s<\n",tmp);*/
    if(dbGetInt(dbsock, tmp, &portnum) == -1) return(FALSE);

    printf("33\n");fflush(stdout);

    sprintf(tmp,"SELECT Host FROM Ports WHERE name='%s'",targetname);
    /*printf("DB select: >%s<\n",tmp);*/
    if(dbGetStr(dbsock, tmp, hostname) == -1) return(FALSE);

    printf("44\n");fflush(stdout);

    dbDisconnect(dbsock);
    printf("from database: hostname=>%s< portnum=%d\n",hostname,portnum);fflush(stdout);
  }
  else
  {
    strcpy(hostname,targetname);
    portnum = port;
    printf("from parameters list: hostname=>%s< portnum=%d\n",hostname,portnum);fflush(stdout);
  }

  serverAddr.sin_port = htons(portnum/*SERVER_PORT_NUM*/); 

  hptr = gethostbyname(hostname);
  if(hptr == NULL)
  {
    printf("unknown hostname >%s<\n",hostname); 
    close(sFd);
    exit(1);
  }
  else
  {
    memcpy(&serverAddr.sin_addr,*(hptr->h_addr_list),sizeof(sizeof(struct in_addr)));
  }

  signal(SIGALRM,alarmHandler);
  alarm(3); /* in 3 seconds connect call will be interrupted if did not finished */

  /* connect to server */ 
  if(connect (sFd, (struct sockaddr *) &serverAddr, sockAddrSize) == -1)
  {
    /*perror ("connect");*/ 
    close (sFd);
	return(FALSE);
  }

  alarm(0); /* clear alarm so it will not interrupt us */

  m_bConnected = TRUE; /*we are connected now*/
  printf("Connected successfully, sFd=%d\n",sFd);



  //ReadBoardRegisters(); sergey: have to do that on connect to get actual registers

  
#define FTOF_BOARD_ADDRESS_1 0x11A00000
#define FTOF_BOARD_ADDRESS_2 0x11A00000

  unsigned int val;

  VMERead32(FTOF_BOARD_ADDRESS_1 + 0x1000, &val);
  printf("Board 1: 'Sector' Firmware Revision: V%u.%u (0x%08x)\n", (unsigned int)(val>>8), (unsigned int)(val & 0xFF), val);

  VMERead32(FTOF_BOARD_ADDRESS_1 + 0x2000, &val);
  printf("Board 1: 'Global' Firmware Revision: V%u.%u (0x%08x)\n", (unsigned int)(val>>8), (unsigned int)(val & 0xFF), val);

  if(FTOF_BOARD_ADDRESS_1 != FTOF_BOARD_ADDRESS_2)
  {
	VMERead32(FTOF_BOARD_ADDRESS_2 + 0x1000, &val);
	printf("Board 2: 'Sector' Firmware Revision: V%u.%u\n", (unsigned int)(val>>8), (unsigned int)(val & 0xFF));

	VMERead32(FTOF_BOARD_ADDRESS_2 + 0x2000, &val);
	printf("Board 2: 'Global' Firmware Revision: V%u.%u\n", (unsigned int)(val>>8), (unsigned int)(val & 0xFF));
  }
  


  return(TRUE);
}

BOOL VMEClient::DisconnectVME()
{
  if(sFd)
  {
    close(sFd);
    sFd = 0;
  }
  m_bConnected = FALSE;
  printf("VMEClient::DisconnectVME: Disconnected\n");
  return(TRUE);
}

BOOL VMEClient::DisconnectVMEOnError()
{
  /*
  if(sFd)
  {
    close(sFd);
    sFd = 0;
  }
  */
  m_bConnected = FALSE;
  printf("Disconnected on ERROR\n");
  return(TRUE);
}

void VMEClient::DebugMsg(const char *msg)
{
  //if(m_pDbgDlg) ((DebugDlg *)m_pDbgDlg)->DebugMsg(msg);
  printf(msg);
}

#else

extern "C"
{

void
vmeclient_dummy()
{
  return;
}

}

#endif
