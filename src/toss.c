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
#include <smapi/compiler.h>

#ifdef __OS2__
#define INCL_DOSFILEMGR /* for hidden() routine */
#include <os2.h>
#endif

#ifdef HAS_IO_H
#  include <io.h>
#endif

#ifdef HAS_SHARE_H
#  include <share.h>
#endif

#ifdef HAS_UNISTD_H
#  include <unistd.h>
#endif

#ifdef HAS_DOS_H
#  include <dos.h>
#endif

#ifdef HAS_PROCESS_H
#  include <process.h>
#endif

/* smapi */
#include <smapi/progprot.h>

/* fidoconf */
#include <fidoconf/fidoconf.h>
#include <fidoconf/adcase.h>
#include <fidoconf/common.h>
#include <fidoconf/dirlayer.h>
#include <fidoconf/xstr.h>
#include <fidoconf/afixcmd.h>
#include <fidoconf/recode.h>
#include <fidoconf/crc.h>

#if defined(A_HIDDEN) && !defined(_A_HIDDEN)
#  define _A_HIDDEN A_HIDDEN
#endif

/* htick */
#include <fcommon.h>
#include <global.h>
#include <toss.h>
#include <areafix.h>
#include <version.h>
#include <add_desc.h>
#include <seenby.h>
#include <filecase.h>
#include "hatch.h"
#include "report.h"

/* tic keywords calculated crc */
#define CRC_CREATED     0x4EC6
#define CRC_FILE        0x1A66
#define CRC_AREADESC    0xB621
#define CRC_DESC        0xECBD
#define CRC_AREA        0x825A
#define CRC_CRC         0x5487
#define CRC_REPLACES    0xCF01
#define CRC_ORIGIN      0xFCDC
#define CRC_FROM        0x1293
#define CRC_TO          0x7B50
#define CRC_PATH        0x5411
#define CRC_SEENBY      0xC3D4
#define CRC_PW          0x24AD
#define CRC_SIZE        0x5593
#define CRC_DATE        0xD4BD
#define CRC_DESTINATION 0xA9D1
#define CRC_MAGIC       0xD858
#define CRC_LDESC       0x5394


/* processTic(), sendToLinks() results */
#define TIC_OK         0
#define TIC_security   1
#define TIC_NotOpen    2
#define TIC_WrongTIC   3
#define TIC_CantRename 3
#define TIC_NotForUs   4
#define TIC_NotRecvd   5


void writeNetmail(s_message *msg, char *areaName)
{
   HAREA  netmail;
   HMSG   msgHandle;
   UINT   len = msg->textLength;
   char   *bodyStart;             /* msg-body without kludgelines start */
   char   *ctrlBuf;               /* Kludgelines */
   XMSG   msgHeader;
   s_area *nmarea;

   if (!config->netMailAreas) {
     w_log(LL_CRIT, "Netmailarea not defined, message dropped!");
     return;
   }
   if ((nmarea=getNetMailArea(config, areaName))==NULL) nmarea = &(config->netMailAreas[0]);

   netmail = MsgOpenArea((UCHAR *) nmarea->fileName, MSGAREA_CRIFNEC, (word)nmarea->msgbType);

   if (netmail != NULL) {
      msgHandle = MsgOpenMsg(netmail, MOPEN_CREATE, 0);

      if (msgHandle != NULL) {
         msgHeader = createXMSG(config,msg,NULL,MSGLOCAL,NULL);
         /* Create CtrlBuf for SMAPI */
         ctrlBuf = (char *) CopyToControlBuf((UCHAR *) msg->text, (UCHAR **) &bodyStart, &len);
         /* write message */
         MsgWriteMsg(msgHandle, 0, &msgHeader, (UCHAR *) bodyStart, len, len, strlen(ctrlBuf)+1, (UCHAR *) ctrlBuf);
         free(ctrlBuf);
         MsgCloseMsg(msgHandle);

         w_log( LL_POSTING , "Wrote Netmail to: %u:%u/%u.%u",
                 msg->destAddr.zone, msg->destAddr.net, msg->destAddr.node, msg->destAddr.point);
      } else {
         w_log( LL_ERROR, "Could not write message to %s", areaName);
      } /* endif */

      MsgCloseArea(netmail);
   } else {
/*     printf("%u\n", msgapierr); */
      w_log( LL_ERROR, "Could not open netmailarea %s", areaName);
   } /* endif */
}

void writeTic(char *ticfile,s_ticfile *tic)
{
    FILE *tichandle;
    unsigned int i;
    s_area *filearea = NULL;

    tichandle=fopen(ticfile,"wb");

    fprintf(tichandle,"Created by HTick, written by Gabriel Plutzar\r\n");
    fprintf(tichandle,"File %s\r\n",tic->file);
    fprintf(tichandle,"Area %s\r\n",tic->area);

    filearea=getFileArea(tic->area);

    if(!tic->areadesc && filearea && filearea->description)
    {
        tic->areadesc = sstrdup(filearea->description);
        if (config->outtab) {
            recodeToTransportCharset(tic->areadesc);
        }
    }
    if (tic->areadesc)
        fprintf(tichandle,"Areadesc %s\r\n",tic->areadesc);

    for (i=0;i<tic->anzdesc;i++)
        fprintf(tichandle,"Desc %s\r\n",tic->desc[i]);

    for (i=0;i<tic->anzldesc;i++)
        fprintf(tichandle,"LDesc %s\r\n",tic->ldesc[i]);

    if (tic->replaces)
        fprintf(tichandle,"Replaces %s\r\n",tic->replaces);
    if (tic->from.zone!=0)
        fprintf(tichandle,"From %s\r\n",aka2str(tic->from));
    if (tic->to.zone!=0)
        fprintf(tichandle,"To %s\r\n",aka2str(tic->to));
    if (tic->origin.zone!=0)
        fprintf(tichandle,"Origin %s\r\n",aka2str(tic->origin));
    if (tic->size!=0)
        fprintf(tichandle,"Size %u\r\n",tic->size);
    if (tic->date!=0)
        fprintf(tichandle,"Date %lu\r\n",tic->date);
    if (tic->crc!=0)
        fprintf(tichandle,"Crc %08lX\r\n",tic->crc);

    for (i=0;i<tic->anzpath;i++)
        fprintf(tichandle,"Path %s\r\n",tic->path[i]);

    for (i=0;i<tic->anzseenby;i++)
        fprintf(tichandle,"Seenby %s\r\n",aka2str(tic->seenby[i]));

    if (tic->password)
        fprintf(tichandle,"Pw %s\r\n",tic->password);

    fclose(tichandle);
}

