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
#if !(defined(_MSC_VER) && (_MSC_VER >= 1200))
#include <unistd.h>
#endif
#endif
#include <sys/types.h>
#include <signal.h>
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#include <sys/stat.h>
#endif

#include <smapi/msgapi.h>

#include <version.h>

#ifndef MSDOS
#include <fidoconf/fidoconf.h>
#else
#include <fidoconf/fidoconf.h>
#endif

#include <smapi/progprot.h>
#include <htick.h>
#include <global.h>

#include <recode.h>
#include <toss.h>
#include <scan.h>
#include <hatch.h>
#include <filelist.h>

int processCommandLine(int argc, char **argv)
{
   unsigned int i = 0;
   int rc = 1;
   char *basename, *extdelim;

   if (argc == 1) {
      printf(
"\nUsage: htick <command>\n"
"\n"
"Commands:\n"
" toss                    Reading *.tic and tossing files\n"
" [announce <area>]       Announce new files in area (toss and hatch)\n"
" [annfile <file>]        Announce new files in text file (toss and hatch)\n"
" [annfecho <file>]       Announce new fileecho in text file\n"
" scan                    Scanning Netmail area for mails to filefix\n"
" hatch <file> <area> [<description>]\n" 
" [replace [<filemask>]]  Hatch file into Area, using Description for file,\n"
"                         if exist \"replace\", then fill replace field in TIC;\n"
"                         if not exist <filemask>, then put <file> in field\n"
" filelist <file>         Generate filelist which includes all files in base\n"
" send <file> <filearea>\n"
"      <address>          Send file from filearea to address\n"
" clean                   Clean passthrough dir\n"
" purge <days>            Purge files older than <days> days (not implemented)\n"
" request <Adress> <file> Request file from adress (not implemented)\n"
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
         if (argc-i < 3) {
            printf("insufficient number of arguments\n");
            return(0);
         } 
         cmHatch = 1;
         i++;
         strcpy(hatchfile, argv[i++]);
         // Check filename for 8.3, warn if not
         basename = strrchr(hatchfile, PATH_DELIM);
         if (basename==NULL) basename = hatchfile; else basename++;
         if( (extdelim = strchr(basename, '.')) == NULL) extdelim = basename+strlen(basename);

         if (extdelim - basename > 8 || strlen(extdelim) > 4) {
            printf("Warning: hatching file with non-8.3 name!\n");
         }
         strcpy(hatcharea, argv[i++]);
         if (i < argc) {
           strcpy(hatchdesc, argv[i]);
#ifdef __NT__
           CharToOem(hatchdesc, hatchdesc);
#endif
         }
         else
           strcpy(hatchdesc, "-- description missing --");
         continue;
      } else if (stricmp(argv[i], "send") == 0) {
         cmSend = 1;
         i++;
         strcpy(sendfile, argv[i++]);
         strcpy(sendarea, argv[i++]);
         strcpy(sendaddr, argv[i]);
         continue;
      } else if (stricmp(argv[i], "replace") == 0) {
         hatchReplace = 1;
         if (i < argc-1) {
	    i++;
	    strcpy(replaceMask, argv[i]);
	 } else strcpy(replaceMask, (strrchr(hatchfile,PATH_DELIM)) ?
	                             strrchr(hatchfile,PATH_DELIM)+1 :
				     hatchfile);
         continue;
      } else if (stricmp(argv[i], "filelist") == 0) {
         cmFlist = 1;
	 if (i < argc-1) {
            i++;
            strcpy(flistfile, argv[i]);
         } else flistfile[0] = 0;
         continue;
      } else if (stricmp(argv[i], "announce") == 0) {
         i++;
         cmAnnounce = 1;
         strcpy(announceArea, argv[i]);
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
      } else if (stricmp(argv[i], "clean") == 0) {
         cmClean = 1;
         continue;
      } else {
         printf("Unrecognized Commandline Option %s!\n", argv[i]);
         rc = 0;
      }

   } /* endwhile */
return rc;
}

