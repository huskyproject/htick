/******************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 ******************************************************************************
 * report.c : htick reporting
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

#include <stdlib.h>
#include <string.h>

#include <huskylib/log.h>
#include <huskylib/xstr.h>
#include <fidoconf/common.h>
#include <huskylib/recode.h>
#include <fidoconf/afixcmd.h>
#include <huskylib/dirlayer.h>
#include <areafix/areafix.h>    /* for repeat_char() */

#include "toss.h"
#include "version.h"
#include "global.h"
#include "report.h"

typedef struct
{
    s_area * farea;
    UINT     begin;
    UINT32   fSize;
    UINT     fCount;
} s_FAreaRepInfo;
s_ticfile * Report = NULL;
/* Report uses field password for saving ticfilename */
/* Report uses field anzpath  for flag of deleting  ticfile*/
UINT rCount            = 0;
s_FAreaRepInfo * aList = NULL;
UINT aCount            = 0;
/* Save cut-down ticket file into special directory for future reports
 */
void doSaveTic4Report(s_ticfile * tic)
{
    FILE * tichandle;
    unsigned int i;
    char * rpTicName = NULL;
    char * desc_line = NULL;

    rpTicName = makeUniqueDosFileName(config->announceSpool, "tic", config);
    tichandle = fopen(rpTicName, "wb");

    if(tichandle == NULL)
    {
        w_log(LL_CRIT, "Can't create file %s for file %s", rpTicName, tic->file);
        return;
    }
    else
    {
        w_log(LL_CREAT, "Report file %s created for file %s", rpTicName, tic->file);
    }

    fprintf(tichandle, "File %s\r\n", tic->file);

    if(tic->altfile != NULL)
    {
        fprintf(tichandle, "AltFile %s\r\n", tic->altfile);
    }

    fprintf(tichandle, "Area %s\r\n", tic->area);

    if(tic->anzldesc > 0)
    {
        for(i = 0; i < tic->anzldesc; i++)
        {
            desc_line = sstrdup(tic->ldesc[i]);

            if(config->intab != NULL)
            {
                recodeToInternalCharset(desc_line);
            }

            fprintf(tichandle, "LDesc %s\r\n", desc_line);
            nfree(desc_line);
        }
    }
    else
    {
        for(i = 0; i < tic->anzdesc; i++)
        {
            desc_line = sstrdup(tic->desc[i]);

            if(config->intab != NULL)
            {
                recodeToInternalCharset(desc_line);
            }

            fprintf(tichandle, "Desc %s\r\n", desc_line);
            nfree(desc_line);
        }
    }

    if(tic->origin.zone != 0)
    {
        fprintf(tichandle, "Origin %s\r\n", aka2str(&tic->origin));
    }

    if(tic->from.zone != 0)
    {
        fprintf(tichandle, "From %s\r\n", aka2str(&tic->from));
    }

    if(tic->size != 0)
    {
        fprintf(tichandle, "Size %u\r\n", tic->size);
    }

    fclose(tichandle);
}                               /* doSaveTic4Report() */

static int cmp_reportEntry(const void * a, const void * b)
{
    const s_ticfile * r1 = (s_ticfile *)a;
    const s_ticfile * r2 = (s_ticfile *)b;

    if(stricmp(r1->area, r2->area) > 0)
    {
        return 1;
    }
    else if(stricmp(r1->area, r2->area) < 0)
    {
        return -1;
    }
    else if(stricmp(r1->file, r2->file) > 0)
    {
        return 1;
    }
    else if(stricmp(r1->file, r2->file) < 0)
    {
        return -1;
    }

    return 0;
}                               /* cmp_reportEntry() */

