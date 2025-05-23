/****************************************************************************
 *
 *  fadc250Config.h  -  configuration library header file for fADC250 board 
 *
 *  SP, 07-Nov-2013
 *
 */


#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */
#define NCHAN      16


/** FADC250 configuration parameters **/
typedef struct {
  int f_rev;
  int b_rev;
  int b_ID;

  int          mode;
  int          compression;
  unsigned int winOffset;
  unsigned int winWidth;
  unsigned int nsb;
  unsigned int nsa;
  unsigned int npeak;

  unsigned int chDisMask;
  unsigned int trigMask;
  unsigned int trigWidth;
  unsigned int trigMinTOT;
  unsigned int trigMinMult;
  unsigned int thr[NCHAN];
  unsigned int dac[NCHAN];
  unsigned int delay[NCHAN];
  float        ped[NCHAN];
  unsigned int thrIgnoreMask;
  float gain[NCHAN];

} FADC250_CONF;


/* functions */

#ifdef	__cplusplus
extern "C" {
#endif

  void fadc250SetExpid(char *string);
void fadc250GetParamsForOffline(float ped[22][16], int tet[22][16], float gain[22][16], int nsa[22], int nsb[22]);
void fadc250Sethost(char *host);
void fadc250InitGlobals();
int fadc250ReadConfigFile(char *filename);
int fadc250DownloadAll();
int fadc250Config(char *fname);
void fadc250Mon(int slot);
int fadc250UploadAll(char *string, int length);

#ifdef	__cplusplus
}
#endif
