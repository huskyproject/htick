#ifndef _FILELIST_H
#define _FILELIST_H

#include <fidoconf/fidoconf.h>
#if defined(__TURBOC__) && defined(MSDOS)
typedef unsigned long off_t;
#endif

void filelist(void);

#endif
