//
// vmeclient.h: interface for the VMEClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(VMECLIENT_INCLUDED_)
#define VMECLIENT_INCLUDED_

#include "V1495VMERemote.h"

#include <stdio.h>
#include <string.h>

typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define SEL_STOF_AND	0
#define SEL_STOF_TOF	1
#define SEL_STOF_ST		2
#define SEL_STOF_0		3
	
#define SEL_ECxCC_AND	0
#define SEL_ECxCC_CC	1
#define SEL_ECxCC_ECx	2
#define SEL_ECxCC_0		3

#define SEL_MORAB_OR	0
#define SEL_MORAB_MORA	1
#define SEL_MORAB_MORB	2
#define SEL_MORAB_1		3

#define SEL_STMULT_AND	0
#define SEL_STMULT_ST0	1
#define SEL_STMULT_ST1	2
#define SEL_STMULT_LUT	3

enum
{
	LUT_TRIG,
	LUT_L2TRIG, LUT_L2SEC,
	LUT_TRIG_A1, LUT_TRIG_A2, LUT_TRIG_A3, LUT_TRIG_A4, LUT_TRIG_A5, LUT_TRIG_A6,
	LUT_TRIG_B1, LUT_TRIG_B2, LUT_TRIG_B3, LUT_TRIG_B4, LUT_TRIG_B5, LUT_TRIG_B6
};

class VMEClient  
{
public:
	BOOL m_bDebugAll;
	RemoteMsgStruct m_VMERemote;
	void DebugMsg(const char *msg);
	BOOL VMERead16(unsigned int addr, unsigned short *val, BOOL bDebug = TRUE, BOOL bIgnoreReady = FALSE);
	BOOL VMEWrite16(unsigned int addr, unsigned short val, BOOL bDebug = TRUE, BOOL bIgnoreReady = FALSE);
	BOOL VMEBlkRead16(unsigned int addr, unsigned short count, unsigned short *vals);

	BOOL VMERead32(unsigned int addr, unsigned int *val, BOOL bDebug = TRUE, BOOL bIgnoreReady = FALSE);
	BOOL VMEWrite32(unsigned int addr, unsigned int val, BOOL bDebug = TRUE, BOOL bIgnoreReady = FALSE);
	BOOL VMEBlkRead32(unsigned int addr, unsigned int count, unsigned int *vals,unsigned int NoAddressInc);

    BOOL VMEGTPReset();
    BOOL VMEGTPEventCount(unsigned int *val);
	BOOL VMEGTPGetEvent(unsigned int *evnum, unsigned int *count, unsigned int *val);

	void SetDebugDlg(void *pDbgDlg);
    int sFd; /* socket file descriptor */ 
    int ReadFromSocket(unsigned char *buf, int lenght);

	BOOL ConnectVME(char *targetname, int port);
	BOOL DisconnectVME();
	BOOL DisconnectVMEOnError();
	BOOL m_bConnected;


	void *m_pDbgDlg;

	VMEClient();
	~VMEClient();

private:
	RemoteMsgStruct		IncomingMsg;
	RemoteMsgStruct		OutgoingMsg;
	void IncDelay(int step, unsigned int * pDelay);
};

#endif // !defined(VMECLIENT_INCLUDED_)
