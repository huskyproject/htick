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
 *****************************************************************************
 * $Id$
 *****************************************************************************/
/* clib */
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
/* compiler.h */
#include <huskylib/compiler.h>
#include <huskylib/huskylib.h>

#ifdef __OS2__
# define INCL_DOSFILEMGR        /* for hidden() routine */
# include <os2.h>
#endif

#ifdef HAS_IO_H
# include <io.h>
#endif

#ifdef HAS_SHARE_H
# include <share.h>
#endif

#ifdef HAS_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAS_DOS_H
# include <dos.h>
#endif

#ifdef HAS_PROCESS_H
# include <process.h>
#endif
/* fidoconf */
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <huskylib/log.h>
#include <huskylib/dirlayer.h>
#include <huskylib/xstr.h>
#include <fidoconf/afixcmd.h>
#include <huskylib/recode.h>
#include <huskylib/crc.h>
#include <areafix/query.h>
#include <areafix/areafix.h>

#if defined (A_HIDDEN) && !defined (_A_HIDDEN)
# define _A_HIDDEN A_HIDDEN
#endif
/* htick */
#include <fcommon.h>
#include "version.h"
#include <global.h>
#include <toss.h>
#include <htickafix.h>
#include <add_desc.h>
#include <seenby.h>
#include <filecase.h>
#include "hatch.h"
#include "report.h"
/* tic keywords calculated crc (CCITT-16, polynom 0x1021) */

#define CRC_CREATED 0xACDA      /*0x4EC6 */
#define CRC_FILE 0x9AF9         /*0x1A66 */
#define CRC_ALTFILE 0x02B3      /* "ALTFILE" */
#define CRC_AREADESC 0xD824     /*0xB621 */
#define CRC_DESC 0x717B         /*0xECBD */
#define CRC_AREA 0x825A         /*0x825A */
#define CRC_CRC 0x5487          /*0x5487 */
#define CRC_REPLACES 0xCE24     /*0xCF01 */
#define CRC_ORIGIN 0xE52A       /*0xFCDC */
#define CRC_FROM 0xFD30         /*0x1293 */
#define CRC_TO 0x7B50           /*0x7B50 */
#define CRC_PATH 0x5411         /*0x5411 */
#define CRC_SEENBY 0xF84C       /*0xC3D4 */
#define CRC_PW 0x24AD           /*0x24AD */
#define CRC_SIZE 0x94CE         /*0x5593 */
#define CRC_DATE 0x54EA         /*0xD4BD */
#define CRC_DESTINATION 0x6F36  /*0xA9D1 */
#define CRC_MAGIC 0x7FF4        /*0xD858 */
#define CRC_LDESC 0xEB38        /*0x5394 */
/* Write netmail message into areaname or first netmail area (if areaname is NULL pointer)
 */
void writeNetmail(s_message * msg, char * areaName)
{
    HAREA netmail;
    HMSG msgHandle;
    size_t len = msg->textLength;
    byte * bodyStart;           /* msg-body without kludgelines start */
    char * ctrlBuf;             /* Kludgelines */
    XMSG msgHeader;
    s_area * nmarea;

    if(msg == NULL)
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: writeNetmail(NULL,areaname). This is a bug, please report it to developers.",
              __LINE__);
        return;
    }

    if(!config->netMailAreas)
    {
        w_log(LL_CRIT, "Netmail areas not defined, message dropped! Please check config.");
        return;
    }

    if(!areaName || (nmarea = getNetMailArea(config, areaName)) == NULL)
    {
        nmarea = &(config->netMailAreas[0]);
    }

    netmail = MsgOpenArea((UCHAR *)nmarea->fileName, MSGAREA_CRIFNEC, (word)nmarea->msgbType);

    if(netmail != NULL)
    {
        msgHandle = MsgOpenMsg(netmail, MOPEN_CREATE, 0);

        if(msgHandle != NULL)
        {
            if((!config->recodeMsgBase) && (config->outtab != NULL))
            {
                recodeToTransportCharset((char *)msg->subjectLine);
                recodeToTransportCharset((char *)msg->fromUserName);
                recodeToTransportCharset((char *)msg->toUserName);
                recodeToTransportCharset((char *)msg->text);
            }

            msgHeader = createXMSG(config, msg, NULL, MSGLOCAL, NULL);
            /* Create CtrlBuf for SMAPI */
            {
                byte * bb;      /* Prevent GCC warning "dereferencing type-punned pointer will
                                   break strict-aliasing rules" */
                ctrlBuf   = (char *)CopyToControlBuf((byte *)msg->text, &bb, &len);
                bodyStart = bb;
            }
            /* write message */
            MsgWriteMsg(msgHandle,
                        0,
                        &msgHeader,
                        bodyStart,
                        (dword)len,
                        (dword)len,
                        (dword)strlen(ctrlBuf) + 1,
                        (UCHAR *)ctrlBuf);
            nfree(ctrlBuf);
            MsgCloseMsg(msgHandle);
            w_log(LL_POSTING,
                  "Wrote Netmail to: %u:%u/%u.%u",
                  msg->destAddr.zone,
                  msg->destAddr.net,
                  msg->destAddr.node,
                  msg->destAddr.point);
        }
        else
        {
            w_log(LL_ERROR, "Could not write message to %s", areaName);
        }                       /* endif */

        MsgCloseArea(netmail);
    }
    else
    {
/*     printf("%u\n", msgapierr); */
        w_log(LL_ERROR, "Could not open netmail area %s", areaName);
    }                           /* endif */
} /* writeNetmail */

/* Write (create) TIC-file
 * Return 0 if error, 1 if success
 */
int writeTic(char * ticfile, s_ticfile * tic)
{
    FILE * tichandle;
    unsigned int i;
    char * p;
    s_area * filearea = NULL;

    if((ticfile == NULL) || (tic == NULL))
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: writeTic(%s%s%s,%s). This is a bug, please report it to developers.",
              __LINE__,
              ticfile ? "\"" : "",
              ticfile ? ticfile : "NULL",
              ticfile ? "\"" : "",
              tic ? "tic" : "NULL");
        return 0;
    }

    tichandle = fopen(ticfile, "wb");

    if(!tichandle)
    {
        w_log(LL_ERROR, "Unable to open TIC file \"%s\": %s!!!\n", ticfile, strerror(errno));
        return 0;
    }

    fprintf(tichandle, "Created by HTick, written by Gabriel Plutzar\r\n");
    fprintf(tichandle, "File %s\r\n", tic->file);
    fprintf(tichandle, "Area %s\r\n", tic->area);
    filearea = getFileArea(tic->area);

    if(!tic->areadesc && filearea && filearea->description)
    {
        tic->areadesc = sstrdup(filearea->description);

        if(config->outtab)
        {
            recodeToTransportCharset(tic->areadesc);
        }
    }

    if(tic->areadesc)
    {
        fprintf(tichandle, "Areadesc %s\r\n", tic->areadesc);
    }

    for(i = 0; i < tic->anzdesc; i++)
    {
        fprintf(tichandle, "Desc %s\r\n", tic->desc[i]);
    }

    for(i = 0; i < tic->anzldesc; i++)
    {
        while((p = strchr(tic->ldesc[i], 26)) != NULL)
        {
            *p = ' ';
        }
        fprintf(tichandle, "LDesc %s\r\n", tic->ldesc[i]);
    }

    if(tic->replaces)
    {
        fprintf(tichandle, "Replaces %s\r\n", tic->replaces);
    }

    if(tic->from.zone != 0)
    {
        fprintf(tichandle, "From %s\r\n", aka2str(&tic->from));
    }

    if(tic->to.zone != 0)
    {
        fprintf(tichandle, "To %s\r\n", aka2str(&tic->to));
    }

    if(tic->origin.zone != 0)
    {
        fprintf(tichandle, "Origin %s\r\n", aka2str(&tic->origin));
    }

    if(tic->size >= 0)
    {
        fprintf(tichandle, "Size %u\r\n", tic->size);
    }

    if(tic->date != 0)
    {
        fprintf(tichandle, "Date %lu\r\n", tic->date);
    }

    if(tic->crc_is_present || tic->crc) /* FIXME: Look through all code to check up simultaneous
                                           change of a tic.crc and a tic.tic->crc */
    {
        fprintf(tichandle, "Crc %08lX\r\n", tic->crc);
    }

    for(i = 0; i < tic->anzpath; i++)
    {
        fprintf(tichandle, "Path %s\r\n", tic->path[i]);
    }

    for(i = 0; i < tic->anzseenby; i++)
    {
        fprintf(tichandle, "Seenby %s\r\n", aka2str(&tic->seenby[i]));
    }

    if(tic->password)
    {
        fprintf(tichandle, "Pw %s\r\n", tic->password);
    }

    fclose(tichandle);
    return 1;
}                               /* writeTic() */

