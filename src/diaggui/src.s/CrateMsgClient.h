#ifndef CrateMsgClient_H
#define CrateMsgClient_H

#ifndef __FUNCTION__
#define __FUNCTION__ "foo"
#endif


#include "RootHeader.h"

#define DEBUG_NOCONNECTION				0
#define DEBUG_PRINT						0

#define CRATEMSG_LISTEN_PORT			6102
#define MAX_MSG_SIZE					10000

#define CRATEMSG_HDR_ID					0x12345678

#define CRATEMSG_TYPE_READ16			0x01
#define CRATEMSG_TYPE_WRITE16			0x02
#define CRATEMSG_TYPE_READ32			0x03
#define CRATEMSG_TYPE_WRITE32			0x04
#define CRATEMSG_TYPE_DELAY			0x05
#define CRATEMSG_TYPE_READSCALERS	0x06

#define CMD_RSP(cmd)					(cmd | 0x80000000)

#define LSWAP(v)						( ((v>>24) & 0x000000FF) |\
                					      ((v<<24) & 0xFF000000) |\
                					      ((v>>8)  & 0x0000FF00) |\
                					      ((v<<8)  & 0x00FF0000) )

#define HSWAP(v)						( ((v>>8) & 0x00FF) |\
                					      ((v<<8) & 0xFF00) )

/*****************************************************/
/*********** Some Board Generic Commands *************/
/*****************************************************/

#define CRATE_MSG_FLAGS_NOADRINC		0x0
#define CRATE_MSG_FLAGS_ADRINC			0x1
#define CRATE_MSG_FLAGS_USEDMA			0x2

typedef struct
{
	int cnt;
	int addr;
	int flags;
} Cmd_Read16;

typedef struct
{
	int cnt;
	short vals[MAX_MSG_SIZE/2];
} Cmd_Read16_Rsp;

typedef struct
{
	int cnt;
	int addr;
	int flags;
	short vals[MAX_MSG_SIZE/2];
} Cmd_Write16;

typedef struct
{
	int cnt;
	int addr;
	int flags;
} Cmd_Read32;

typedef struct
{
	int cnt;
	int vals[MAX_MSG_SIZE/4];
} Cmd_Read32_Rsp;

typedef struct
{
	int cnt;
	int addr;
	int flags;
	int vals[MAX_MSG_SIZE/4];
} Cmd_Write32;

typedef struct
{
	int ms;
} Cmd_Delay;

typedef struct
{
	int cnt;
	unsigned int vals[MAX_MSG_SIZE/4];
} Cmd_ReadScalers_Rsp;

/*****************************************************/
/*************** Main message structure **************/
/*****************************************************/
typedef struct
{
	int len;
	int type;
  
	union
	{
		Cmd_Read16				m_Cmd_Read16;
		Cmd_Read16_Rsp			m_Cmd_Read16_Rsp;
		Cmd_Write16				m_Cmd_Write16;
		Cmd_Read32				m_Cmd_Read32;
		Cmd_Read32_Rsp			m_Cmd_Read32_Rsp;
		Cmd_Write32				m_Cmd_Write32;
		Cmd_Delay				m_Cmd_Delay;
		Cmd_ReadScalers_Rsp		m_Cmd_ReadScalers_Rsp;
		unsigned char			m_raw[MAX_MSG_SIZE];
	} msg;
} CrateMsgStruct;

class CrateMsgClient		: public TNamed
{
public:
	TString hostname;
	int hostport;
  bool tcp64bit_mode;

  CrateMsgClient(const char *pHost, int port, bool tcp64bit)
	{
		hostname = Form("%s",pHost);
		hostport = port;
    tcp64bit_mode = tcp64bit;

#if DEBUG_NOCONNECTION
		pSocket = NULL;
		return;
#endif
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
      printf("Successfully connected to host:(64bitmode=%d) %s\n" , tcp64bit_mode ? 1 : 0, pHost);

		InitConnection();
	}

