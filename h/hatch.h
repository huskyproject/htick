/* $Id$ */
#ifndef _HATCH_H
#define _HATCH_H

#include "global.h"
/* return values of hatch() */
typedef enum
{
    HATCH_OK = 0, HATCH_ERROR_FILE = 1, HATCH_NOT_FILEAREA = 2
} HATCH_HATCH_RETURN_CODES;
typedef enum
{
    HATCH_SEND_E_INVALID          = -1, HATCH_SEND_OK = 0, HATCH_SEND_E_PASSTHROUGH = 1,
    HATCH_SEND_E_FILEAREANOTFOUND = 2,  /* filearea not found */
    HATCH_SEND_E_FILENOTFOUND     = 3,  /* file not found     */
    HATCH_SEND_E_LINKNOTFOUND     = 4,  /* link not found     */
    HATCH_SEND_E_PUT_ON_LINK      = 5   /* put on link error  */
} HATCH_SEND_RETURN_CODES;
HATCH_HATCH_RETURN_CODES hatch(void);
HATCH_SEND_RETURN_CODES send(char * filename, char * area, char * addr);

/* Return values: 1 if success, 0 if error */
int PutFileOnLink(char * filename, s_ticfile * tic, s_link * downlink);

#endif
