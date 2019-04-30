
/* tsConfig.h */

void tsSetExpid(char *string);
int  tsConfig(char *fname);
int  tsInitGlobals();
int  tsReadConfigFile(char *filename);
int  tsConfigGetBlockLevel();
int  tsConfigGetBufferLevel();
int  tsDownloadAll();
void tsMon(int slot);
int  tsUploadAll(char *string, int length);
int  tsUploadAllPrint();
