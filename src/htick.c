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
#if ((!(defined(_MSC_VER) && (_MSC_VER >= 1200))) && (!defined(__TURBOC__)))
#  include <unistd.h>
#endif
#endif
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#if ((defined(_MSC_VER) && (_MSC_VER >= 1200)) || defined(__TURBOC__) || defined(__DJGPP__)) || defined(__MINGW32__)
#  include <io.h>
#endif

#ifdef OS2
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include <os2.h>
#ifndef __OS2__
#define __OS2__
#endif
#endif

#include <smapi/msgapi.h>

#include <version.h>

#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <fidoconf/log.h>

#include <smapi/progprot.h>
#include <htick.h>
#include <global.h>
#include <cvsdate.h>

#include <fidoconf/recode.h>
#include <fidoconf/xstr.h>
#include <toss.h>
#include <scan.h>
#include <hatch.h>
#include <filelist.h>

#if (defined(__EMX__) || defined(__MINGW32__)) && defined(__NT__)
/* we can't include windows.h for several reasons ... */
#ifdef __MINGW32__
int __stdcall CharToOemA(char *, char *);
int __stdcall SetFileApisToOEM(void);
#endif
#define CharToOem CharToOemA
#endif

int processCommandLine(int argc, char **argv)
{
   int i = 0;
   int rc = 1;
   char *basename, *extdelim;

   if (argc == 1) {
      printf(
"\nUsage: htick [-q] <command>\n"
"\n"
" -q                      Quiet mode (display only urgent messages to console)\n"
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
"\n"
"Not all features are implemented yet, you are welcome to implement them :)\n"
            );
      return 0;
  }

   while (i < argc-1) {
      i++;
      if ( !strcmp(argv[i], "-q") ) {
         quiet=1;
         continue;
      }
      if (0 == stricmp(argv[i], "toss")) {
         cmToss = 1;
         continue;
      } else if (stricmp(argv[i], "scan") == 0) {
         cmScan = 1;
         continue;
      } else if (stricmp(argv[i], "hatch") == 0) {
         if (argc-i < 3) {
            fprintf(stderr, "Insufficient number of arguments\n");
            return(0);
         }
         cmHatch = 1;
         i++;
         strcpy(hatchfile, (argv[i]!=NULL)?argv[i++]:"");
         // Check filename for 8.3, warn if not
         basename = strrchr(hatchfile, PATH_DELIM);
         if (basename==NULL) basename = hatchfile; else basename++;
         if( (extdelim = strchr(basename, '.')) == NULL) extdelim = basename+strlen(basename);

         if (extdelim - basename > 8 || strlen(extdelim) > 4) {
            if (!quiet) fprintf(stderr, "Warning: hatching file with non-8.3 name!\n");
         }
         strcpy(hatcharea, (argv[i]!=NULL)?argv[i++]:"");
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
         strcpy(sendfile, (argv[i]!=NULL)?argv[i++]:"");
         strcpy(sendarea, (argv[i]!=NULL)?argv[i++]:"");
         strcpy(sendaddr, (argv[i]!=NULL)?argv[i]:"");
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
	 if (i < argc-1) {
            i++;
            cmAnnounce = 1;
            strcpy(announceArea, argv[i]);
         } else {
	    fprintf(stderr, "Insufficient number of arguments\n");
	 }
         continue;
      } else if (stricmp(argv[i], "annfile") == 0) {
	 if (i < argc-1) {
            i++;
            cmAnnFile = 1;
            strcpy(announcefile, argv[i]);
         } else {
	    fprintf(stderr, "Insufficient number of arguments\n");
	 }
         continue;
      } else if (stricmp(argv[i], "annfecho") == 0) {
	 if (i < argc-1) {
            i++;
            cmAnnNewFileecho = 1;
            strcpy(announcenewfileecho, argv[i]);
         } else {
	    fprintf(stderr, "Insufficient number of arguments\n");
	 }
         continue;
      } else if (stricmp(argv[i], "clean") == 0) {
         cmClean = 1;
         continue;
      } else {
         fprintf(stderr, "Unrecognized Commandline Option %s!\n", argv[i]);
         rc = 0;
      }

   } /* endwhile */
return rc;
}

