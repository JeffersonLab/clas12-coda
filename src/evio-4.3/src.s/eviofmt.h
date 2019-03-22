
/* eviofmt.h */

int eviofmt(char *fmt, unsigned short *ifmt, int ifmtLen);
int eviofmtswap(int32_t *iarr, int nwrd, unsigned short *ifmt, int nfmt, int tolocal, int padding);
int eviofmtdump(int *arr, int nwrd, unsigned short *ifmt, int nfmt, int nextrabytes, int unpack, char *xml);
