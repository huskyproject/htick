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

#ifndef TOSS_H
#define TOSS_H
#include "global.h"

enum tossSecurity {secLocalInbound, secProtInbound, secInbound};
typedef enum tossSecurity e_tossSecurity;

void writeMsgToSysop(s_message *msg, char *areaName, char* origin);
void disposeTic(s_ticfile *tic);
void writeTic(char *ticfile,s_ticfile *tic);
void checkTmpDir(void);
void processTmpDir(void);
void toss(void);
void writeNetmail(s_message *msg, char *areaName);
int  readCheck(s_filearea *echo, s_link *link);
int  writeCheck(s_filearea *echo, ps_addr aka);
int  sendToLinks(int isToss, s_filearea *filearea, s_ticfile *tic, const char *filename);
int  parseTic(char *ticfile,s_ticfile *tic);


#endif
