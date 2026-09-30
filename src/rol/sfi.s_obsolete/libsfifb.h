/* libsfifb.h */

#ifndef __LIBSFIFB_H__
#define __LIBSFIFB_H__


/* Routine prototypes for libsfifb.c */
unsigned int fpwc(unsigned int PAddr, unsigned int SAddr, unsigned int WData);
unsigned int fprc(unsigned int PAddr, unsigned int SAddr, volatile unsigned int *rData);
unsigned int fpwd(unsigned int PAddr, unsigned int SAddr, unsigned int WData);
unsigned int fprd(unsigned int PAddr, unsigned int SAddr, volatile unsigned int *rData);
unsigned int fpwcm(unsigned int PAddr, unsigned int SAddr, unsigned int WData);
unsigned int fprcm(unsigned int PAddr, unsigned int SAddr, volatile unsigned int *rData);
unsigned int fpwdm(unsigned int PAddr, unsigned int SAddr, unsigned int WData);
unsigned int fprdm(unsigned int PAddr, unsigned int SAddr, volatile unsigned int *rData);
unsigned int fpac(unsigned int PAddr, unsigned int SAddr);
unsigned int fpad(unsigned int PAddr, unsigned int SAddr);
unsigned int fpsar();
unsigned int fpsaw(unsigned int SAddr);
unsigned int fpr();
unsigned int fpw(unsigned int WData);
unsigned int fprdb(unsigned int PAddr,
		    unsigned int SAddr,
		    unsigned int Buffer,
		    unsigned int *next_buffer,
		    unsigned int Max_ExpLWord,
		    unsigned int *cnt_RecLWord,
		    unsigned int Mode,
		    unsigned int Wait);
unsigned int fprel();
unsigned int fpram(unsigned int ramAddr, int nRet, volatile unsigned int *rData);

/* SFI Sequencer RAM Functions */
void sfiRamLoadEnable(unsigned int a);
void sfiRamLoadDisable();
void sfiLoadAddr(unsigned int a);
void sfiWait();
void sfiSetOutReg(unsigned int m);
void sfiClearOutReg(unsigned int m);

/* FASTBUS Routines for RAM List Programming */
void sfiRAMFBAC(unsigned int p, unsigned int s);
void sfiRAMFBAD(unsigned int p, unsigned int s);
void sfiRAMFBSAW(unsigned int s);
void sfiRAMFBWC(unsigned int p, unsigned int s, unsigned int d);
void sfiRAMFBWD(unsigned int p, unsigned int s, unsigned int d);
void sfiRAMFBWCM(unsigned int p, unsigned int s, unsigned int d);
void sfiRAMFBWDM(unsigned int p, unsigned int s, unsigned int d);
void sfiRAMFBR();
void sfiRAMFBW(unsigned int d);
void sfiRAMFBREL();
void sfiRAMFBBR(unsigned int c);
void sfiRAMFBBRF(unsigned int c);
void sfiRAMFBBRNC(unsigned int c);
void sfiRAMFBBRNCF(unsigned int c);
void sfiRAMFBSWC();
void sfiRAMFBEND();

/* MACROS for SEQUENCER RAM Functions */
#define SFI_RAM_LOAD_ENABLE(a) sfiRamLoadEnable(a);
#define SFI_RAM_LOAD_DISABLE   sfiRamLoadDisable();
#define SFI_LOAD_ADDR(a)       sfiLoadAddr(a);
#define SFI_WAIT               sfiWait();
#define SFI_SET_OUT_REG(m)     sfiSetOutReg(m);
#define SFI_CLEAR_OUT_REG(m)   sfiClearOutReg(m);


/* FASTBUS MACROS for RAM List Programming */
#define FBAC(p,s)    sfiRAMFBAC(p,s);
#define FBAD(p,s)    sfiRAMFBAD(p,s);
#define FBSAW(s)     sfiRAMFBSAW(s);
#define FBWC(p,s,d)  sfiRAMFBWC(p,s,d);
#define FBWD(p,s,d)  sfiRAMFBWD(p,s,d)
#define FBWCM(p,s,d) sfiRAMFBWCM(p,s,d);
#define FBWDM(p,s,d) sfiRAMFBWDM(p,s,d);
#define FBR          sfiRAMFBR();
#define FBW(d)       sfiRAMFBW(d);
#define FBREL        sfiRAMFBREL();
#define FBBR(c)      sfiRAMFBBR(c);
#define FBBRF(c)     sfiRAMFBBRF(c);
#define FBBRNC(c)    sfiRAMFBBRNC(c);
#define FBBRNCF(c)   sfiRAMFBBRNCF(c);
#define FBSWC        sfiRAMFBSWC();
#define FBEND        sfiRAMFBEND();


/* Routine prototypes for coda_sfi.h */
void SFI_ShowStatusReg();
unsigned int *GetFastbusPtr(unsigned int keyAddress);
int InitSFI(unsigned int SFIBaseAddr);
void InitFastbus(unsigned int arbReg, unsigned int timReg);
void sfi_pulse(unsigned int pmode, unsigned int pmask, unsigned int pcount);
void sfi_error_decode(int pflag);
int checkSFIinit();

#define SFI_REPORT_ERROR  sfi_error_decode(0)

