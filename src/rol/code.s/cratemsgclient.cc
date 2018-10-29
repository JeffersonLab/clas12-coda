
/* cratemsgclient.cc */

#if !defined(NIOS) && !defined(VXWORKS)


#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "arpa/inet.h" // htonl

#include "cratemsgclient.h"
#include "libtcp.h"
#include "libdb.h"

/*
#define DEBUG_PRINT 1
*/

#define MAX_DWORD_TRANSFER	160

static void
alarmHandler(int sig)
{
  printf("alarmHandler reached\n");fflush(stdout);
  /*signal(sig, alarmHandler);*/
}



CrateMsgClient::CrateMsgClient(const char *pHost, int port)
{
  strncpy(hostname,pHost,255);
  hostport = port;

	/*
	  pSocket = new TSocket(pHost,port,32768);
	  pSocket->SetOption(kNoDelay, 1);
      
	  if(!pSocket->IsValid())
	  {
		  printf("Failed to connected to host: %s\n", pHost);
		  Close();
		  delete pSocket;
		  pSocket = NULL;
	  }
	  else
	  	printf("Successfully connected to host: %s\n" , pHost);
	*/
  Connect(hostname,hostport);

  InitConnection();
}



CrateMsgClient::~CrateMsgClient()
{
}



bool CrateMsgClient::IsValid() 
{
	/*
    if(pSocket==NULL)
      return kFALSE;
    else
      return pSocket->IsValid();
	*/
  return(m_bConnected);
}


void CrateMsgClient::Close(char *opt)
{
  printf("CrateMsgClient::Close reached\n");
	/*
	if(pSocket)
		pSocket->Close();
	*/
  if(IsValid())
  {
    printf("CrateMsgClient::Close: calling 'Disconnect()'\n");
    Disconnect();
  }
}


/*
  const char*  CrateMsgClient::GetUrl() const
    {
      return pSocket->GetUrl();
    }
*/


int CrateMsgClient::SendRaw(const void* buffer, int length/*, ESendRecvOptions opt = kDefault*/)
{
	/*
    return pSocket->SendRaw(buffer,length,opt);
	*/
 
  if(write(sFd,(void *)buffer, length) == -1)
  {
    printf("ERROR: SendRaw() failed");
    DisconnectOnError();
    return(0);
  }
 

}



int CrateMsgClient::RecvRaw(void* buffer, int length/*, ESendRecvOptions opt = kDefault*/)
{
	/*
    return pSocket->RecvRaw(buffer,length,opt);
	*/

  int len, lll;
  unsigned char *buf = (unsigned char *) buffer;

  //printf("ReadFromSocket INFO: trying to read...\n");fflush(stdout);
  len = read(sFd,(void *)buf, length);
  if(len<=0)
  {
    perror ("read");
    printf("errno=%d len=%d\n",errno,len);
    return(len);
  }
  //printf("ReadFromSocket INFO: len=%d length=%d\n",len,length);fflush(stdout);

  while(len<length)
  {
    //printf("ReadFromSocket INFO: trying to read the rest ...\n");fflush(stdout);
    lll = read(sFd,(void *)&buf[len], length-len);
    if(lll<=0)
    {
      perror ("read");
      printf("errno=%d lll=%d\n",errno,lll);
      return(lll);
    }
    //printf("ReadFromSocket INFO: ... received %d (expected %d)\n",lll,length-len);fflush(stdout);
    len += lll;
  }
 
  return(len);
}




bool CrateMsgClient::InitConnection()
{
  int val;

  if(!IsValid()) return(0);

  // send endian test word to server
  val = CRATEMSG_HDR_ID;
  SendRaw(&val, 4);

  if(RecvRaw(&val, 4) != 4) return kFALSE;

  if(val == CRATEMSG_HDR_ID)             swap = 0;
  else if(val == LSWAP(CRATEMSG_HDR_ID)) swap = 1;
  else
  {
    printf("CrateMsgClient::InitConnection: calling 'Close()'\n");
	Close();
	return kFALSE;
  }
  return kTRUE;
}




bool CrateMsgClient::Reconnect()
{
  printf("CrateMsgClient::Reconnect: calling 'Close()'\n");
  Close();
  /*delete pSocket;*/

  printf("Reconnect... %s %d\n",hostname,hostport);

  /*pSocket = new TSocket(hostname,hostport,32768);*/
  Connect(hostname,hostport);

  return InitConnection();
}





