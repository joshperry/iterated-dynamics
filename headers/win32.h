/* win32.h - Win32 port declarations */
#ifndef WIN32_H
#define WIN32_H

typedef float FLOAT4;
#define remove(x) unlink(x)
#if !defined(_MAX_FNAME)
#define _MAX_FNAME 20
#endif
#if !defined(_MAX_EXT)
#define _MAX_EXT 4
#endif

/* dysize may conflict with time.h */
#define dysize dysize1

#define bcopy(src,dst,n) memcpy(dst,src,n)
#define bzero(buf,siz) memset(buf,0,siz)
#define bcmp(buf1,buf2,len) memcmp(buf1,buf2,len)

/* ftime replacement */
#include <sys/types.h>
typedef struct  timebx
{
    time_t  time;
    unsigned short millitm;
    int   timezone;
    int   dstflag;
} timebx;


#endif
