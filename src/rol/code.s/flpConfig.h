
/* flpConfig.h */

void flpSetExpid(char *string);
int flpConfig(char *fname);
int flpUploadAll(char *string, int length);
int flpUploadAllPrint();
int flpReadConfigFile(char *filename);
int flpDownloadAll();
