
/* hist_lib.h */

void* hist_find(char *name, TList*  histptr);
void* hist_client(void * arg);
void* hist_client2(void * arg);

int STREQ(const char*s1, const char*s2);

struct SRV_PARAM {
  char REM_HOST[256];
  char thread_name[64];
  TCanvas  *canvas;
  int pad;
  int tab;

  TH1 *RunInfo;
};

