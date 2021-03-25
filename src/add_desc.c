/*****************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 *****************************************************************************
 * This file is part of HTICK, part of the Husky fidosoft project
 * http://husky.physcip.uni-stuttgart.de
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <huskylib/compiler.h>

#ifdef HAS_PROCESS_H
# include <process.h>
#endif

#ifdef HAS_DOS_H
# include <dos.h>
#endif

#if HAS_UNISTD_H
# include <unistd.h>
#endif

#include <huskylib/huskylib.h>

#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <huskylib/recode.h>
#include <huskylib/xstr.h>
#include <huskylib/dirlayer.h>
#include <areafix/areafix.h>    /* for repeat_char() */

#ifdef USE_HPTZIP
# include <hptzip/hptzip.h>
#endif


#include <add_desc.h>
#include <global.h>
#include <toss.h>
#include <filecase.h>
/*  Return:
 * 0 if success
 * 1 if error
 * -1 if ivalid parameter
 */
int add_description(char * descr_file_name,
                    char * file_name,
                    char ** description,
                    unsigned int count_desc)
{
    FILE * descr_file;
    unsigned int i;
    char * desc_line  = NULL;
    char * namefile   = NULL;
    int descOnNewLine = 0;

    if(!descr_file_name || !file_name || !description)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: add_description(%s,%s,%s,count_desc). This is serious error in program, please report to developers.",
              descr_file_name ? "descr_file_name" : "NULL",
              file_name ? "file_name" : "NULL",
              description ? "description" : "NULL");
        return -1;
    }

    for(i = 0; i < count_desc; i++)
    {
        if(!description[i])
        {
            w_log(LL_CRIT,
                  __FILE__ "::add_description(...,description,%i) Array in parameter has NULL element: description[%i]. This is serious error in program, please report to developers.",
                  count_desc,
                  i);
            return -1;
        }
    }
    descr_file = fopen(descr_file_name, "a");

    if(descr_file == NULL)
    {
        return 1;
    }

    namefile = sstrdup(file_name);
    MakeProperCase(namefile);
    xscatprintf(&desc_line, "%-12s", namefile);

    /* fprintf (descr_file, "%-12s", namefile); */
    if(config->addDLC && config->DLCDigits > 0 && config->DLCDigits < 10)
    {
        char dlc[10];
        dlc[0] = ' ';
        dlc[1] = '[';

        for(i = 1; i <= config->DLCDigits; i++)
        {
            dlc[i + 1] = '0';
        }
        dlc[i + 1] = ']';
        dlc[i + 2] = '\x00';
        /* fprintf (descr_file, "%s", dlc); */
        xstrcat(&desc_line, dlc);
    }

    if((strlen(namefile) > 12) && (strlen(desc_line) + strlen(description[0]) > 78))
    {
        fprintf(descr_file, "%s\n", desc_line);
        descOnNewLine = 1;
    }
    else
    {
        fprintf(descr_file, "%s", desc_line);
    }

    for(i = 0; i < count_desc; i++)
    {
        desc_line = sstrdup(description[i]);

        if(config->intab != NULL)
        {
            recodeToInternalCharset(desc_line);
        }

        if(descOnNewLine == 0)
        {
            fprintf(descr_file, " %s\n", desc_line);
            descOnNewLine = 1;
        }
        else
        {
            if(config->fileDescPos == 0)
            {
                config->fileDescPos = 1;
            }

            fprintf(descr_file, "%s%s%s\n", repeat_char(config->fileDescPos - 1, ' '),
                    (config->fileLDescString == NULL) ? " " : config->fileLDescString, desc_line);
        }

        nfree(desc_line);
    }

    if(count_desc == 0)
    {
        fprintf(descr_file, "\n");
    }

    fclose(descr_file);
    nfree(namefile);
    return 0;
} /* add_description */

/*  Return:
 * 0 if success
 * 1 if error
 * -1 if ivalid parameter
 */