void disposeTic(s_ticfile *tic)
{
   unsigned int i;

   nfree(tic->seenby);
   nfree(tic->file);
   nfree(tic->area);
   nfree(tic->areadesc);
   nfree(tic->replaces);
   nfree(tic->password);

   for (i=0;i<tic->anzpath;i++)
      nfree(tic->path[i]);
   nfree(tic->path);

   for (i=0;i<tic->anzdesc;i++)
      nfree(tic->desc[i]);
   nfree(tic->desc);

   for (i=0;i<tic->anzldesc;i++)
      nfree(tic->ldesc[i]);
   nfree(tic->ldesc);
}

/* Check TIC file validity
 */
int checkTic(const char *ticfilename,const s_ticfile *tic)
{
  int nRet=0;

  if(!tic->file){
    w_log(LL_ERR,"File name not presents in TIC %s", ticfilename);
    nRet++;
  }

  if(!tic->area){
    w_log(LL_ERR,"Filearea name not presents in TIC %s", ticfilename);
    nRet++;
  }

  if(!tic->crc){
    w_log(LL_SECURITY,"CRC not presents in TIC %s", ticfilename);
  }

  if(!tic->size){
    w_log(LL_ALERT,"File size not presents in TIC %s", ticfilename);
  }

  if(!tic->to.zone && !tic->to.net && !tic->to.node){
    w_log(LL_ERR,"'To' address not presents in TIC %s", ticfilename);
    nRet++;
  }else if(!tic->to.zone || !tic->to.net){
    w_log(LL_ERR,"'To' address is illegal in TIC %s", ticfilename);
    nRet++;
  }

  if(!tic->from.zone && !tic->from.net && !tic->from.node){
    w_log(LL_ERR,"'From' address not presents in TIC %s", ticfilename);
    nRet++;
  }else if(!tic->from.zone || !tic->from.node){
    w_log(LL_ERR,"'From' address is illegal in TIC %s", ticfilename);
    nRet++;
  }

  if(!tic->origin.zone && !tic->origin.net && !tic->origin.node){
    w_log(LL_ERR,"Originating address not presents in TIC %s", ticfilename);
    nRet++;
  }else if(!tic->origin.zone || !tic->origin.node){
    w_log(LL_ERR,"Originating address is illegal in TIC %s", ticfilename);
    nRet++;
  }

  if(!tic->path){
    w_log(LL_ALERT,"PATH line(s) not presents in TIC %s", ticfilename);
  }

  if(!tic->seenby){
    w_log(LL_ALERT,"SEENBY line(s) not presents in TIC %s", ticfilename);
  }

  if(!tic->password){  /* Mandatory field (FSC-87) */
    w_log(LL_SECURITY,"PW not presents in TIC %s", ticfilename);
  }

  return nRet;
}

/* Read tic-file and store values into 2nd parameter.
 * Return 1 if success and 0 if error
 */
int parseTic(char *ticfile,s_ticfile *tic)
{
    FILE *tichandle;
    char *line, *token, *param, *linecut = "", *emptyline = "";
    s_link *ticSourceLink=NULL;
    UINT16 key;
    hs_addr Aka;

    memset(tic,'\0',sizeof(s_ticfile));

#ifndef HAS_sopen
    tichandle=fopen(ticfile,"r");
#else
    {
        int fh = 0;
        /* insure that ticfile won't be removed while parsing */
        fh = sopen( ticfile, O_RDWR | O_BINARY, SH_DENYWR);
        if( fh<0 ){
            w_log(LL_ERROR, "Can't open '%s': %s (sopen())", ticfile, strerror(errno));
            return 0;
        }
        tichandle = fdopen(fh,"r");
    }
#endif
    
    if(!tichandle){
        w_log(LL_ERROR, "Can't open '%s': %s", ticfile, strerror(errno));
        return 0;
    }
    
    while ((line = readLine(tichandle)) != NULL) {
        line = trimLine(line);
        
        if (*line==0 || *line==10 || *line==13 || *line==';' || *line=='#') 
            continue;

        if (config->MaxTicLineLength) {
            linecut = (char *) smalloc(config->MaxTicLineLength+1);
            strncpy(linecut,line,config->MaxTicLineLength);
            linecut[config->MaxTicLineLength] = 0;
            token=strtok(linecut, " \t");
        } else token=strtok(line, " \t");

        if (token) {
            key = strcrc16(strUpper(token), 0);
            /* calculate crc16 of tic                                   */
            /* w_log('1', "#define CRC_%s 0x%X;",strUpper(token),key);*/
            param=stripLeadingChars(strtok(NULL, "\0"), "\t");
            if(!param)
            {
                if( key == CRC_DESC || key == CRC_LDESC )
                    param = emptyline;
                else
                    continue;
            }
            switch (key)
            {
            case CRC_CREATED:
            case CRC_MAGIC:
                break;
            case CRC_FILE:      tic->file = sstrdup(param);
                break;
            case CRC_AREADESC:  tic->areadesc = sstrdup(param);
                break;
            case CRC_AREA:      tic->area = sstrdup(param);
                break;
            case CRC_CRC:       tic->crc = strtoul(param,NULL,16);
                break;
            case CRC_SIZE:      tic->size = tic->size=atoi(param);
                break;
            case CRC_DATE:      tic->date=atoi(param);
                break;
            case CRC_REPLACES:  tic->replaces = sstrdup(param);
                break;
            case CRC_PW:        tic->password = sstrdup(param);
                break;
            case CRC_FROM:      string2addr(param,&tic->from);
                ticSourceLink = getLinkFromAddr(config, tic->from);
                break;
            case CRC_ORIGIN:    string2addr(param,&tic->origin);
                break;
            case CRC_TO:        string2addr(param,&tic->to);
                break;
            case CRC_DESTINATION:
                if(ticSourceLink && !ticSourceLink->FileFixFSC87Subset)
                    string2addr(param,&tic->to);
                break;
            case CRC_DESC:
                tic->desc=
                    srealloc(tic->desc,(tic->anzdesc+1)*sizeof(*tic->desc));
                tic->desc[tic->anzdesc]=sstrdup(param);
                tic->anzdesc++;
                break;
            case CRC_LDESC:
                tic->ldesc=
                    srealloc(tic->ldesc,(tic->anzldesc+1)*sizeof(*tic->ldesc));
                tic->ldesc[tic->anzldesc]=sstrdup(param);
                tic->anzldesc++;
                break;
            case CRC_SEENBY:
                string2addr(param,&Aka);
                seenbyAdd ( &tic->seenby, &tic->anzseenby, &Aka);
                break;
            case CRC_PATH:
                tic->path=
                    srealloc(tic->path,(tic->anzpath+1)*sizeof(*tic->path));
                tic->path[tic->anzpath]=sstrdup(param);
                tic->anzpath++;
                break;
            default:
                if (ticSourceLink && !ticSourceLink->FileFixFSC87Subset)
                    w_log( '7', "Unknown Keyword %s in Tic File",token);
            } /* switch */
        } /* endif */
        if (config->MaxTicLineLength) nfree(linecut);
        nfree(line);
    } /* endwhile */
    
    fclose(tichandle);

    if (!tic->anzdesc) {
        tic->desc = srealloc(tic->desc,sizeof(*tic->desc));
        tic->desc[0] = sstrdup("no desc");
        tic->anzdesc = 1;
    }

    return 1;
}


