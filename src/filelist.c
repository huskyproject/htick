/*****************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 *****************************************************************************
 * Filelist generator routines.
 *
 * This file is part of HTICK, part of the Husky fidosoft project
 * http://husky.sf.net
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
 *****************************************************************************
 * $Id$
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <smapi/progprot.h>

#include <fidoconf/log.h>
#include <fidoconf/common.h>
#include <fidoconf/dirlayer.h>
#include <fidoconf/adcase.h>
#include <fidoconf/xstr.h>
#include <fidoconf/afixcmd.h>

#include "add_desc.h"
#include "global.h"
#include "version.h"
#include "filelist.h"
#include "toss.h"


unsigned long totalfilessize = 0;
unsigned int totalfilesnumber = 0;

/*
#define LenDesc 46

char** formatDesc(char **desc, int *count)
{
    int i;
    char *descLine=NULL;
    char *begin,*end;
    
    for(i = 0; i < (*count); i++ )
    {
        if(strlen(desc[i]) > LenDesc)
            break;
    }
    if(i == *count)
    {
        printf("%s\n",desc[0]);
        return desc;
    }

    for(i = 0; i < (*count); i++ )
    {
        if(strlen(desc[i]) < LenDesc)
            xstrcat(&desc[i],"\r");
        if(i == 0)
            xstrcat(&descLine,desc[i]);
        else
            xstrscat(&descLine, " " , desc[i],NULL);
        nfree(desc[i]);
    }
    begin = end =descLine; i = 0;
    while(*end)
    {
        if(end == "\r")
        {
            *end     = '\0';
            if(i == *count)
            {
                (*count)++;
                desc = srealloc(desc,sizeof(char *) * (*count) );
            }
            desc[i] = sstrdup(begin);
            begin = end = end + 1;
            i++;
            continue;
        }
        if(end-begin > LenDesc)
        {   
            end--;
            while(!isspace(*(end)) && end+1 > begin)
                end--;
            *end     = '\0';
            if(i == *count)
            {
                (*count)++;
                desc = srealloc(desc,sizeof(char *) * (*count) );
            }
            desc[i] = sstrdup(begin);
            begin = end = end + 1;
            i++;
            continue;
        }
        end++;
    }
    if(i == *count)
    {
        (*count)++;
        desc = srealloc(desc,sizeof(char *) * (*count) );
    }
    desc[i] = sstrdup(begin);
    printf("%s\n",descLine);
    nfree(descLine);
    return desc;
}
*/

void putFileInFilelist(FILE *f, char *filename, off_t size, int day, int month, int year, int countdesc, char **desc)
{
    int i;
    
    fprintf(f,"%-12s",filename);
    fprintf(f,"%8lu ",(unsigned long) size);
    fprintf(f, "%02u-", day);
    switch (month) {
    case 0: fprintf(f, "Jan");
        break;
    case 1: fprintf(f, "Feb");
        break;
    case 2: fprintf(f, "Mar");
        break;
    case 3: fprintf(f, "Apr");
        break;
    case 4: fprintf(f, "May");
        break;
    case 5: fprintf(f, "Jun");
        break;
    case 6: fprintf(f, "Jul");
        break;
    case 7: fprintf(f, "Aug");
        break;
    case 8: fprintf(f, "Sep");
        break;
    case 9: fprintf(f, "Oct");
        break;
    case 10: fprintf(f, "Nov");
        break;
    case 11: fprintf(f, "Dec");
        break;
    default:
        break;
    }
    fprintf(f, "-%02u", year % 100);
    if (countdesc == 0) fprintf(f," Description not avaliable\n");
    else {
        /* desc = formatDesc(desc, &countdesc); */
        for (i=0;i<countdesc;i++) {
            if (i == 0) fprintf(f," %s\n",desc[i]);
            else fprintf(f,"                               %s\n",desc[i]);
        }
    }
    return;
}