bool CrateMsgClient::CheckConnection(const char *fcn_name)
{
  if(!IsValid())
  {
	printf("Function %s FAILED\n", fcn_name);
	return Reconnect();
  }
  return kTRUE;
}



bool CrateMsgClient::RcvRsp(int type)
{
  if(RecvRaw(&Msg, 8) == 8)
  {
	//printf("RcvRsp: len=%d\n",Msg.len);
	if(swap)
	{
	  Msg.len = LSWAP(Msg.len);
	  Msg.type = LSWAP(Msg.type);
	}

	if((Msg.len <= MAX_MSG_SIZE) && (Msg.len >= 0) && (Msg.type == (int)CMD_RSP(type)))
	{
	  if(!Msg.len) return kTRUE;

	  if(RecvRaw(&Msg.msg, Msg.len) == Msg.len) return kTRUE;
	}
  }

  printf("CrateMsgClient::RcvRsp: calling 'Close()'\n");
  Close();

  return kFALSE;
}



bool CrateMsgClient::Write16(unsigned int addr, unsigned short *val, int cnt, int flags)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.len = 12+2*cnt;
  Msg.type = CRATEMSG_TYPE_WRITE16;
  Msg.msg.m_Cmd_Write16.cnt = cnt;
  Msg.msg.m_Cmd_Write16.addr = addr;
  Msg.msg.m_Cmd_Write16.flags = flags;
  for(int i = 0; i < cnt; i++) Msg.msg.m_Cmd_Write16.vals[i] = val[i];
  SendRaw(&Msg, Msg.len+8);

#if DEBUG_PRINT
  printf("Write16 @ 0x%08X, Count = %d, Flag = %d, Vals = ", addr, cnt, flags);
  for(int i = 0; i < cnt; i++) printf("0x%04hX ", val[i]);
  printf("\n");
#endif

  return kTRUE;
}



bool CrateMsgClient::Read16(unsigned int addr, unsigned short *val, int cnt, int flags)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.len = 12;
  Msg.type = CRATEMSG_TYPE_READ16;
  Msg.msg.m_Cmd_Read16.cnt = cnt;
  Msg.msg.m_Cmd_Read16.addr = addr;
  Msg.msg.m_Cmd_Read16.flags = flags;
  SendRaw(&Msg, Msg.len+8);

#if DEBUG_PRINT
  printf("Read16 @ 0x%08X, Count = %d, Flag = %d, Vals = ", addr, cnt, flags);
#endif

  if(RcvRsp(Msg.type))
  {
	if(swap)
	{
	  Msg.msg.m_Cmd_Read16_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_Read16_Rsp.cnt);
	  for(int i = 0; i < Msg.msg.m_Cmd_Read16_Rsp.cnt; i++)
	  {
	    val[i] = HSWAP(Msg.msg.m_Cmd_Read16_Rsp.vals[i]);
	  }
	}
	else
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_Read16_Rsp.cnt; i++)
	  {
        val[i] = Msg.msg.m_Cmd_Read16_Rsp.vals[i];
	  }
	}
#if DEBUG_PRINT
	for(int i = 0; i < cnt; i++) printf("0x%04hX ", val[i]);
	printf("\n");
#endif
	return kTRUE;
  }
#if DEBUG_PRINT
  printf("failed...\n");
#endif
  return kFALSE;
}




bool CrateMsgClient::Write32(unsigned int addr, unsigned int *val, int cnt, int flags)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.len = 12+4*cnt;
  Msg.type = CRATEMSG_TYPE_WRITE32;
  Msg.msg.m_Cmd_Write32.cnt = cnt;
  Msg.msg.m_Cmd_Write32.addr = addr;
  Msg.msg.m_Cmd_Write32.flags = flags;
  for(int i = 0; i < cnt; i++) Msg.msg.m_Cmd_Write32.vals[i] = val[i];
  SendRaw(&Msg, Msg.len+8);

#if DEBUG_PRINT
  printf("Write32 @ 0x%08X, Count = %d, Flag = %d, Vals = ", addr, cnt, flags);
  for(int i = 0; i < cnt; i++) printf("0x%08X ", val[i]);
  printf("\n");
#endif

  return kTRUE;
}



