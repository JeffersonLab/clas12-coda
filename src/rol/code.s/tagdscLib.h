
/* tagdscLib.h */

#define NTAGDSC       30
#define NTAGDSCCHAN   16

void         tagdscSetExpid(char *string);
int          tagdscInit();
unsigned int tagdscReadCSR(int id);
unsigned int tagdscReadThreshold(int id);
unsigned int tagdscWriteThreshold(int id, int threshold);
unsigned int tagdscReadPulseWidth(int id);
unsigned int tagdscWritePulseWidth(int id, int width);
unsigned int tagdscLatchGateScalers(int id);
unsigned int tagdscReadGateScaler1(int id);
unsigned int tagdscReadGateScaler2(int id);
unsigned int tagdscReadChannelDelay(int id, int chan);
unsigned int tagdscWriteChannelDelay(int id, int chan, int delay);
unsigned int tagdscLatchChannelScalers(int id);
unsigned int tagdscReadChannelScaler1(int id, int chan);
unsigned int tagdscReadChannelScaler2(int id, int chan);
int          tagdscConfig(char *fname);
void         tagdscInitGlobals();
int          tagdscUploadAll(char *string, int length);
int          tagdscUploadAllPrint();
int          tagdscReadConfigFile(char *filename);
int          tagdscDownloadAll();
