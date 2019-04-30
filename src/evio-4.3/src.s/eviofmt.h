
/* eviofmt.h */

int eviofmt(char *fmt, unsigned short *ifmt, int ifmtLen);
int eviofmtswap(int32_t *iarr, int nwrd, unsigned short *ifmt, int nfmt, int tolocal, int padding);
int eviofmtdump(int *arr, int nwrd, unsigned short *ifmt, int nfmt, int nextrabytes, int unpack, char *xml);

/* 'internal' prototypes */
/*static*/ void swap_bank(uint32_t *buf, int tolocal, uint32_t *dest);
/*static*/ void swap_segment(uint32_t *buf, int tolocal, uint32_t *dest);
/*static*/ void swap_tagsegment(uint32_t *buf, int tolocal, uint32_t *dest);
/*static*/ void swap_data(uint32_t *data, int type, int length, int tolocal, uint32_t *dest);
/*static*/ void swap_int64_t(uint64_t *data, int length, uint64_t *dest);
/*static*/ void swap_short(uint16_t *data, int length, uint16_t *dest);
/*static*/ void copy_data(uint32_t *data, int length, uint32_t *dest);
/*static*/ void swap_composite_t(uint32_t *data, int tolocal, uint32_t *dest);