void doSaveTic(char *ticfile,s_ticfile *tic, s_area *filearea)
{
    char *filename = NULL;
    unsigned int i;
    s_savetic *savetic;

    for (i = 0; i< config->saveTicCount; i++)
    {
        savetic = &(config->saveTic[i]);
        if (patimat(tic->area,savetic->fileAreaNameMask)==1) {

            char *ticFname = GetFilenameFromPathname(ticfile);

            w_log(LL_FILENAME,"Saving Tic-File %s to %s", ticFname, savetic->pathName);
            xscatprintf(&filename,"%s%s", savetic->pathName,ticFname);
            if (copy_file(ticfile,filename,1)!=0) { /* overwrite existing file if not same */
                w_log(LL_ERROR,"File %s not found or not moveable",ticfile);
            };
            if( filearea &&
               filearea->msgbType != MSGTYPE_PASSTHROUGH && !filearea->sendorig && savetic->fileAction)
            {
                char *from = NULL, *to = NULL;
                xstrscat(&from, filearea->fileName, tic->file, NULL);
                xstrscat(&to,   savetic->pathName, tic->file, NULL);
                if( savetic->fileAction == 1)
                   copy_file(from,to,1); /* overwrite existing file if not same */
                if( savetic->fileAction == 2)
                   link_file(from,to);
                nfree(from); nfree(to);
            }
            break;
        };

    };
    nfree(filename);
    return;
}

