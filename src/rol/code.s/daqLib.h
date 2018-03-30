
/* daqLib.h - data aquisition-related library's header */

#ifndef DAQLIB_H
#define DAQLIB_H

int  daqInit();
int  daqGetReportRawData();
int  daqGetExternalVmeReadoutInterval();

int  daqConfig(char *fname);
int  daqInitGlobals();
int  daqReadConfigFile(char *filename);
int  daqDownloadAll();
void daqMon(int slot);
int  daqUploadAll(char *string, int length);
int  daqUploadAllPrint();

#endif /* DAQLIB_H */