  Bool_t IsValid() 
  {
    if(pSocket==NULL)
      return kFALSE;
    else
      return pSocket->IsValid();
  }

  void Close(Option_t* opt = "")
  {
	if(pSocket)
		pSocket->Close();
  }

  const char* GetUrl() const
    {
      return pSocket->GetUrl();
    }

  Int_t SendRaw(const void* buffer, Int_t length, ESendRecvOptions opt = kDefault)
  {
    return pSocket->SendRaw(buffer,length,opt);
  }
  
  Int_t	RecvRaw(void* buffer, Int_t length, ESendRecvOptions opt = kDefault)
  {
    return pSocket->RecvRaw(buffer,length,opt);
  }

  Bool_t InitConnection()
  {
		int val;

		if(!pSocket || !pSocket->IsValid())
			return kFALSE;

		// send endian test word to server
		val = CRATEMSG_HDR_ID;
		SendRaw(&val, 4);

    if(tcp64bit_mode)
    {
      val = 0;
      SendRaw(&val, 4);
    }

		if(RecvRaw(&val, 4) != 4)
			return kFALSE;

    printf("CRATEMSG_HDR_ID=0x%08X\n", val);

		if(val == CRATEMSG_HDR_ID)
			swap = 0;
		else if(val == LSWAP(CRATEMSG_HDR_ID))
			swap = 1;
		else
		{
			Close();
			return kFALSE;
		}
		return kTRUE;
  }

	Bool_t Reconnect()
	{
		Close();
		delete pSocket;
		printf("Reconnect... %s %d\n",hostname.Data(),hostport);
		pSocket = new TSocket(hostname.Data(),hostport,32768);

		return InitConnection();
	}

	Bool_t CheckConnection(const char *fcn_name)
	{
#if DEBUG_NOCONNECTION
		return kFALSE;
#endif
		if(!IsValid())
		{
			printf("Function %s FAILED\n", fcn_name);
			return Reconnect();
		}
		return kTRUE;
	}

	Bool_t RcvRsp(int type)
	{
		if(RecvRaw(&Msg, 8) == 8)
		{
			if(swap)
			{
				Msg.len = LSWAP(Msg.len);
				Msg.type = LSWAP(Msg.type);
			}
			if((Msg.len <= MAX_MSG_SIZE) && (Msg.len >= 0) && (Msg.type == (int)CMD_RSP(type)))
			{
				if(!Msg.len)
					return kTRUE;

#if DEBUG_PRINT
				printf(">>> Msg.len=%d\n",Msg.len);
#endif
				if(RecvRaw(&Msg.msg, Msg.len) == Msg.len)
					return kTRUE;
			}
		}
		Close();
		return kFALSE;
	}

	Bool_t Write16(unsigned int addr, unsigned short *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC)
	{
		if(!CheckConnection(__FUNCTION__))
			return kFALSE;

		Msg.len = 12+2*cnt;
		Msg.type = CRATEMSG_TYPE_WRITE16;
		Msg.msg.m_Cmd_Write16.cnt = cnt;
		Msg.msg.m_Cmd_Write16.addr = addr;
		Msg.msg.m_Cmd_Write16.flags = flags;
		for(int i = 0; i < cnt; i++)
			Msg.msg.m_Cmd_Write16.vals[i] = val[i];
		SendRaw(&Msg, Msg.len+8);

#if DEBUG_PRINT
		printf("Write16 @ 0x%08X, Count = %d, Flag = %d, Vals = ", addr, cnt, flags);
		for(int i = 0; i < cnt; i++)
			printf("0x%04hX ", val[i]);
		printf("\n");
#endif

		return kTRUE;
	}

	Bool_t Read16(unsigned int addr, unsigned short *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC)
	{
		if(!CheckConnection(__FUNCTION__))
			return kFALSE;

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
					val[i] = HSWAP(Msg.msg.m_Cmd_Read16_Rsp.vals[i]);
			}
			else
			{
				for(int i = 0; i < Msg.msg.m_Cmd_Read16_Rsp.cnt; i++)
					val[i] = Msg.msg.m_Cmd_Read16_Rsp.vals[i];
			}
#if DEBUG_PRINT
		for(int i = 0; i < cnt; i++)
			printf("0x%04hX ", val[i]);
		printf("\n");
#endif
			return kTRUE;
		}
