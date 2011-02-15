/* $Id$ */
/******************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 ******************************************************************************
 * report.h : htick reporting 
 *
 * Copyright (C) 2002 by
 *
 * Max Chernogor
 *
 * Fido:      2:464/108@fidonet 
 * Internet:  <mihz@mail.ru>,<mihz@ua.fm> 
 *
 * This file is part of HTICK 
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * FIDOCONFIG library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FIDOCONFIG library; see the file COPYING.  If not, write
 * to the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA
 * or visit http://www.gnu.org
 *****************************************************************************
 * $Id$
 */
#ifndef	_REPORT_FLAG_
#define	_REPORT_FLAG_

#include "fcommon.h"

void doSaveTic4Report(s_ticfile *tic);
void report();

#endif 