int sendToLinks(int isToss, s_area *filearea, s_ticfile *tic,
                const char *filename)
                /*
                isToss == 1 - tossing
                isToss == 0 - hatching
                */
{
    unsigned int i, z;
    char descr_file_name[256], newticedfile[256], fileareapath[256];
    char timestr[40];
    time_t acttime;
    hs_addr *old_seenby = NULL;
    hs_addr old_from, old_to;
    int old_anzseenby = 0;
    int readAccess;
    int cmdexit;
    char *comm;
    char *p;
    unsigned int minLinkCount;
    
    if (isToss == 1) minLinkCount = 2; /*  uplink and downlink */
    else minLinkCount = 1;             /*  only downlink */

    if( !fexist(filename) ) {
        /* origin file does not exist */
        w_log(LL_ERROR,"File %s not found",filename);
        return TIC_NotRecvd;
    }
    
    if (filearea->msgbType != MSGTYPE_PASSTHROUGH)
        strcpy(fileareapath,filearea->fileName);
    else
        strcpy(fileareapath,config->passFileAreaDir);
    p = strrchr(fileareapath,PATH_DELIM);
    if(p) strLower(p+1);
    
    _createDirectoryTree(fileareapath);
    
    if (isToss == 1 && tic->replaces!=NULL && filearea->msgbType != MSGTYPE_PASSTHROUGH && !filearea->noreplace) {
        /* Delete old file[s] */
        int num_files;
        char *repl;
        
        repl = strrchr(tic->replaces,PATH_DELIM);
        if (repl==NULL) repl = tic->replaces;
        else repl++;
        num_files = removeFileMask(fileareapath,repl);
        if (num_files>0) {
            w_log(LL_DEL,"Removed %d file[s]. Filemask: %s",num_files,repl);
        }
    }
    
    
    strcpy(newticedfile,fileareapath);
    strcat(newticedfile,MakeProperCase(tic->file));
    
    if(filearea->msgbType != MSGTYPE_PASSTHROUGH && filearea->noreplace && fexist(newticedfile)) {
        w_log(LL_ERROR,"File %s already exist in filearea %s. Can't replace it",tic->file,tic->area);
        return(3);
    }
    
    if (isToss == 1) {
        if (!filearea->sendorig) {
            /* overwrite existing file if not same */
            if (move_file(filename,newticedfile,1)!=0) { 
                w_log( LL_ERROR,"File %s not moveable to %s: %s",
                    filename, newticedfile, strerror(errno) );
                return TIC_NotOpen;
            } else {
                w_log('6',"Moved %s to %s",filename,newticedfile);
            }
        } else {
            /* overwrite existing file if not same */
            if (copy_file(filename,newticedfile,1)!=0) { 
                w_log( LL_ERROR,"File %s not moveable to %s: %s",
                    filename, newticedfile, strerror(errno) );
                return TIC_NotOpen;
            } else {
                w_log('6',"Put %s to %s",filename,newticedfile);
            }
            strcpy(newticedfile,config->passFileAreaDir);
            strcat(newticedfile,MakeProperCase(tic->file));
            /* overwrite existing file if not same */
            if (move_file(filename,newticedfile,1)!=0) { 
                w_log( LL_ERROR, "File %s not moveable to %s: %s",
                    filename, newticedfile, strerror(errno) );
                return TIC_NotOpen;
            } else {
                w_log('6',"Moved %s to %s",filename,newticedfile);
            }
        }
    } else if (strcasecmp(filename,newticedfile) != 0) {
            /* overwrite existing file if not same */
        if (copy_file(filename,newticedfile,1)!=0) { 
            w_log( LL_ERROR, "File %s not moveable to %s: %s",
                filename, newticedfile, strerror(errno) );
            return TIC_NotOpen;
        } else {
            w_log('6',"Put %s to %s",filename,newticedfile);
        }
        if (filearea->sendorig) {
            strcpy(newticedfile,config->passFileAreaDir);
            strcat(newticedfile,MakeProperCase(tic->file));
			if (copy_file(filename,newticedfile,1)!=0) { 
				w_log( LL_ERROR, "File %s not moveable to %s: %s",
					filename, newticedfile, strerror(errno) );
				return TIC_NotOpen;
			} else {
				w_log('6',"Put %s to %s",filename,newticedfile);
			}
        }
    }
    
    if (tic->anzldesc==0 && config->fDescNameCount && !filearea->nodiz && isToss)
        GetDescFormDizFile(newticedfile, tic);
    
    
    if (config->announceSpool) doSaveTic4Report(tic);
    
    
    if (filearea->msgbType != MSGTYPE_PASSTHROUGH) {
        strcpy(descr_file_name, filearea->fileName);
        strcat(descr_file_name, "files.bbs");
        adaptcase(descr_file_name);
        removeDesc(descr_file_name,tic->file);
        if (tic->anzldesc>0)
            add_description (descr_file_name, tic->file, tic->ldesc, tic->anzldesc);
        else
            add_description (descr_file_name, tic->file, tic->desc, tic->anzdesc);
    }
    
    if (filearea->downlinkCount>=minLinkCount) {
        /* Adding path & seenbys */
        time(&acttime);
        strcpy(timestr,asctime(gmtime(&acttime)));
        timestr[strlen(timestr)-1]=0;
        if (timestr[8]==' ') timestr[8]='0';
        
        
        tic->path=srealloc(tic->path,(tic->anzpath+1)*sizeof(*tic->path));
        tic->path[tic->anzpath] = NULL;
        xscatprintf(&tic->path[tic->anzpath],"%s %lu %s UTC %s",
            aka2str(*filearea->useAka), (unsigned long) time(NULL), timestr,versionStr);
        tic->anzpath++;
    }
    
    if (isToss == 1) {
        /*  Save seenby structure */
        old_seenby = smalloc(tic->anzseenby*sizeof(hs_addr));
        memcpy(old_seenby,tic->seenby,tic->anzseenby*sizeof(hs_addr));
        old_anzseenby = tic->anzseenby;
        memcpy(&old_from,&tic->from,sizeof(hs_addr));
        memcpy(&old_to,&tic->to,sizeof(hs_addr));
    }
    
    for (i=0;i<filearea->downlinkCount;i++) {
        s_link* downlink = filearea->downlinks[i]->link;
        if ( (seenbyComp (tic->seenby, tic->anzseenby,downlink->hisAka) ) &&
            (e_readCheck(config, filearea, downlink) == 0)
            )
        {  /*  if link is not in seen-by list & */
            /*  if link can recive files from filearea */
            /*  Adding Downlink to Seen-By */
            /*
            tic->seenby=srealloc(tic->seenby,(tic->anzseenby+1)*sizeof(hs_addr));
            memcpy(&tic->seenby[tic->anzseenby],&downlink->hisAka,sizeof(hs_addr));
            tic->anzseenby++;
            */
            seenbyAdd(&tic->seenby,&tic->anzseenby,&downlink->hisAka);
        }
    }
    
    seenbySort(tic->seenby,tic->anzseenby);
    
    /* Checking to whom I shall forward */
    for (i=0;i<filearea->downlinkCount;i++) {
        s_link* downlink = filearea->downlinks[i]->link;
        if (  addrComp(old_from,downlink->hisAka)   !=0 &&
            addrComp(old_to,downlink->hisAka)     !=0 &&
            addrComp(tic->origin,downlink->hisAka)!=0)
        {
            /* Forward file to */
            
            readAccess = e_readCheck(config, filearea, downlink);
            switch (readAccess) {
            case 0: break;
            case 5:
                w_log(LL_FROUTE,"Link %s paused",
                    aka2str(downlink->hisAka));
                break;
            case 4:
                w_log(LL_FROUTE,"Link %s not subscribe to File Area %s",
                    aka2str(old_from), tic->area);
                break;
            case 3:
                w_log(LL_FROUTE,"Not export to link %s",
                    aka2str(downlink->hisAka));
                break;
            case 2:
                w_log(LL_FROUTE,"Link %s no access level",
                    aka2str(downlink->hisAka));
                break;
            case 1:
                w_log(LL_FROUTE,"Link %s no access group",
                    aka2str(downlink->hisAka));
                break;
            }
            
            if (readAccess == 0) {
                if (isToss == 1 && seenbyComp(old_seenby, old_anzseenby, downlink->hisAka) == 0)
                {
                    w_log(LL_FROUTE,"File %s already seenby %s",
                        tic->file,
                        aka2str(downlink->hisAka));
                } else {
                    PutFileOnLink(newticedfile, tic,  downlink);
                }
            } /* if readAccess == 0 */
        } /* Forward file */
    }
    /* execute external program */
    for (z = 0; z < config->execonfileCount; z++) {
        if (stricmp(filearea->areaName,config->execonfile[z].filearea) != 0) continue;
        if (patimat(tic->file,config->execonfile[z].filename) == 0) continue;
        else {
            comm = (char *) smalloc(strlen(config->execonfile[z].command)+1
                +(filearea->msgbType != MSGTYPE_PASSTHROUGH ? strlen(filearea->fileName) : strlen(config->passFileAreaDir))
                +strlen(tic->file)+1);
            if (comm == NULL) {
                w_log( LL_ERROR, "Exec failed - not enough memory");
                continue;
            }
            sprintf(comm,"%s %s%s",config->execonfile[z].command,
                (filearea->msgbType != MSGTYPE_PASSTHROUGH ? filearea->fileName : config->passFileAreaDir),
                tic->file);
            w_log( LL_EXEC, "Executing %s", comm);
            if ((cmdexit = system(comm)) != 0) {
                w_log( LL_ERROR, "Exec failed, code %d", cmdexit);
            }
            nfree(comm);
        }
    }
    
    if (isToss == 1) nfree(old_seenby);
    return(0);
}