bool CrateMsgClient::Read32(unsigned int addr, unsigned int *val, int cnt, int flags)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.len = 12;
  Msg.type = CRATEMSG_TYPE_READ32;
  Msg.msg.m_Cmd_Read16.cnt = cnt;
  Msg.msg.m_Cmd_Read16.addr = addr;
  Msg.msg.m_Cmd_Read16.flags = flags;
  SendRaw(&Msg, Msg.len+8);

#if DEBUG_PRINT
  printf("Read32 @ 0x%08X, Count = %d, Flag = %d, Vals = ", addr, cnt, flags);
#endif

  if(RcvRsp(Msg.type))
  {
	if(swap)
	{
	  Msg.msg.m_Cmd_Read32_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_Read32_Rsp.cnt);
	  for(int i = 0; i < Msg.msg.m_Cmd_Read32_Rsp.cnt; i++)
		val[i] = LSWAP(Msg.msg.m_Cmd_Read32_Rsp.vals[i]);
	}
	else
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_Read32_Rsp.cnt; i++)
		val[i] = Msg.msg.m_Cmd_Read32_Rsp.vals[i];
	}
#if DEBUG_PRINT
	for(int i = 0; i < cnt; i++) printf("0x%08X ", val[i]);
	printf("\n");
#endif
	return kTRUE;
  }
#if DEBUG_PRINT
  printf("failed...\n");
#endif
  return kFALSE;
}






/*****************/
/*****************/
/* start scalers */


bool CrateMsgClient::ReadScalers(int slot, unsigned int **val, int *len)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.type = SCALER_SERVER_READ_BOARD;
  Msg.len = 8;
  Msg.msg.m_Cmd_ReadScalers.cnt = 70; /* ignored by server */
  Msg.msg.m_Cmd_ReadScalers.slot = slot;

  //printf("CrateMsgClient::ReadScalers: befor cnt %d %d\n",Msg.msg.m_Cmd_ReadScalers.cnt,Msg.msg.m_Cmd_ReadScalers_Rsp.cnt);fflush(stdout);
  SendRaw(&Msg, Msg.len+8);
  //printf("CrateMsgClient::ReadScalers: after SendRaw()\n");fflush(stdout);

  if(RcvRsp(Msg.type))
  {
    //printf("CrateMsgClient::ReadScalers: after cnt %d %d\n",Msg.msg.m_Cmd_ReadScalers.cnt,Msg.msg.m_Cmd_ReadScalers_Rsp.cnt);fflush(stdout);
	if(swap)
	{
	  Msg.msg.m_Cmd_ReadScalers_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_ReadScalers_Rsp.cnt);
	}

	*val = new unsigned int[Msg.msg.m_Cmd_ReadScalers_Rsp.cnt];
	if(!(*val))
	{
      printf("CrateMsgClient::ReadScalers ERROR: cannot allocate memory - return\n");
      return kFALSE;
	}

    *len = Msg.msg.m_Cmd_ReadScalers_Rsp.cnt;
    //printf("CrateMsgClient::ReadScalers: *len=%d\n",*len);fflush(stdout);
    if(swap)
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_ReadScalers_Rsp.cnt; i++)
	  {
		(*val)[i] = LSWAP(Msg.msg.m_Cmd_ReadScalers_Rsp.vals[i]);
	  }
	}
	else
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_ReadScalers_Rsp.cnt; i++)
	  {
		(*val)[i] = Msg.msg.m_Cmd_ReadScalers_Rsp.vals[i];
		//printf("[%2d] 0x%08x\n",i,(*val)[i]);fflush(stdout);
	  }
	}
	return kTRUE;
  }

  return kFALSE;
}




