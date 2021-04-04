/*****************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 *****************************************************************************
 * Hatch file to fileecho implementation
 *
 * This file is part of HTICK, part of the Husky fidosoft project
 * http://husky.sourceforge.net
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
#include <errno.h>
/*  compiler.h */
#include <huskylib/compiler.h>

#ifdef HAS_UNISTD_H
# include <unistd.h>
#endif
/* smapi */
#include <huskylib/huskylib.h>
/* fidoconf */
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <huskylib/dirlayer.h>
#include <huskylib/xstr.h>
#include <fidoconf/afixcmd.h>
#include <huskylib/recode.h>
#include <huskylib/crc.h>

#include <areafix/areafix.h>
/*  htick  */
#include <filecase.h>
#include <seenby.h>
#include <add_desc.h>
#include <hatch.h>
#include "version.h"
#include <global.h>
#include <fcommon.h>
#include <toss.h>

typedef enum _descrMacro
{
    DESCRERROR = 0, BBSONELINE = 1, BBSMLTLINE, DIZONELINE, DIZMLTLINE, FILONELINE, FILMLTLINE
} descrMacro;
static descrMacro getDescOptions(char * desc, char ** filename)
{
    byte descOPT = 0;

    if(!desc || !filename)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: getDescOptions(%s,%s). This is serious error in program, please report to developers.",
              desc ? "desc" : "NULL",
              filename ? "filename" : "NULL");
        return DESCRERROR;
    }

    if(stricmp(desc, "@@BBS") == 0)
    {
        descOPT = BBSONELINE;
    }
    else if(stricmp(desc, "@@DIZ") == 0)
    {
        descOPT = DIZONELINE;
    }
    else if(stricmp(desc, "@BBS") == 0)
    {
        descOPT = BBSMLTLINE;
    }
    else if(stricmp(desc, "@DIZ") == 0)
    {
        descOPT = DIZMLTLINE;
    }
    else if(desc[0] == '@')
    {
        char * basename = desc;
        basename++;

        if(*basename == '@')
        {
            basename++;
        }

        if(fexist(basename))
        {
            descOPT   = (basename - desc == 1) ? FILMLTLINE : FILONELINE;
            *filename = sstrdup(basename);
        }
    }

    return descOPT;
} /* getDescOptions */

