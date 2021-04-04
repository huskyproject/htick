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
 *****************************************************************************
 * $Id$
 *****************************************************************************/

#include <string.h>

#include <fidoconf/fidoconf.h>
#include <areafix/areafix.h>
#include <toss.h>
#include <global.h>

s_log * htick_log;
s_fidoconfig * config;
unsigned char quiet   = 0;
char * cfgFile        = NULL;
s_robot * robot       = NULL;
char * versionStr     = NULL;
e_tossMode cmToss     = noToss;
int cmScan            = 0;
int cmHatch           = 0;
int cmSend            = 0;
int cmFlist           = 0;
int cmClean           = 0;
int cmAfix            = 0;
int cmNotifyLink      = 0;
e_relinkType cmRelink = modeNone;
char * flistfile      = NULL;
char * dlistfile      = NULL;
s_ticfile * hatchInfo = NULL;
char sendfile[256];
char sendarea[256];
char sendaddr[256];
int cmAnnounce = 0;
char announceArea[256];
int cmAnnNewFileecho = 0;
char announcenewfileecho[256];
int lock_fd;
int silent_mode  = 0;
hs_addr afixAddr =
{
    0
};
char * afixCmd         = NULL;
hs_addr relinkFromAddr =
{
    0, 0, 0, 0
};
hs_addr relinkToAddr =
{
    0, 0, 0, 0
};
char * relinkPattern          = NULL;
char * resubscribePatternFile = NULL;