bool CrateMsgClient::ReadData(int slot, unsigned int **val, int *len)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.type = DATA_SERVER_READ_BOARD;
  Msg.len = 8;
  Msg.msg.m_Cmd_ReadScalers.cnt = 70; /* ignored by server */
  Msg.msg.m_Cmd_ReadScalers.slot = slot;

  /*printf("CrateMsgClient::ReadData: befor cnt %d %d\n",Msg.msg.m_Cmd_ReadScalers.cnt,Msg.msg.m_Cmd_ReadScalers_Rsp.cnt);*/
  SendRaw(&Msg, Msg.len+8);

  if(RcvRsp(Msg.type))
  {
    /*printf("CrateMsgClient::ReadData: after cnt %d %d\n",Msg.msg.m_Cmd_ReadScalers.cnt,Msg.msg.m_Cmd_ReadScalers_Rsp.cnt);*/
	if(swap)
	{
	  Msg.msg.m_Cmd_ReadScalers_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_ReadScalers_Rsp.cnt);
	}

	*val = new unsigned int[Msg.msg.m_Cmd_ReadScalers_Rsp.cnt];
	if(!(*val))
	{
      printf("CrateMsgClient::ReadScalers ERROR: cannot allocate memory - return\n");
      return kFALSE;
	}

    *len = Msg.msg.m_Cmd_ReadScalers_Rsp.cnt;
    /*printf("CrateMsgClient::ReadData: *len=%d\n",*len);*/
    if(swap)
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_ReadScalers_Rsp.cnt; i++)
	  {
		(*val)[i] = LSWAP(Msg.msg.m_Cmd_ReadScalers_Rsp.vals[i]);
	  }
	}
	else
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_ReadScalers_Rsp.cnt; i++)
	  {
		(*val)[i] = Msg.msg.m_Cmd_ReadScalers_Rsp.vals[i];
		/*printf("[%2d] 0x%08x\n",i,(*val)[i]);fflush(stdout);*/
	  }
	}
	return kTRUE;
  }
  return kFALSE;
}








bool CrateMsgClient::GetCrateMap(unsigned int **val, int *len)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.type = SCALER_SERVER_GET_CRATE_MAP;
  Msg.len = 8; /* always have 'cnt' and 'vals[1]' */
  Msg.msg.m_Cmd_GetCrateMap.cnt = 0; /* not used in current command */
  SendRaw(&Msg, Msg.len+8); /* count 'len' and 'type' from union */

  if(RcvRsp(Msg.type))
  {
	if(swap)
	{
	  Msg.msg.m_Cmd_GetCrateMap_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_GetCrateMap_Rsp.cnt);
	}

    *val = new unsigned int[Msg.msg.m_Cmd_GetCrateMap_Rsp.cnt];
	if(!(*val))
	{
      printf("CrateMsgClient::GetCrateMap ERROR: cannot allocate memory - return\n");
      return kFALSE;
	}
    *len = Msg.msg.m_Cmd_GetCrateMap_Rsp.cnt;
	if(swap)
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_GetCrateMap_Rsp.cnt; i++)
	  {
		(*val)[i] = LSWAP(Msg.msg.m_Cmd_GetCrateMap_Rsp.vals[i]);
	  }
	}
	else
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_GetCrateMap_Rsp.cnt; i++)
	  {
		(*val)[i] = Msg.msg.m_Cmd_GetCrateMap_Rsp.vals[i];
		/*printf("GetCrateMap: [%2d] 0x%08x\n",i,(*val)[i]);fflush(stdout);*/
	  }
	}
	return kTRUE;
  }
  return kFALSE;
}


bool CrateMsgClient::GetBoardParams(int slot, int partype, unsigned int **val, int *len)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.type = SCALER_SERVER_GET_BOARD_PARAMS;
  Msg.msg.m_Cmd_GetBoardParams.slot = slot;
  Msg.msg.m_Cmd_GetBoardParams.partype = partype;
  Msg.len = 8; /* count 'slot' and 'partype' */
 // printf("CrateMsgClient::GetBoardParams: slot=%d partype=%d\n",slot,partype);
  SendRaw(&Msg, Msg.len+8); /* count 'len' and 'type' from union */

  if(RcvRsp(Msg.type))
  {
	if(swap)
	{
	  Msg.msg.m_Cmd_GetBoardParams_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_GetBoardParams_Rsp.cnt);
	}
	*val = new unsigned int[Msg.msg.m_Cmd_GetBoardParams_Rsp.cnt];
	if(!(*val))
	{
      printf("CrateMsgClient::GetBoardParams ERROR: cannot allocate memory - return\n");
      return kFALSE;
	}
    *len = Msg.msg.m_Cmd_GetBoardParams_Rsp.cnt;
   // printf("GetBoardParams: len=%d\n",*len);fflush(stdout);
	if(swap)
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_GetBoardParams_Rsp.cnt; i++)
	  {
		(*val)[i] = LSWAP(Msg.msg.m_Cmd_GetBoardParams_Rsp.vals[i]);
	  }
	}
	else
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_GetBoardParams_Rsp.cnt; i++)
	  {
		(*val)[i] = Msg.msg.m_Cmd_GetBoardParams_Rsp.vals[i];
		//printf("GetBoardParams: [%2d] 0x%08x (%d)\n",i,(*val)[i],(*val)[i]);fflush(stdout);
	  }
	}
	return kTRUE;
  }
  return kFALSE;
}