#if !defined(__UNIX__)

/* FIXME: This code is nonportable and should therefore really be part
          of a porting library like huskylib or !!!
*/

#if defined(__NT__)
/* we can't include windows.h for several reasons ... */
#define GetFileAttributes GetFileAttributesA
#endif


int hidden (char *filename)
{
#if (defined(__TURBOC__) && !defined(__OS2__)) || defined(__DJGPP__)
   unsigned fattrs;
   _dos_getfileattr(filename, &fattrs);
   return fattrs & _A_HIDDEN;
#elif defined(__NT__)
   unsigned fattrs;
   fattrs = (GetFileAttributes(filename) & 0x2) ? _A_HIDDEN : 0;
   return fattrs & _A_HIDDEN;
#elif defined (__OS2__)
   FILESTATUS3 fstat3;
   const char *p;
   char *q,*backslashified;

   /* Under OS/2 users can also use "forward slashes" in filenames because
      the OS/2 C libraries support this, but the OS/2 API itself does not
      support this, and as we are calling an OS/2 API here we must first
      transform slashes into backslashes */

   backslashified=(char *)smalloc(strlen(filename)+1);
   for (p=filename,q=backslashified;*p;q++,p++)
   {
       if (*p=='/')
           *q='\\';
       else
           *q=*p;
   }
   *q='\0';

   DosQueryPathInfo((PSZ)backslashified,1,&fstat3,sizeof(fstat3));

   free(backslashified);

   return fstat3.attrFile & FILE_HIDDEN;
#else
#error "Don't know how to check for hidden files on this platform"
   return 0; /* well, we can't check if we don't know about the host */
#endif
}
#endif


