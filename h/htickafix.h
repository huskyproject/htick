/* $Id$ */
/*****************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 *****************************************************************************
 * Copyright (C) 1999 by
 *
 * Gabriel Plutzar
 *
 * Fido:     2:31/1
 * Internet: gabriel@hit.priv.at
 *
 * Vienna, Austria, Europe
 *
 * This file is part of HTICK, which is based on HPT by Matthias Tichy,
 * 2:2432/605.14 2:2433/1245, mtt@tichy.de
 *
 * HTICK is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * HTICK is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HTICK; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#ifndef _HTICKAFIX_H
#define _HTICKAFIX_H

#include "fcommon.h"

#define FFERROR 255

int processFileFix(s_message * msg);
void ffix(hs_addr addr, char * cmd);
int init_htickafix(void);

/* these two functions are to be removed from htick after merging filefix and */
/* areafix */
int readCheck(s_area * echo, s_link * link);

/*  '\x0000' access o'k */
/*  '\x0001' no access group */
/*  '\x0002' no access level */
/*  '\x0003' no access export */
/*  '\x0004' not linked */
int e_writeCheck(s_fidoconfig * config, s_area * echo, s_link * link);

/*  '\x0000' access o'k */
/*  '\x0001' no access group */
/*  '\x0002' no access level */
/*  '\x0003' no access import */
/*  '\x0004' not linked */


#endif // ifndef _HTICKAFIX_H
