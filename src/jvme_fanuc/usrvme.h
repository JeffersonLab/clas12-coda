
/* usrvme.h */

void usrVmeDmaSetMemSize(int size);
int  usrVmeDmaGetMemSize();
int  usrVmeDmaSetChannel(int chan);
int  usrVmeDmaGetChannel();
void usrVmeDmaInit();
void usrChangeVmeDmaMemory(unsigned int pMemBase,unsigned int uMemBase, unsigned int mSize);
void usrRestoreVmeDmaMemory();
void usrVmeDmaMemory(unsigned long int *pMemBase, unsigned long int *uMemBase, unsigned int *mSize);
void usrVmeDmaGetConfig(unsigned int *addrType, unsigned int *dataType, unsigned int *sstMode);
void usrVmeDmaSetConfig(unsigned int addrType, unsigned int dataType, unsigned int sstMode);
int  usrVme2MemDmaStart(unsigned int vmeAdrs, unsigned int locAdrs, int nbytes);
int  usrVme2MemDmaDone();
void usrVme2MemDmaListSet(unsigned int *vmeAddr, unsigned long int locAddrBase, unsigned int *dmaSize, unsigned int numt);
void usrVmeDmaListStart();
unsigned int usrDmaLocalToVmeAdrs(unsigned int locAdrs);
void usrVmeDmaShow();
