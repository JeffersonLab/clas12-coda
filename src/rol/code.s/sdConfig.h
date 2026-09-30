
/* sdConfig.h */

void sdSetExpid(char *string);
void sdInitGlobals();
int sdConfig(char *fname);
int sdReadConfigFile(char *filename);
int sdDownloadAll();
int sdUploadAll(char *string, int length);
int sdUploadAllPrint();