#if DEBUG_PRINT
		printf("failed...\n");
#endif
		return kFALSE;
	}

	Bool_t Write32(unsigned int addr, unsigned int *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC)
	{
		if(!CheckConnection(__FUNCTION__))
			return kFALSE;

		Msg.len = 12+4*cnt;
		Msg.type = CRATEMSG_TYPE_WRITE32;
		Msg.msg.m_Cmd_Write32.cnt = cnt;
		Msg.msg.m_Cmd_Write32.addr = addr;
		Msg.msg.m_Cmd_Write32.flags = flags;
		for(int i = 0; i < cnt; i++)
			Msg.msg.m_Cmd_Write32.vals[i] = val[i];
		SendRaw(&Msg, Msg.len+8);

#if DEBUG_PRINT
		printf("Write32 @ 0x%08X, Count = %d, Flag = %d, Vals = ", addr, cnt, flags);
		for(int i = 0; i < cnt; i++)
			printf("0x%08X ", val[i]);
		printf("\n");
#endif

		return kTRUE;
	}

	Bool_t Read32(unsigned int addr, unsigned int *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC)
	{
		if(!CheckConnection(__FUNCTION__))
			return kFALSE;

    if(tcp64bit_mode)
    {
      int val = 0;
      SendRaw(&val, 4);
    }
		Msg.len = 12;
		Msg.type = CRATEMSG_TYPE_READ32;
    Msg.msg.m_Cmd_Read32.cnt = cnt;
    Msg.msg.m_Cmd_Read32.addr = addr;
    Msg.msg.m_Cmd_Read32.flags = flags;
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
			printf("Msg.msg.m_Cmd_Read32_Rsp.cnt=%d\n",Msg.msg.m_Cmd_Read32_Rsp.cnt);
		for(int i = 0; i < cnt; i++)
			printf("0x%08X ", val[i]);
		printf("\n");
#endif
			return kTRUE;
		}
#if DEBUG_PRINT
		printf("failed...\n");
#endif
		return kFALSE;
	}

	Bool_t ReadScalers(unsigned int **val, int *len)
	{
		if(!CheckConnection(__FUNCTION__))
			return kFALSE;

		Msg.len = 0;
		Msg.type = CRATEMSG_TYPE_READSCALERS;
		SendRaw(&Msg, Msg.len+8);

		if(RcvRsp(Msg.type))
		{
			if(swap)
				Msg.msg.m_Cmd_Read32_Rsp.cnt = LSWAP(Msg.msg.m_Cmd_Read32_Rsp.cnt);

			*val = new unsigned int[Msg.msg.m_Cmd_Read32_Rsp.cnt];
			if(!(*val))
				return kFALSE;

			if(swap)
			{
				for(int i = 0; i < Msg.msg.m_Cmd_Read32_Rsp.cnt; i++)
					(*val)[i] = LSWAP(Msg.msg.m_Cmd_Read32_Rsp.vals[i]);
			}
			else
			{
				for(int i = 0; i < Msg.msg.m_Cmd_Read32_Rsp.cnt; i++)
					(*val)[i] = Msg.msg.m_Cmd_Read32_Rsp.vals[i];
			}
			return kTRUE;
		}
		return kFALSE;
	}

	Bool_t Delay(unsigned int ms)
	{
		if(!CheckConnection(__FUNCTION__))
			return kFALSE;

		Msg.len = 4;
		Msg.type = CRATEMSG_TYPE_DELAY;
		Msg.msg.m_Cmd_Delay.ms = ms;
		SendRaw(&Msg, Msg.len+8);

		return kTRUE;
	}

private:
	int				swap;
	CrateMsgStruct	Msg;
	TSocket			*pSocket;
};

#endif
