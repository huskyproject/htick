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
#include <ctype.h>


#include <huskylib/huskylib.h>

#include <huskylib/log.h>
#include <huskylib/dirlayer.h>
#include <huskylib/xstr.h>

#include <fidoconf/common.h>
#include <fidoconf/afixcmd.h>

#include <areafix/areafix.h>

#include "add_desc.h"
#include "global.h"
#include "fcommon.h"
#include "filelist.h"
#include "toss.h"

typedef struct _FileDescEntry
{
    char *  filename;
    char ** desc;               /*  Short Description of file */
    UINT    anzdesc;            /*  Number of Desc Lines */
} FileDescEntry;
static BigSize totalfilessize;
unsigned int totalfilesnumber = 0;
static tree * DescTree        = NULL;
static FileDescEntry fdesccmp;
char * DescNA = "Description not avaliable";
int DescTreeDeleteEntry(char * entry)
{
    if(entry)
    {
        FileDescEntry * entxt;
        UINT i;
        entxt = (FileDescEntry *)entry;
        nfree(entxt->filename);

        for(i = 0; i < entxt->anzdesc; i++)
        {
            nfree(entxt->desc[i]);
        }
        nfree(entxt->desc);
        nfree(entxt);
    }

    return 1;
}

int DescTreeCompareEntries(char * p_e1, char * p_e2)
{
    FileDescEntry * e1 = (FileDescEntry *)p_e1;
    FileDescEntry * e2 = (FileDescEntry *)p_e2;

    if(!p_e1 || !p_e2)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: DescTreeCompareEntries(%s,%s). This is serious error in program, please report to developers.",
              p_e1 ? "p_e1" : "NULL",
              p_e2 ? "p_e2" : "NULL");
        return -1;
    }

    return strcmp(e1->filename, e2->filename);
}

int ParseBBSFile(const char * fbbsname)
{
    FILE * filehandle;
    char * line = NULL, * tmp = NULL, * token = NULL, * p = NULL;
    int flag = 0, rc = 1;
    FileDescEntry * fdesc = NULL;

    if(!fbbsname)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: ParseBBSFile(%s). This is serious error in program, please report to developers.",
              fbbsname ? "fbbsname" : "NULL");
        return -1;
    }

    if(DescTree)
    {
        tree_mung(&DescTree, DescTreeDeleteEntry);
    }

    tree_init(&DescTree, 1);
    filehandle = fopen(fbbsname, "r+b");

    if(filehandle == NULL)
    {
        return 1;
    }

    while((line = readLine(filehandle)) != NULL)
    {
        if(flag && (*line == '\t' || *line == ' ' || *line == '>'))
        {
            token = stripLeadingChars(line, " >");

            if(*token == '>')
            {
                token++;
            }

            fdesc->desc = srealloc(fdesc->desc, (fdesc->anzdesc + 1) * sizeof(*fdesc->desc));
            fdesc->desc[fdesc->anzdesc] = sstrdup(token);
            fdesc->anzdesc++;
            nfree(line);
            continue;
        }
        else
        {
            flag = 0;

            if(rc == 0)
            {
                tree_add(&DescTree, DescTreeCompareEntries, (char *)fdesc, DescTreeDeleteEntry);
            }
        }

        tmp   = sstrdup(line);
        token = tmp;
        p     = token;

        while(p && *p != '\0' && !isspace(*p))
        {
            p++;
        }

        if(p && *p != '\0')
        {
            *p = '\0';
        }
        else
        {
            p = NULL;
        }

        fdesc           = scalloc(1, sizeof(FileDescEntry));
        fdesc->filename = sstrdup(token);

        if(p == NULL)
        {
            token = "";
        }
        else
        {
            p++;

            while(p && *p != '\0' && isspace(*p))
            {
                p++;
            }

            if(p && *p != '\0')
            {
                token = p;
            }
            else
            {
                token = "";
            }
        }

        if(config->addDLC && config->DLCDigits > 0 && config->DLCDigits < 10 && token[0] == '[')
        {
            while(']' != *p)
            {
                p++;
            }
            p++;

            while(p && !isspace(*p))
            {
                p++;
            }
            token = p;
        }

        fdesc->anzdesc = 0;
        fdesc->desc    = srealloc(fdesc->desc, (fdesc->anzdesc + 1) * sizeof(*fdesc->desc));
        fdesc->desc[fdesc->anzdesc] = sstrdup(token);
        fdesc->anzdesc++;
        flag = 1;
        rc   = 0;
        nfree(line);
        nfree(tmp);
    }

    if(rc == 0)
    {
        tree_add(&DescTree, DescTreeCompareEntries, (char *)fdesc, DescTreeDeleteEntry);
    }

    fclose(filehandle);
    return rc;
} /* ParseBBSFile */