int processTic(char *ticfile, e_tossSecurity sec)
{
   s_ticfile tic;
   size_t j;
   FILE   *flohandle;
   DIR    *dir;
   struct dirent  *file;

   char ticedfile[256], linkfilepath[256];
   char dirname[256], *realfile, *findfile, *pos;
   char *newticfile;
   s_area *filearea;
   s_link *from_link, *to_link;
   int busy;
   unsigned long crc;
   struct stat stbuf;
   int writeAccess;
   int fileisfound = 0;
   int rc = 0;
   char *tic_origin;

   w_log('6',"Processing Tic-File %s",ticfile);

   if(!parseTic(ticfile,&tic))
     return TIC_NotOpen;

   if( checkTic(ticfile,&tic) ) return TIC_WrongTIC;

   if ( tic.file && strpbrk(tic.file, "/\\:") )
   {
       w_log( LL_ALERT, "Directory separator found in 'File' token: '%s' of %s TIC file",tic.file,ticfile);
       return TIC_security;
   }
   if ( tic.replaces && strpbrk(tic.replaces, "/\\:") )
   {
       w_log( LL_ALERT, "Directory separator found in 'Replace' token: '%s' of %s TIC file",tic.replaces,ticfile);
       return TIC_security;
   }

   w_log('6',"File: %s size: %ld area: %s from: %s orig: %s",
         tic.file, tic.size, tic.area, aka2str(tic.from), tic_origin=aka2str5d(tic.origin));
   nfree(tic_origin);

   if (tic.to.zone!=0) {
      if (!isOurAka(config,tic.to)) {
         /* Forwarding tic and file to other link? */
         to_link=getLinkFromAddr(config,tic.to);
         if ( (to_link != NULL) && (to_link->forwardPkts != fOff) ) {
            if ( (to_link->forwardPkts==fSecure) && (sec != secProtInbound) && (sec != secLocalInbound) );
            else { /* Forwarding */
                busy = 0;
                if (createOutboundFileNameAka(to_link,
                     to_link->fileEchoFlavour, FLOFILE, SelectPackAka(to_link))==0) {
                  strcpy(linkfilepath,to_link->floFile);
                  if (!busy) { /*  FIXME: it always not busy!!! */
                     *(strrchr(linkfilepath,PATH_DELIM))=0;
                     newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
                     if (move_file(ticfile,newticfile,0)==0) { /* don't overwrite existing file */
                        strcpy(ticedfile,ticfile);
                        *(strrchr(ticedfile,PATH_DELIM)+1)=0;
                        j = strlen(ticedfile);
                        strcat(ticedfile,tic.file);
                        adaptcase(ticedfile);
                        strcpy(tic.file, ticedfile + j);
                        flohandle=fopen(to_link->floFile,"a");
                        fprintf(flohandle,"^%s\n",ticedfile);
                        fprintf(flohandle,"^%s\n",newticfile);
                        fclose(flohandle);
                        if(to_link->bsyFile){
                          remove(to_link->bsyFile);
                          nfree(to_link->bsyFile);
                        }
                        nfree(to_link->floFile);
                     }
                  }
               }
               doSaveTic(ticfile,&tic,NULL);
               disposeTic(&tic);
               return TIC_OK;
            }
         }
         /* not to us and no forward */
         w_log(LL_ERROR,"Tic File adressed to %s, not to us", aka2str(tic.to));
         disposeTic(&tic);
         return TIC_NotForUs;
      }
   }

   /* Security Check */
   from_link=getLinkFromAddr(config,tic.from);
   if (from_link == NULL) {
      w_log(LL_ERROR,"Link for Tic From Adress '%s' not found",
              aka2str(tic.from));
      disposeTic(&tic);
      return TIC_security;
   }

   if (tic.password && ((from_link->ticPwd==NULL) ||
       (stricmp(tic.password,from_link->ticPwd)!=0))) {
      w_log(LL_ERROR,"Wrong Password %s from %s",
              tic.password,aka2str(tic.from));
      disposeTic(&tic);
      return TIC_security;
   }

   strcpy(ticedfile,ticfile);
   *(strrchr(ticedfile,PATH_DELIM)+1)=0;
   j = strlen(ticedfile);
   strcat(ticedfile,tic.file);
   adaptcase(ticedfile);
   strcpy(tic.file, ticedfile + j);

   /* Receive file? */
   if (!fexist(ticedfile)) {
      if (from_link->delNotReceivedTIC) {
         w_log('6',"File %s from filearea %s not received, remove his TIC",tic.file,tic.area);
         disposeTic(&tic);
         return TIC_OK;
      } else {
         w_log('6',"File %s from filearea %s not received, wait",tic.file,tic.area);
         disposeTic(&tic);
         return TIC_NotRecvd;
      }
   }

#if !defined(__UNIX__)
   if(hidden(ticedfile)) {
      w_log('6',"File %s from filearea %s not completely received, wait",tic.file,tic.area);
      disposeTic(&tic);
      return TIC_NotRecvd;
   }
#endif


   filearea=getFileArea(tic.area);

   w_log(LL_DEBUGz, __FILE__ ":%u:processTic(): filearea %sfound", __LINE__, filearea? "" : "not");
   if (filearea==NULL && from_link->autoFileCreate) {
       char *descr = NULL;
       if(tic.areadesc)           descr = sstrdup(tic.areadesc);
       if(config->intab && descr) recodeToInternalCharset(descr);
       autoCreate(tic.area,descr,&(tic.from),NULL);
       filearea=getFileArea(tic.area);
       w_log(LL_DEBUGz, __FILE__ ":%u:processTic(): filearea %sfound", __LINE__, filearea? "" : "not");
       nfree(descr);
   }

   if (filearea == NULL) {
      if( from_link->autoFileCreate ){
        w_log(LL_ERROR,"Cannot create File Area %s",tic.area);
        if (!quiet) fprintf(stderr,"Cannot create File Area %s !\n",tic.area);
      }else{
        w_log(LL_ERROR,"Cannot open File Area %s, autocreate not allowed",tic.area);
        if (!quiet) fprintf(stderr,"Cannot open File Area %s, autocreate not allowed !\n",tic.area);
      }
      disposeTic(&tic);
      return TIC_NotOpen;
   }
   /* Check CRC Value and reject faulty files depending on noCRC flag */
   if (!filearea->noCRC) {
      crc = filecrc32(ticedfile);
      if (tic.crc != crc) {
          strcpy(dirname, ticedfile);
          pos = strrchr(dirname, PATH_DELIM);
          if (pos) {
              *pos = 0;
              findfile = pos+1;
              pos = strrchr(findfile, '.');
              if (pos) {

                  *(++pos) = 0;
                  strcat(findfile, "*");
#ifdef DEBUG_HPT
                  printf("NoCRC! dirname = %s, findfile = %s\n", dirname, findfile);
#endif
                  dir = opendir(dirname);

                  if (dir) {
                      while ((file = readdir(dir)) != NULL) {
                          if (patimat(file->d_name, findfile)) {
                              stat(file->d_name,&stbuf);
                              if (stbuf.st_size == tic.size) {
                                  crc = filecrc32(file->d_name);
                                  if (crc == tic.crc) {
                                      fileisfound = 1;
                                      sprintf(dirname+strlen(dirname), "%c%s", PATH_DELIM, file->d_name);
                                      break;
                                  }
                              }

                          }
                      }
                      closedir(dir);
                  }
              }
          }

          if (fileisfound) {
              realfile = smalloc(strlen(dirname)+1);
              strcpy(realfile, dirname);
              *(strrchr(dirname, PATH_DELIM)) = 0;
              findfile = makeUniqueDosFileName(dirname,"tmp",config);
              if (rename(ticedfile, findfile) != 0 ) {
                  w_log('9',"Can't file %s rename to %s", ticedfile, findfile);
                  nfree(findfile);
                  nfree(realfile);
                  disposeTic(&tic);
                  return TIC_NotRecvd;
              }
              if (rename(realfile, ticedfile) != 0) {
                  w_log('9',"Can't file %s rename to %s", realfile, ticedfile);
                  nfree(findfile);
                  nfree(realfile);
                  disposeTic(&tic);
                  return TIC_CantRename;
              }
              if (rename(findfile, realfile) != 0) {
                  remove(findfile);
              }
              nfree(findfile);
              nfree(realfile);
          } else {
              w_log(LL_ERROR,"Wrong CRC for file %s - in tic:%08lx, need:%08lx",tic.file,tic.crc,crc);
              disposeTic(&tic);
              return TIC_WrongTIC;
          }
      }
   }


   stat(ticedfile,&stbuf);
   tic.size = stbuf.st_size;
   if (!tic.size) {
      w_log('6',"File %s from filearea %s has zero size",tic.file,tic.area);
      disposeTic(&tic);
      return TIC_NotRecvd;
   }

   writeAccess = e_writeCheck(config,filearea,getLinkFromAddr(config,tic.from));

   switch (writeAccess) {
   case 0: break;
   case 4:
      w_log(LL_ERROR,"Link %s not subscribed to File Area %s",
              aka2str(tic.from), tic.area);
      disposeTic(&tic);
      return TIC_WrongTIC;
   case 3:
      w_log(LL_ERROR,"Not import from link %s",
              aka2str(from_link->hisAka));
      disposeTic(&tic);
      return TIC_WrongTIC;
   case 2:
      w_log(LL_ERROR,"Link %s no access level",
      aka2str(from_link->hisAka));
      disposeTic(&tic);
      return TIC_WrongTIC;
   case 1:
      w_log(LL_ERROR,"Link %s no access group",
      aka2str(from_link->hisAka));
      disposeTic(&tic);
      return TIC_WrongTIC;
   }

   rc = sendToLinks(1, filearea, &tic, ticedfile);
   
   if(rc == 0)   
       doSaveTic(ticfile,&tic,filearea);

   disposeTic(&tic);
   return(rc);
}