static void expandDescMacros(s_ticfile * tic, char * hatchedFile)
{
    char * descr_file_name = NULL;
    char * basename        = NULL;
    char * ldFileName      = NULL;
    char * sdFileName      = NULL;
    char ctmp;
    int SdescOPT = 0;
    int LdescOPT = 0;
    s_ticfile tmptic;
    char ** tmpArray = NULL;
    UINT i;

    if(!tic || !hatchedFile)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: expandDescMacros(%s,%s). This is serious error in program, please report to developers.",
              tic ? "tic" : "NULL",
              hatchedFile ? "hatchedFile" : "NULL");
        return;
    }

    memset(&tmptic, 0, sizeof(s_ticfile));

    if(tic->anzdesc > 0)
    {
        SdescOPT = getDescOptions(tic->desc[0], &sdFileName);

        if(SdescOPT > 0)
        {
            nfree(tic->desc[0]);
            tic->desc[0] = sstrdup("-- description missing --");
        }
    }

    if(tic->anzldesc > 0)
    {
        LdescOPT = getDescOptions(tic->ldesc[0], &ldFileName);

        if(LdescOPT > 0)
        {
            nfree(tic->ldesc[0]);
            nfree(tic->ldesc);
            tic->anzldesc = 0;
        }
    }

    if(!SdescOPT && !LdescOPT)
    {
        goto recode;
    }

    if(SdescOPT == BBSONELINE || LdescOPT == BBSONELINE || SdescOPT == BBSMLTLINE ||
       LdescOPT == BBSMLTLINE)
    {
        if(strrchr(tic->file, PATH_DELIM))
        {
            basename  = strrchr(tic->file, PATH_DELIM);
            ctmp      = *basename;
            *basename = '\0';
            xscatprintf(&descr_file_name, "%s%c%s", tic->file, PATH_DELIM,
                        config->fileDescription);
            *basename = ctmp;
        }
        else
        {
            xstrcat(&descr_file_name, config->fileDescription);
        }

        adaptcase(descr_file_name);

        if(GetDescFormBbsFile(descr_file_name, tic->file, &tmptic) == 0)
        {
            if(LdescOPT == BBSONELINE)
            {
                tic->anzldesc = 1;
                tic->ldesc    = scalloc(sizeof(*tic->ldesc), tic->anzldesc);
                tic->ldesc[0] = sstrdup(tmptic.desc[0]);
                LdescOPT      = 0;
            }
            else if(LdescOPT == BBSMLTLINE)
            {
                tic->anzldesc = tmptic.anzdesc;
                tic->ldesc    = scalloc(sizeof(*tic->ldesc), tic->anzldesc);

                for(i = 0; i < tic->anzldesc; i++)
                {
                    tic->ldesc[i] = sstrdup(tmptic.desc[i]);
                }
                LdescOPT = 0;
            }

            if(SdescOPT == BBSONELINE)
            {
                tic->anzdesc = 1;
                tic->desc    = scalloc(sizeof(*tic->desc), tic->anzdesc);
                tic->desc[0] = sstrdup(tmptic.desc[0]);
                SdescOPT     = 0;
            }
            else if(SdescOPT == BBSMLTLINE)
            {
                tic->anzdesc = tmptic.anzdesc;
                tic->desc    = scalloc(sizeof(*tic->desc), tic->anzdesc);

                for(i = 0; i < tic->anzdesc; i++)
                {
                    tic->desc[i] = sstrdup(tmptic.desc[i]);
                }
                SdescOPT = 0;
            }

            disposeTic(&tmptic);
            memset(&tmptic, 0, sizeof(s_ticfile));
        }
    }

    if(SdescOPT == DIZONELINE || LdescOPT == DIZONELINE || SdescOPT == DIZMLTLINE ||
       LdescOPT == DIZMLTLINE)
    {
        if(GetDescFormDizFile(hatchedFile, &tmptic) == 1)
        {
            if(SdescOPT == DIZMLTLINE || LdescOPT == DIZMLTLINE)
            {
                tmpArray = scalloc(sizeof(char *), tmptic.anzldesc);

                for(i = 0; i < tmptic.anzldesc; i++)
                {
                    tmpArray[i] = sstrdup(tmptic.ldesc[i]);
                }
            }

            if(LdescOPT == DIZONELINE)
            {
                tic->anzldesc = 1;
                tic->ldesc    = scalloc(sizeof(*tic->ldesc), tic->anzldesc);
                tic->ldesc[0] = sstrdup(tmptic.ldesc[0]);
            }
            else if(LdescOPT == DIZMLTLINE)
            {
                tic->anzldesc = tmptic.anzldesc;
                tic->ldesc    = tmpArray;
            }

            if(SdescOPT == DIZONELINE)
            {
                nfree(tic->desc[0]);
                tic->desc[0] = sstrdup(tmptic.ldesc[0]);
            }
            else if(SdescOPT == DIZMLTLINE)
            {
                tic->anzdesc = tmptic.anzldesc;
                tic->desc    = tmpArray;
            }

            disposeTic(&tmptic);
            memset(&tmptic, 0, sizeof(s_ticfile));
        }
    }

    if(SdescOPT == FILONELINE || SdescOPT == FILMLTLINE)
    {
        if(GetDescFormFile(sdFileName, &tmptic) == 1)
        {
            if(SdescOPT == FILMLTLINE || LdescOPT == FILMLTLINE)
            {
                tmpArray = scalloc(sizeof(char *), tmptic.anzldesc);

                for(i = 0; i < tmptic.anzldesc; i++)
                {
                    tmpArray[i] = sstrdup(tmptic.ldesc[i]);
                }
            }

            if(SdescOPT == FILONELINE)
            {
                nfree(tic->desc[0]);
                tic->desc[0] = sstrdup(tmptic.ldesc[0]);
            }
            else if(SdescOPT == FILMLTLINE)
            {
                tic->anzdesc = tmptic.anzldesc;
                tic->desc    = tmpArray;
            }
        }
    }

    if(LdescOPT == FILONELINE || LdescOPT == FILMLTLINE)
    {
        if(sdFileName && ldFileName && stricmp(sdFileName, ldFileName) == 0)
        {
            if(LdescOPT == FILONELINE)
            {
                tic->anzldesc = 1;
                tic->ldesc    = scalloc(sizeof(*tic->ldesc), tic->anzldesc);
                tic->ldesc[0] = sstrdup(tmptic.ldesc[0]);
            }
            else if(LdescOPT == FILMLTLINE)
            {
                tic->anzldesc = tmptic.anzldesc;
                tic->ldesc    = tmpArray;
            }
        }
        else
        {
            disposeTic(&tmptic);
            memset(&tmptic, 0, sizeof(s_ticfile));

            if(GetDescFormFile(ldFileName, &tmptic) == 1)
            {
                if(LdescOPT == FILMLTLINE)
                {
                    tic->anzldesc = tmptic.anzldesc;
                    tic->ldesc    = scalloc(sizeof(char *), tmptic.anzldesc);

                    for(i = 0; i < tmptic.anzldesc; i++)
                    {
                        tic->ldesc[i] = sstrdup(tmptic.ldesc[i]);
                    }
                }
                else if(LdescOPT == FILONELINE)
                {
                    tic->anzldesc = 1;
                    tic->ldesc    = scalloc(sizeof(*tic->ldesc), tic->anzldesc);
                    tic->ldesc[0] = sstrdup(tmptic.ldesc[0]);
                }
            }
        }
    }

