#ifndef _HATCH_H
#define _HATCH_H

#include "global.h"

void hatch(void);
int  send(char *filename, char *area, char *addr);
int PutFileOnLink(char *filename, s_ticfile *tic, s_link* downlink);

#endif
