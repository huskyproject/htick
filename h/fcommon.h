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

#include <typesize.h>
#include <stdio.h>

#include <fidoconfig.h>

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
 


enum prio {CRASH, HOLD, NORMAL, DIRECT, IMMEDIATE};
enum type {PKT, REQUEST, FLOFILE};
typedef enum prio e_prio;
typedef enum type e_type;

/* common functions */

e_prio cvtFlavour2Prio(e_flavour flavour);
/*DOC
  Input:  a fidoconfig flavour
  Output: a hpt prio
  FZ:     obvious
*/

void addAnotherPktFile(s_link *link, char *filename);
/*DOC
  Input:  a pointer to a link structure, a pointer to a filename
  FZ:     Adds the string pointed to by filename to the list off additional
          pktfiles for the specified link. No checks are performed. The
          string is duplicated internally.
*/
  
int   createTempPktFileName(s_link *link);
/*DOC
  Input:  a pointer to a link structure
  Output: 0 is returned if a filename and a packedfilename could be created.
          1 else
  FZ:     createTempPktFile tries to compose a new, not used pktfilename.
          It takes the least 24bit of the actual time. The last 2 Bytes
          area filled with a counter. So you can get up to 256 different files
          in a second and have the same timestamp only every 291 days.
          The pktFileName and the packFileName are stored in the link
          structure
*/

int    createDirectoryTree(const char *pathName);
/*DOC
  Input:  a pointer to a 
  Output: 0 if successfull, 1 else
  FZ:     pathName is a correct directory name
          createDirectoryTree creates the directory and all parental directories
          if they do not exist.
*/
  
int    createOutboundFileName(s_link *link, e_prio prio, e_type typ);
/*DOC
  Input:  link is the link whose OutboundFileName should be created.
          prio is some kind of CRASH, HOLD, NORMAL
          typ is some kind of PKT, REQUEST, FLOFILE
  Output: a pointer to a char is returned.
  FZ:     1 is returned if link is busy
          0 else
          */

#ifdef __TURBOC__
 int truncate(const char *fileName, long length);
 /*DOC
   Truncates the file at given position
 */
#endif


int    createLockFile(char *lockFile);
#endif