recode:

    if(config->outtab != NULL)
    {
        for(i = 0; i < tic->anzdesc; i++)
        {
            recodeToTransportCharset(tic->desc[i]);
        }

        for(i = 0; i < tic->anzldesc; i++)
        {
            recodeToTransportCharset(tic->ldesc[i]);
        }
    }

    disposeTic(&tmptic);
} /* expandDescMacros */

HATCH_HATCH_RETURN_CODES hatch()
{
    s_area * filearea;
    struct stat stbuf;
    char * hatchedFile = NULL;
    char buffer[256]   = "";

    w_log(LL_INFO, "Start file hatch...");
    hatchedFile = sstrdup(hatchInfo->file);

    /*  Exist file? */
    adaptcase(hatchedFile);
    {
        FILE * f = fopen(hatchedFile, "rb");

        if(f)
        {
            fclose(f);
        }
        else
        {
            w_log(LL_ALERT, "File %s: %s", hatchedFile, strerror(errno));
            return HATCH_ERROR_FILE;
        }
    }
    xstrcpy(&hatchInfo->file, GetFilenameFromPathname(hatchedFile));

    if(stricmp(hatchedFile, hatchInfo->file) == 0)      /* hatch from current dir */
    {
        size_t len = 0;
        getcwd(buffer, 256);
        len = strlen(buffer);

        if(buffer[len - 1] == PATH_DELIM)
        {
            Strip_Trailing(buffer, PATH_DELIM);
        }

        nfree(hatchedFile);
        xscatprintf(&hatchedFile, "%s%c%s", buffer, (char)PATH_DELIM, hatchInfo->file);
    }

    MakeProperCase(hatchInfo->file);
    filearea = getFileArea(hatchInfo->area);

    if(filearea == NULL)
    {
        w_log('9', "Cannot open or create File Area %s", hatchInfo->area);
        return HATCH_NOT_FILEAREA;
    }

    expandDescMacros(hatchInfo, hatchedFile);
    stat(hatchedFile, &stbuf);
    hatchInfo->size   = stbuf.st_size;
    hatchInfo->origin = hatchInfo->from = *filearea->useAka;
    seenbyAdd(&hatchInfo->seenby, &hatchInfo->anzseenby, filearea->useAka);

    if(filearea->description)
    {
        hatchInfo->areadesc = sstrdup(filearea->description);
    }

    /*  Adding crc */
    hatchInfo->crc = filecrc32(hatchedFile);

    if(sendToLinks(0, filearea, hatchInfo, hatchedFile) == 0)
    {
        doSaveTic(NULL, hatchInfo, filearea);
    }

    nfree(hatchedFile);
    return HATCH_OK;
} /* hatch */

