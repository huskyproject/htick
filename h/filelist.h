#ifndef _FILELIST_H
#define _FILELIST_H

#include <smapi/compiler.h>

#include <fidoconf/fidoconf.h>
#if defined(__TURBOC__) && defined(__DOS__)
typedef unsigned long off_t;
#endif

void filelist(void);

#endif
