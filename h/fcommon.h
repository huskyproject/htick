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

#include <stdio.h>

#include <huskylib/typesize.h>
#include <fidoconf/fidoconf.h>


/* common functions */

void exit_htick(char *logstr, int print);
/*DOC
  exit to shell with errorlevel 1.
  print logstr to log file
  print logstr to stderr if print!=0
  closed log file, removed lockfile, disposed config
*/

int    createOutboundFileName(s_link *link, e_flavour prio, e_pollType typ);
int    createOutboundFileNameAka(s_link *link, e_flavour prio, e_pollType typ, hs_addr *aka);
/*DOC
  Input:  link is the link whose OutboundFileName should be created.
          prio is some kind of CRASH, HOLD, NORMAL
          typ is some kind of PKT, REQUEST, FLOFILE
  Output: a pointer to a char is returned.
  FZ:     1 is returned if link is busy
          0 else
          */

int removeFileMask(char *directory, char *mask);
/*DOC
  Input:  directory is the directory where remove file[s]
          mask is the file mask for remove file[s]
  Output: 
*/
int link_file(const char *from, const char *to);

#endif
