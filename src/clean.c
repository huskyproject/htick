/******************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 ******************************************************************************
 * clean.c : htick cleaning functionality
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
 * HTICK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with HTICK; see the file COPYING.  If not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA
 *
 * See also http://www.gnu.org
 *****************************************************************************
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <smapi/progprot.h>

#include <fidoconf/log.h>
#include <fidoconf/xstr.h>
#include <fidoconf/common.h>
#include <fidoconf/dirlayer.h>
#include <fidoconf/tree.h>

#include "toss.h"
#include "htick.h"
#include "global.h"
#include "fcommon.h"


static tree* fileTree = NULL;

int htick_compareEntries(char *p_e1, char *p_e2)
{
    return stricmp(p_e1,p_e2);
}

int htick_deleteEntry(char *p_e1)
{
    nfree(p_e1);
    return 1;
}

void addFileToTree(char* dir, char *filename)
{
    s_ticfile tic;
    
    if (patimat(filename, "*.TIC") == 1) {
        char  *ticfile = NULL;
        xstrscat(&ticfile,dir,filename,NULL);
        memset(&tic,0,sizeof(tic));
        parseTic(ticfile,&tic);
        if(tic.file) {
            tree_add(&fileTree, htick_compareEntries, sstrdup(tic.file), htick_deleteEntry);
        }
        disposeTic(&tic);
        nfree(ticfile);
    }
}

void cleanPassthroughDir(void)
{
    DIR            *dir;
    struct dirent  *file;
    char           *ticfile = NULL;
    w_log( LL_FUNC, "cleanPassthroughDir(): begin" );
    
    tree_init(&fileTree, 1);

    /* check busyFileDir */
    if (direxist(config->busyFileDir)) {
        dir = opendir(config->busyFileDir);
        if (dir != NULL) {
            while ((file = readdir(dir)) != NULL) {
                addFileToTree(config->busyFileDir, file->d_name);
            }
            closedir(dir);
        }
    }
    /* check separateBundles dirs (does anybody use this?) */
    if (config->separateBundles) {
        UINT i;
        char tmpdir[256];
        int  pcnt;
        hs_addr *aka;

        for (i = 0; i < config->linkCount; i++) {
            if (config->links[i].hisPackAka.zone != 0)
              pcnt = 0;
            else
              pcnt = 1;
            aka = &(config->links[i].hisAka);
            do
            {
              if (createOutboundFileNameAka(&(config->links[i]), normal, FLOFILE, aka) == 0)  {
                  strcpy(tmpdir, config->links[i].floFile);
                  sprintf(strrchr(tmpdir, '.'), ".sep");
                  if (direxist(tmpdir)) {
                      sprintf(tmpdir+strlen(tmpdir), "%c", PATH_DELIM);
                      dir = opendir(tmpdir);
                      if (dir == NULL) continue;
                    
                      while ((file = readdir(dir)) != NULL) {
                          addFileToTree(tmpdir, file->d_name);
                      } /* while */
                      closedir(dir);
                      if(config->links[i].bsyFile){
                        remove(config->links[i].bsyFile);
                        nfree(config->links[i].bsyFile);
                      }
                      nfree(config->links[i].floFile);
                  }
              }
              if (pcnt == 1)
              {
                pcnt = 0;
                aka = &(config->links[i].hisPackAka);
              }
            } while (pcnt!=0);
        }
    }
    /* check ticOutbound */
    dir = opendir(config->ticOutbound);
    if (dir != NULL) {
        while ((file = readdir(dir)) != NULL) {
            addFileToTree(config->ticOutbound, file->d_name);
        }
        closedir(dir);
    }
    /* purge passFileAreaDir */
    dir = opendir(config->passFileAreaDir);
    if (dir != NULL) {
        while ((file = readdir(dir)) != NULL) {
            if (patimat(file->d_name, "*.TIC") != 1) {
                xstrscat(&ticfile,config->passFileAreaDir,file->d_name,NULL);
        
                if (direxist(ticfile)) { /*  do not touch dirs */
                    nfree(ticfile);
                    continue;
                }
                if( !tree_srch(&fileTree, htick_compareEntries, file->d_name) )
                {
                    w_log(LL_DEL,"Remove file %s from passthrough dir", ticfile);
                    remove(ticfile);
                    nfree(ticfile);
                    continue;
                }
                nfree(ticfile);
            }
        }
        closedir(dir);
    }
    tree_mung(&fileTree, htick_deleteEntry);

    w_log( LL_FUNC, "cleanPassthroughDir(): end" );
}

void purgeFileEchos()
{
    UINT i;
    char* filename = NULL;
    time_t tnow;

    DIR            *dir;
    struct dirent  *file;
    struct stat    st;
      
    time( &tnow );


    for (i=0; i<config->fileAreaCount; i++) {
        if(config->fileAreas[i].purge == 0)
            continue;
        w_log(LL_INFO,  "Cleaning %s ...", config->fileAreas[i].areaName);
        dir = opendir(config->fileAreas[i].pathName);
        if (dir == NULL) 
            continue;

        while ((file = readdir(dir)) != NULL) {
            if (patimat(file->d_name, "*.BBS") == 1)
                continue;

            xstrscat(&filename,config->fileAreas[i].pathName,file->d_name,NULL);
            if (direxist(filename)) { /*  do not touch dirs */
                nfree(filename);
                continue;
            }
            memset(&st, 0, sizeof(st));
            stat(filename, &st);
            if(st.st_mtime + (time_t)(config->fileAreas[i].purge * 86400) > tnow ) {
                nfree(filename);
                continue;
            }
            w_log(LL_INFO,  "Deleting file %s that is %d days old", file->d_name,(tnow-st.st_mtime)/86400);
            removeFileMask(config->fileAreas[i].pathName, file->d_name);
            nfree(filename);
        }
        closedir(dir);
    }

}

void Purge(void)
{
    w_log( LL_INFO, "Start clean (purging files in passthrough dir) ...");
    cleanPassthroughDir();
    w_log(LL_STOP,  "End   Cleaning Passthrough Dir");

    purgeFileEchos();
}