/* Routine prototypes for fb_sfi_1.h */
void fb_init_1(unsigned int SFIAddr);
unsigned int fb_fwc_1(unsigned int PAddr,
		       unsigned int SAddr,
		       unsigned int WData,
		       unsigned int EG,
		       unsigned int noarb,
		       unsigned int nopa,
		       unsigned int nosa,
		       unsigned int nodata,
		       unsigned int holdas,
		       unsigned int hold);

unsigned int fb_frc_1(unsigned int PAddr,
		       unsigned int SAddr,
		       volatile unsigned int *rData,
		       unsigned int EG,
		       unsigned int noarb,
		       unsigned int nopa,
		       unsigned int nosa,
		       unsigned int nodata,
		       unsigned int holdas,
		       unsigned int hold);

unsigned int fb_fwd_1(unsigned int PAddr,
		       unsigned int SAddr,
		       unsigned int WData,
		       unsigned int EG,
		       unsigned int noarb,
		       unsigned int nopa,
		       unsigned int nosa,
		       unsigned int nodata,
		       unsigned int holdas,
		       unsigned int hold);

unsigned int fb_frd_1(unsigned int PAddr,
		       unsigned int SAddr,
		       unsigned int *rData,
		       unsigned int EG,
		       unsigned int noarb,
		       unsigned int nopa,
		       unsigned int nosa,
		       unsigned int nodata,
		       unsigned int holdas,
		       unsigned int hold);

unsigned int fb_fwcm_1(unsigned int PAddr,
			unsigned int SAddr,
			unsigned int WData,
			unsigned int noarb,
			unsigned int nopa,
			unsigned int nosa,
			unsigned int nodata,
			unsigned int holdas,
			unsigned int hold);

unsigned int fb_frcm_1(unsigned int PAddr,
			unsigned int SAddr,
			volatile unsigned int *rData,
			unsigned int noarb,
			unsigned int nopa,
			unsigned int nosa,
			unsigned int nodata,
			unsigned int holdas,
			unsigned int hold);


unsigned int fb_fwdm_1(unsigned int PAddr,
			unsigned int SAddr,
			unsigned int WData,
			unsigned int noarb,
			unsigned int nopa,
			unsigned int nosa,
			unsigned int nodata,
			unsigned int holdas,
			unsigned int hold);

unsigned int fb_frdm_1(unsigned int PAddr,
			unsigned int SAddr,
			unsigned int *rData,
			unsigned int noarb,
			unsigned int nopa,
			unsigned int nosa,
			unsigned int nodata,
			unsigned int holdas,
			unsigned int hold);

unsigned int fb_frdb_1(unsigned int PAddr,           /* Primary Address */
			unsigned int SAddr,	       /* Secondary Address */
			unsigned int *Buffer,         /* Address of LWord-Buffer (VME-Slave Address!!! )*/
			unsigned int MaxBytes,        /* Max. Count of bytes transfered  */
			volatile unsigned int *RetBytes,       /* Actual byte count of transfer */
			unsigned int noarb,           /* No arbitration cycle*/
			unsigned int nopa,            /* No Primary address cycle */
			unsigned int nosa,            /* No Secondary address cycle */
			unsigned int noda,            /* No data cycle */
			unsigned int pipe,            /* Readout-Modus (Direct and/or VME) */
			                               /* 0x10 only Direct            */
			                               /* 0x09 VMED32 Data cycle  */
			                               /* 0x0A VMED32 Blocktransfer  */
			                               /* 0x0C VMED64 Blocktransfer  */
			                               /* 0x19 Direct and VME32Datacycl. */
			                               /* 0x1A Direct and VME32Blocktr.  */
			                               /* 0x1C Direct and VME64Blocktr.  */
			unsigned int holdas,          /* Hold address lock              */
			unsigned int hold,            /* Hold address lock and bus      */
			unsigned int Wait);            /* Count for CPU idle loop before polling the Sequencer status */


/* Routine prototypes for sfiTrigLib.h */
#ifndef SFI_EXT_INT
#define SFI_EXT_INT    0
#endif
#ifndef SFI_TS_INT
#define SFI_TS_INT     1
#endif
#ifndef SFI_EXT_POLL
#define SFI_EXT_POLL   2
#endif
#ifndef SFI_TS_POLL
#define SFI_TS_POLL    3
#endif

int  sfiTrigInit(int mode);
int  sfiTrigIntConnect(unsigned int vector, VOIDFUNCPTR routine, unsigned int arg);
void sfiTrigIntDisconnect();
void sfiTrigEnable(int iflag);
void sfiTrigDisable();
int  sfiAckConnect(VOIDFUNCPTR routine, unsigned int arg);
void firstPart();
void lastPart();
void sfiTrigAck();
unsigned int sfiTrigType();
int  sfiTrigData(unsigned int *itype, unsigned int *isyncFlag, unsigned int *ilateFail);
int  sfiTrigDecodeData(unsigned int idata, unsigned int *itype,
		      unsigned int *isyncFlag, unsigned int *ilateFail);
int  sfiTrigTest();
void sfiTrigClearIntCount();
unsigned int  sfiTrigGetIntCount();
int  sfiTrigStatus();
void sfiAuxWrite(unsigned int val32);
unsigned int sfiAuxRead();

#endif /*  __LIBSFIFB_H__ */
