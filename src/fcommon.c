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
#include <fcommon.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef __EMX__
#include <sys/types.h>
#endif
#include <sys/stat.h>
#if ((!(defined(_MSC_VER) && (_MSC_VER >= 1200)) ) && (!defined(__TURBOC__)))
#include <unistd.h>
#endif
#ifdef __IBMC__
#include <direct.h>
#endif
#ifdef __WATCOMC__
#include <fcntl.h>
#include <process.h>
#endif
#if defined (__TURBOC__)
#include <process.h>
#include <dir.h>
#endif

#include <global.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <fidoconf/dirlayer.h>
#include <fidoconf/adcase.h>
#include <fidoconf/xstr.h>
#include <fidoconf/recode.h>


#include <smapi/typedefs.h>
#include <smapi/compiler.h>
#include <smapi/stamp.h>
#include <smapi/progprot.h>
#include <smapi/ffind.h>
#include <smapi/patmat.h>

#if defined (_MSC_VER)
#include <process.h>
#endif

#include <toss.h>
#include <add_desc.h>


void exit_htick(char *logstr, int print) {

    w_log(LL_FUNC,"exit_htick()");
    w_log(LL_CRIT, logstr);
    if (!config->logEchoToScreen && print) fprintf(stderr, "%s\n", logstr);

    //writeDupeFiles();
    disposeConfig(config);
    doneCharsets();
    w_log(LL_STOP, "Exit");
    closeLog();
    if (_lockfile) {
       close(lock_fd);
       remove(_lockfile);
       nfree(_lockfile);
    }
    exit(EX_SOFTWARE);
}

/*
int createLockFile(char *lockfile) {
        FILE *f;

        if ((f=fopen(lockfile,"a")) == NULL)
           {
                   if (!quiet) fprintf(stderr,"createLockFile: cannot create lock file\"%s\"\n",lockfile);
                   w_log( '9', "createLockFile: cannot create lock file \"%s\"m", lockfile);
                   return 1;
           }
        fprintf(f, "%u\n", (unsigned)getpid());
        fclose(f);
        return 0;
}
*/
#if defined(__TURBOC__) || defined(__IBMC__) || (defined(_MSC_VER) && (_MSC_VER >= 1200))
#include <io.h>
#include <fcntl.h>
#endif

#if defined(__TURBOC__) || defined(__IBMC__) || defined(__WATCOMC__) || (defined(_MSC_VER) && (_MSC_VER >= 1200))

int truncate(const char *fileName, long length)
{
   int fd = open(fileName, O_RDWR | O_BINARY);
   if (fd != -1) {
          lseek(fd, length, SEEK_SET);
          chsize(fd, tell(fd));
          close(fd);
          return 1;
   };
   return 0;
}

int fTruncate( int fd, long length )
{
   if( fd != -1 )
   {
      lseek(fd, length, SEEK_SET);
      chsize(fd, tell(fd) );
      return 1;
   }
   return 0;
}

#endif

#ifdef __MINGW32__
int fTruncate (int fd, long length)
{
   if( fd != -1 )
   {
      lseek(fd, length, SEEK_SET);
      chsize(fd, tell(fd) );
      return 1;
   }
   return 0;
}
#endif

e_prio cvtFlavour2Prio(e_flavour flavour)
{
   switch (flavour) {
      case hold:      return HOLD;
      case normal:    return NORMAL;
      case direct:    return DIRECT;
      case crash:     return CRASH;
      case immediate: return IMMEDIATE;
      default:        return NORMAL;
   }
   return NORMAL;
}

int fileNameAlreadyUsed(char *pktName, char *packName) {
   unsigned int i;

   for (i=0; i < config->linkCount; i++) {
      if ((config->links[i].pktFile != NULL) && (pktName != NULL))
         if ((stricmp(pktName, config->links[i].pktFile)==0)) return 1;
      if ((config->links[i].packFile != NULL) && (packName != NULL))
         if ((stricmp(packName, config->links[i].packFile)==0)) return 1;
   }

   return 0;
}

