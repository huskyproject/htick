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

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <log.h>
#include <global.h>
#include <fcommon.h>
#include <smapi/prog.h>

static char *mnames[] = {
"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

static char *wdnames[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

s_log *openLog(char *fileName, char *appN, char *keys, unsigned int echoLog)
{
   s_log      *temp;

   temp = (s_log *) malloc(sizeof(s_log));
   temp->logFile = fopen(fileName, "a");
   if (NULL == temp->logFile) {
      free(temp);
      return NULL;
   } /* endif */

   temp->open = 1;

   /* copy all informations */
   temp->appName = (char *) malloc (strlen(appN)+1);
   strcpy(temp->appName, appN);

   temp->keysAllowed = (char *) malloc (strlen(keys)+1);
   strcpy(temp->keysAllowed, keys);

   temp->firstLinePrinted=0;

   temp->logEcho = echoLog;

   return temp;
}

void closeLog(s_log *htick_log)
{
   if (htick_log != NULL) {
      if (htick_log->open != 0) {
         if (htick_log->firstLinePrinted)
            fprintf(htick_log->logFile, "\n");
         fclose(htick_log->logFile);
         htick_log->open = 0;
      } /* endif */
      free(htick_log->appName);
      free(htick_log->keysAllowed);
      free(htick_log);
      htick_log = NULL;
   }
}

void writeLogEntry(s_log *htick_log, char key, char *logString, ...)
{
   time_t     currentTime;
   struct tm  *locTime;
	va_list	  ap;

   if (htick_log) {
     if (htick_log->open && strchr(htick_log->keysAllowed, key)) 
        {
        currentTime = time(NULL);
        locTime = localtime(&currentTime);
	
        if (!htick_log->firstLinePrinted)
           {
           /* make first line of log */
           fprintf(htick_log->logFile, "----------  ");

	   fprintf(htick_log->logFile, "%3s %02u %3s %02u, %s\n",
             wdnames[locTime->tm_wday], locTime->tm_mday,
	     mnames[locTime->tm_mon], locTime->tm_year % 100, htick_log->appName);

           htick_log->firstLinePrinted=1;
           }

        fprintf(htick_log->logFile, "%c %02u.%02u.%02u  ", key, locTime->tm_hour, locTime->tm_min, locTime->tm_sec);
 
	va_start(ap, logString);
	vfprintf(htick_log->logFile, logString, ap);
	va_end(ap);
	fputc('\n', htick_log->logFile); 

        fflush(htick_log->logFile);
	if (htick_log->logEcho) {
	   fprintf(stdout, "%c %02u.%02u.%02u  ", key, locTime->tm_hour, locTime->tm_min, locTime->tm_sec);
	   va_start(ap, logString);
	   vfprintf(stdout, logString, ap);
	   va_end(ap);
	   fputc('\n', stdout);
	};
     }
   }
}