/* Free memory allocated in structure s_ticfile *tic
 */
void disposeTic(s_ticfile * tic)
{
    unsigned int i;

    if(tic == NULL)
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: disposeTic(NULL). This is a bug, please report it to developers.",
              __LINE__);
        return;
    }

    nfree(tic->seenby);
    nfree(tic->file);
    nfree(tic->altfile);
    nfree(tic->area);
    nfree(tic->areadesc);
    nfree(tic->replaces);
    nfree(tic->password);

    for(i = 0; i < tic->anzpath; i++)
    {
        nfree(tic->path[i]);
    }
    nfree(tic->path);

    for(i = 0; i < tic->anzdesc; i++)
    {
        nfree(tic->desc[i]);
    }
    nfree(tic->desc);

    for(i = 0; i < tic->anzldesc; i++)
    {
        nfree(tic->ldesc[i]);
    }
    nfree(tic->ldesc);
}                               /* disposeTic() */

/* Check TIC file validity
 * Return: 0 if TIC is valid,
 *         negative integer if error found in function arguments,
 *         positive integer if information in TIC is bad (the value is the number of errors)
 */
static int checkTic(const char * ticfilename, const s_ticfile * tic)
{
    int nRet = 0;

    if((ticfilename == NULL) || (tic == NULL))
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: checkTic(%s%s%s,%s). This is a bug, please report it to developers.",
              __LINE__,
              ticfilename ? "\"" : "",
              ticfilename ? ticfilename : "NULL",
              ticfilename ? "\"" : "",
              tic ? "tic" : "NULL");
        return -1;
    }

    if(!tic->file)
    {
        w_log(LL_ERR, "File name is missing in TIC \"%s\" but required by FSC-87", ticfilename);
        nRet++;
    }

    if(!tic->area)
    {
        w_log(LL_ERR, "Filearea name is missing in TIC \"%s\" but required by FSC-87",
              ticfilename);
        nRet++;
    }

    if(!tic->crc_is_present)
    {
        w_log(LL_SECURITY, "CRC is missing in TIC \"%s\" but required by FSC-87", ticfilename);
    }

    if(tic->size < 0)
    {
        w_log(LL_ALERT,
              "There is no file size in TIC %s, FSC-87 says: \"SHOULD be required\"",
              ticfilename);
    }

    if((!tic->crc) && (tic->size > 0))
    {
        w_log(LL_ERR,
              "The size is not zero but CRC32 equals zero in TIC %s, it is impossible and this ticket file is wrong.",
              ticfilename);
        nRet++;
    }

    if((!tic->size) && (tic->crc_is_present))
    {
        w_log(LL_ERR,
              "The size equals zero but CRC32 is not zero in TIC %s, it is impossible and this ticket file is wrong.",
              ticfilename);
        nRet++;
    }

    if(!tic->to.zone && !tic->to.net && !tic->to.node)
    {
        w_log(LL_SECURITY, "'To' address is missing in TIC \"%s\"", ticfilename);
    }
    else if(!tic->to.zone || !tic->to.net)
    {
        w_log(LL_ERR, "'To' address (%s) is illegal in TIC \"%s\"", aka2str(&tic->to), ticfilename);
        nRet++;
    }

    if(!tic->from.zone && !tic->from.net && !tic->from.node)
    {
        w_log(LL_ERR, "'From' address is missing in TIC \"%s\", required by FSC-87", ticfilename);
        nRet++;
    }
    else if(!tic->from.zone || !tic->from.net)
    {
        w_log(LL_ERR, "Zone or net is missing in the 'From' address in TIC \"%s\"", ticfilename);
        nRet++;
    }

    if(!tic->origin.zone && !tic->origin.net && !tic->origin.node)
    {
        w_log(LL_ERR,
              "Originating address is missing in TIC \"%s\", required by FSC-87",
              ticfilename);
        /* nRet++; */
    }
    else if(!tic->origin.zone || !tic->origin.node)
    {
        w_log(LL_ERR,
              "Zone or node is missing in the originating address in TIC \"%s\"",
              ticfilename);
        /* nRet++; */
    }

    if(!tic->path)
    {
        w_log(LL_ALERT, "PATH lines are missing in TIC \"%s\", required by FSC-87", ticfilename);
    }

    if(!tic->seenby)
    {
        w_log(LL_ALERT, "SEENBY lines are missing in TIC \"%s\", required by FSC-87", ticfilename);
    }

    if(!tic->password)          /* Mandatory field (FSC-87) */
    {
        w_log(LL_SECURITY, "Password is missing in TIC \"%s\", required by FSC-87", ticfilename);
    }

    return nRet;
}                               /* checkTic */

/* Read TIC file and store values into 2nd parameter.
 * Return 1 if success, 2 if bad data in tic, and 0 if file or parameter error.
 * Clean 2nd parameter before processing if not error.
 */
enum parseTic_result parseTic(char * ticfile, s_ticfile * tic)
{
    FILE * tichandle;
    char * line, * token, * param, * linecut = "", * emptyline = "";
    s_link * ticSourceLink = NULL;
    UINT16 key;
    hs_addr Aka;
    int rc = parseTic_success;

    if((ticfile == NULL) || (tic == NULL))
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: parseTic(%s%s%s,%s). This is a bug, please report it to developers.",
              __LINE__,
              ticfile ? "\"" : "",
              ticfile ? ticfile : "NULL",
              ticfile ? "\"" : "",
              tic ? "tic" : "NULL");
        return parseTic_error;
    }

    memset(tic, '\0', sizeof(s_ticfile));
    tic->size = -1;  /* To check later the existing token "size" field in TIC file */

#ifndef HAS_sopen
    tichandle = fopen(ticfile, "r");
#else
    {
        int fh = 0;
        /* ensure that ticfile won't be removed while parsing */
# if defined (__UNIX__) || defined (__DJGPP__)
        fh = sopen(ticfile, O_RDWR | O_BINARY, SH_DENYNO, S_IWRITE | S_IREAD);
# else
        fh = sopen(ticfile, O_RDWR | O_BINARY, SH_DENYRW);
# endif

        if(fh < 0)
        {
            w_log(LL_ERROR, "Can't open '%s': %s (sopen())", ticfile, strerror(errno));
            return parseTic_error;
        }

        tichandle = fdopen(fh, "r");
    }
