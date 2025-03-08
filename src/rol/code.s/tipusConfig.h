
/* tipusConfig.h */

void tipusSetExpid(char *string);
int  tipusConfig(char *fname);
int  tipusInitGlobals();
int  tipusReadConfigFile(char *filename);
int  tipusConfigGetBlockLevel();
int  tipusDownloadAll();
void tipusMon(int slot);
int  tipusUploadAll(char *string, int length);
int  tipusUploadAllPrint();