HATCH_SEND_RETURN_CODES send(char * filename, char * area, char * addr)
/* 0 - OK */
/* 1 - Passthrough filearea */
/* 2 - filearea not found */
/* 3 - file not found */
/* 4 - link not found */
/* 5 - put on link error */
/* -1 - invalid parameter */
{
    s_ticfile tic;
    s_link * link = NULL;
    s_area * filearea;
    char * fileToSend = NULL, * descr_file_name = NULL, * tmpfile = NULL;
    char timestr[40];
    struct stat stbuf;
    time_t acttime;
    HATCH_SEND_RETURN_CODES rc;

    if(!filename || !area || !addr)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: send(%s,%s,%s). This is serious error in program, please report to developers.",
              filename ? "filename" : "NULL",
              area ? "area" : "NULL",
              addr ? "addr" : "NULL");
        return HATCH_SEND_E_INVALID;
    }

    w_log(LL_INFO, "Start file send (%s in %s to %s)", filename, area, addr);
    filearea = getFileArea(area);

    if(filearea == NULL)
    {
        if(!quiet)
        {
            fprintf(stderr, "Error: Filearea not found\n");
        }

        return HATCH_SEND_E_FILEAREANOTFOUND;
    }

    link = getLink(config, addr);

    if(link == NULL)
    {
        if(!quiet)
        {
            fprintf(stderr, "Error: Link not found\n");
        }

        return HATCH_SEND_E_LINKNOTFOUND;
    }

    memset(&tic, 0, sizeof(tic));

    if(filearea->msgbType == MSGTYPE_PASSTHROUGH)
    {
        fileToSend = sstrdup(config->passFileAreaDir);
    }
    else
    {
        fileToSend = sstrdup(filearea->fileName);
    }

    strLower(fileToSend);
    _createDirectoryTree(fileToSend);
    xstrcat(&fileToSend, filename);

    /* Does file exist? */
    adaptcase(fileToSend);

    if(!fexist(fileToSend))
    {
        if(!quiet)
        {
            fprintf(stderr, "Error: File not found\n");
        }

        w_log('6', "File %s, not found", fileToSend);
        nfree(fileToSend);
        disposeTic(&tic);
        return HATCH_SEND_E_FILENOTFOUND;
    }

    tic.file = sstrdup(filename);

    if(filearea->sendorig)
    {
        xstrscat(&tmpfile, config->passFileAreaDir, tic.file, NULL);
        adaptcase(tmpfile);

        if(copy_file(fileToSend, tmpfile, 1) != 0) /* overwrite existing file if not the same */
        {
            adaptcase(fileToSend);

            if(copy_file(fileToSend, tmpfile, 1) == 0) /* overwrite existing file if not the same */
            {
                w_log('6', "Copied %s to %s", fileToSend, tmpfile);
            }
            else
            {
                w_log('9', "File %s not found or cannot be copied", fileToSend);
                disposeTic(&tic);
                nfree(fileToSend);
                nfree(tmpfile);
                return HATCH_SEND_E_FILEAREANOTFOUND;
            }
        }
        else
        {
            w_log('6', "Copied %s to %s", fileToSend, tmpfile);
            strcpy(fileToSend, tmpfile);
        }
    }

    tic.area = sstrdup(area);
    stat(fileToSend, &stbuf);
    tic.size   = stbuf.st_size;
    tic.origin = tic.from = *filearea->useAka;
    /*  Adding crc */
    tic.crc = filecrc32(fileToSend);
    xstrscat(&descr_file_name, filearea->fileName, config->fileDescription, NULL);
    adaptcase(descr_file_name);
    GetDescFormBbsFile(descr_file_name, tic.file, &tic);
    /*  Adding path */
    time(&acttime);
    strcpy(timestr, asctime(gmtime(&acttime)));
    timestr[strlen(timestr) - 1] = 0;

    if(timestr[8] == ' ')
    {
        timestr[8] = '0';
    }

    tic.path = srealloc(tic.path, (tic.anzpath + 1) * sizeof(*tic.path));
    tic.path[tic.anzpath] = NULL;
    xscatprintf(&tic.path[tic.anzpath], "%s %lu %s UTC %s", aka2str(filearea->useAka),
                (unsigned long)time(NULL), timestr, versionStr);
    tic.anzpath++;
    /*  Adding Downlink to Seen-By */
    seenbyAdd(&tic.seenby, &tic.anzseenby, &link->hisAka);

    /*  Forward file to */
    if(PutFileOnLink(fileToSend, &tic, link))
    {
        rc = HATCH_SEND_OK;
    }
    else
    {
        rc = HATCH_SEND_E_PUT_ON_LINK;
    }

    disposeTic(&tic);
    nfree(fileToSend);
    nfree(tmpfile);
    nfree(descr_file_name);
    return rc;
} /* send */

/* Return values:
 * 1 if success
 * 0 if error
 */