#endif

    if(!tichandle)
    {
        w_log(LL_ERROR, "Can't open '%s': %s", ticfile, strerror(errno));
        return parseTic_error;
    }

    while((line = readLine(tichandle)) != NULL)
    {
        line = trimLine(line);

        if(*line == 0 || *line == 10 || *line == 13 || *line == ';' || *line == '#')
        {
            continue;
        }

        if(config->MaxTicLineLength)
        {
            linecut = (char *)smalloc(config->MaxTicLineLength + 1);
            strncpy(linecut, line, config->MaxTicLineLength);
            linecut[config->MaxTicLineLength] = 0;
            token = strtok(linecut, " \t");
        }
        else
        {
            token = strtok(line, " \t");
        }

        if(token)
        {
            key = strcrc16(strUpper(token), 0);
            /* calculate crc16 of tic                                   */
            w_log(LL_DEBUGz, "#define CRC_%-12s 0x%X;", strUpper(token), key);
            param = stripLeadingChars(strtok(NULL, "\0"), "\t");

            if(!param)
            {
                switch(key)
                {
                    case CRC_DESC:
                    case CRC_LDESC:
                        param = emptyline;
                        break;

                    case CRC_SIZE:
                        w_log(LL_ERR, "Wrong TIC \"%s\": \"Size\" without value!", ticfile);
                        tic->size = -1;
                        rc        = parseTic_bad;
                        break;

                    default:
                        continue;
                }
            }

            switch(key)
            {
                case CRC_CREATED:
                case CRC_MAGIC:
                    break;

                case CRC_FILE:
                    tic->file = sstrdup(param);
                    break;

                case CRC_ALTFILE:
                    tic->altfile = sstrdup(param); /* just for reports */
                    break;

                case CRC_AREADESC:
                    tic->areadesc = sstrdup(param);
                    break;

                case CRC_AREA:
                    tic->area = sstrdup(param);
                    break;

                case CRC_CRC:
                    tic->crc            = strtoul(param, NULL, 16);
                    tic->crc_is_present = 1;
                    break;

                case CRC_SIZE:

                    if(param[0] == '-')
                    {
                        w_log(LL_ERR, "Wrong TIC \"%s\": negative size (\"%s\")!", ticfile, param);
                        rc = parseTic_bad;
                    }
                    else
                    {
                        tic->size = tic->size = atoi(param);

                        if((!tic->size) && (!strcmp(param, "0")))
                        {
                            w_log(LL_ERR,
                                  "Wrong TIC \"%s\": value of size is \"%s\"! " "(May be pozitive integer or one zero",
                                  ticfile,
                                  param);
                            rc = parseTic_bad;
                        }
                    }

                    break;

                case CRC_DATE:
                    tic->date = atoi(param);
                    break;

                case CRC_REPLACES:

                    if(*param == '*')
                    {
                        w_log(LL_TIC,
                              "TIC %s: Illegal value: 'REPLACES %s', ignored",
                              ticfile,
                              param);
                        break;
                    }

                    tic->replaces = sstrdup(param);
                    break;

                case CRC_PW:
                    tic->password = sstrdup(param);
                    break;

                case CRC_FROM:
                    parseFtnAddrZS(param, &tic->from);
                    ticSourceLink = getLinkFromAddr(config, tic->from);
                    break;

                case CRC_ORIGIN:
                    parseFtnAddrZS(param, &tic->origin);
                    break;

                case CRC_TO:
                    parseFtnAddrZS(param, &tic->to);

                    if(!tic->to.zone || !tic->to.net)
                    {
                        w_log(LL_ERR,
                              "No zone or net in the 'To' address (%s) in TIC %s",
                              param,
                              ticfile);
                        rc = parseTic_bad;
                    }

                    break;

                case CRC_DESTINATION:

                    if(ticSourceLink && !ticSourceLink->FileFixFSC87Subset)
                    {
                        parseFtnAddrZS(param, &tic->to);
                    }

                    break;

                case CRC_DESC:
                    tic->desc = srealloc(tic->desc, (tic->anzdesc + 1) * sizeof(*tic->desc));
                    tic->desc[tic->anzdesc] = sstrdup(param);
                    tic->anzdesc++;
                    break;

                case CRC_LDESC:
                    tic->ldesc = srealloc(tic->ldesc, (tic->anzldesc + 1) * sizeof(*tic->ldesc));
                    tic->ldesc[tic->anzldesc] = sstrdup(param);
                    tic->anzldesc++;
                    break;

                case CRC_SEENBY:
                    memset(&Aka, 0, sizeof(Aka));

                    if(parseFtnAddrZS(param, &Aka) & FTNADDR_ERROR)
                    {
                        w_log(LL_TIC, "TIC %s: Illegal value: 'Seenby %s', ignored", ticfile,
                              param);
                    }
                    else
                    {
                        seenbyAdd(&tic->seenby, &tic->anzseenby, &Aka);
                    }

                    break;

                case CRC_PATH:
                    tic->path = srealloc(tic->path, (tic->anzpath + 1) * sizeof(*tic->path));
                    tic->path[tic->anzpath] = sstrdup(param);
                    tic->anzpath++;
                    break;

                default:

                    if(ticSourceLink && !ticSourceLink->FileFixFSC87Subset)
                    {
                        w_log(LL_WARNING, "Unknown Keyword %s in Tic File", token);
                        rc = parseTic_bad;
                    }
            }                   /* switch */
        }                       /* endif */

        if(config->MaxTicLineLength)
        {
            nfree(linecut);
        }

        nfree(line);
    }                           /* endwhile */
    fclose(tichandle);

    if(!tic->anzdesc)
    {
        tic->desc    = srealloc(tic->desc, sizeof(*tic->desc));
        tic->desc[0] = sstrdup("no desc");
        tic->anzdesc = 1;
    }

    return rc;
}                               /* parseTic */

/* Save ticket file into filearea-specific directory
 */
void doSaveTic(char * ticfile, s_ticfile * tic, s_area * filearea)
{
    unsigned int i;
    s_savetic * savetic;

    if((tic == NULL) || (filearea == NULL))
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: doSaveTic(%s%s%s,%s,%s). This is a bug, please report it to developers.",
              __LINE__,
              ticfile ? "\"" : "",
              ticfile ? ticfile : "",
              ticfile ? "\"" : "",
              tic ? "tic" : "NULL",
              filearea ? "filearea" : "NULL");
        return;
    }

    for(i = 0; i < config->saveTicCount; i++)
    {
        savetic = &(config->saveTic[i]);

        if(patimat(tic->area, savetic->fileAreaNameMask) == 1)
        {
            char * ticFname = NULL;
            char * filename = NULL;

            if(ticfile)
            {
                ticFname = GetFilenameFromPathname(ticfile);
                w_log(LL_FILENAME, "Saving Tic-File \"%s\" to \"%s\"", ticFname,
                      savetic->pathName);
                xscatprintf(&filename, "%s%s", savetic->pathName, ticFname);

                if(copy_file(ticfile, filename, 1) != 0) /* overwrite existing file if not same
                                                            */
                {
                    w_log(LL_ERROR, "File \"%s\" not found or not moveable", ticfile);
                }
            }
            else
            {
                filename = makeUniqueDosFileName(savetic->pathName, "tic", config);
                w_log(LL_FILENAME, "Saving Tic-File \"%s\" to \"%s\"",
                      GetFilenameFromPathname(filename), savetic->pathName);
                writeTic(filename, tic);
            }

            nfree(filename);

            if(filearea && filearea->msgbType != MSGTYPE_PASSTHROUGH && !filearea->sendorig &&
               savetic->fileAction)
            {
                char * from = NULL, * to = NULL;
                xstrscat(&from, filearea->fileName, tic->file, NULL);
                xstrscat(&to, savetic->pathName, tic->file, NULL);

                if(savetic->fileAction == 1)
                {
                    copy_file(from, to, 1); /* overwrite existing file if not same */
                }

                if(savetic->fileAction == 2)
                {
                    link_file(from, to);
                }

                nfree(from);
                nfree(to);
            }

            break;
        }
    }
    return;
}                               /* doSaveTic */

/*
 *  create alternative filename for ticked file
 *  - add counter 01 up to 99
 *
 *  requires:
 *  - path of destination directory
 *  - name of ticked file
 *
 *  returns:
 *  - string pointer (allocated buffer) on success
 *  - NULL on any problem
 */
char * altFilename(char * path, char * name)
{
    char * altName      = NULL;              /* return value */
    char * tempName     = NULL;              /* temporary name */
    char * tempFilepath = NULL;              /* temporary filepath */
    char * modStr;
    char * remStr  = NULL;
    unsigned int n = 1;
    size_t size;

    /* sanity checks */
    if((path == NULL) || (name == NULL))
    {
        return altName;
    }

    /* allocate buffers */
    size         = strlen(name) + 4; /* + dot + 2 digits + \0 */
    tempName     = smalloc(size);
    size        += strlen(path);
    tempFilepath = smalloc(size);
    /* prepare temp. name */
    strcpy(tempName, name);
    modStr = strchr(tempName, '.'); /* find first dot */

    if(modStr == NULL)              /* no dot found */
    {
        /* find end of string */
        modStr = tempName;

        while(modStr[0] != 0)
        {
            modStr++;
        }
        modStr[0] = '.';           /* add dot */
    }
    else                           /* dot found */
    {
        /* get pointer to remaining part in original string */
        remStr = strchr(name, '.');
    }

    modStr++;                      /* skip dot */
    modStr[0] = 0;                 /* end string */

    if(isDOSLikeName(name))        /* DOS filename */
    {
        if(remStr)                 /* more chars follow */
        {
            remStr++;              /* skip dot */
            modStr[0] = remStr[0]; /* copy first char behind dot */
            modStr++;              /* and skip it */
            remStr = NULL;         /* don't care about the remaining chars */
        }
    }

    /* check alternative names */
    while(n < 99)
    {
        /* complete temp. filename */
        sprintf(modStr, "%.2d", n);          /* add counter */

        if(remStr)
        {
            strcat(modStr, remStr);          /* add remaining part */
        }

        strcpy(tempFilepath, path);          /* build full filepath */
        strcat(tempFilepath, tempName);

        if(!fexist(tempFilepath))
        {
            break;                           /* unused filename */
        }
        else
        {
            n++;                             /* try next one */
        }
    }

    if(n <= 99)
    {
        altName = tempName;                  /* found alternative */
    }
    else
    {
        nfree(tempName);                     /* found none */
    }

    nfree(tempFilepath);
    return altName;
} /* altFilename */

/* Send a file "filename" to all subscribers of fileecho.
 * Return: 0 if success, positive integer if error, negative integer if illegal call
 */