void processDir(char *directory, e_tossSecurity sec)
{
   DIR            *dir;
   struct dirent  *file;
   char           *dummy;
   int            rc;

   if (directory == NULL) return;

   dir = opendir(directory);
   if (dir == NULL) return;

   while ((file = readdir(dir)) != NULL) {
#ifdef DEBUG_HPT
      printf("testing %s\n", file->d_name);
#endif

      if (patimat(file->d_name, "*.TIC") == 1) {
         dummy = (char *) smalloc(strlen(directory)+strlen(file->d_name)+1);
         strcpy(dummy, directory);
         strcat(dummy, file->d_name);

#if !defined(__UNIX__)
         if (!hidden(dummy)) {
#endif
            rc=processTic(dummy,sec);

            switch (rc) {
               case TIC_security:  /* pktpwd problem */
                  changeFileSuffix(dummy, "sec", 1);
                  break;
               case TIC_NotOpen:   /* could not open file */
                  changeFileSuffix(dummy, "acs", 1);
                  break;
               case TIC_WrongTIC:  /* not/wrong pkt */
                  changeFileSuffix(dummy, "bad", 1);
                  break;
               case TIC_NotForUs:  /* not to us */
                  changeFileSuffix(dummy, "ntu", 1);
                  break;
               case TIC_NotRecvd:  /* file not recieved */
                  break;
               default:            /* OK */
                  remove (dummy);
                  break;
            } /* switch */
#if !defined(__UNIX__)
         }
#endif
         nfree(dummy);
      } /* if */
   } /* while */
   closedir(dir);
}

void checkTmpDir(void)
{
    char tmpdir[256], newticedfile[256], newticfile[256];
    DIR            *dir=NULL;
    struct dirent  *file=NULL;
    char           *ticfile=NULL;
    s_link         *link=NULL;
    s_ticfile      tic;
    s_area *filearea=NULL;
    FILE *flohandle=NULL;
    int error = 0;
    
    w_log('6',"Checking tmp dir");
    strcpy(tmpdir, config->busyFileDir);
    dir = opendir(tmpdir);
    if (dir == NULL) return;
    
    while ((file = readdir(dir)) != NULL) {
        if (strlen(file->d_name) != 12) continue;
        /* if (!file->d_size) continue; */
        ticfile = (char *) smalloc(strlen(tmpdir)+strlen(file->d_name)+1);
        strcpy(ticfile, tmpdir);
        strcat(ticfile, file->d_name);
        if (stricmp(file->d_name+8, ".TIC") == 0) {
            memset(&tic,0,sizeof(tic));
            if(!parseTic(ticfile,&tic)) continue;
            if( checkTic(ticfile,&tic) ) continue;
            link = getLinkFromAddr(config, tic.to);
            if(!link) continue;
            /*  createFlo doesn't  support ASO!!! */
            /* if (createFlo(link,cvtFlavour2Prio(link->fileEchoFlavour))==0) { */
            if (createOutboundFileNameAka(link, link->fileEchoFlavour, FLOFILE, SelectPackAka(link))==0) {
                filearea=getFileArea(tic.area);
                if (filearea!=NULL) {
                    if (filearea->msgbType != MSGTYPE_PASSTHROUGH && !filearea->sendorig) strcpy(newticedfile,filearea->fileName);
                    else strcpy(newticedfile,config->passFileAreaDir);
                    strcat(newticedfile,tic.file);
                    adaptcase(newticedfile);
                    if (!link->noTIC) {
                        if (config->separateBundles) {
                            strcpy(newticfile,link->floFile);
                            sprintf(strrchr(newticfile, '.'), ".sep%c", PATH_DELIM);
                        } else {
                            strcpy(newticfile,config->ticOutbound);
                        }
                        _createDirectoryTree(newticfile);
                        strcat(newticfile,strrchr(ticfile,PATH_DELIM)+1);
                        if( !fexist(ticfile) )
                            w_log(LL_ERROR,"File %s not found",ticfile);
                        else if (move_file(ticfile,newticfile,1)!=0) { /* overwrite existing file if not same */
                            w_log( LL_ERROR, "File %s not moveable to %s: %s",
                                ticfile, newticfile, strerror(errno) );
                            error = 1;
                        }
                    } else remove(ticfile);
                    if (!error) {
                        flohandle=fopen(link->floFile,"a");
                        fprintf(flohandle,"%s\n",newticedfile);
                        if (!link->noTIC)
                            fprintf(flohandle,"^%s\n",newticfile);
                        fclose(flohandle);
                        
                        w_log('6',"Forwarding save file %s for %s",
                            tic.file, aka2str(link->hisAka));
                    }
                } /* if filearea */
            } /* if createFlo */
            if( link->bsyFile){
                remove(link->bsyFile);
                nfree(link->bsyFile);
            }
            nfree(link->floFile);
            disposeTic(&tic);
        } /* if ".TIC" */
        nfree(ticfile);
    } /* while */
    closedir(dir);
}


