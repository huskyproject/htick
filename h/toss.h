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

/* Append tearline and origin lines into message text and create message into
 * specified area.
 * If areaName is NULL pointer or not exist: netmail will placed into first
 * netmail area, echomail will placed into badarea.
 */
void writeMsgToSysop(s_message *msg, char *areaName, char* origin);

/* Free memory allocated in elements of structure */
void disposeTic(s_ticfile *tic);

/* Write (create) TIC-file
 * Return 0 if error, 1 if success
 */
int writeTic(char *ticfile,s_ticfile *tic);

/* Scan directory with postponed files
 */
void checkTmpDir(void);
void processTmpDir(void);

/* Process TIC-files in all inbounds */
void toss(void);

/* Write netmail message into areaname or first netmail area (if areaname is NULL pointer) */
void writeNetmail(s_message *msg, char *areaName);

/*  Check lick access to fileecho for receiving files to link */
int  readCheck(s_filearea *echo, s_link *link);
/*  Check lick access to fileecho for receiving files from link */
int  writeCheck(s_filearea *echo, ps_addr aka);

/* Send a file "filename" to all subscribers of fileecho.
 * Return: 0 if success, positive integer if error, negative integer if illegal call
 */
int  sendToLinks(int isToss, s_filearea *filearea, s_ticfile *tic, const char *filename);

/* Read TIC file and store values into 2nd parameter.
 * Return 1 if success, 0 if error.
 * Clean 2nd parameter before processing if not error.
 */
int  parseTic(char *ticfile,s_ticfile *tic);

/* Save ticket file into filearea-specific directory */
void doSaveTic(char *ticfile,s_ticfile *tic, s_filearea *filearea);


#endif