bool CrateMsgClient::GetChannelParams(int slot, int channel, int partype, unsigned int **val, int *len)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.type = SCALER_SERVER_GET_CHANNEL_PARAMS;
  Msg.msg.m_Cmd_GetChannelParams.slot = slot;
  Msg.msg.m_Cmd_GetChannelParams.channel = channel;
  Msg.msg.m_Cmd_GetChannelParams.partype = partype;
  Msg.len = 12; /* count 'slot', 'channel' and 'partype' */
  printf("CrateMsgClient::GetChannelParams: slot=%d partype=%d\n",slot,partype);
  SendRaw(&Msg, Msg.len+8); /* count 'len' and 'type' from union */

  if(RcvRsp(Msg.type))
  {
	if(swap)
	{
	  Msg.msg.m_Cmd_GetChannelParams_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_GetChannelParams_Rsp.cnt);
	}
	*val = new unsigned int[Msg.msg.m_Cmd_GetChannelParams_Rsp.cnt];
	if(!(*val))
	{
      printf("CrateMsgClient::GetChannelParams ERROR: cannot allocate memory - return\n");
      return kFALSE;
	}
    *len = Msg.msg.m_Cmd_GetChannelParams_Rsp.cnt;
    printf("GetChannelParams: len=%d\n",*len);fflush(stdout);
	if(swap)
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_GetChannelParams_Rsp.cnt; i++)
	  {
		(*val)[i] = LSWAP(Msg.msg.m_Cmd_GetChannelParams_Rsp.vals[i]);
	  }
	}
	else
	{
	  for(int i = 0; i < Msg.msg.m_Cmd_GetChannelParams_Rsp.cnt; i++)
	  {
		(*val)[i] = Msg.msg.m_Cmd_GetChannelParams_Rsp.vals[i];
		printf("GetChannelParams: [%2d] 0x%08x (%d)\n",i,(*val)[i],(*val)[i]);fflush(stdout);
	  }
	}
	return kTRUE;
  }
  return kFALSE;
}


bool  CrateMsgClient::SetChannelParams(int slot, int channel, int partype, unsigned int *val, int cnt)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.type = SCALER_SERVER_SET_CHANNEL_PARAMS;
  Msg.msg.m_Cmd_SetChannelParams.slot = slot;
  Msg.msg.m_Cmd_SetChannelParams.channel = channel;
  Msg.msg.m_Cmd_SetChannelParams.partype = partype;
  Msg.msg.m_Cmd_SetChannelParams.cnt = cnt;
  Msg.len = 16+4*cnt;
  for(int i = 0; i < cnt; i++) Msg.msg.m_Cmd_SetChannelParams.vals[i] = val[i];
  SendRaw(&Msg, Msg.len+8);

  return kTRUE;
}

/* end scalers */
/***************/
/***************/






bool  CrateMsgClient::Delay(unsigned int ms)
{
  if(!CheckConnection(__FUNCTION__)) return kFALSE;

  Msg.len = 4;
  Msg.type = CRATEMSG_TYPE_DELAY;
  Msg.msg.m_Cmd_Delay.ms = ms;
  SendRaw(&Msg, Msg.len+8);

  return kTRUE;
}

























bool CrateMsgClient::Connect(char *targetname, int port)
{
  struct hostent *hptr;
  struct sockaddr_in serverAddr; /* server's socket address */ 
  int sockAddrSize; /* size of socket address structure */ 
  int portnum = 0;
  char hostname[128];
  MYSQL *dbsock;
  char tmp[1000], *mysqlhost;

  printf("CrateMsgClient::Connect: targetname=>%s< \n",targetname);fflush(stdout);

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


  return(TRUE);
}






bool CrateMsgClient::Disconnect()
{
  if(sFd)
  {
    close(sFd);
    sFd = 0;
  }
  m_bConnected = FALSE;
  printf("Disconnected\n");
  return(TRUE);
}




bool CrateMsgClient::DisconnectOnError()
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



void CrateMsgClient::DebugMsg(const char *msg)
{
  //if(m_pDbgDlg) ((DebugDlg *)m_pDbgDlg)->DebugMsg(msg);
  printf(msg);
}




#else

extern "C"
{

void
cratemsgclient_dummy()
{
  return;
}

}

#endif
