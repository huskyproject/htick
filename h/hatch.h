#ifndef _HATCH_H
#define _HATCH_H

#include "global.h"

/* return values of hatch() */
enum HATCH_RETURN_CODES { HATCH_OK=0, HATCH_ERROR_FILE=1, HATCH_NOT_FILEAREA=2};

int hatch(void);
int  send(char *filename, char *area, char *addr);
int PutFileOnLink(char *filename, s_ticfile *tic, s_link* downlink);

#endif