int removeDesc(char * descr_file_name, char * file_name)
{
    FILE * f1, * f2;
    char * line, * tmp, * token, * descr_file_name_tmp, * LDescString;
    int flag = 0;

    if(!descr_file_name || !file_name)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: removeDesc(%s,%s). This is serious error in program, please report to developers.",
              descr_file_name ? "descr_file_name" : "NULL",
              file_name ? "file_name" : "NULL");
        return -1;
    }

    line = tmp = token = descr_file_name_tmp = LDescString = NULL;
    f1   = fopen(descr_file_name, "r");

    if(f1 == NULL)
    {
        return 1;
    }

    /* strcpy(descr_file_name_tmp,descr_file_name); */
    /* strcat(descr_file_name_tmp,".tmp"); */
    xstrscat(&descr_file_name_tmp, descr_file_name, ".tmp", NULL);
    f2 = fopen(descr_file_name_tmp, "w");

    if(f2 == NULL)
    {
        fclose(f1);
        nfree(descr_file_name_tmp);
        return 1;
    }

    if(config->fileLDescString == NULL)
    {
        LDescString = sstrdup(">");
    }
    else
    {
        LDescString = sstrdup(config->fileLDescString);
    }

    while((line = readLine(f1)) != NULL)
    {
        if(*line == 0 || *line == 10 || *line == 13)
        {
            continue;
        }

        if(line[strlen(line) - 1] == '\r')
        {
            line[strlen(line) - 1] = 0;
        }

        if(flag && (*line == '\t' || *line == ' ' || *line == *LDescString))
        {
            continue;
        }
        else
        {
            flag = 0;
        }

        tmp   = sstrdup(line);
        token = strtok(tmp, " \t\0");

        if(token != NULL)
        {
            if(stricmp(token, file_name) != 0)
            {
                fputs(line, f2);
                fputs("\n", f2);
            }
            else
            {
                flag = 1;
            }
        }

        nfree(tmp);
        nfree(line);
    }
    nfree(LDescString);
    fclose(f1);
    fclose(f2);
    move_file(descr_file_name_tmp, descr_file_name, 1); /* overwrite old file */
    nfree(descr_file_name_tmp);
    return 0;
} /* removeDesc */

/*  Return:
 * 0 if success
 * 1 if error
 * -1 if ivalid parameter
 */
int announceNewFileecho(char * announcenewfileecho, char * c_area, char * hisaddr)
{
    FILE * ann_file;

    if(!announcenewfileecho || !c_area || !hisaddr)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: announceNewFileecho(%s,%s,%s). This is serious error in program, please report to developers.",
              announcenewfileecho ? "announcenewfileecho" : "NULL",
              c_area ? "c_area" : "NULL",
              hisaddr ? "hisaddr" : "NULL");
        return -1;
    }

    if(!fexist(announcenewfileecho))
    {
        ann_file = fopen(announcenewfileecho, "w");

        if(ann_file == NULL)
        {
            return 1;
        }

        fprintf(ann_file, "Action   Name                                                 By\n");
        fprintf(ann_file,
                "-------------------------------------------------------------------------------\n");
    }
    else
    {
        ann_file = fopen(announcenewfileecho, "a");

        if(ann_file == NULL)
        {
            return 1;
        }
    }

    fprintf(ann_file, "Created  %-52s %s\n", c_area, hisaddr);
    fclose(ann_file);
    return 0;
} /* announceNewFileecho */

/*  Return:
 * 0 if success
 * 1 if error
 * -1 if ivalid parameter
 */
int GetDescFormBbsFile(char * descr_file_name, char * file_name, s_ticfile * tic)
{
    FILE * filehandle;
    char * line = NULL, * tmp = NULL, * token = NULL;
    char * p = token;
    int flag = 0, rc = 1;

    if(!descr_file_name || !file_name || !tic)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: GetDescFormBbsFile(%s,%s,%s). This is serious error in program, please report to developers.",
              descr_file_name ? "descr_file_name" : "NULL",
              file_name ? "file_name" : "NULL",
              tic ? "tic" : "NULL");
        return -1;
    }

    filehandle = fopen(descr_file_name, "r+b");

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

            tic->desc = srealloc(tic->desc, (tic->anzdesc + 1) * sizeof(*tic->desc));
            tic->desc[tic->anzdesc] = sstrdup(token);
            tic->anzdesc++;
            nfree(line);
            continue;
        }
        else
        {
            if(rc == 0)
            {
                nfree(line);
                break;
            }
            else
            {
                flag = 0;
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

        if(stricmp(token, file_name) == 0)
        {
            UINT i;

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

            if(config->addDLC && config->DLCDigits > 0 && config->DLCDigits < 10 &&
               token[0] == '[')
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

            for(i = 0; i < tic->anzdesc; i++)
            {
                nfree(tic->desc[i]);
            }
            tic->anzdesc            = 0;
            tic->desc               = srealloc(tic->desc, (tic->anzdesc + 1) * sizeof(*tic->desc));
            tic->desc[tic->anzdesc] = sstrdup(token);
            tic->anzdesc++;
            flag = 1;
            rc   = 0;
        }

        nfree(line);
        nfree(tmp);
    }
    fclose(filehandle);
    w_log(LL_FILE, "getDesc OK for file: %s", file_name);
    return rc;
} /* GetDescFormBbsFile */

