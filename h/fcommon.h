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

#ifndef _FCOMMON_H
#define _FCOMMON_H

#include <fidoconf/typesize.h>
#include <stdio.h>

#include <fidoconf/fidoconf.h>

struct ticfiletype {
                char file[50];      // Name of the file affected by Tic
                char area[50];      // Name of File Area
                char areadesc[100]; // Description of File Area
                char **desc;        // Short Description of file
                int anzdesc;        // Number of Desc Lines
                char replaces[50];  // Replaces File
                int size;           // Size of file
                unsigned long crc;  // CRC of File
                unsigned long date; // Date
                s_addr from;        // From Addr
                s_addr to;          // To Addr
                s_addr origin;      // Origin
                char password[50];  // Password
                char **ldesc;       // Array of Pointer to Strings with ldescs
                int anzldesc;       // Number of Ldesc Lines
                s_addr *seenby;     // Array of Pointer to Seenbys
                int anzseenby;      // Number of seenbys
                char **path;        // Array of Pointer to Strings with Path
                int anzpath;        // Numer of Path lines
                };

typedef struct ticfiletype s_ticfile;
 

// moved to fidoconfig
//enum prio {CRASH, HOLD, NORMAL, DIRECT, IMMEDIATE};
//enum type {PKT, REQUEST, FLOFILE}; moved to fidoconfig
//typedef enum prio e_prio;
//typedef enum type e_type;

/* common functions */

void exit_htick(char *logstr, int print);
/*DOC
  exit to shell with errorlevel 1.
  print logstr to log file
  print logstr to stderr if print!=0
  closed log file, removed lockfile, disposed config
*/

int    createOutboundFileName(s_link *link, e_flavour prio, e_pollType typ);
/*DOC
  Input:  link is the link whose OutboundFileName should be created.
          prio is some kind of CRASH, HOLD, NORMAL
          typ is some kind of PKT, REQUEST, FLOFILE
  Output: a pointer to a char is returned.
  FZ:     1 is returned if link is busy
          0 else
          */

#if defined (__TURBOC__) || defined(__IBMC__) || defined(__WATCOMC__) || (defined(_MSC_VER) && (_MSC_VER >= 1200))
 int truncate(const char *fileName, long length);
 /*DOC
   Truncates the file at given position
 */
 int fTruncate( int fd, long length );
 /*DOC
   Truncates the file at given position
 */
#endif

//int    createLockFile(char *lockFile);
#endif

int removeFileMask(char *directory, char *mask);
/*DOC
  Input:  directory is the directory where remove file[s]
          mask is the file mask for remove file[s]
  Output: 
*/
int link_file(const char *from, const char *to);
