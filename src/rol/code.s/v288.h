
/* header file for v288.c */

/* LOW-level functions */

void v288ActiveLoop(int spas);
int v288Transmit(ULONGINT addr, UINT32 offset, UINT16 vmedat, int spas);
int v288Wait(ULONGINT addr, int spas);
int v288Reset(ULONGINT addr);


/* MID-level functions */

int v288Send1(ULONGINT addr, UINT16 crate, UINT16 code, int delay);
int v288Send2(ULONGINT addr, UINT16 value, int delay);
int v288Send3(ULONGINT addr, int delay);
int v288Send(ULONGINT addr, UINT16 crate, UINT16 code, UINT16 *value);
int v288Get(ULONGINT addr, int nw, UINT16 *buffer);


/* CAEN1527-style API functions */

int  CAENHVInitSystem(const char *SystemName, int LinkType, void *Arg,
                      const char *UserName, const char *Passwd);

int  CAENHVDeinitSystem(const char *SystemName);

char *CAENHVGetError(const char *SystemName);

int  CAENHVGetCrateMap(const char *SystemName, ushort *NrOfSlot,
                      ushort **NrofChList, char **ModelList,
                      char **DescriptionList, ushort **SerNumList,
                      unsigned char **FmwRelMinList, 
                      unsigned char **FmwRelMaxList);

int  CAENHVGetChParam(const char *SystemName, ushort slot, const char *ParName,
                      ushort ChNum, const ushort *ChList, void *ParValList);

int  CAENHVSetChParam(const char *SystemName, ushort slot, const char *ParName,
                      ushort ChNum, const ushort *ChList, void *ParValue);

int  CAENHVGetChParamProp(const char *SystemName, ushort slot, ushort Ch,
                      const char *ParName, const char *PropName, void *retval);

