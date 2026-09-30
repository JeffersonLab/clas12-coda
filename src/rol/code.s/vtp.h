#ifndef __VTP_H
#define __VTP_H

#define INT16  short
#define UINT16 unsigned short
#define INT32  int
#define UINT32 unsigned int
#define STATUS int
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef OK
#define OK    0
#endif
#ifndef ERROR
#define ERROR (-1)
#endif
#define LOCAL
#ifndef _ROLDEFINED
typedef void            (*VOIDFUNCPTR) ();
typedef int             (*FUNCPTR) ();
#endif
typedef char            BOOL;

#endif /* __VTP_H */