int sendToLinks(int isToss, s_area * filearea, s_ticfile * tic, const char * filename)
/*
 * isToss == 1 - tossing
 * isToss == 0 - hatching
 */
{
    unsigned int i, z;
    char descr_file_name[256], newtickedfile[256], fileareapath[256];
    char timestr[40];
    time_t acttime;
    hs_addr * old_seenby = NULL;
    hs_addr old_from, old_to;
    int old_anzseenby = 0;
    int readAccess;
    int cmdexit;
    char * comm;
    char * p;
    unsigned int minLinkCount;

    if((filearea == NULL) || (tic == NULL) || (filename == NULL))
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: sendToLinks(%i,%s,%s,%s%s%s). This is a bug, please report it to developers.",
              __LINE__,
              isToss,
              filearea ? "filearea" : "NULL",
              tic ? "tic" : "NULL",
              filename ? "\"" : "",
              filename ? filename : "NULL",
              filename ? "\"" : "");
        return TIC_UnknownError;
    }

    if(isToss == 1)
    {
        minLinkCount = 2;       /*  uplink and downlink */
    }
    else
    {
        minLinkCount = 1;       /*  only downlink */
    }

    if(!fexist(filename))
    {
        /* origin file does not exist */
        w_log(LL_ERROR, "File \"%s\" not found, nothing to send to links", filename);
        return TIC_NotRecvd;
    }

    if(filearea->msgbType != MSGTYPE_PASSTHROUGH)
    {
        strcpy(fileareapath, filearea->fileName);
    }
    else
    {
        strcpy(fileareapath, config->passFileAreaDir);
    }

    p = strrchr(fileareapath, PATH_DELIM);

    if(p)
    {
        strLower(p + 1);
    }

    _createDirectoryTree(fileareapath);

    if(isToss == 1 && tic->replaces != NULL && filearea->msgbType != MSGTYPE_PASSTHROUGH &&
       !filearea->noreplace)
    {
        /* Delete old file[s] */
        int num_files;
        char * repl;
        repl = strrchr(tic->replaces, PATH_DELIM);

        if(repl == NULL)
        {
            repl = tic->replaces;
        }
        else
        {
            repl++;
        }

        num_files = removeFileMask(fileareapath, repl);

        if(num_files > 0)
        {
            w_log(LL_DEL, "%d file[s] with filemask %s are removed", num_files, repl);
        }
    }

    strcpy(newtickedfile, fileareapath);
    strcat(newtickedfile, MakeProperCase(tic->file));

    /* check for duplicates in the filearea */
    if(filearea->msgbType != MSGTYPE_PASSTHROUGH && fexist(newtickedfile))
    {
        if(filearea->rename) /* rename new file */
        {
            comm = altFilename(&fileareapath[0], tic->file);

            if(comm != NULL) /* found alternative name */
            {
                strcpy(newtickedfile, fileareapath);
                strcat(newtickedfile, comm);
                tic->altfile = comm;
                w_log('6', "Created alternative name %s for file %s.", comm, tic->file);
            }
            else            /* namespace exceeded or error */
            {
                w_log(LL_ERROR,
                      "Couldn't create alternative name for file %s in filearea %s.",
                      tic->file,
                      tic->area);
                return TIC_NotOpen;
            }
        }
        else if(filearea->noreplace)  /* don't overwrite old file */
        {
            w_log(LL_ERROR,
                  "File %s in filearea %s already exists. Can't replace it",
                  tic->file,
                  tic->area);
            return TIC_NotOpen;
        }
    }

    if(isToss == 1)
    {
        if(!filearea->sendorig && tic->altfile == NULL)
        {
            /* overwrite existing file if not same */
            if(move_file(filename, newtickedfile, 1) != 0)
            {
                w_log(LL_ERROR, "Can't copy file %s to %s: %s", filename, newtickedfile,
                      strerror(errno));
                return TIC_NotOpen;
            }
            else
            {
                w_log(LL_CREAT, "Moved %s to %s", filename, newtickedfile);
            }
        }
        else
        {
            /* overwrite existing file if not same */
            if(copy_file(filename, newtickedfile, 1) != 0)
            {
                w_log(LL_ERROR, "Can't copy file %s to %s: %s", filename, newtickedfile,
                      strerror(errno));
                return TIC_NotOpen;
            }
            else
            {
                w_log(LL_CREAT, "Put %s to %s", filename, newtickedfile);
            }

            strcpy(newtickedfile, config->passFileAreaDir);
            strcat(newtickedfile, MakeProperCase(tic->file));

            /* overwrite existing file if not same */
            if(move_file(filename, newtickedfile, 1) != 0)
            {
                w_log(LL_ERROR, "Can't move file %s  to %s: %s", filename, newtickedfile,
                      strerror(errno));
                return TIC_NotOpen;
            }
            else
            {
                w_log(LL_CREAT, "Moved %s to %s", filename, newtickedfile);
            }
        }
    }
    else if(strcasecmp(filename, newtickedfile) != 0)
    {
        /* overwrite existing file if not same */
        if(copy_file(filename, newtickedfile, 1) != 0)
        {
            w_log(LL_ERROR,
                  "Can't copy file %s to %s: %s",
                  filename,
                  newtickedfile,
                  strerror(errno));
            return TIC_NotOpen;
        }
        else
        {
            w_log(LL_CREAT, "Put %s to %s", filename, newtickedfile);
        }

        if(filearea->sendorig || tic->altfile)
        {
            strcpy(newtickedfile, config->passFileAreaDir);
            strcat(newtickedfile, MakeProperCase(tic->file));

            if(copy_file(filename, newtickedfile, 1) != 0)
            {
                w_log(LL_ERROR,
                      "Can't copy file \"%s\" to \"%s\": %s",
                      filename,
                      newtickedfile,
                      strerror(errno));
                return TIC_NotOpen;
            }
            else
            {
                w_log(LL_CREAT, "Put \"%s\" to \"%s\"", filename, newtickedfile);
            }
        }
    }

    if(tic->anzldesc == 0 && config->fDescNameCount && !filearea->nodiz && isToss)
    {
        GetDescFormDizFile(newtickedfile, tic);
    }

    if(config->announceSpool)
    {
        doSaveTic4Report(tic);
    }

    if(filearea->msgbType != MSGTYPE_PASSTHROUGH)
    {
        strcpy(descr_file_name, filearea->fileName);
        strcat(descr_file_name, config->fileDescription);
        adaptcase(descr_file_name);

        if(tic->altfile)
        {
            comm = tic->altfile;              /* use alternative name */
        }
        else
        {
            comm = tic->file;                 /* use original name */
        }

        removeDesc(descr_file_name, comm);

        if(tic->anzldesc > 0)
        {
            add_description(descr_file_name, comm, tic->ldesc, tic->anzldesc);
        }
        else
        {
            add_description(descr_file_name, comm, tic->desc, tic->anzdesc);
        }
    }

    if(filearea->downlinkCount >= minLinkCount)
    {
        /* Adding path & seenbys */
        time(&acttime);
        strcpy(timestr, asctime(gmtime(&acttime)));
        timestr[strlen(timestr) - 1] = 0;

        if(timestr[8] == ' ')
        {
            timestr[8] = '0';
        }

        tic->path = srealloc(tic->path, (tic->anzpath + 1) * sizeof(*tic->path));
        tic->path[tic->anzpath] = NULL;
        xscatprintf(&tic->path[tic->anzpath], "%s %lu %s UTC %s", aka2str(filearea->useAka),
                    (unsigned long)time(NULL), timestr, versionStr);
        tic->anzpath++;
    }

    if(isToss == 1)
    {
        /*  Save seenby structure */
        old_seenby = smalloc(tic->anzseenby * sizeof(hs_addr));
        memcpy(old_seenby, tic->seenby, tic->anzseenby * sizeof(hs_addr));
        old_anzseenby = tic->anzseenby;
        memcpy(&old_from, &tic->from, sizeof(hs_addr));
        memcpy(&old_to, &tic->to, sizeof(hs_addr));
    }
    else
    {
        memset(&old_from, 0, sizeof(hs_addr));
        memset(&old_to, 0, sizeof(hs_addr));
    }

    if(tic->anzseenby > 0)
    {
        for(i = 0; i < filearea->downlinkCount; i++)
        {
            s_link * downlink = filearea->downlinks[i]->link;

            if((seenbyComp(tic->seenby, tic->anzseenby, downlink->hisAka)) &&
               (readCheck(filearea, downlink) == 0))
            {
                /* if link is not in seen-by list & */
                /* if link can receive files from the filearea */
                /* Adding Downlink to Seen-By */
                /*
                 * tic->seenby=srealloc(tic->seenby,(tic->anzseenby+1)*sizeof(hs_addr));
                 * memcpy(&tic->seenby[tic->anzseenby],&downlink->hisAka,sizeof(hs_addr));
                 * tic->anzseenby++;
                 */
                seenbyAdd(&tic->seenby, &tic->anzseenby, &downlink->hisAka);
            }
        }
    }
    else
    {
        w_log(LL_WARN,
              "Seen-By list is empty in TIC file for \"%s\" (wrong TIC)!",
              tic->file ? tic->file : "");
    }

    /* (dmitry) FixMe: Put correct AKA here if To: is missing in tic */
    if(isOurAka(config, tic->to) && seenbyComp(tic->seenby, tic->anzseenby, tic->to))
    {
        seenbyAdd(&tic->seenby, &tic->anzseenby, &tic->to);
    }
    else
    {
        seenbyAdd(&tic->seenby, &tic->anzseenby, filearea->useAka);
    }

    seenbySort(tic->seenby, tic->anzseenby);

    /* Checking to whom I shall forward */
    for(i = 0; i < filearea->downlinkCount; i++)
    {
        s_link * downlink = filearea->downlinks[i]->link;

        if(addrComp(&old_from, &(downlink->hisAka)) != 0 &&
           addrComp(&old_to, &(downlink->hisAka)) != 0 &&
           addrComp(&(tic->origin), &(downlink->hisAka)) != 0)
        {
            /* Forward file to */
            readAccess = readCheck(filearea, downlink);

            switch(readAccess)
            {
                case 0:
                    break;

                case 5:
                    w_log(LL_FROUTE, "Link %s is paused", aka2str(&downlink->hisAka));
                    break;

                case 4:
                    w_log(LL_FROUTE,
                          "Link %s is not subscribed to File Area %s",
                          aka2str(&old_from),
                          tic->area);
                    break;

                case 3:
                    w_log(LL_FROUTE, "No export (or write only) to link %s",
                          aka2str(&downlink->hisAka));
                    break;

                case 2:
                    w_log(LL_FROUTE,
                          "Link's %s level provides no access",
                          aka2str(&downlink->hisAka));
                    break;

                case 1:
                    w_log(LL_FROUTE,
                          "The group the area %s belongs to is neither in PublicGroup nor in link's %s AccessGrp",
                          filearea->areaName,
                          aka2str(&downlink->hisAka));
                    break;
            }

            if(readAccess == 0)
            {
                if(isToss == 1 && seenbyComp(old_seenby, old_anzseenby, downlink->hisAka) == 0)
                {
                    w_log(LL_FROUTE, "File %s was already seen by %s", tic->file,
                          aka2str(&downlink->hisAka));
                }
                else
                {
                    PutFileOnLink(newtickedfile, tic, downlink);
                }
            }                   /* if readAccess == 0 */
        }                       /* Forward file */
    }

    /* execute external program */
    for(z = 0; z < config->execonfileCount; z++)
    {
        if(stricmp(filearea->areaName, config->execonfile[z].filearea) != 0)
        {
            continue;
        }

        if(patimat(tic->file, config->execonfile[z].filename) == 0)
        {
            continue;
        }
        else
        {
            comm =
                (char *)smalloc(strlen(config->execonfile[z].command) + 1 +
                                (filearea->msgbType !=
                                 MSGTYPE_PASSTHROUGH ? strlen(filearea->fileName) : strlen(
                                     config->passFileAreaDir)) + strlen(tic->file) + 1);

            if(comm == NULL)
            {
                w_log(LL_ERROR, "Exec failed - not enough memory");
                continue;
            }

            sprintf(comm, "%s %s%s", config->execonfile[z].command,
                    (filearea->msgbType !=
                     MSGTYPE_PASSTHROUGH ? filearea->fileName : config->passFileAreaDir),
                    tic->file);
            w_log(LL_EXEC, "Executing \"%s\"", comm);

            if((cmdexit = system(comm)) != 0)
            {
                w_log(LL_ERROR, "Exec failed with exit code %d", cmdexit);
            }

            nfree(comm);
        }
    }

    if(isToss == 1)
    {
        nfree(old_seenby);
    }

    return TIC_OK;
}                               /* sendToLinks */

