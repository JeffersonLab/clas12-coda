
/* vmmLib.h */


#define MAIN_CLOCK_PHASE_SHIFT   58
#define DIRECT_CLOCK_PHASE_SHIFT 55
#define BC_CLOCK_FREQUENCY_MODE   2 /* BC clock frequency used: 1 = 40.00 MHz,  2 = 41.66667 MHz */



/* functions */

int vmmInit();
int vmmEnable();
int vmmReadBlock(uint32_t *data, int);
int vmmDisable();
int vmmSetWindowParamters(int latency, int width);
int vmmSetGain(int igain);
int vmmSetThreshold(int thres);
int vmmSetTrimThreshold(int chan, int trimthres);
int vmmPrintMemArray();
int vmmPrintRegisters();