int createOutboundFileName(s_link *link, e_prio prio, e_type typ)
{
   FILE *f; // bsy file for current link
   char name[32], bsyname[32], zoneSuffix[6], pntDir[14];
   int namelen;
   e_bundleFileNameStyle bundleNameStyle;

#ifdef UNIX
   char limiter='/';
#else
   char limiter='\\';
#endif

   if (link->linkBundleNameStyle != eUndef)
      bundleNameStyle = link->linkBundleNameStyle;
   else if (config->bundleNameStyle != eUndef)
      bundleNameStyle = config->bundleNameStyle;
   else
      bundleNameStyle = eUndef;

   if (bundleNameStyle == eAmiga) {
	 pntDir[0] = 0;
	 sprintf(name, "%u.%u.%u.%u.flo",
		 link->hisAka.zone, link->hisAka.net,
		 link->hisAka.node, link->hisAka.point);
   } else {
      if (link->hisAka.point != 0) {
	 sprintf(pntDir, "%04x%04x.pnt%c", link->hisAka.net, link->hisAka.node, limiter);
	 sprintf(name, "%08x.flo", link->hisAka.point);
      } else {
	 pntDir[0] = 0;
	 sprintf(name, "%04x%04x.flo", link->hisAka.net, link->hisAka.node);
      }
   }

   namelen = strlen(name);

   if ((link->hisAka.zone != config->addr[0].zone) &&
       (bundleNameStyle != eAmiga)) {
      // add suffix for other zones
      sprintf(zoneSuffix, ".%03x%c", link->hisAka.zone, limiter);
   } else {
      zoneSuffix[0] = 0;
   }

   switch (typ) {
      case PKT:
         name[namelen-3] = 'o'; name[namelen-2] = 'u'; name[namelen-1] = 't';
         break;
      case REQUEST:
         name[namelen-3] = 'r'; name[namelen-2] = 'e'; name[namelen-1] = 'q';
         break;
      case FLOFILE: break;
   } /* endswitch */

   if (typ != REQUEST) {
      switch (prio) {
         case CRASH :    name[namelen-3] = 'c';
                         break;
         case HOLD  :    name[namelen-3] = 'h';
                         break;
	 case DIRECT:    name[namelen-3] = 'd';
	                 break;
	 case IMMEDIATE: name[namelen-3] = 'i';
	                 break;
         case NORMAL:    break;
      } /* endswitch */
   } /* endif */

   // create floFile
   link->floFile = (char *) smalloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+namelen+1);
   link->bsyFile = (char *) smalloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+namelen+1);
   strcpy(link->floFile, config->outbound);
   if (zoneSuffix[0] != 0) strcpy(link->floFile+strlen(link->floFile)-1, zoneSuffix);
   strcat(link->floFile, pntDir);
   _createDirectoryTree(link->floFile); // create directoryTree if necessary
   strcpy(link->bsyFile, link->floFile);
   strcat(link->floFile, name);

   // create bsyFile
   strcpy(bsyname, name);
   bsyname[namelen-3]='b';bsyname[namelen-2]='s';bsyname[namelen-1]='y';
   strcat(link->bsyFile, bsyname);

   // maybe we have session with this link?
   if (fexist(link->bsyFile)) {

           w_log( '7', "link %s is busy.", addr2string(&link->hisAka));
           //free (link->floFile); link->floFile = NULL;
           free (link->bsyFile); link->bsyFile = NULL;

           return 1;

   } else {

       if ((f=fopen(link->bsyFile,"a")) == NULL)
       {
           if (!quiet) fprintf(stderr,"cannot create *.bsy file for %s\n",addr2string(&link->hisAka));
           remove(link->bsyFile);
           free(link->bsyFile);
           link->bsyFile=NULL;
           free(link->floFile);
           if (config->lockfile != NULL) remove(config->lockfile);
           w_log( '9', "cannot create *.bsy file");
           w_log( '1', "End");
           closeLog(htick_log);
           disposeConfig(config);
           exit(1);
       }
       fclose(f);
   }

   return 0;
}

int removeFileMask(char *directory, char *mask)
{
    DIR           *dir;
    struct dirent *file;
    char          *removefile;
    char          tmpDir[256], descr_file_name[256];
    unsigned int  numfiles = 0, dirLen;

   if (directory == NULL) return(0);
   dirLen = strlen(directory);
   if (directory[dirLen-1] == PATH_DELIM) {
      strcpy(tmpDir, directory);
   } else {
      strcpy(tmpDir, directory);
      tmpDir[dirLen] = PATH_DELIM;
      tmpDir[dirLen+1] = '\0';
      dirLen++;
   }

   dir = opendir(tmpDir);
   if (dir != NULL) {
      while ((file = readdir(dir)) != NULL) {
         if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
         if (patimat(file->d_name, mask) == 1) {

            //remove file
            removefile = (char *) smalloc(dirLen+strlen(file->d_name)+1);
            strcpy(removefile, tmpDir);
            strcat(removefile, file->d_name);
            remove(removefile);
            w_log('6',"Removed file: %s",removefile);
            numfiles++;
            free(removefile);

            //remove description for file
            strcpy(descr_file_name, tmpDir);
            strcat(descr_file_name, "files.bbs");
            adaptcase(descr_file_name);
            removeDesc(descr_file_name,file->d_name);
         }
      }
      closedir(dir);
   }
   return(numfiles);
}