#if !defined (__UNIX__)
/* FIXME: This code is nonportable and should therefore really be part
          of a porting library like huskylib or !!!
 */

# if defined (__NT__)
/* we can't include windows.h for several reasons ... */
#  define GetFileAttributes GetFileAttributesA
# endif


int hidden(char * filename)
{
# if (defined (__TURBOC__) && !defined (__OS2__)) || defined (__DJGPP__)
    unsigned fattrs;
    _dos_getfileattr(filename, &fattrs);
    return fattrs & _A_HIDDEN;

# elif defined (__NT__)
    unsigned fattrs;
    fattrs = (GetFileAttributes(filename) & 0x2) ? _A_HIDDEN : 0;
    return fattrs & _A_HIDDEN;

# elif defined (__OS2__)
    FILESTATUS3 fstat3;
    const char * p;
    char * q, * backslashified;
    /* Under OS/2 users can also use "forward slashes" in filenames because
     * the OS/2 C libraries support this, but the OS/2 API itself does not
     * support this, and as we are calling an OS/2 API here we must first
     * transform slashes into backslashes */
    backslashified = (char *)smalloc(strlen(filename) + 1);

    for(p = filename, q = backslashified; *p; q++, p++)
    {
        if(*p == '/')
        {
            *q = '\\';
        }
        else
        {
            *q = *p;
        }
    }
    *q = '\0';
    DosQueryPathInfo((PSZ)backslashified, 1, &fstat3, sizeof(fstat3));
    nfree(backslashified);
    return fstat3.attrFile & FILE_HIDDEN;

# else  /* if (defined (__TURBOC__) && !defined (__OS2__)) || defined (__DJGPP__) */
#  error "Don't know how to check for hidden files on this platform"
    return 0;                   /* well, we can't check if we don't know about the host */

# endif /* if (defined (__TURBOC__) && !defined (__OS2__)) || defined (__DJGPP__) */
} /* hidden */

#endif /* if !defined (__UNIX__) */
/*
 * Return: 0 if success (look for TIC_OK in htick/h/toss.h),
 * positive integer if error in TIC or file (see htick/h/toss.h),
 * negative integer if illegal call (look for TIC_UnknownError in htick/h/toss.h).
 */
enum TIC_state processTic(char * ticfile, e_tossSecurity sec)
{
    s_ticfile tic;
    size_t j;
    FILE * flohandle;
    husky_DIR * dir;
    char * file;
    char tickedfile[256], linkfilepath[256];
    char dirname[256], * realfile, * findfile, * pos;
    char * newticfile;
    s_area * filearea;
    s_link * from_link, * to_link;
    int busy;
    struct stat stbuf;
    int writeAccess;
    int fileisfound = 0;
    int rc          = 0;
    char * tic_origin;

    w_log(LL_TIC, "Processing Tic-File %s", ticfile);

