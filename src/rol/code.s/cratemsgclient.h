//
// cratemsgclient.h: interface for the CrateMsgClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(CRATEMSGCLIENT_INCLUDED_)
#define CRATEMSGCLIENT_INCLUDED_

#include <stdio.h>
#include <string.h>

typedef int BOOL;
#define TRUE 1
#define FALSE 0


#define DEBUG_NOCONNECTION				0
#define DEBUG_PRINT						0




#include "CrateMsgTypes.h"




/* ROOT stuff - temporary !!! */
#define kDefault 0
#define kFALSE 0
#define kTRUE 1




class CrateMsgClient  
{

public:
  char hostname[256];
  int hostport;

  CrateMsgClient(const char *pHost, int port);
  ~CrateMsgClient();

  bool  IsValid();
  void  Close(char *opt = (char *)"");
  /*const char* GetUrl() const;*/
  int   SendRaw(const void* buffer, int length/*, ESendRecvOptions opt = kDefault*/);
  int   RecvRaw(void* buffer, int length/*, ESendRecvOptions opt = kDefault*/);
  bool  InitConnection();
  bool  Reconnect();
  bool  CheckConnection(const char *fcn_name);
  bool  RcvRsp(int type);
  bool  Write16(unsigned int addr, unsigned short *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC);
  bool  Read16(unsigned int addr, unsigned short *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC);
  bool  Write32(unsigned int addr, unsigned int *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC);
  bool  Read32(unsigned int addr, unsigned int *val, int cnt = 1, int flags = CRATE_MSG_FLAGS_ADRINC);

  bool  ReadScalers(int slot, unsigned int **val, int *len);
  bool  GetScalers(int slot, unsigned int *val, int *len);
  bool  ReadData(int slot, unsigned int **val, int *len);
  bool  GetCrateMap(unsigned int **val, int *len);
  bool  GetBoardParams(int slot, int partype, unsigned int **val, int *len);
  bool  GetChannelParams(int slot, int channel, int partypes, unsigned int **val, int *len);
  bool  SetChannelParams(int slot, int channel, int partypes, unsigned int *val, int cnt);

  bool  Delay(unsigned int ms);

  bool  Connect(char *targetname, int port);
  bool  Disconnect();
  bool  DisconnectOnError();
  void  DebugMsg(const char *msg);


private:
  int				swap;
  CrateMsgStruct	Msg;

  /*TSocket			*pSocket;*/
  int sFd;
  bool m_bConnected;

};

#endif // !defined(CRATEMSGCLIENT_INCLUDED_)