int putMsgInArea(s_area *echo, s_message *msg, int strip, dword forceattr)
{
    char *ctrlBuff, *textStart, *textWithoutArea;
    UINT textLength = (UINT) msg->textLength;
    HAREA harea = NULL;
    HMSG  hmsg;
    XMSG  xmsg;
    char /**slash,*/ *p, *q, *tiny;
    int rc = 0;

    if (echo->msgbType==MSGTYPE_PASSTHROUGH) {
        w_log(LL_ERROR, "Can't put message to passthrough area %s!", echo->areaName);
        return rc;
    }

    if (!msg->netMail) {
	msg->destAddr.zone  = echo->useAka->zone;
	msg->destAddr.net   = echo->useAka->net;
	msg->destAddr.node  = echo->useAka->node;
	msg->destAddr.point = echo->useAka->point;
    }

	harea = MsgOpenArea((UCHAR *) echo->fileName, MSGAREA_CRIFNEC,
			(word)(echo->msgbType | (msg->netMail ? 0 : MSGTYPE_ECHO)));
    if (harea!= NULL) {
	    hmsg = MsgOpenMsg(harea, MOPEN_CREATE, 0);
	if (hmsg != NULL) {


	    textWithoutArea = msg->text;

	    if ((strip==1) && (strncmp(msg->text, "AREA:", 5) == 0)) {
		/*  jump over AREA:xxxxx\r */
		while (*(textWithoutArea) != '\r') textWithoutArea++;
		textWithoutArea++;
		textLength -= (size_t) (textWithoutArea - msg->text);
	    }

	    if (echo->killSB) {
		tiny = strrstr(textWithoutArea, " * Origin:");
		if (tiny == NULL) tiny = textWithoutArea;
		if (NULL != (p = strstr(tiny, "\rSEEN-BY: "))) {
		    p[1]='\0';
		    textLength = (size_t) (p - textWithoutArea + 1);
		}
	    } else if (echo->tinySB) {
		tiny = strrstr(textWithoutArea, " * Origin:");
		if (tiny == NULL) tiny = textWithoutArea;
		if (NULL != (p = strstr(tiny, "\rSEEN-BY: "))) {
		    p++;
		    if (NULL != (q = strstr(p,"\001PATH: "))) {
			/*  memmove(p,q,strlen(q)+1); */
			memmove(p,q,textLength-(size_t)(q-textWithoutArea)+1);
			textLength -= (size_t) (q - p);
		    } else {
			p[0]='\0';
			textLength = (size_t) (p - textWithoutArea);
		    }
		}
	    }

	
	    ctrlBuff = (char *) CopyToControlBuf((UCHAR *) textWithoutArea,
						 (UCHAR **) &textStart,
						 &textLength);
	    /*  textStart is a pointer to the first non-kludge line */
	    xmsg = createXMSG(config,msg, NULL, forceattr,NULL);

	    if (MsgWriteMsg(hmsg, 0, &xmsg, (byte *) textStart, (dword)
			    textLength, (dword) textLength,
			    (dword)strlen(ctrlBuff), (byte*)ctrlBuff)!=0)
		w_log(LL_ERROR, "Could not write msg in %s!", echo->fileName);
	    else rc = 1; /*  normal exit */

	    if (MsgCloseMsg(hmsg)!=0) {
		w_log(LL_ERROR, "Could not close msg in %s!", echo->fileName);
		rc = 0;
	    }
	    nfree(ctrlBuff);

	} else w_log(LL_ERROR, "Could not create new msg in %s!", echo->fileName);
	/* endif */
    MsgCloseArea(harea);
    } else w_log(LL_ERROR, "Could not open/create EchoArea %s!", echo->fileName);
    /* endif */
    return rc;
}

int putMsgInBadArea(s_message *msg, hs_addr pktOrigAddr)
{
    char *tmp = NULL, *line = NULL, *textBuff=NULL, *areaName=NULL, *reason = NULL;

    /*  get real name area */
    line = strchr(msg->text, '\r');
    if (strncmp(msg->text,"AREA:",5)==0) {
        *line = 0;
        xstrcat(&areaName, msg->text+5);
        *line = '\r';
    }
	reason = "report to passthrough area";
    tmp = msg->text;

    while ((line = strchr(tmp, '\r')) != NULL) {
	if (*(line+1) == '\x01') tmp = line+1;
	else { tmp = line+1; *line = 0; break; }
    }
	
    xstrscat(&textBuff, msg->text, "\rFROM: ", aka2str(pktOrigAddr), "\rREASON: ", reason, "\r", NULL);

    if (areaName) xscatprintf(&textBuff, "AREANAME: %s\r\r", areaName);
    xstrcat(&textBuff, tmp);
    nfree(areaName);
    nfree(msg->text);
    msg->text = textBuff;
    msg->textLength = strlen(msg->text);
    if (putMsgInArea(&(config->badArea), msg, 0, 0)) {
        return 1;
    }
    return 0;
}

void writeMsgToSysop(s_message *msg, char *areaName, char* origin)
{
   s_area       *echo;

   xscatprintf(&(msg->text), " \r--- %s\r * Origin: %s (%s)\r",
       (config->tearline) ? config->tearline : "",
       origin ? origin : (config->origin) ? config->origin : config->name,
       aka2str(msg->origAddr));

   msg->textLength = strlen(msg->text);
   if (msg->netMail == 1) writeNetmail(msg, areaName);
   else {
      echo = getArea(config, areaName);
      if (echo != &(config->badArea) && echo->msgbType != MSGTYPE_PASSTHROUGH) {
          putMsgInArea(echo, msg,1,MSGLOCAL);
          w_log( LL_POSTING , "Post report message to %s area", echo->areaName);
      } else {
          w_log( LL_POSTING , "Post report message to %s area", config->badArea.areaName);
          putMsgInBadArea(msg, msg->origAddr);
      }
   }
}

void toss()
{

   w_log( LL_INFO, "Start tossing...");

   processDir(config->localInbound, secLocalInbound);
   processDir(config->protInbound, secProtInbound);
   processDir(config->inbound, secInbound);

}