    if(ticfile == NULL)
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: processTic(%s,sec). This is a bug, please report it to developers.",
              __LINE__,
              ticfile ? "\"" : "",
              ticfile ? ticfile : "NULL",
              ticfile ? "\"" : "");
        return TIC_UnknownError;
    }

    if((j = strlen(ticfile)) > sizeof(tickedfile))
    {
        w_log(LL_ERROR, "File name too long: %u. Htick limit is %u characters.", j,
              sizeof(tickedfile));
        w_log(LL_TIC, "Ticket file \"%s\" is skipped", ticfile);
        return TIC_UnknownError;
    }

    switch(parseTic(ticfile, &tic))
    {
        case parseTic_error:
            return TIC_NotOpen;

        case parseTic_bad:
            return TIC_WrongTIC;

        case parseTic_success:
            break;
    }
    {
        int r = checkTic(ticfile, &tic);

        if(r > 0)
        {
            return TIC_WrongTIC;
        }

        if(r < 0)
        {
            return TIC_UnknownError;
        }
    }

    if(tic.file && strpbrk(tic.file, "/\\:"))
    {
        w_log(LL_SECURITY,
              "Directory separator found in 'File' token: '%s' of %s TIC file",
              tic.file,
              ticfile);
        return TIC_Security;
    }

    if(tic.replaces && strpbrk(tic.replaces, "/\\:"))
    {
        w_log(LL_SECURITY,
              "Directory separator found in 'Replace' token: '%s' of %s TIC file",
              tic.replaces,
              ticfile);
        return TIC_Security;
    }

    if(tic.size < 0)
    {
        w_log(LL_TIC,
              "File \"%s\": size: not specified, area: %s, from: %s, orig: %s",
              tic.file,
              tic.area,
              aka2str(&tic.from),
              tic_origin = aka2str5d(tic.origin));
    }
    else
    {
        w_log(LL_TIC,
              "File \"%s\": size: %ld, area: %s, from: %s, orig: %s",
              tic.file,
              tic.size,
              tic.area,
              aka2str(&tic.from),
              tic_origin = aka2str5d(tic.origin));
    }

    nfree(tic_origin);

    if(tic.to.zone != 0)
    {
        if(!isOurAka(config, tic.to))
        {
            /* Forwarding tic and file to another link? */
            to_link = getLinkFromAddr(config, tic.to);

            if((to_link != NULL) && (to_link->forwardPkts != fOff)) /*FIXME: should use (new)
                                                                       forwardTICs! */
            /* Send the ticket and a file to the address in line "To" of ticket. ("Transit TICs
               routing") */
            {
                if(!((to_link->forwardPkts == fSecure) && (sec != secProtInbound) &&
                     (sec != secLocalInbound))) /* Forwarding */
                {
                    w_log(LL_TIC,
                          "Forward TIC \"%s\" (and file \"%s\") to %s",
                          ticfile,
                          tic.file,
                          aka2str(&tic.to));
                    busy = 0;

                    if(createOutboundFileNameAka(to_link, to_link->fileEchoFlavour, FLOFILE,
                                                 SelectPackAka(to_link)) == 0)
                    {
                        strcpy(linkfilepath, to_link->floFile);

                        if(!busy) /*  FIXME: it is always not busy!!! */
                        {
                            *(strrchr(linkfilepath, PATH_DELIM)) = 0;
                            newticfile = makeUniqueDosFileName(linkfilepath, "tic", config);

                            if(move_file(ticfile, newticfile, 0) == 0) /* don't overwrite
                                                                          existing file */
                            {
                                strcpy(tickedfile, ticfile);
                                *(strrchr(tickedfile, PATH_DELIM) + 1) = 0;
                                j = strlen(tickedfile);
                                strcat(tickedfile, tic.file);
                                adaptcase(tickedfile);
                                strcpy(tic.file, tickedfile + j);
                                flohandle = fopen(to_link->floFile, "a");
                                fprintf(flohandle, "^%s\n", tickedfile);
                                fprintf(flohandle, "^%s\n", newticfile);
                                fclose(flohandle);

                                if(to_link->bsyFile)
                                {
                                    remove(to_link->bsyFile);
                                    nfree(to_link->bsyFile);
                                }

                                nfree(to_link->floFile);
                            }
                        }
                    }

                    doSaveTic(ticfile, &tic, NULL);
                    disposeTic(&tic);
                    return TIC_OK;
                }
            }

            /* not to us and no forward */
            w_log(LL_SECURITY, "Tic File addressed to %s, not to us", aka2str(&tic.to));
            disposeTic(&tic);
            return TIC_NotForUs;
        }
    }

    /* Security Check */
    from_link = getLinkFromAddr(config, tic.from);

    if(from_link == NULL)
    {
        w_log(LL_SECURITY, "Tic from unknown link \"%s\".", aka2str(&tic.from));
        disposeTic(&tic);
        return TIC_Security;
    }

    if(tic.password &&
       ((from_link->ticPwd == NULL) || (stricmp(tic.password, from_link->ticPwd) != 0)))
    {
        w_log(LL_SECURITY, "Wrong Password \"%s\" from \"%s\"", tic.password, aka2str(&tic.from));
        disposeTic(&tic);
        return TIC_Security;
    }

    /* Extract directory name from ticket pathname (strip file name), construct full name of
       file,
     * adopt full pathname into real disk file, update file name in ticket */
    strcpy(tickedfile, ticfile);
    *(strrchr(tickedfile, PATH_DELIM) + 1) = 0;
    j = strlen(tickedfile);
    strcat(tickedfile, tic.file);
    adaptcase(tickedfile);
    strcpy(tic.file, tickedfile + j);

    /* Receive file? */
    if(!fexist(tickedfile))
    {
        if(from_link->delNotReceivedTIC)
        {
            w_log(LL_TIC,
                  "File %s from filearea %s has not been received, removing its TIC",
                  tic.file,
                  tic.area);
            disposeTic(&tic);
            return TIC_OK;
        }
        else
        {
            w_log(LL_TIC,
                  "File %s from filearea %s has not been received, waiting",
                  tic.file,
                  tic.area);
            disposeTic(&tic);
            return TIC_NotRecvd;
        }
    }

#if !defined (__UNIX__)

    if(hidden(tickedfile))
    {
        w_log(LL_TIC,
              "File %s from filearea %s has not been completely received, waiting",
              tic.file,
              tic.area);
        disposeTic(&tic);
        return TIC_NotRecvd;
    }