static void getReportInfo()
{
    husky_DIR * dir;
    char * file;
    s_ticfile tmptic;
    char * fname = NULL;
    UINT i       = 0;

    dir = husky_opendir(config->announceSpool);

    while((file = husky_readdir(dir)) != NULL)
    {
        if(patimat(file, "*.TIC") == 0)
        {
            continue;
        }

        xstrscat(&fname, config->announceSpool, file, NULL);
        w_log(LL_DEBUG, "Parsing Report file %s", file);

        if((parseTic(fname, &tmptic) != parseTic_error) && tmptic.area && tmptic.file)
        {
            Report = srealloc(Report, (rCount + 1) * sizeof(s_ticfile));
            memset(&(Report[rCount]), 0, sizeof(s_ticfile));
            Report[rCount].size    = tmptic.size;
            Report[rCount].origin  = tmptic.origin;
            Report[rCount].from    = tmptic.from;
            Report[rCount].area    = sstrdup(tmptic.area);
            Report[rCount].file    = sstrdup(tmptic.file);
            Report[rCount].altfile = sstrdup(tmptic.altfile);
            Report[rCount].anzdesc = tmptic.anzdesc;
            Report[rCount].desc    = scalloc(sizeof(char *), tmptic.anzdesc);

            for(i = 0; i < tmptic.anzdesc; i++)
            {
                Report[rCount].desc[i] = sstrdup(tmptic.desc[i]);
            }
            Report[rCount].anzldesc = tmptic.anzldesc;
            Report[rCount].ldesc    = scalloc(sizeof(char *), tmptic.anzldesc);

            for(i = 0; i < tmptic.anzldesc; i++)
            {
                Report[rCount].ldesc[i] = sstrdup(tmptic.ldesc[i]);
            }
            xstrscat(&(Report[rCount].password), config->announceSpool, file, NULL);
            rCount++;
        }

        disposeTic(&tmptic);
        nfree(fname);
    }
    husky_closedir(dir);
    w_log(LL_DEBUG, "Sorting report information. Number of entries: %d", rCount);
    qsort((void *)Report, rCount, sizeof(s_ticfile), cmp_reportEntry);
}                               /* getReportInfo() */

static void buildAccessList()
{
    UINT i             = 0;
    s_area * currFArea = NULL;

    for(i = 0; i < rCount; i++)
    {
        if(currFArea == NULL || stricmp(Report[i].area, currFArea->areaName))
        {
            if(getFileArea(Report[i].area) == NULL)
            {
                continue;
            }

            aCount++;
            aList     = srealloc(aList, (aCount) * sizeof(s_FAreaRepInfo));
            currFArea = getFileArea(Report[i].area);
            aList[aCount - 1].farea  = currFArea;
            aList[aCount - 1].begin  = i;
            aList[aCount - 1].fCount = 0;
            aList[aCount - 1].fSize  = 0;
        }

        aList[aCount - 1].fCount++;
        aList[aCount - 1].fSize += Report[i].size;
    }
}                               /* buildAccessList() */

static void parseRepMessAttr()
{
    UINT i;
    long attr;
    ps_anndef RepDef;
    char * flag = NULL;

    for(i = 0; i < config->ADCount; i++)
    {
        RepDef = &(config->AnnDefs[i]);

        if(!RepDef->annmessflags)
        {
            continue;
        }

        flag = strtok(RepDef->annmessflags, " \t");

        while(flag)
        {
            attr = str2attr(flag);

            if(attr != -1L)
            {
                RepDef->attributes |= attr;
            }

            flag = strtok(NULL, " \t");
        }
    }
}                               /* parseRepMessAttr() */

/* report generation */
static char * formDescStr(char * desc)
{
    char * keepDesc, * newDesc, * tmp, * ch, * buff = NULL;

    keepDesc = sstrdup(desc);

    if(strlen(desc) <= 50)
    {
        return keepDesc;
    }

    newDesc = (char *)scalloc(1, sizeof(char));
    tmp     = keepDesc;
    ch      = strtok(tmp, " \t\r\n");

    while(ch)
    {
        if(strlen(ch) > 54 && !buff)
        {
            newDesc = (char *)srealloc(newDesc, strlen(newDesc) + 55);
            strncat(newDesc, ch, 54);
            xstrscat(&newDesc, "\r", repeat_char(24, ' '), NULL);
            ch += 54;
        }
        else
        {
            if(buff && strlen(buff) + strlen(ch) > 54)
            {
                xstrscat(&newDesc, buff, "\r", repeat_char(24, ' '), NULL);
                nfree(buff);
            }
            else
            {
                xstrscat(&buff, ch, " ", NULL);
                ch = strtok(NULL, " \t\r\n");
            }
        }
    }

    if(buff && strlen(buff) != 0)
    {
        xstrcat(&newDesc, buff);
    }

    nfree(buff);
    nfree(keepDesc);
    return newDesc;
}                               /* formDescStr() */

static char * formDesc(char ** desc, int count)
{
    char * buff = NULL, * tmp;
    int i;

    for(i = 0; i < count; i++)
    {
        tmp = formDescStr(desc[i]);

        if(i == 0)
        {
            xstrscat(&buff, tmp, "\r", NULL);
        }
        else
        {
            xstrscat(&buff, repeat_char(24, ' '), tmp, "\r", NULL);
        }

        nfree(tmp);
    }
    return buff;
}                               /* formDesc() */

static void freeReportInfo()
{
    unsigned i = 0;

    w_log(LL_INFO, "Deleting report files");

    for(i = 0; i < rCount; i++)
    {
        if(Report[i].anzpath)
        {
            remove(Report[i].password);
            w_log(LL_DELETE, "Removed file: %s", Report[i].password);
            Report[i].anzpath = 0;
        }

        disposeTic(&(Report[i]));
    }
    nfree(Report);
    nfree(aList);
}                               /* freeReportInfo() */

