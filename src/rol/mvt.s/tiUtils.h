#ifndef TIUTILS_H
#define TIUTILS_H

#ifndef VXWORKS
#include <pthread.h>

pthread_mutex_t tiISR_mutex=PTHREAD_MUTEX_INITIALIZER;
#else
/* #include <intLib.h> */
extern int intLock();
extern int intUnlock();
#endif

#ifdef VXWORKS
int intLockKeya;
#define INTLOCK {				\
    intLockKeya = intLock();			\
}

#define INTUNLOCK {				\
    intUnlock(intLockKeya);			\
}
#else
#define INTLOCK {				\
    vmeBusLock();				\
}
#define INTUNLOCK {				\
    vmeBusUnlock();				\
}
#endif

void my_tiDeadtime();
int  my_tiLive();
void my_tiIntAck();
int  my_tiIntEnable(int iflag);
void my_tiSyncReset();
void my_tiStatusDump(int pflag, FILE* fptr);

#endif /* TILIB_H */
