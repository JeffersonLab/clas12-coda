
/* helicity.h - delay reporting correction main function */

#define INTERVAL 0x7f2815

#define FORCE_RECOVERY \
  forceRecovery(); \
  quad = -1; \
  strob = -1; \
  helicity = -1; \
  for(i=0; i<NPREV; i++) \
  { \
    str_pipe[i]=-1; \
    hel_pipe[i]=-1; \
    quad_pipe[i]=-1; \
  } \
  remember_helicity[0]=remember_helicity[1]=remember_helicity[2]=-1; \
  done = -1; /* will be change to '0' in the beginning of quartet and to '1' when prediction is ready */ \
  offset = 0



#ifdef  __cplusplus
extern "C" {
#endif


/* functions */

void forceRecovery();
int loadHelicity(int, unsigned int *, unsigned int *);
int ranBit(unsigned int *seed);
int ranBit0(unsigned int *seed);
unsigned int getSeed();

int helicity(unsigned int *jw, int type);


#ifdef  __cplusplus
}
#endif