void putFileInFilelist(FILE * f,
                       char * filename,
                       off_t size,
                       int day,
                       int month,
                       int year,
                       int countdesc,
                       char ** desc)
{
    int i;
    static BigSize bs;

    if(!f || !filename || !desc)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: putFileInFilelist(%s,%s,size,day,month,year,countdesc,%s). This is serious error in program, please report to developers.",
              f ? "f" : "NULL",
              filename ? "filename" : "NULL",
              desc ? "desc" : "NULL");
        return;
    }

    memset(&bs, 0, sizeof(BigSize));
    IncBigSize(&bs, (ULONG)size);
    fprintf(f, "%-12s", filename);
    fprintf(f, "%8s ", PrintBigSize(&bs));
    fprintf(f, "%02u-", day);

    switch(month)
    {
        case 0:
            fprintf(f, "Jan");
            break;

        case 1:
            fprintf(f, "Feb");
            break;

        case 2:
            fprintf(f, "Mar");
            break;

        case 3:
            fprintf(f, "Apr");
            break;

        case 4:
            fprintf(f, "May");
            break;

        case 5:
            fprintf(f, "Jun");
            break;

        case 6:
            fprintf(f, "Jul");
            break;

        case 7:
            fprintf(f, "Aug");
            break;

        case 8:
            fprintf(f, "Sep");
            break;

        case 9:
            fprintf(f, "Oct");
            break;

        case 10:
            fprintf(f, "Nov");
            break;

        case 11:
            fprintf(f, "Dec");
            break;

        default:
            break;
    } /* switch */
    fprintf(f, "-%02u", year % 100);

    if(countdesc == 0)
    {
        fprintf(f, " Description not avaliable\n");
    }
    else
    {
        /* desc = formatDesc(desc, &countdesc); */
        for(i = 0; i < countdesc; i++)
        {
            if(desc[i])
            {
                if(i == 0)
                {
                    fprintf(f, " %s\n", desc[i]);
                }
                else
                {
                    fprintf(f, "                               %s\n", desc[i]);
                }
            }
            else
            {
                w_log(LL_CRIT,
                      __FILE__ "::putFileInFilelist() Description line %i is NULL. This is serious error in program, please report to developers.",
                      i);
                break;
            }
        }
    }

    return;
} /* putFileInFilelist */

