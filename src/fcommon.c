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
#if ((!(defined(_MSC_VER) && (_MSC_VER >= 1200)) ) && (!defined(__TURBOC__)))
#include <unistd.h>
#endif
#ifdef __IBMC__
#include <direct.h>
#endif
#ifdef __WATCOMC__
#include <process.h>
#endif
#if defined (__TURBOC__)
#include <process.h>
#include <dir.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


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

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

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

int createOutboundFileName(s_link *link, e_flavour prio, e_pollType typ)
{
   int nRet = NCreateOutboundFileName(config,link,prio,typ);
   if(nRet == -1) 
      exit_htick("cannot create *.bsy file!",0);
   return nRet;
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

#ifdef _MAKE_DLL_MVC_
#	include <Winbase.h>
#endif

int link_file(const char *from, const char *to)
{
   int rc = FALSE;
#if (_WIN32_WINNT >= 0x0500)
   rc = CreateHardLink(to, from, NULL);
#elif defined (_UNISTD_H)
   rc = (link(from, to) == 0);
#endif
   return rc;
}