void processConfig()
{
#if !defined(__OS2__) && !defined(UNIX)
   time_t   time_cur, locklife = 0;
   struct   stat stat_file;
#endif
   char *buff = NULL;
   
   unsigned long pid;
   
   FILE *f;

   config = readConfig();
   if (NULL == config) {
      printf("Config not found\n");
      exit(1);
   };
   
   // lock...
   if (config->lockfile!=NULL && fexist(config->lockfile)) {
      f = fopen(config->lockfile, "rt");
      fscanf(f, "%lu\n", &pid);
      fclose(f);
      /* Checking process PID */
#ifdef __OS2__
      if (DosKillProcess(DKP_PROCESSTREE, pid) == ERROR_NOT_DESCENDANT) {
#elif UNIX
      if (kill(pid, 0) == 0) {
#else
      if (stat(config->lockfile, &stat_file) != -1) {
          time_cur = time(NULL);
	  locklife = (time_cur - stat_file.st_mtime)/60;
      }
      if (locklife < 180) {
#endif
           printf("lock file found! exit...\n");
           disposeConfig(config);
           exit(1);
      } else {
         remove(config->lockfile);
         createLockFile(config->lockfile);
      } /* endif */
   }

   // open Logfile
   htick_log = NULL;
   if (config->logFileDir != NULL) {
     buff = (char *) malloc(strlen(config->logFileDir)+9+1); /* 9 for htick.log */
     strcpy(buff, config->logFileDir),
     strcat(buff, "htick.log");
     if (config->loglevels==NULL)
        htick_log = openLog(buff, versionStr, "123456789", config->logEchoToScreen);
       else
        htick_log = openLog(buff, versionStr, config->loglevels, config->logEchoToScreen);

     free(buff);
   } else printf("You have no logFileDir in your config, there will be no log created");
   if (htick_log==NULL) printf("Could not open logfile: %s\n", buff);
   writeLogEntry(htick_log, '1', "Start");

   if (config->addrCount == 0) printf("at least one addr must be defined\n");
   if (config->linkCount == 0) printf("at least one link must be specified\n");
   if (config->fileAreaBaseDir == NULL) printf("you must set FileAreaBaseDir in fidoconfig first\n");
   if (config->passFileAreaDir == NULL) printf("you must set PassFileAreaDir in fidoconfig first\n");
   if (config->MaxTicLineLength && config->MaxTicLineLength<80)
       printf("parameter MaxTicLineLength in fidoconfig must be 0 or >80\n");

   if (config->addrCount == 0 ||
       config->linkCount == 0 ||
       config->fileAreaBaseDir == NULL ||
       config->passFileAreaDir == NULL ||
       (config->MaxTicLineLength && config->MaxTicLineLength<80)) {
      if (config->lockfile != NULL) remove(config->lockfile);
      writeLogEntry(htick_log, '9', "wrong config file");
      writeLogEntry(htick_log, '1', "End");
      closeLog(htick_log);
      disposeConfig(config);
      exit(1);
   }
   if (config->busyFileDir == NULL) {
      config->busyFileDir = (char*) malloc(strlen(config->outbound) + 10);
      strcpy(config->busyFileDir, config->outbound);
      sprintf(config->busyFileDir + strlen(config->outbound), "busy.htk%c", PATH_DELIM);
   }
}

int main(int argc, char **argv)
{
   struct _minf m;

#ifdef __linux__
   sprintf(versionStr, "HTick/LNX v%u.%02u", VER_MAJOR, VER_MINOR);
#elif __freebsd__
   sprintf(versionStr, "HTick/BSD v%u.%02u", VER_MAJOR, VER_MINOR);
#elif __OS2__
    sprintf(versionStr, "HTick/OS2 v%u.%02u", VER_MAJOR, VER_MINOR);
#elif __NT__
    sprintf(versionStr, "HTick/NT v%u.%02u", VER_MAJOR, VER_MINOR);
#elif __sun__
    sprintf(versionStr, "HTick/SUN v%u.%02u", VER_MAJOR, VER_MINOR);
#else
    sprintf(versionStr, "HTick v%u.%02u", VER_MAJOR, VER_MINOR);
#endif


   printf("Husky Tick v%u.%02u by Gabriel Plutzar\n",VER_MAJOR,VER_MINOR);

   if (processCommandLine(argc, argv) == 0) exit(1);
   processConfig();

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

   checkTmpDir();

   if (1 == cmToss) toss();
   if (cmScan == 1) scan();
   if (cmHatch == 1) hatch();
   if (cmSend == 1) send(sendfile, sendarea, sendaddr);
   if (cmFlist == 1) filelist();
   if (cmClean == 1) cleanPassthroughDir();

   // deinit SMAPI
   MsgCloseApi();

   if (config->lockfile != NULL) remove(config->lockfile);
   writeLogEntry(htick_log, '1', "End");
   closeLog(htick_log);
   disposeConfig(config);
   return 0;
}