/*  Return:
 * 0 if description is not set
 * 1 if description is set from specified file
 * 3 if error opening file
 * -1 if ivalid parameter
 */
int GetDescFormFile(char * fileName, s_ticfile * tic)
{
    FILE * filehandle = NULL;
    UINT i;
    char * line = NULL;

    if(!fileName || !tic)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: GetDescFormFile(%s,%s). This is serious error in program, please report to developers.",
              fileName ? "fileName" : "NULL",
              tic ? "tic" : "NULL");
        return -1;
    }

    if((filehandle = fopen(fileName, "r")) == NULL)
    {
        w_log(LL_ERROR, "Error at opening file \"%s\": %s", fileName, strerror(errno));
        return 3;
    }

    for(i = 0; i < tic->anzldesc; i++)
    {
        nfree(tic->ldesc[i]);
    }
    tic->anzldesc = 0;

    while((line = readLine(filehandle)) != NULL)
    {
        tic->ldesc = srealloc(tic->ldesc, (tic->anzldesc + 1) * sizeof(*tic->ldesc));
        tic->ldesc[tic->anzldesc] = sstrdup(line);
        tic->anzldesc++;
        nfree(line);
    }                           /* endwhile */

    fclose(filehandle);
    return tic->anzldesc ? 1 : 0;
} /* GetDescFormFile */

/*  Return:
 * 1 if description is set from  description file ("file_id.diz" from archive)
 * 0 if description not set
 * -1 if ivalid parameter
 * 2 if error opening file
 * 3 if unpacker not found for this file or all diz-files don't unpacked
 */
int GetDescFormDizFile(char * fileName, s_ticfile * tic)
{
    FILE * filehandle = NULL;
    char * dizfile = NULL;
    int j, found;
    UINT i, unpacker;
    signed int cmdexit;
    char cmd[256]    = "";
    char buffer[256] = "";

    if(!fileName || !tic)
    {
        w_log(LL_CRIT,
              __FILE__ ":: Parameter is NULL: GetDescFormFile(%s,%s). This is serious error in program, please report to developers.",
              fileName ? "fileName" : "NULL",
              tic ? "tic" : "NULL");
        return -1;
    }

    /*  find what unpacker to use */
    for(i = 0, found = 0; (i < config->unpackCount) && !found; i++)
    {
        filehandle = fopen(fileName, "rb");

        if(filehandle == NULL)
        {
            return 2;
        }

        /*  is offset is negative we look at the end */
        fseek(filehandle,
              config->unpack[i].offset,
              config->unpack[i].offset >= 0 ? SEEK_SET : SEEK_END);

        if(ferror(filehandle))
        {
            fclose(filehandle);
            continue;
        }

        for(found = 1, j = 0; j < config->unpack[i].codeSize; j++)
        {
            if((getc(filehandle) & config->unpack[i].mask[j]) != config->unpack[i].matchCode[j])
            {
                found = 0;
            }
        }
        fclose(filehandle);
    }

    if(found == 0)
    {
        w_log(LL_ALERT, "file %s: cannot find unpacker", fileName);
        return 3;
    }

    unpacker = i - 1;
    getcwd(buffer, 256);

    /*  unpack file_id.diz (config->fileDescName) */
    for(i = 0; i < config->fDescNameCount; i++)
    {
        fillCmdStatement(cmd,
                         config->unpack[unpacker].call,
                         fileName,
                         config->fileDescNames[i],
                         config->tempInbound);
        w_log(LL_EXEC, "File %s: unpacking with \"%s\"", fileName, cmd);
        chdir(config->tempInbound);

        if(fc_stristr(config->unpack[unpacker].call, ZIPINTERNAL))
        {
#ifdef USE_HPTZIP
            cmdexit = UnPackWithZlib(fileName, config->fileDescNames[i], config->tempInbound);
#else
            cmdexit = 0;
            w_log(LL_ERROR,
                  "\"%s\" don't implemented in this build (disabled at compile time). Please use your unzip program.",
                  ZIPINTERNAL);
            continue;
#endif
        }
        else
        {
            cmdexit = cmdcall(cmd);
        }

        chdir(buffer);

        if(cmdexit != 0)
        {
            w_log(LL_ERROR, "exec failed, code %d", cmdexit);
            continue;
        }

        xscatprintf(&dizfile, "%s%s", config->tempInbound, config->fileDescNames[i]);

        if(fexist(dizfile))
        {
            found = GetDescFormFile(dizfile, tic);
        }

        if(found == 1)
        {
            remove(dizfile);
            i = config->fDescNameCount;
        }

        nfree(dizfile);
    }
    return found;
} /* GetDescFormDizFile */
