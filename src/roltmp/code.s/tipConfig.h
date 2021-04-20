
/* tipConfig.h */

void tipSetExpid(char *string);
int tipConfig(char *fname);
int tipInitGlobals();
int tipReadConfigFile(char *filename);
int tipConfigGetBlockLevel();
int tipDownloadAll();
void tipMon(int slot);
int tipUploadAll(char *string, int length);
int tipUploadAllPrint();