void printFileArea(char * area_areaName,
                   char * area_pathName,
                   char * area_description,
                   FILE * f,
                   int bbs)
{
    char * fileareapath = NULL, * fbbsname = NULL, * filename = NULL;
    char * fbbsline;
    husky_DIR * dir;
    char * file;
    struct stat stbuf;
    FileDescEntry * fdesc;
    time_t fileTime;
    struct tm * locTime;
    unsigned int totalnumber = 0;
    char ** removeFiles = NULL;
    unsigned int removeCount = 0, i;
    size_t Len;
    FILE * fbbs;
    char * token = "";
    int flag = 0;
    static BigSize bs;

    if(!area_areaName || !area_pathName || !f)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: printFileArea(%s,%s,%s,%s). This is serious error in program, please report to developers.",
              area_areaName ? "area_areaName" : "NULL",
              area_pathName ? "area_pathName" : "NULL",
              "area_description",
              f ? "f" : "NULL");
        return;
    }

    memset(&bs, 0, sizeof(BigSize));
    fileareapath = sstrdup(area_pathName);
    adaptcase(fileareapath);
    xstrscat(&fbbsname, fileareapath, config->fileDescription, NULL);
    adaptcase(fbbsname);
    dir = husky_opendir(fileareapath);

    if(dir == NULL)
    {
        return;
    }

    w_log(LL_INFO, "Processing: %s", area_areaName);
    ParseBBSFile(fbbsname);

    while((file = husky_readdir(dir)) != NULL)
    {
        if(strcmp(file, ".") == 0 || strcmp(file, "..") == 0)
        {
            continue;
        }

        nfree(filename);
        xstrscat(&filename, fileareapath, file, NULL);

        if(stricmp(filename, fbbsname) == 0)
        {
            continue;
        }

        if(direxist(filename))
        {
            continue;
        }

        if(!flag)
        {
            if(bbs)
            {
                fprintf(f, "BbsArea: %s", area_areaName);
            }
            else
            {
                fprintf(f, "FileArea: %s", area_areaName);
            }

            if(area_description != NULL)
            {
                fprintf(f, " (%s)\n", area_description);
            }
            else
            {
                fprintf(f, "\n");
            }

            fprintf(f,
                    "-----------------------------------------------------------------------------\n");
            flag = 1;
        }

        fdesccmp.filename = file;
        fdesc             =
            (FileDescEntry *)tree_srch(&DescTree, DescTreeCompareEntries, (char *)(&fdesccmp));

        if(!fdesc)
        {
            add_description(fbbsname, file, &DescNA, 1);
        }

        stat(filename, &stbuf);
        fileTime = stbuf.st_mtime > 0 ? stbuf.st_mtime : 0;
        locTime  = localtime(&fileTime);
        IncBigSize(&bs, (ULONG)stbuf.st_size);
        totalnumber++;

        if(fdesc)
        {
            putFileInFilelist(f,
                              file,
                              stbuf.st_size,
                              locTime->tm_mday,
                              locTime->tm_mon,
                              locTime->tm_year,
                              fdesc->anzdesc,
                              fdesc->desc);
        }
        else
        {
            putFileInFilelist(f,
                              file,
                              stbuf.st_size,
                              locTime->tm_mday,
                              locTime->tm_mon,
                              locTime->tm_year,
                              1,
                              &DescNA);
        }
    }
    husky_closedir(dir);

    if(flag)
    {
        fprintf(f,
                "-----------------------------------------------------------------------------\n");
        fprintf(f,
                "Total files in area: %6d, total size: %10s bytes\n\n",
                totalnumber,
                PrintBigSize(&bs));
    }

    /* collect information about files in files.bbs that realy do not exist */
    if((fbbs = fopen(fbbsname, "r")) == NULL)
    {
        return;
    }

    while((fbbsline = readLine(fbbs)) != NULL)
    {
        if(*fbbsline == 0 || *fbbsline == 10 || *fbbsline == 13 || *fbbsline == ' ' ||
           *fbbsline == '\t' || *fbbsline == '>')
        {
            continue;
        }

        Len = strlen(fbbsline);

        if(fbbsline[Len - 1] == '\n')
        {
            fbbsline[--Len] = 0;
        }

        if(fbbsline[Len - 1] == '\r')
        {
            fbbsline[--Len] = 0;
        }

        token = strtok(fbbsline, " \t\0");

        if(token == NULL)
        {
            continue;
        }

        nfree(filename);
        xstrscat(&filename, fileareapath, token, NULL);
        adaptcase(filename);

        if(!fexist(filename))
        {
            removeFiles = srealloc(removeFiles, sizeof(char *) * (removeCount + 1));
            removeFiles[removeCount] = (char *)smalloc(strlen(strrchr(filename,
                                                                      PATH_DELIM) + 1) + 1);
            strcpy(removeFiles[removeCount], strrchr(filename, PATH_DELIM) + 1);
            removeCount++;
        }

        nfree(fbbsline);
    }
    fclose(fbbs);

    /* remove descriptions from files.bbs on files that realy do not exist */
    if(removeCount > 0)
    {
        for(i = 0; i < removeCount; i++)
        {
            removeDesc(fbbsname, removeFiles[i]);
            nfree(removeFiles[i]);
        }
        nfree(removeFiles);
    }

    IncBigSize2(&totalfilessize, &bs);
    totalfilesnumber += totalnumber;
    nfree(filename);
    return;
} /* printFileArea */

void filelist()
{
    FILE * f = NULL;
    FILE * d = NULL;
    unsigned int i;

    w_log(LL_INFO, "Start filelist...");

    if(flistfile == NULL)
    {
        w_log('6', "Could not find output file");
        return;
    }

    if((f = fopen(flistfile, "w")) == NULL)
    {
        w_log('6', "Could not open for write file %s", flistfile);
        return;
    }

    if(dlistfile)
    {
        if((d = fopen(dlistfile, "w")) == NULL)
        {
            w_log('6', "Could not open for write file %s", dlistfile);
        }
    }

    for(i = 0; i < config->fileAreaCount; i++)
    {
        if(config->fileAreas[i].msgbType != MSGTYPE_PASSTHROUGH && !(config->fileAreas[i].hide))
        {
            printFileArea(config->fileAreas[i].areaName,
                          config->fileAreas[i].fileName,
                          config->fileAreas[i].description,
                          f,
                          0);

            if(d)
            {
                fprintf(d, "%s\n", config->fileAreas[i].fileName);
            }
        }
    }

    for(i = 0; i < config->bbsAreaCount; i++)
    {
        printFileArea(config->bbsAreas[i].areaName, config->bbsAreas[i].pathName,
                      config->bbsAreas[i].description, f, 1);

        if(d)
        {
            fprintf(d, "%s\n", config->bbsAreas[i].pathName);
        }
    }
    fprintf(f, "=============================================================================\n");
    fprintf(f,
            "Total files in filelist: %6d, total size: %10s bytes\n",
            totalfilesnumber,
            PrintBigSize(&totalfilessize));
    fprintf(f,
            "=============================================================================\n\n");
    fclose(f);

    if(d)
    {
        fclose(d);
    }
} /* filelist */
