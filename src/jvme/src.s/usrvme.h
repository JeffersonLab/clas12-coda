
/* usrvme.h */

void usrVmeDmaSetMemSize(int size);
int  usrVmeDmaGetMemSize();
int  usrVmeDmaSetChannel(int chan);
int  usrVmeDmaGetChannel();
void usrVmeDmaInit();
void usrVmeDmaMemory(unsigned long int *pMemBase, unsigned long int *uMemBase, int32_t *mSize);
void usrVmeDmaGetConfig(unsigned int *addrType, unsigned int *dataType, unsigned int *sstMode);
void usrVmeDmaSetConfig(unsigned int addrType, unsigned int dataType, unsigned int sstMode);
int  usrVme2MemDmaStart(uint32_t vmeAdrs, unsigned long locAdrs, int32_t nbytes);
int  usrVme2MemDmaDone();
int  usrVme2MemDmaListSet(uint32_t *vmeAddr, unsigned long locAddr, int32_t *dmaSize, unsigned int numt);
void usrVmeDmaListStart();
unsigned int usrDmaLocalToVmeAdrs(unsigned long int locAdrs);
void usrVmeDmaShow();
