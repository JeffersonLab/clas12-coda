
/* v812.h */

#define V812_MODULE_CODE 0xFAF5
#define V812_MODULE_TYPE 0x0851

#define V812_NBOARDS   19
#define V812_ADDRSTART 0x11100000
#define V812_ADDRSTEP  0x00080000


struct v812_struct {
/*00*/ volatile unsigned short threshold[16]; /* thresholds for 16 channels */
/*20*/ volatile unsigned short res1[16];
/*40*/ volatile unsigned short width1;        /* output width channels 0-7 */
/*42*/ volatile unsigned short width2;        /* output width channels 8-15 */
/*44*/ volatile unsigned short deadtime1;     /* dead time channels 0-7 */
/*46*/ volatile unsigned short deadtime2;     /* dead time channels 8-15 */
/*48*/ volatile unsigned short majority;      /* majority threshold */
/*4A*/ volatile unsigned short inhibit;       /* pattern inhibit */
/*4C*/ volatile unsigned short test;          /* test pulse */
/*4E*/ volatile unsigned short res2[86];
/*FA*/ volatile unsigned short code;          /* fixed code */
/*FC*/ volatile unsigned short type;          /* manufacturer & module type */
/*FE*/ volatile unsigned short version;       /* version & serial number */
};


int v812Init();
int v812SetThreshold(int id, int chan, unsigned short threshold);
int v812SetThresholds(int id, unsigned short thresholds[16]);
int v812SetWidth(int id, unsigned short width1);
int v812SetDeadtime(int id, unsigned short deadtime);
int v812SetMajority(int id, unsigned short majority);
int v812DisableChannels(int id, unsigned short mask);
int v812TestPulse(int id);