#endif

    filearea = getFileArea(tic.area);
    w_log(LL_DEBUGU,
          __FILE__ ":%u:processTic(): filearea %sfound",
          __LINE__,
          filearea ? "" : "not ");

    if(filearea == NULL && from_link->filefix.autoCreate)
    {
        char * descr = NULL;

        if(tic.areadesc)
        {
            descr = sstrdup(tic.areadesc);
        }

        if(config->intab && descr)
        {
            recodeToInternalCharset(descr);
        }

        autoCreate(tic.area, descr, tic.from, NULL);
        filearea = getFileArea(tic.area);
        w_log(LL_DEBUGU,
              __FILE__ ":%u:processTic(): filearea %sfound",
              __LINE__,
              filearea ? "" : "not ");
        nfree(descr);
    }

    if(filearea == NULL)
    {
        if(from_link->filefix.autoCreate)
        {
            w_log(LL_ERROR, "Cannot create File Area %s", tic.area);

            if(!quiet)
            {
                fprintf(stderr, "Cannot create File Area %s !\n", tic.area);
            }
        }
        else
        {
            w_log(LL_ERROR, "Cannot open File Area %s, autocreate not allowed", tic.area);

            if(!quiet)
            {
                fprintf(stderr, "Cannot open File Area %s, autocreate not allowed !\n", tic.area);
            }
        }

        disposeTic(&tic);
        return TIC_NotOpen;
    }

    if(stat(tickedfile, &stbuf))
    {
        w_log(LL_ERR,
              "Can't check size of file \"%s\": %s. Skipping the file.",
              tickedfile,
              strerror(errno));
        stbuf.st_size = tic.size;
        return TIC_NotOpen;
    }

    /* Check CRC Value and reject faulty files depending on noCRC flag */
    if(!filearea->noCRC && tic.crc_is_present)
    {
        unsigned long crc;
        crc = filecrc32(tickedfile);

        if((tic.size >= 0 && (tic.size != stbuf.st_size)) || (tic.crc != crc))
        {
            w_log(LL_TIC,
                  "%s of file '%s' differs with TIC. I try to find " "file with another suffix",
                  (tic.size != stbuf.st_size) ? "Size" : "CRC32",
                  tickedfile);
            strncpy(dirname, tickedfile, sizeof(dirname));
            pos = strrchr(dirname, PATH_DELIM);

            if(pos && (pos - dirname + 4 < sizeof(dirname)))
            {
                *pos     = 0;
                findfile = pos + 1;
                pos      = strrchr(findfile, '.');

                if(pos && (pos - dirname + 2 < sizeof(dirname)))
                {
                    *(++pos) = 0;
                    strcat(findfile, "*");
                    dir = husky_opendir(dirname);

                    if(dir)
                    {
                        while((file = husky_readdir(dir)) != NULL)
                        {
                            if(patimat(file, findfile))
                            {
                                if(stat(file, &stbuf))
                                {
                                    w_log(LL_ERR,
                                          "Can't check size of file \"%s\": %s. Skipping the file.",
                                          file,
                                          strerror(errno));
                                    stbuf.st_mode = 0; /* to indicate empty structure (mode
                                                          can't be zero in POSIX) */
                                    stbuf.st_size = 0;
                                }
                                else
                                {
                                    if(!stbuf.st_size && !tic.size) /* file empty AND size from
                                                                       TIC is 0 */
                                    /* CRC32 of an empty file is equal to zero */
                                    {
                                        if(strlen(dirname) + strlen(file) + 1 >= sizeof(dirname))
                                        {
                                            w_log(LL_ERR,
                                                  "Path too long (max %i bytes): \"%s%c%s\"",
                                                  sizeof(dirname),
                                                  dirname,
                                                  PATH_DELIM,
                                                  file);
                                        }
                                        else
                                        {
                                            fileisfound = 1;
                                            sprintf(dirname + strlen(dirname),
                                                    "%c%s",
                                                    PATH_DELIM,
                                                    file);
                                            break;
                                        }
                                    }
                                    else if((tic.size < 0) || (stbuf.st_size == tic.size))
                                    {
                                        /* size not specified in TIC OR size from TIC eq
                                           filesize */
                                        crc = filecrc32(file);

                                        if(crc == tic.crc)
                                        {
                                            if(strlen(dirname) + strlen(file) + 1 >=
                                               sizeof(dirname))
                                            {
                                                w_log(LL_ERR,
                                                      "Path too long (max %i bytes): \"%s%c%s\"",
                                                      sizeof(dirname),
                                                      dirname,
                                                      PATH_DELIM,
                                                      file);
                                            }
                                            else
                                            {
                                                fileisfound = 1;
                                                sprintf(dirname + strlen(dirname),
                                                        "%c%s",
                                                        PATH_DELIM,
                                                        file);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        husky_closedir(dir);
                    }
                }
            }

            if(fileisfound)
            {
                realfile = sstrdup(dirname);
                *(strrchr(dirname, PATH_DELIM)) = 0;
                findfile = makeUniqueDosFileName(dirname, "tmp", config);

                if(rename(tickedfile, findfile) != 0)
                {
                    w_log(LL_ERROR,
                          "Can't rename file \"%s\" to temporary \"%s\": %s",
                          tickedfile,
                          findfile,
                          strerror(errno));
                    nfree(findfile);
                    nfree(realfile);
                    disposeTic(&tic);
                    return TIC_NotRecvd;
                }

                if(rename(realfile, tickedfile) != 0)
                {
                    w_log(LL_ERROR,
                          "Can't rename file \"%s\" to \"%s\": %s",
                          realfile,
                          tickedfile,
                          strerror(errno));

                    if(rename(findfile, tickedfile) != 0)
                    {
                        w_log(LL_ERROR,
                              "Can't rename a file back from temporary \"%s\" to \"%s\": %s",
                              tickedfile,
                              findfile,
                              strerror(errno));
                    }

                    nfree(findfile);
                    nfree(realfile);
                    disposeTic(&tic);
                    return TIC_CantRename;
                }

                if(rename(findfile, realfile) != 0)
                {
                    remove(findfile);
                }

                nfree(findfile);
                nfree(realfile);
            }
            else
            {
                w_log(LL_TIC,
                      "Wrong CRC for file \"%s\", skipping this file (in tic:%08lx, need:%08lx)",
                      tic.file,
                      tic.crc,
                      crc);
                disposeTic(&tic);
                return TIC_NotRecvd;
            }
        }
        else
        {
            fileisfound = 1;
        }
    }
    else                        /* Search of a file in the size */
    {
        if(stbuf.st_size == tic.size)
        {
            fileisfound = 1;
        }
        else
        {
            /* FIXME: Place here search of a file in his size (in same inbound as a ticket) */
        }
    }

    if(!stbuf.st_mode)
    {
        stat(tickedfile, &stbuf); /* Read the size of a file if function stat() has not been
                                     called earlier */
    }

    tic.size = stbuf.st_size;   /* FIXME: may be wrong result for big file */

    /* do toss zero-length files */
    if(!tic.size)
    {
        w_log(LL_WARNING, "File \"%s\" from filearea %s has zero size", tic.file, tic.area);
    }

    writeAccess = e_writeCheck(config, filearea, getLinkFromAddr(config, tic.from));

    switch(writeAccess)
    {
        case 0:
            break;

        case 4:
            w_log(LL_SECURITY,
                  "Link %s not subscribed to File Area %s",
                  aka2str(&tic.from),
                  tic.area);
            disposeTic(&tic);
            return TIC_WrongTIC;

        case 3:
            w_log(LL_SECURITY, "No import (or read only) from link %s",
                  aka2str(&from_link->hisAka));
            disposeTic(&tic);
            return TIC_WrongTIC;

        case 2:
            w_log(LL_SECURITY, "Link's %s level provides no access", aka2str(&from_link->hisAka));
            disposeTic(&tic);
            return TIC_WrongTIC;

        case 1:
            w_log(LL_SECURITY,
                  "The group the area %s belongs to is neither in PublicGroup nor in link's %s AccessGrp",
                  filearea->areaName,
                  aka2str(&from_link->hisAka));
            disposeTic(&tic);
            return TIC_WrongTIC;
    }
    rc = sendToLinks(1, filearea, &tic, tickedfile);

    if(rc == TIC_OK)            /* TIC_OK = 0 in htick/h/toss.h */
    {
        doSaveTic(ticfile, &tic, filearea);
    }

    disposeTic(&tic);
    return rc;
}                               /* processTic */

/* Toss TIC-files in the specified directory
 */
void processDir(char * directory, e_tossSecurity sec)
{
    husky_DIR * dir;
    char * file;
    char * dummy = NULL;
    int rc;

    if(directory == NULL)
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: processDir(%s%s%s,sec). This is a bug, please report it to developers.",
              __LINE__,
              directory ? "\"" : "",
              directory ? directory : "NULL",
              directory ? "\"" : "");
        return;
    }

    dir = husky_opendir(directory);

    if(dir == NULL)
    {
        return;
    }

    while((file = husky_readdir(dir)) != NULL)
    {
#ifdef DEBUG_HPT
        printf("testing %s\n", file);
#endif

        if(((cmToss == tossRegular) && (patimat(file, "*.TIC") == 1)) ||
           ((cmToss == tossBad) && ((patimat(file, "*.BAD") == 1) ||
                                    (patimat(file, "*.SEC") == 1) ||
                                    (patimat(file, "*.ACS") == 1) ||
                                    (patimat(file, "*.NTU") == 1))))
        {
            xstrscat(&dummy, directory, file, NULL);

#if !defined (__UNIX__)

            if(!hidden(dummy))
            {
#endif
            rc = processTic(dummy, sec);

            if(cmToss == tossRegular)
            {
                switch(rc)
                {
                    case TIC_Security: /* ticpwd problem */
                        changeFileSuffix(config, dummy, "sec", RENAME_FILE);
                        break;

                    case TIC_NotOpen: /* could not open file */
                    case TIC_CantRename:
                    case TIC_IOError:
                        changeFileSuffix(config, dummy, "acs", RENAME_FILE);
                        break;

                    case TIC_WrongTIC: /* not/wrong tic */
                        changeFileSuffix(config, dummy, "bad", RENAME_FILE);
                        break;

                    case TIC_NotForUs: /* not to us */
                        changeFileSuffix(config, dummy, "ntu", RENAME_FILE);
                        break;

                    case TIC_NotRecvd: /* file not recieved */
                        break;

                    case TIC_OK: /* OK */
                        remove(dummy);
                        break;

                    default:     /* Unknown, do nothing */
                        break;
                }                /* switch */
            }
            else if(rc == TIC_OK)
            {
                remove(dummy);
            }

#if !defined (__UNIX__)
        }

#endif
            nfree(dummy);
        }                       /* if */
    }                           /* while */
    husky_closedir(dir);
}                               /* processDir() */

/* Scan directory with postponed files
 */
void checkTmpDir(void)
{
    char tmpdir[256], newtickedfile[256], newticfile[256];
    husky_DIR * dir = NULL;
    char * file     = NULL;
    s_link * link   = NULL;
    s_ticfile tic;
    s_area * filearea = NULL;
    FILE * flohandle  = NULL;
    int error         = 0;

    w_log(LL_TIC, "Checking tmp dir");
    strcpy(tmpdir, config->busyFileDir);
    dir = husky_opendir(tmpdir);

    if(dir == NULL)
    {
        return;
    }

    while((file = husky_readdir(dir)) != NULL)
    {
        size_t file_length = strlen(file);

        if(file_length != 12)
        {
            continue;           /* Check only ticket files: 8 characters followed by ".tic" */
        }

        if(stricmp(file + 8, ".TIC") == 0)
        {
            char * ticfile = (char *)smalloc(strlen(tmpdir) + file_length + 1);

            if(!ticfile)
            {
                continue;
            }

            strcpy(ticfile, tmpdir);
            strcat(ticfile, file);
            memset(&tic, 0, sizeof(tic));

            if(parseTic(ticfile, &tic) != parseTic_success)
            {
                continue;
            }

            if(checkTic(ticfile, &tic))
            {
                continue;
            }

            link = getLinkFromAddr(config, tic.to);

            if(!link)
            {
                continue;
            }

            /*  createFlo doesn't  support ASO!!! */
            /* if (createFlo(link,cvtFlavour2Prio(link->fileEchoFlavour))==0) { */
            if(createOutboundFileNameAka(link, link->fileEchoFlavour, FLOFILE,
                                         SelectPackAka(link)) == 0)
            {
                filearea = getFileArea(tic.area);

                if(filearea != NULL)
                {
                    if(filearea->msgbType != MSGTYPE_PASSTHROUGH && !filearea->sendorig)
                    {
                        strcpy(newtickedfile, filearea->fileName);
                    }
                    else
                    {
                        strcpy(newtickedfile, config->passFileAreaDir);
                    }

                    strcat(newtickedfile, tic.file);
                    adaptcase(newtickedfile);

                    if(!link->noTIC)
                    {
                        if(config->separateBundles)
                        {
                            strcpy(newticfile, link->floFile);
                            sprintf(strrchr(newticfile, '.'), ".sep%c", PATH_DELIM);
                        }
                        else
                        {
                            strcpy(newticfile, config->ticOutbound);
                        }

                        _createDirectoryTree(newticfile);
                        strcat(newticfile, strrchr(ticfile, PATH_DELIM) + 1);

                        if(!fexist(ticfile))
                        {
                            w_log(LL_ERROR, "File %s not found", ticfile);
                        }
                        else if(move_file(ticfile, newticfile, 1) != 0) /* overwrite existing
                                                                           file if not the same
                                                                           */
                        {
                            w_log(LL_ERROR,
                                  "File %s not moveable to %s: %s",
                                  ticfile,
                                  newticfile,
                                  strerror(errno));
                            error = 1;
                        }
                    }
                    else
                    {
                        remove(ticfile);
                    }

                    if(!error)
                    {
                        flohandle = fopen(link->floFile, "a");
                        fprintf(flohandle, "%s\n", newtickedfile);

                        if(!link->noTIC)
                        {
                            fprintf(flohandle, "^%s\n", newticfile);
                        }

                        fclose(flohandle);
                        w_log(LL_TIC, "Forwarding save file \"%s\" for %s", tic.file,
                              aka2str(&link->hisAka));
                    }
                }               /* if filearea */
            }                   /* if createFlo */

            if(link->bsyFile)
            {
                remove(link->bsyFile);
                nfree(link->bsyFile);
            }

            nfree(link->floFile);
            disposeTic(&tic);
            nfree(ticfile);
        }                       /* if ".TIC" */
    }                           /* while */
    husky_closedir(dir);
} /* checkTmpDir */

/*
 * Return: 0 if error, 1 if success
 */
int putMsgInArea(s_area * echo, s_message * msg, int strip, dword forceattr)
{
    char * ctrlBuff, * textWithoutArea;
    byte * textStart;
    size_t textLength = (size_t)msg->textLength;
    HAREA harea     = NULL;
    HMSG hmsg;
    XMSG xmsg;
    char /**slash,*/ * p, * q, * tiny;
    int rc = 0;

    if((echo == NULL) || (msg == NULL))
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: putMsgInArea(%s,%s,%i,%u). " "This is a bug, please report it to developers.",
              __LINE__,
              echo ? "echo" : "NULL",
              msg ? "msg" : "NULL",
              strip,
              (unsigned)forceattr);
        return 0;
    }

    if(echo->msgbType == MSGTYPE_PASSTHROUGH)
    {
        w_log(LL_ERROR, "Can't put a message to passthrough area %s!", echo->areaName);
        return rc;
    }

    if(!msg->netMail)
    {
        msg->destAddr.zone  = echo->useAka->zone;
        msg->destAddr.net   = echo->useAka->net;
        msg->destAddr.node  = echo->useAka->node;
        msg->destAddr.point = echo->useAka->point;
    }

    harea =
        MsgOpenArea((UCHAR *)echo->fileName, MSGAREA_CRIFNEC,
                    (word)(echo->msgbType | (msg->netMail ? 0 : MSGTYPE_ECHO)));

    if(harea != NULL)
    {
        hmsg = MsgOpenMsg(harea, MOPEN_CREATE, 0);

        if(hmsg != NULL)
        {
            textWithoutArea = msg->text;

            if((strip == 1) && (strncmp(msg->text, "AREA:", 5) == 0))
            {
                /*  jump over AREA:xxxxx\r */
                while(*(textWithoutArea) != '\r')
                {
                    textWithoutArea++;
                }
                textWithoutArea++;
                textLength -= (size_t)(textWithoutArea - msg->text);
            }

            if(echo->killSB)
            {
                tiny = strrstr(textWithoutArea, " * Origin:");

                if(tiny == NULL)
                {
                    tiny = textWithoutArea;
                }

                if(NULL != (p = strstr(tiny, "\rSEEN-BY: ")))
                {
                    p[1]       = '\0';
                    textLength = (size_t)(p - textWithoutArea + 1);
                }
            }
            else if(echo->tinySB)
            {
                tiny = strrstr(textWithoutArea, " * Origin:");

                if(tiny == NULL)
                {
                    tiny = textWithoutArea;
                }

                if(NULL != (p = strstr(tiny, "\rSEEN-BY: ")))
                {
                    p++;

                    if(NULL != (q = strstr(p, "\001PATH: ")))
                    {
                        /*  memmove(p,q,strlen(q)+1); */
                        memmove(p, q, textLength - (size_t)(q - textWithoutArea) + 1);
                        textLength -= (size_t)(q - p);
                    }
                    else
                    {
                        p[0]       = '\0';
                        textLength = (size_t)(p - textWithoutArea);
                    }
                }
            }

            {
                byte * bb;      /* Prevent GCC warning "dereferencing type-punned pointer will
                                   break strict-aliasing rules" */
                ctrlBuff  = (char *)CopyToControlBuf((byte *)textWithoutArea, &bb, &textLength);
                textStart = bb;
            }
            /*  textStart is a pointer to the first non-kludge line */
            xmsg = createXMSG(config, msg, NULL, forceattr, NULL);

            if(MsgWriteMsg(hmsg, 0, &xmsg, (byte *)textStart, (dword)textLength, (dword)textLength,
                           (dword)strlen(ctrlBuff), (byte *)ctrlBuff) != 0)
            {
                w_log(LL_ERROR, "Could not write msg in %s!", echo->fileName);
            }
            else
            {
                rc = 1;         /*  normal exit */
            }

            if(MsgCloseMsg(hmsg) != 0)
            {
                w_log(LL_ERROR, "Could not close msg in %s!", echo->fileName);
                rc = 0;
            }

            nfree(ctrlBuff);
        }
        else
        {
            w_log(LL_ERROR, "Could not create new msg in %s!", echo->fileName);
        }

        /* endif */
        MsgCloseArea(harea);
    }
    else
    {
        w_log(LL_ERROR, "Could not open/create EchoArea %s!", echo->fileName);
    }

    /* endif */
    return rc;
} /* putMsgInArea */

/*
 * Return: o if error, 1 of success
 */
int putMsgInBadArea(s_message * msg, hs_addr pktOrigAddr)
{
    char * tmp = NULL, * line = NULL, * textBuff = NULL, * areaName = NULL, * reason = NULL;

    if(msg == NULL)
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: putMsgInBadArea(NULL,pktOrigAddr). This is a bug, please report it to developers.",
              __LINE__);
        return 0;
    }

    /*  get real name area */
    line = strchr(msg->text, '\r');

    if(strncmp(msg->text, "AREA:", 5) == 0)
    {
        *line = 0;
        xstrcat(&areaName, msg->text + 5);
        *line = '\r';
    }

    reason = "report to passthrough area";
    tmp    = msg->text;

    while((line = strchr(tmp, '\r')) != NULL)
    {
        if(*(line + 1) == '\x01')
        {
            tmp = line + 1;
        }
        else
        {
            tmp   = line + 1;
            *line = 0;
            break;
        }
    }
    xstrscat(&textBuff,
             msg->text,
             "\rFROM: ",
             aka2str(&pktOrigAddr),
             "\rREASON: ",
             reason,
             "\r",
             NULL);

    if(areaName)
    {
        xscatprintf(&textBuff, "AREANAME: %s\r\r", areaName);
    }

    xstrcat(&textBuff, tmp);
    nfree(areaName);
    nfree(msg->text);
    msg->text       = textBuff;
    msg->textLength = (hINT32)strlen(msg->text);

    if(putMsgInArea(&(config->badArea), msg, 0, 0))
    {
        return 1;
    }

    return 0;
} /* putMsgInBadArea */