static void MakeDefaultRepDef()
{
    char * annArea;

    if(config->ADCount == 0)
    {
        config->AnnDefs = scalloc(1, sizeof(s_anndef));

        if(config->ReportTo)
        {
            annArea = config->ReportTo;
        }
        else
        {
            annArea = config->netMailAreas[0].areaName;
        }

        config->ADCount = 1;
        config->AnnDefs[0].annAreaTag = sstrdup(annArea);
    }
}                               /* MakeDefaultRepDef() */

static s_message * MakeReportMessage(ps_anndef pRepDef)
{
    s_message * msg = NULL;

    enum
    {
        dstfile = -1, dstechomail = 0, dstnetmail = 1
    } reportDst = dstechomail;  /*  report destination  */

    if(pRepDef->annAreaTag[0] == '@')
    {
        reportDst = dstfile;
    }
    else if(stricmp(pRepDef->annAreaTag, "netmail") == 0)
    {
        reportDst = dstnetmail;
    }
    else if(getNetMailArea(config, pRepDef->annAreaTag) != NULL)
    {
        reportDst = dstnetmail;
    }

    if(reportDst > dstfile)     /* dstechomail or dstnetmail */
    {
        s_area * AreaToPost = getArea(config, pRepDef->annAreaTag);

        if(AreaToPost == &(config->badArea))
        {
            pRepDef->annaddrfrom =
                pRepDef->annaddrfrom ? pRepDef->annaddrfrom : &(config->addr[0]);
        }
        else
        {
            pRepDef->annaddrfrom =
                pRepDef->annaddrfrom ? pRepDef->annaddrfrom : AreaToPost->useAka;
        }

        msg =
            makeMessage(pRepDef->annaddrfrom,
                        pRepDef->annaddrto ? pRepDef->annaddrto : &(config->addr[0]),
                        pRepDef->annfrom ? pRepDef->annfrom : versionStr,
                        pRepDef->annto ? pRepDef->annto : (reportDst ? NULL : "All"),
                        /* reportDst!=dstechomail ? */
                        pRepDef->annsubj ? pRepDef->annsubj : "New Files",
                        (int)reportDst,
                        robot->reportsAttr);
        msg->attributes = pRepDef->attributes;
        msg->text       = createKludges(config,
                                        reportDst ? NULL : pRepDef->annAreaTag,
                                        /* reportDst!=dstechomail ? */
                                        pRepDef->annaddrfrom,
                                        pRepDef->annaddrto ? pRepDef->annaddrto : &(config->addr[0]),
                                        versionStr);

        if(robot->reportsFlags)
        {
            xstrscat(&(msg->text), "\001FLAGS ", robot->reportsFlags, "\r", NULL);
        }
    }
    else                        /* report to file */
    {
        msg          = (s_message *)scalloc(1, sizeof(s_message));
        msg->netMail = 2;
        xstrcat(&(msg->subjectLine), pRepDef->annAreaTag + 1);
    }

    return msg;
}                               /* MakeReportMessage() */

static void ReportOneFile(s_message * msg, ps_anndef pRepDef, s_ticfile * tic)
{
    char * tmp = NULL;
    char * name;
    static BigSize bs;

    memset(&bs, 0, sizeof(BigSize));
    IncBigSize(&bs, (ULONG)tic->size);
    tic->anzpath = 1;           /* mark tic file as deleted */

    if(tic->altfile != NULL)
    {
        name = tic->altfile;
    }
    else
    {
        name = tic->file;
    }

    if(strlen(name) > 12)
    {
        xscatprintf(&(msg->text), " %s\r% 23s ", name, PrintBigSize(&bs));
    }
    else
    {
        xscatprintf(&(msg->text), " %-12s % 9s ", name, PrintBigSize(&bs));
    }

    if(tic->anzldesc > 0)
    {
        tmp = formDesc(tic->ldesc, tic->anzldesc);
    }
    else
    {
        tmp = formDesc(tic->desc, tic->anzdesc);
    }

    xstrcat(&(msg->text), tmp);

    if(pRepDef->annforigin && tic->origin.zone != 0)
    {
        xscatprintf(&(msg->text), "%sOrig: %s\r", repeat_char(24, ' '), aka2str(&tic->origin));
    }

    if(pRepDef->annfrfrom && tic->from.zone != 0)
    {
        xscatprintf(&(msg->text), "%sFrom: %s\r", repeat_char(24, ' '), aka2str(&tic->from));
    }

    if(tic->altfile != NULL)
    {
        xscatprintf(&(msg->text), "%sTIC: %s\r", repeat_char(24, ' '), tic->file);
    }

    if(tmp == NULL || tmp[0] == 0)
    {
        xstrcat(&(msg->text), "\r");
    }

    nfree(tmp);
}                               /* ReportOneFile() */

