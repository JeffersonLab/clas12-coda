
/* vmeserver.h */

#ifndef VXWORKS
#define TRUE  1
#define FALSE 0
#define STATUS int
#endif

#ifndef OK
#define OK 0
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

/* functions */

int vmeSetScalersReadInterval(int time);
int vmeGetScalersReadInterval();
void vmeReadTask();
int vmeScalersReadoutAdd(unsigned int addr, unsigned int len, unsigned int enable);
int vmeScalersEnableAdd(unsigned int addr, unsigned int enable);
int vmeScalersDisableAdd(unsigned int addr, unsigned int disable);
int vmeScalersReadoutStart();
int vmeScalersReadoutStop();
int vmeScalersRead();
int vmeScalersReadFromMemory();
unsigned int vmeGetScalerMemoryAddress(unsigned int addr);
unsigned int vmeGetScalerFromMemory(unsigned int addr);
int vmeProcessRemoteMsg(RemoteMsgStruct *pRemoteMsgStruct);


#ifdef VXWORKS
int vmeServer(void);
#else
int vmeServer(char *name);
#endif

STATUS vmetcpServer(void);
void vmetcpServerWorkTask(TWORK *targ);
