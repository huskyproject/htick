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

#ifndef _PKT_H
#define _PKT_H

#include <time.h>
#include <stdio.h>

#include <fidoconfig/fidoconfig.h>
#include <fidoconfig/typesize.h>
#include <fcommon.h>

#if !defined(__DOS__) && !defined(__MSDOS__)
#define TEXTBUFFERSIZE 512*1024    // for real os
#else
#define TEXTBUFFERSIZE 32*1024     // for Dose
#endif

struct message {
   /*Address block */
   s_addr destAddr, origAddr;

   UINT16 attributes;
   CHAR   datetime[21];
   CHAR   netMail;
   INT32  textLength;
   INT    recode;

   CHAR   *toUserName, *fromUserName, *subjectLine;
   CHAR   *text;
};

typedef struct message   s_message;

char        *getKludge(s_message msg, char *what);
/*DOC
  Input:  a s_message struct
          the kludge which is searched for
  Output: getKludge returns a pointer to the text which followed the kludge
          If the kludge does not exist it returns NULL
*/
  
void createKludges(char *buff, const char *area, const s_addr *ourAka, const s_addr *destAka);
/*DOC
   Taken from the areafix.c
   Input:  a s_addr struct
   Output: createmsgid fills char space msgid with MSGID kludge

 * Note:
 * This is a simply msgid without any hash function...
 * Imho it is not necessary to create better msgid for this purpose.
 */

#endif