void processConfig()
{
   char *buff = NULL;

   setvar("module", "htick");
   SetAppModule(M_HTICK);
   config = readConfig(NULL);
   if (NULL == config) {
      fprintf(stderr, "Config file not found\n");
      exit(1);
   };

   if (config->lockfile) {
      _lockfile = sstrdup(config->lockfile);
      if (config->advisoryLock) {
         if ((lock_fd=open(config->lockfile,O_CREAT|O_RDWR,S_IREAD|S_IWRITE))<0) {
            fprintf(stderr,"cannot open/create lock file: %s\n",config->lockfile);
            disposeConfig(config);
            exit(EX_CANTCREAT);
         } else {
            if (write(lock_fd," ", 1)!=1) {
               fprintf(stderr,"can't write to lock file! exit...\n");
               disposeConfig(config);
               exit(EX_IOERR);
            }
            if (lock(lock_fd,0,1)<0) {
               fprintf(stderr,"lock file used by another process! exit...\n");
               disposeConfig(config);
               exit(EX_TEMPFAIL);
            }
         }
      } else { // normal locking
         if ((lock_fd=open(config->lockfile,
            O_CREAT|O_RDWR|O_EXCL,S_IREAD|S_IWRITE))<0) {
            fprintf(stderr,"cannot create new lock file: %s\n",config->lockfile);
            fprintf(stderr,"lock file probably used by another process! exit...\n");
            disposeConfig(config);
            exit(EX_CANTCREAT);
         }
      }
   }   

   // open Logfile
   htick_log = NULL;
   if (config->logFileDir != NULL) {
     buff = (char *) smalloc(strlen(config->logFileDir)+strlen(LogFileName)+1);
     strcpy(buff, config->logFileDir),
     strcat(buff, LogFileName);
     htick_log = openLog(buff, versionStr, config);
     if (htick_log && quiet) htick_log->logEcho = 0;
   } else
       fprintf(stderr, "You have no logFileDir in your config, there will be no log created");

   nfree(buff);

   w_log( '1', "Start");

   if (config->addrCount == 0) w_log( LL_CRIT, "At least one addr must be defined\n");
   if (config->linkCount == 0) w_log( LL_CRIT, "At least one link must be specified\n");
   if (config->fileAreaBaseDir == NULL) w_log( LL_CRIT, "You must set FileAreaBaseDir in fidoconfig first\n");
   if (config->passFileAreaDir == NULL) w_log( LL_CRIT, "You must set PassFileAreaDir in fidoconfig first\n");
   if (config->MaxTicLineLength && config->MaxTicLineLength<80)
       w_log( LL_CRIT, "Parameter MaxTicLineLength in fidoconfig must be 0 or >80\n");

   if (config->addrCount == 0 ||
       config->linkCount == 0 ||
       config->fileAreaBaseDir == NULL ||
       config->passFileAreaDir == NULL ||
       (config->MaxTicLineLength && config->MaxTicLineLength<80)) {
      w_log( LL_CRIT, "Wrong config file, exit.");
      closeLog();
      if (config->lockfile != NULL) remove(config->lockfile);
      disposeConfig(config);
      exit(1);
   }
   if (config->busyFileDir == NULL) {
      config->busyFileDir = (char*) smalloc(strlen(config->outbound) + 10);
      strcpy(config->busyFileDir, config->outbound);
      sprintf(config->busyFileDir + strlen(config->outbound), "busy.htk%c", PATH_DELIM);
   }
   if (config->ticOutbound == NULL) config->ticOutbound = sstrdup(config->passFileAreaDir);
}

int main(int argc, char **argv)
{
   struct _minf m;
   char *version = NULL;


   xscatprintf(&version, "%u.%u.%u%s%s", VER_MAJOR, VER_MINOR, VER_PATCH, VER_SERVICE, VER_BRANCH);

#ifdef __linux__
   xstrcat(&version, "/lnx");
#elif defined(__FreeBSD__) || defined(__NetBSD__)
   xstrcat(&version, "/bsd");
#elif defined(__OS2__) || defined(OS2)
   xstrcat(&version, "/os2");
#elif defined(__NT__)
   xstrcat(&version, "/w32");
#elif defined(__sun__)
   xstrcat(&version, "/sun");
#elif defined(MSDOS)
   xstrcat(&version, "/dos");
#elif defined(__BEOS__)
   xstrcat(&version, "/beos");
#endif


   if (strcmp(VER_BRANCH,"-stable")!=0) xscatprintf(&version, " %s", cvs_date);
   xscatprintf(&versionStr,"HTick %s", version);
   nfree(version);

   if (!quiet) printf("%s (by Gabriel Plutzar)\n", versionStr);
   if (processCommandLine(argc, argv) == 0) exit(1);
   processConfig();

   // init SMAPI
   m.req_version = 0;
   m.def_zone = config->addr[0].zone;
   if (MsgOpenApi(&m) != 0) {
      exit_htick("MsgApiOpen Error",1);
   } /*endif */

   // load recoding tables
   initCharsets();
   if (config->intab != NULL) getctab(intab, (unsigned char*) config->intab);
   if (config->outtab != NULL) getctab(outtab, (unsigned char*) config->outtab);
#ifdef __NT__
   SetFileApisToOEM();
#endif

   checkTmpDir();

   if (cmScan) scan();
   if (cmToss) toss();
   if (cmHatch) hatch();
   if (cmSend) send(sendfile, sendarea, sendaddr);
   if (cmFlist) filelist();
   if (cmClean) cleanPassthroughDir();

   // deinit SMAPI
   MsgCloseApi();

   doneCharsets();
   w_log( '1', "End");
   closeLog();
   if (config->lockfile) {
      close(lock_fd);
      remove(config->lockfile);
   }
   disposeConfig(config);
   nfree(versionStr);


   return 0;
}
