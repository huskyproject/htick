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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <fidoconf/fidoconf.h>
#include <huskylib/log.h>

struct ticfiletype
{
    char *        file;           /*  Name of the file affected by Tic */
    char *        altfile;        /*  alternative filename for duplicates */
    char *        area;           /*  Name of File Area */
    char *        areadesc;       /*  Description of File Area */
    char **       desc;           /*  Short Description of file */
    unsigned int  anzdesc;  /*  Number of Desc Lines */
    char *        replaces;       /*  Replaces File */
    int           size;           /*  Size of file */
    unsigned long crc;            /*  CRC of File */
    int           crc_is_present; /*  If CRC is present in file, for tic w/o CRC */
    unsigned long date;           /*  Date */
    hs_addr       from;           /*  From Addr */
    hs_addr       to;             /*  To Addr */
    hs_addr       origin;         /*  Origin */
    char *        password;       /*  Password */
    char **       ldesc;          /*  Array of Pointer to Strings with ldescs */
    unsigned int  anzldesc; /*  Number of Ldesc Lines */
    hs_addr *     seenby;         /*  Array of Pointer to Seenbys */
    unsigned int  anzseenby; /*  Number of seenbys */
    char **       path;           /*  Array of Pointer to Strings with Path */
    unsigned int  anzpath;  /*  Numer of Path lines */
};

typedef struct ticfiletype s_ticfile;
extern s_log * htick_log;
extern s_fidoconfig * config;
extern unsigned char quiet; /* Quiet mode */
extern char * cfgFile;
extern s_robot * robot;
/*  variables for config statements */
/*  variables for commandline statements */
extern enum tossMode cmToss;
extern int cmScan;
extern int cmHatch;
extern int cmSend;
extern int cmFlist;
extern int cmAnnounce;
extern int cmAnnNewFileecho;
extern int cmClean;
extern int cmAfix;
extern int cmNotifyLink;
extern e_relinkType cmRelink;
extern char * flistfile;
extern char * dlistfile;
extern s_ticfile * hatchInfo;
extern char sendfile[256];
extern char sendarea[256];
extern char sendaddr[256];
extern char announceArea[256];
extern char announcenewfileecho[256];
extern int lock_fd;
extern int silent_mode;
extern hs_addr afixAddr;
extern char * afixCmd;
extern hs_addr relinkFromAddr;
extern hs_addr relinkToAddr;
extern char * relinkPattern;
extern char * resubscribePatternFile;


#endif // ifndef GLOBAL_H