int PutFileOnLink(char * newticedfile, s_ticfile * tic, s_link * downlink)
{
    int busy            = 0;
    char * linkfilepath = NULL;
    char * newticfile   = NULL;
    FILE * flohandle    = NULL;
    hs_addr * aka;
    char * str_hisAka, * str_Aka;

    if(!newticedfile || !tic || !downlink)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: PutFileOnLink(%s,%s,%s). This is serious error in program, please report to developers.",
              newticedfile ? "newticedfile" : "NULL",
              tic ? "tic" : "NULL",
              downlink ? "downlink" : "NULL");
        return 0;
    }

    aka = SelectPackAka(downlink);
    memcpy(&tic->from, downlink->ourAka, sizeof(hs_addr));
    memcpy(&tic->to, &downlink->hisAka, sizeof(hs_addr));
    nfree(tic->password);

    if(downlink->ticPwd != NULL)
    {
        tic->password = sstrdup(downlink->ticPwd);
    }

    if(createOutboundFileNameAka(downlink, downlink->fileEchoFlavour, FLOFILE, aka) == 1)
    {
        busy = 1;
    }

    if(busy)
    {
        xstrcat(&linkfilepath, config->busyFileDir);
    }
    else
    {
        if(config->separateBundles)
        {
            char * point = strrchr(downlink->floFile, '.');
            *point = '\0';
            xscatprintf(&linkfilepath, "%s.sep%c", downlink->floFile, PATH_DELIM);
            *point = '.';
        }
        else
        {
            xstrcat(&linkfilepath, config->ticOutbound);
        }
    }

    /*  fileBoxes support */
    if(needUseFileBoxForLinkAka(config, downlink, aka))
    {
        nfree(linkfilepath);

        if(!downlink->fileBox)
        {
            downlink->fileBox = makeFileBoxNameAka(config, downlink, aka);
        }

        xstrcat(&linkfilepath, downlink->fileBox);
    }

    _createDirectoryTree(linkfilepath);

    /* Don't create TICs for everybody */
    if(!downlink->noTIC)
    {
        newticfile = makeUniqueDosFileName(linkfilepath, "tic", config);

        if(!writeTic(newticfile, tic))
        {
            /* try to toss into config->busyFileDir */
            w_log(LL_CRIT, "Can't write into: %s", linkfilepath);
            nfree(newticfile);
            nfree(linkfilepath);
            xstrcat(&linkfilepath, config->busyFileDir);
            newticfile = makeUniqueDosFileName(linkfilepath, "tic", config);

            if(!writeTic(newticfile, tic))
            {
                w_log(LL_CRIT, "Can't write into: %s", linkfilepath);
                nfree(newticfile);
                return 0;
            }

            busy = 1;           /* do not touch floFile if it's BSO */
        }
    }
    else
    {
        newticfile = NULL;
    }

    if(needUseFileBoxForLinkAka(config, downlink, aka))
    {
        /* do not copy file into busyFileDir */
        if(strcasecmp(linkfilepath, config->busyFileDir))
        {
            xstrcat(&linkfilepath, tic->file);

            if(link_file(newticedfile, linkfilepath) == 0)
            {
                if(copy_file(newticedfile, linkfilepath, 1) == 0) /* overwrite existing file if
                                                                     not same */
                {
                    w_log(LL_FILESENT, "Copied: %s", newticedfile);
                    w_log(LL_FILESENT, "    to: %s", linkfilepath);
                }
                else
                {
                    w_log('9', "File %s not found or not copyable", newticedfile);

                    if(!busy && downlink->bsyFile)
                    {
                        remove(downlink->bsyFile);
                    }

                    nfree(downlink->bsyFile);
                    nfree(downlink->floFile);
                    nfree(linkfilepath);
                }
            }
            else
            {
                w_log(LL_FILESENT, "Linked: %s", newticedfile);
                w_log(LL_FILESENT, "    to: %s", linkfilepath);
            }
        }
    }
    else if(!busy)
    {
        flohandle = fopen(downlink->floFile, "a");
        fprintf(flohandle, "%s\n", newticedfile);

        if(newticfile != NULL)
        {
            fprintf(flohandle, "^%s\n", newticfile);
        }

        fclose(flohandle);
    }

    if(!busy || needUseFileBoxForLinkAka(config, downlink, aka))
    {
        str_hisAka = aka2str5d(downlink->hisAka);
        str_Aka    = aka2str5d(*aka);

        if(newticfile != NULL)
        {
            w_log(LL_LINK,
                  "Forwarding %s with tic %s for %s via %s",
                  tic->file,
                  GetFilenameFromPathname(newticfile),
                  str_hisAka,
                  str_Aka);
        }
        else
        {
            w_log(LL_LINK, "Forwarding %s for %s via %s", tic->file, str_hisAka, str_Aka);
        }

        nfree(str_hisAka);
        nfree(str_Aka);

        if(!busy && downlink->bsyFile)
        {
            remove(downlink->bsyFile);
        }
    }

    nfree(downlink->bsyFile);
    nfree(downlink->floFile);
    nfree(linkfilepath);
    nfree(newticfile);
    return 1;
} /* PutFileOnLink */
