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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef __IBMC__
#include <unistd.h>
#endif

#include <msgapi.h>

#include <version.h>

#ifndef MSDOS
#include <fidoconfig.h>
#else
#include <fidoconf.h>
#endif

#include <progprot.h>
#include <htick.h>
#include <global.h>

#include <toss.h>
#include <scan.h>
#include <hatch.h>
#include <filelist.h>
#include <recode.h>

void processCommandLine(int argc, char **argv)
{
   unsigned int i = 0;

   if (argc == 1) {
      printf(
"\nUsage: htick <command>\n"
"\n"
"Commands:\n"
" toss [announce <area>]  Reading *.tic and tossing files, announce it in area\n"
" annfile <file>          Announce new files in text file (toss and hatch)\n"
" annfecho <file>         Announce new fileecho in text file\n"
" scan                    Scanning Netmail area for mails to filefix\n"
" hatch <file> <area>\n" 
"       <description>     Hatch file into Area, using Description for file\n"
" filelist <file>         Generate filelist which includes all files in base\n"
" purge <days>            Purge files older than <days> days\n"
" send <Adress> <file>    Send file to Adress\n"
" request <Adress> <file> Request file from adress\n"
"\n"
"Not all features are implemented yet, you are welcome to implement them :)\n"
);  
  }

   while (i < argc-1) {
      i++;
      if (0 == stricmp(argv[i], "toss")) {
         cmToss = 1;
         continue;
      } else if (stricmp(argv[i], "scan") == 0) {
         cmScan = 1;
         continue;
      } else if (stricmp(argv[i], "hatch") == 0) {
         cmHatch = 1;
         i++;
         strcpy(hatchfile, argv[i++]);
         strcpy(hatcharea, argv[i++]);
         strcpy(hatchdesc, argv[i]);
         continue;
      } else if (stricmp(argv[i], "filelist") == 0) {
         cmFlist = 1;
         i++;
         strcpy(flistfile, argv[i]);
         continue;
      } else if (stricmp(argv[i], "annfile") == 0) {
         i++;
         cmAnnFile = 1;
         strcpy(announcefile, argv[i]);
         continue;
      } else if (stricmp(argv[i], "annfecho") == 0) {
         i++;
         cmAnnNewFileecho = 1;
         strcpy(announcenewfileecho, argv[i]);
         continue;
      } else printf("Unrecognized Commandline Option %s!\n", argv[i]);

   } /* endwhile */
}

void processConfig()
{
   char *buff = NULL;
   
   config = readConfig();
   if (NULL == config) {
      printf("Config not found\n");
      exit(1);
   };
   
   // lock...
   if (config->lockfile!=NULL && fexist(config->lockfile)) {
           printf("lock file found! exit...\n");
           disposeConfig(config);
           exit(1);
   } 
   else if (config->lockfile!=NULL) createLockFile(config->lockfile);

   // open Logfile
   htick_log = NULL;
   if (config->logFileDir != NULL) {
     buff = (char *) malloc(strlen(config->logFileDir)+9+1); /* 9 for htick.log */
     strcpy(buff, config->logFileDir),
     strcat(buff, "htick.log");
     if (config->loglevels==NULL)
        htick_log = openLog(buff, versionStr, "123456789");
       else
        htick_log = openLog(buff, versionStr, config->loglevels);

     free(buff);
   } else printf("You have no logFileDir in your config, there will be no log created");
   if (htick_log==NULL) printf("Could not open logfile: %s\n", buff);
   writeLogEntry(htick_log, '1', "Start");

   if (config->addrCount == 0) printf("at least one addr must be defined\n");
   if (config->linkCount == 0) printf("at least one link must be specified\n");
   if (config->fileAreaBaseDir == NULL) printf("you must set FileAreaBaseDir in fidoconfig first\n");

   if (config->addrCount == 0 ||
       config->linkCount == 0 ||
       config->fileAreaBaseDir == NULL) {
      if (config->lockfile != NULL) remove(config->lockfile);
      writeLogEntry(htick_log, '9', "wrong config file");
      writeLogEntry(htick_log, '1', "End");
      closeLog(htick_log);
      disposeConfig(config);
      exit(1);
   }
}

int main(int argc, char **argv)
{
   struct _minf m;

   sprintf(versionStr, "HTick/Linux v%u.%02u", VER_MAJOR, VER_MINOR);

   printf("Husky Tick v%u.%02u by Gabriel Plutzar\n",VER_MAJOR,VER_MINOR);

   processConfig();
   processCommandLine(argc, argv);

   // init SMAPI
   m.req_version = 0;
   m.def_zone = config->addr[0].zone;
   if (MsgOpenApi(&m) != 0) {
      writeLogEntry(htick_log, '9', "MsgApiOpen Error");
          if (config->lockfile != NULL) remove(config->lockfile);
      closeLog(htick_log);
      disposeConfig(config);
      exit(1);
   } /*endif */

   // load recoding tables
   if (config->intab != NULL) getctab(intab, config->intab);
   if (config->outtab != NULL) getctab(outtab, config->outtab);

   processTmpDir();

   if (1 == cmToss) toss();
   if (cmScan == 1) scan();
   if (cmHatch == 1) hatch();
   if (cmFlist == 1) filelist();

   // deinit SMAPI
   MsgCloseApi();

   if (config->lockfile != NULL) remove(config->lockfile);
   writeLogEntry(htick_log, '1', "End");
   closeLog(htick_log);
   disposeConfig(config);
   return 0;
}