void printFileArea(char *area_areaName, char *area_pathName, char *area_description, FILE *f, int bbs) {
    
    char *fileareapath=NULL, *fbbsname=NULL, *filename=NULL;
    char *fbbsline;
    DIR            *dir;
    struct dirent  *file;
    s_ticfile tic;
    struct stat stbuf;
    time_t fileTime;
    struct tm *locTime;
    unsigned int totalnumber = 0;
    unsigned long totalsize = 0;
    char **removeFiles = NULL;
    unsigned int removeCount = 0, i, Len;
    FILE *fbbs;
    char *token = "";
    int flag = 0;
    
    fileareapath = sstrdup(area_pathName);
    strLower(fileareapath);
    _createDirectoryTree(fileareapath);
    xstrscat(&fbbsname,fileareapath,"files.bbs",NULL);
    adaptcase(fbbsname);
    
    dir = opendir(fileareapath);
    if (dir == NULL) return;
    
    w_log( LL_INFO, "Processing: %s",area_areaName);
    
    while ((file = readdir(dir)) != NULL) {
        if (strcmp(file->d_name,".") == 0 || strcmp(file->d_name,"..") == 0)
            continue;
        nfree(filename);
        xstrscat(&filename,fileareapath,file->d_name,NULL);
        if (stricmp(filename, fbbsname) == 0) continue;
        if (!flag) {
            if (bbs) fprintf(f,"BbsArea: %s", area_areaName);
            else fprintf(f,"FileArea: %s", area_areaName);
            if (area_description!=NULL)
                fprintf(f," (%s)\n", area_description);
            else
                fprintf(f,"\n");
            fprintf(f,"-----------------------------------------------------------------------------\n");
            flag = 1;
        }
        memset(&tic,0,sizeof(tic));
        if (GetDescFormBbsFile(fbbsname, file->d_name, &tic) == 1) {
            tic.desc=srealloc(tic.desc,(tic.anzdesc+1)*sizeof(*tic.desc));
            tic.desc[tic.anzdesc]=sstrdup("Description not avaliable");
            tic.anzdesc = 1;
            add_description(fbbsname, file->d_name, tic.desc, 1);
        }
        stat(filename,&stbuf);
        fileTime = stbuf.st_mtime > 0 ? stbuf.st_mtime : 0;
        locTime = localtime(&fileTime);
        totalsize += stbuf.st_size;
        totalnumber++;
        putFileInFilelist(f, file->d_name, stbuf.st_size, locTime->tm_mday, locTime->tm_mon, locTime->tm_year, tic.anzdesc, tic.desc);
        disposeTic(&tic);
    }
    if (flag) {
        fprintf(f,"-----------------------------------------------------------------------------\n");
        fprintf(f,"Total files in area: %4d, total size: %10lu bytes\n\n",totalnumber,totalsize);
    }
    if ( (fbbs = fopen(fbbsname,"r")) == NULL ) return;
    while ((fbbsline = readLine(fbbs)) != NULL) {
        if (*fbbsline == 0 || *fbbsline == 10 || *fbbsline == 13
            || *fbbsline == ' ' || *fbbsline == '\t' || *fbbsline == '>')
            continue;
        
        Len = strlen(fbbsline);
        if (fbbsline[Len-1]=='\n')
            fbbsline[--Len]=0;
        
        if (fbbsline[Len-1]=='\r')
            fbbsline[--Len]=0;
        
        token = strtok(fbbsline, " \t\0");
        
        if (token==NULL)
            continue;
        nfree(filename);
        xstrscat(&filename,fileareapath,token,NULL);
        adaptcase(filename);
        if (!fexist(filename)) {
            removeFiles = srealloc(removeFiles, sizeof(char *)*(removeCount+1));
            removeFiles[removeCount] = (char *) smalloc(strlen(strrchr(filename,PATH_DELIM)+1)+1);
            strcpy(removeFiles[removeCount], strrchr(filename,PATH_DELIM)+1);
            removeCount++;
        }
        nfree(fbbsline);
    }
    fclose(fbbs);
    if (removeCount > 0) {
        for (i=0; i<removeCount; i++) {
            removeDesc(fbbsname,removeFiles[i]);
            nfree(removeFiles[i]);
        }
        nfree(removeFiles);
    }
    totalfilessize += totalsize;
    totalfilesnumber += totalnumber;
    nfree(filename);
    return;
}

void filelist()
{
    FILE *f = NULL;
    FILE *d = NULL;
    unsigned int i;
    
    w_log( LL_INFO, "Start filelist...");
    
    if (flistfile == NULL) {
        w_log('6',"Not found output file");
        return;
    }
    
    if ( (f = fopen(flistfile,"w")) == NULL ) {
        w_log('6',"Could not open for write file %s",flistfile);
        return;
    }

    if(dlistfile)
    {
        if ( (d = fopen(dlistfile,"w")) == NULL ) {
            w_log('6',"Could not open for write file %s",dlistfile);
        }
    }
    for (i=0; i<config->fileAreaCount; i++) {
        if (config->fileAreas[i].pass != 1 && !(config->fileAreas[i].hide))
        {
            printFileArea(config->fileAreas[i].areaName, config->fileAreas[i].pathName, config->fileAreas[i].description, f, 0);
            if(d) fprintf(d, "%s\n",config->fileAreas[i].pathName);
        }
    }
    
    for (i=0; i<config->bbsAreaCount; i++) {
        printFileArea(config->bbsAreas[i].areaName, config->bbsAreas[i].pathName, config->bbsAreas[i].description, f, 1);
        if(d) fprintf(d, "%s\n",config->bbsAreas[i].pathName);
    }
    
    fprintf(f,"=============================================================================\n");
    fprintf(f,"Total files in filelist: %4d, total size: %10lu bytes\n",totalfilesnumber,totalfilessize);
    fprintf(f,"=============================================================================\n\n");
    
    fclose(f);
    if(d) fclose(d);
}

