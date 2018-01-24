
/* usrvme.h */

void usrVmeDmaSetMemSize(int size);
int  usrVmeDmaSetChannel(int chan);
int  usrVmeDmaGetChannel();
void usrVmeDmaInit();
void usrChangeVmeDmaMemory(UINT32 pMemBase, UINT32 uMemBase, UINT32 mSize);
void usrRestoreVmeDmaMemory();
void usrVmeDmaMemory(UINT32 *pMemBase, UINT32 *uMemBase, UINT32 *mSize);
void usrVmeDmaGetConfig(unsigned int *addrType, unsigned int *dataType, unsigned int *sstMode);
void usrVmeDmaSetConfig(unsigned int addrType, unsigned int dataType, unsigned int sstMode);
int  usrVme2MemDmaStart(unsigned int vmeAdrs, unsigned int locAdrs, int nbytes);
int  usrVme2MemDmaDone();
void usrVme2MemDmaListSet(unsigned int *vmeAddr, unsigned int *locAddr, unsigned int *dmaSize, unsigned int numt);
void usrVmeDmaListStart();
unsigned int usrDmaLocalToVmeAdrs(unsigned int locAdrs);
void usrVmeDmaShow();
