#ifndef _SEENBY_H_
#define _SEENBY_H_

#include <fidoconf/fidoconf.h>
#include <fcommon.h>

int seenbyAdd  ( s_addr **seenby, int* anzseenby, s_addr *Aka);
int seenbyComp ( s_addr *seenby, int anzseenby, s_addr Aka);
int seenbySort ( s_addr *seenby, int anzseenby);

#endif
