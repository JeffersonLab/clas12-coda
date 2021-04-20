
/* tiConfig.h */


void tiSetExpid(char *string);
int  tiConfig(char *fname);
int  tiInitGlobals();
int  tiReadConfigFile(char *filename);
int  tiConfigGetBlockLevel();
int  tiDownloadAll();
void tiMon(int slot);
int  tiUploadAll(char *string, int length);
int  tiUploadAllPrint();