/* Append tearline and origin lines into message text and create message in the
 * specified area.
 * If areaName is NULL pointer or does not exist: netmail will be placed in the first
 * netmail area, echomail will be placed in badarea.
 */
void writeMsgToSysop(s_message * msg, char * areaName, char * origin)
{
    s_area * echo;

    if(msg == NULL)
    {
        w_log(LL_ERROR,
              __FILE__ ":%i: Parameter is a NULL pointer: writeMsgToSysop(NULL,...). This is a bug, please report it to developers.",
              __LINE__);
        return;
    }

    xscatprintf(&(msg->text),
                " \r--- %s\r * Origin: %s (%s)\r",
                (config->tearline) ? config->tearline : "",
                origin ? origin : (config->origin) ? config->origin : config->name,
                aka2str(&msg->origAddr));
    msg->textLength = (hINT32)strlen(msg->text);

    if(msg->netMail == 1)
    {
        writeNetmail(msg, areaName);
    }
    else
    {
        echo = getArea(config, areaName);

        if(echo != &(config->badArea) && echo->msgbType != MSGTYPE_PASSTHROUGH)
        {
            putMsgInArea(echo, msg, 1, MSGLOCAL);
            w_log(LL_POSTING, "Post report message to %s area", echo->areaName);
        }
        else
        {
            w_log(LL_POSTING, "Post report message to %s area", config->badArea.areaName);
            putMsgInBadArea(msg, msg->origAddr);
        }
    }
} /* writeMsgToSysop */

void toss()
{
    w_log(LL_INFO, "Start tossing...");

    if(config->localInbound && config->localInbound[0])
    {
        processDir(config->localInbound, secLocalInbound);
    }

    if(config->protInbound && config->protInbound[0])
    {
        processDir(config->protInbound, secProtInbound);
    }

    if(config->inbound && config->inbound[0])
    {
        processDir(config->inbound, secInbound);
    }
}