static int IsAreaMatched(char * areaname, ps_anndef pRepDef)
{
    int nRet = 0;
    UINT i   = 0;

    if(pRepDef->numbI == 0)
    {
        nRet = 1;
    }

    for(i = 0; i < pRepDef->numbI; i++)
    {
        if(patimat(areaname, pRepDef->annInclude[i]) == 1)
        {
            nRet = 1;
            break;
        }
    }

    if(nRet == 1)
    {
        for(i = 0; i < pRepDef->numbE; i++)
        {
            if(patimat(areaname, pRepDef->annExclude[i]) == 1)
            {
                nRet = 0;
                break;
            }
        }
    }

    return nRet;
}                               /* IsAreaMatched() */

static void reportNewFiles()
{
    UINT fileCountTotal = 0;
    BigSize fileSizeTotal, bs;
    UINT i, j, ii;
    s_message * msg = NULL;
    FILE * echotosslog;
    FILE * rp;
    ps_anndef RepDef;
    char * cp = NULL;

    for(i = 0; i < config->ADCount; i++)
    {
        RepDef         = &(config->AnnDefs[i]);
        fileCountTotal = 0;
        memset(&fileSizeTotal, 0, sizeof(BigSize));

        for(j = 0; j < aCount; j++)
        {
            if(!IsAreaMatched(aList[j].farea->areaName, RepDef))
            {
                continue;
            }

            if(!msg)
            {
                msg = MakeReportMessage(RepDef);
            }

            xscatprintf(&(msg->text), "\r>Area : %s", strUpper(aList[j].farea->areaName));

            if(aList[j].farea->description)
            {
                xscatprintf(&(msg->text), " : %s", aList[j].farea->description);
            }

            xscatprintf(&(msg->text), "\r %s\r", repeat_char(77, '-'));

            for(ii = aList[j].begin; ii < aList[j].begin + aList[j].fCount; ii++)
            {
                ReportOneFile(msg, RepDef, &(Report[ii]));
            }
            memset(&bs, 0, sizeof(BigSize));
            IncBigSize(&bs, (ULONG)aList[j].fSize);
            xscatprintf(&(msg->text), " %s\r", repeat_char(77, '-'));
            xscatprintf(&(msg->text),
                        " %s bytes in %u file(s)\r",
                        PrintBigSize(&bs),
                        aList[j].fCount);
            fileCountTotal += aList[j].fCount;
            IncBigSize(&fileSizeTotal, aList[j].fSize);
        }

        if(!msg)
        {
            continue;
        }

        xscatprintf(&(msg->text), "\r %s\r", repeat_char(77, '='));
        xscatprintf(&(msg->text),
                    ">Total %s bytes in %u file(s)\r\r",
                    PrintBigSize(&fileSizeTotal),
                    fileCountTotal);

        if(msg->netMail > 1)
        {
            if(NULL == (rp = fopen(msg->subjectLine, "wt")))
            {
                w_log(LL_ERR, "Could not create report file: %s", msg->subjectLine);
            }
            else
            {
                /* if output to file:  \r ==> \r\n */
                cp = msg->text;

                while((cp = strchr(cp, '\r')) != NULL)
                {
                    *cp = '\n';
                }
                fprintf(rp, "%s", msg->text); /* Automatic translate "\n" to "\015\012" */
                w_log(LL_FLAG, "Created report file: %s", msg->subjectLine);
                fclose(rp);
            }
        }
        else
        {
            writeMsgToSysop(msg, RepDef->annAreaTag, RepDef->annorigin);

            if(config->echotosslog != NULL)
            {
                echotosslog = fopen(config->echotosslog, "a");

                if(echotosslog != NULL)
                {
                    fprintf(echotosslog, "%s\n", RepDef->annAreaTag);
                    fclose(echotosslog);
                }
            }
        }

        freeMsgBuffers(msg);
        nfree(msg);
    }
}                               /* reportNewFiles() */

/* Make thick report
 */
void report()
{
    w_log(LL_INFO, "Start report generating...");
    getReportInfo();

    if(Report)
    {
        buildAccessList();
        MakeDefaultRepDef();
        parseRepMessAttr();
        reportNewFiles();
        freeReportInfo();
    }

    w_log(LL_INFO, "End report generating");
}                               /* report() */
