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

#ifdef OS2
#define INCL_DOSFILEMGR /* for hidden() routine */
#include <os2.h>
#endif


#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#ifdef __EMX__
#include <sys/types.h>
#endif
#include <sys/stat.h>
#if ((!(defined(_MSC_VER) && (_MSC_VER >= 1200))) && (!defined(__TURBOC__)))
#include <unistd.h>
#endif

#include <fidoconf/fidoconf.h>
#include <fidoconf/adcase.h>
#include <fidoconf/common.h>
#include <fcommon.h>
#include <global.h>

#include <toss.h>
#include <smapi/patmat.h>

#include <fidoconf/dirlayer.h>
#include <fidoconf/xstr.h>

#include <smapi/msgapi.h>
#include <smapi/typedefs.h>
#include <smapi/compiler.h>
#include <pkt.h>
#include <areafix.h>
#include <version.h>
#include <smapi/ffind.h>

#include <smapi/stamp.h>
#include <smapi/progprot.h>

#include <add_desc.h>
#include <seenby.h>
#include <fidoconf/recode.h>
#include <fidoconf/crc.h>
#include <filecase.h>

#ifndef OS2
#ifdef __EMX__
#include <sys/types.h>
#ifndef _A_HIDDEN
#define _A_HIDDEN A_HIDDEN
#endif
#endif
#endif

#if defined(__WATCOMC__) || defined(__TURBOC__) || defined(__DJGPP__)
#include <dos.h>
#include <process.h>
#endif

s_newfilereport **newFileReport = NULL;
unsigned newfilesCount = 0;

void changeFileSuffix(char *fileName, char *newSuffix) {

   int   i = 1;
   char  buff[200];

   char *beginOfSuffix = strrchr(fileName, '.')+1;
   char *newFileName;
   int  length = strlen(fileName)-strlen(beginOfSuffix)+strlen(newSuffix);

   newFileName = (char *) scalloc(length+1+2, 1);
   strncpy(newFileName, fileName, length-strlen(newSuffix));
   strcat(newFileName, newSuffix);

#ifdef DEBUG_HPT
   printf("old: %s      new: %s\n", fileName, newFileName);
#endif

   while (fexist(newFileName) && (i<255)) {
      sprintf(buff, "%02x", i);
      beginOfSuffix = strrchr(newFileName, '.')+1;
      strncpy(beginOfSuffix+1, buff, 2);
      i++;
   }

   if (!fexist(newFileName)) rename(fileName, newFileName);
   else {
      w_log( '9', "Could not change suffix for %s. File already there and the 255 files after", fileName);
   }
}

char *createKludges(const char *area, const s_addr *ourAka, const s_addr *destAka)
{
   static time_t preTime=0L;
   time_t curTime;
   char *buff = NULL;

   if (area != NULL)
      xscatprintf(&buff, "AREA:%s\r", area);
   else {
      if (ourAka->point) xscatprintf(&buff, "\001FMPT %d\r",
          ourAka->point);
      if (destAka->point) xscatprintf(&buff, "\001TOPT %d\r",
          destAka->point);
   };

   // what a fucking bullshit?
   curTime = time(NULL);
   while (curTime == preTime) {
      sleep(1);
      curTime = time(NULL);
   }
   preTime = curTime;

   if (ourAka->point)
      xscatprintf(&buff,"\001MSGID: %u:%u/%u.%u %08lx\r",
          ourAka->zone,ourAka->net,ourAka->node,ourAka->point,(unsigned long) curTime);
   else
      xscatprintf(&buff,"\001MSGID: %u:%u/%u %08lx\r",
              ourAka->zone,ourAka->net,ourAka->node,(unsigned long) curTime);

   xscatprintf(&buff, "\001PID: %s\r", versionStr);

   return buff;
}


XMSG createXMSG(s_message *msg)
{
        XMSG  msgHeader;
        struct tm *date;
        time_t    currentTime;
        union stamp_combo dosdate;
        unsigned int i,remapit;
        char *subject;

        if (msg->netMail == 1) {
                /* attributes of netmail must be fixed */
                msgHeader.attr = msg->attributes;

                if (isOurAka(config,msg->destAddr)==0) {
                        msgHeader.attr &= ~(MSGCRASH | MSGREAD | MSGSENT | MSGKILL | MSGLOCAL | MSGHOLD
                          | MSGFRQ | MSGSCANNED | MSGLOCKED | MSGFWD); /* kill these flags */
                        msgHeader.attr |= MSGPRIVATE; /* set this flags */
                } /*else if (header!=NULL) msgHeader.attr |= MSGFWD; */ /* set TRS flag, if the mail is not to us */

      /* Check if we must remap */
      remapit=0;

      for (i=0;i<config->remapCount;i++)
          if ((config->remaps[i].toname==NULL ||
               stricmp(config->remaps[i].toname,msg->toUserName)==0) &&
              (config->remaps[i].oldaddr.zone==0 ||
               (config->remaps[i].oldaddr.zone==msg->destAddr.zone &&
                config->remaps[i].oldaddr.net==msg->destAddr.net &&
                config->remaps[i].oldaddr.node==msg->destAddr.node &&
                config->remaps[i].oldaddr.point==msg->destAddr.point) ) )
             {
             remapit=1;
             break;
             }

      if (remapit)
         {
         msg->destAddr.zone=config->remaps[i].newaddr.zone;
         msg->destAddr.net=config->remaps[i].newaddr.net;
         msg->destAddr.node=config->remaps[i].newaddr.node;
         msg->destAddr.point=config->remaps[i].newaddr.point;
         }

   }
   else
     {
       msgHeader.attr = msg->attributes;
       msgHeader.attr &= ~(MSGCRASH | MSGREAD | MSGSENT | MSGKILL | MSGLOCAL | MSGHOLD | MSGFRQ | MSGSCANNED | MSGLOCKED); /* kill these flags */
       msgHeader.attr |= MSGLOCAL;
     }

   strcpy((char *) msgHeader.from,msg->fromUserName);
   strcpy((char *) msgHeader.to, msg->toUserName);
   subject=msg->subjectLine;
   if (((msgHeader.attr & MSGFILE) == MSGFILE) && (msg->netMail==1)) {
     int size=strlen(msg->subjectLine)+strlen(config->inbound)+1;
     if (size < XMSG_SUBJ_SIZE) {
       subject = (char *) smalloc (size);
       sprintf (subject,"%s%s",config->inbound,msg->subjectLine);
     }
   }
   strcpy((char *) msgHeader.subj,subject);
   if (subject != msg->subjectLine)
     free(subject);

   msgHeader.orig.zone  = (word)msg->origAddr.zone;
   msgHeader.orig.node  = (word)msg->origAddr.node;
   msgHeader.orig.net   = (word)msg->origAddr.net;
   msgHeader.orig.point = (word)msg->origAddr.point;
   msgHeader.dest.zone  = (word)msg->destAddr.zone;
   msgHeader.dest.node  = (word)msg->destAddr.node;
   msgHeader.dest.net   = (word)msg->destAddr.net;
   msgHeader.dest.point = (word)msg->destAddr.point;

   memset(&(msgHeader.date_written), 0, 8);    /* date to 0 */

   msgHeader.utc_ofs = 0;
   msgHeader.replyto = 0;
   memset(msgHeader.replies, 0, MAX_REPLY * sizeof(UMSGID));   /* no replies */
   strcpy((char *) msgHeader.__ftsc_date, msg->datetime);
   ASCII_Date_To_Binary(msg->datetime, (union stamp_combo *) &(msgHeader.date_written));

   currentTime = time(NULL);
   date = localtime(&currentTime);
   TmDate_to_DosDate(date, &dosdate);
   msgHeader.date_arrived = dosdate.msg_st;

   return msgHeader;
}

void writeNetmail(s_message *msg, char *areaName)
{
   HAREA  netmail;
   HMSG   msgHandle;
   UINT   len = msg->textLength;
   char   *bodyStart;             /* msg-body without kludgelines start */
   char   *ctrlBuf;               /* Kludgelines */
   XMSG   msgHeader;
   s_area *nmarea;

   if ((nmarea=getNetMailArea(config, areaName))==NULL) nmarea = &(config->netMailAreas[0]);

   netmail = MsgOpenArea((unsigned char *) nmarea->fileName, MSGAREA_CRIFNEC, nmarea->msgbType);

   if (netmail != NULL) {
      msgHandle = MsgOpenMsg(netmail, MOPEN_CREATE, 0);

      if (msgHandle != NULL) {
         nmarea->imported = 1; /* area has got new messages */
/*
         if (config->intab != NULL) {
            recodeToInternalCharset(msg->text);
            recodeToInternalCharset(msg->subjectLine);
         }
*/
         msgHeader = createXMSG(msg);
         /* Create CtrlBuf for SMAPI */
         ctrlBuf = (char *) CopyToControlBuf((UCHAR *) msg->text, (UCHAR **) &bodyStart, &len);
         /* write message */
         MsgWriteMsg(msgHandle, 0, &msgHeader, (UCHAR *) bodyStart, len, len, strlen(ctrlBuf)+1, (UCHAR *) ctrlBuf);
         free(ctrlBuf);
         MsgCloseMsg(msgHandle);

         w_log( '6', "Wrote Netmail to: %u:%u/%u.%u",
                 msg->destAddr.zone, msg->destAddr.net, msg->destAddr.node, msg->destAddr.point);
      } else {
         w_log( '9', "Could not write message to %s", areaName);
      } /* endif */

      MsgCloseArea(netmail);
   } else {
/*     printf("%u\n", msgapierr); */
      w_log( '9', "Could not open netmailarea %s", areaName);
   } /* endif */
}

void writeTic(char *ticfile,s_ticfile *tic)
{
   FILE *tichandle;
   int i;

   tichandle=fopen(ticfile,"wb");

   fprintf(tichandle,"Created by HTick, written by Gabriel Plutzar\r\n");
   fprintf(tichandle,"File %s\r\n",tic->file);
   fprintf(tichandle,"Area %s\r\n",tic->area);
   if (tic->areadesc[0]!=0)
      fprintf(tichandle,"Areadesc %s\r\n",tic->areadesc);

   for (i=0;i<tic->anzdesc;i++)
      fprintf(tichandle,"Desc %s\r\n",tic->desc[i]);

   for (i=0;i<tic->anzldesc;i++)
       fprintf(tichandle,"LDesc %s\r\n",tic->ldesc[i]);

   if (tic->replaces[0]!=0)
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

   if (tic->password[0]!=0)
      fprintf(tichandle,"Pw %s\r\n",tic->password);

     fclose(tichandle);
}

void disposeTic(s_ticfile *tic)
{
   int i;

   nfree(tic->seenby);

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

int parseTic(char *ticfile,s_ticfile *tic)
{
   FILE *tichandle;
   char *line, *token, *param, *linecut = "";
   s_link *ticSourceLink=NULL;

   tichandle=fopen(ticfile,"r");
   memset(tic,'\0',sizeof(s_ticfile));

   while ((line = readLine(tichandle)) != NULL) {
      line = trimLine(line);

      if (*line!=0 || *line!=10 || *line!=13 || *line!=';' || *line!='#') {
         if (config->MaxTicLineLength) {
            linecut = (char *) smalloc(config->MaxTicLineLength+1);
            strncpy(linecut,line,config->MaxTicLineLength);
            linecut[config->MaxTicLineLength] = 0;
            token=strtok(linecut, " \t");
         } else token=strtok(line, " \t");
         param=stripLeadingChars(strtok(NULL, "\0"), "\t");
         if (token && param) {
            if (stricmp(token,"created")==0);
            else if (stricmp(token,"file")==0) strcpy(tic->file,param);
            else if (stricmp(token,"areadesc")==0) strcpy(tic->areadesc,param);
            else if (stricmp(token,"area")==0) strcpy(tic->area,param);
            else if (stricmp(token,"desc")==0) {
               tic->desc=
               srealloc(tic->desc,(tic->anzdesc+1)*sizeof(*tic->desc));
               tic->desc[tic->anzdesc]=sstrdup(param);
               tic->anzdesc++;
            }
            else if (stricmp(token,"replaces")==0) strcpy(tic->replaces,param);
            else if (stricmp(token,"pw")==0) strcpy(tic->password,param);
            else if (stricmp(token,"size")==0) tic->size=atoi(param);
            else if (stricmp(token,"crc")==0) tic->crc = strtoul(param,NULL,16);
            else if (stricmp(token,"date")==0) tic->date=atoi(param);

            else if (stricmp(token,"from")==0) {
	       string2addr(param,&tic->from);
	       ticSourceLink = getLinkFromAddr(config, tic->from);
	    }
	    else if (stricmp(token,"to")==0) string2addr(param,&tic->to);
            else if ((stricmp(token,"Destination")==0) &&
		     (ticSourceLink && !ticSourceLink->FileFixFSC87Subset))
	                string2addr(param,&tic->to);
            else if (stricmp(token,"origin")==0) string2addr(param,&tic->origin);
            else if (stricmp(token,"magic")==0);
            else if (stricmp(token,"seenby")==0) {
               tic->seenby=
                  srealloc(tic->seenby,(tic->anzseenby+1)*sizeof(s_addr));
               string2addr(param,&tic->seenby[tic->anzseenby]);
               tic->anzseenby++;
            }
            else if (stricmp(token,"path")==0) {
               tic->path=
                  srealloc(tic->path,(tic->anzpath+1)*sizeof(*tic->path));
               tic->path[tic->anzpath]=sstrdup(param);
               tic->anzpath++;
            }
            else if (stricmp(token,"ldesc")==0) {
               tic->ldesc=
                  srealloc(tic->ldesc,(tic->anzldesc+1)*sizeof(*tic->ldesc));
               tic->ldesc[tic->anzldesc]=sstrdup(param);
               tic->anzldesc++;
            }
            else {
               /*   printf("Unknown Keyword %s in Tic File\n",token); */
               if (ticSourceLink && !ticSourceLink->FileFixFSC87Subset) w_log( '7', "Unknown Keyword %s in Tic File",token);
            }
         } /* endif */
         if (config->MaxTicLineLength) nfree(linecut);
      } /* endif */
      nfree(line);
   } /* endwhile */

  fclose(tichandle);

  if (!tic->anzdesc) {
     tic->desc = srealloc(tic->desc,sizeof(*tic->desc));
     tic->desc[0] = sstrdup("no desc");
     tic->anzdesc = 1;
  }

  return(1);
}

char *hpt_stristr(char *str, char *find)
{
    char ch, sc, *str1, *find1;

    find++;
    if ((ch = *(find-1)) != 0) {
	do {
	    do {
		str++;
		if ((sc = *(str-1)) == 0) return (NULL);
	    } while (tolower((unsigned char) sc) != tolower((unsigned char) ch));
			
	    for(str1=str,find1=find; *find1 && *str1 && tolower(*find1)==tolower(*str1); str1++,find1++);
			
	} while (*find1);
	str--;
    }
    return ((char *)str);
}

#ifdef __WATCOMC__
void *mk_lst(char *a) {
    char *p=a, *q=a, **list=NULL, end=0, num=0;

    while (*p && !end) {
	while (*q && !isspace(*q)) q++;
	if (*q=='\0') end=1;
	*q ='\0';
	list = (char **) realloc(list, ++num*sizeof(char*));
	list[num-1]=(char*)p;
	if (!end) {
	    p=q+1;
	    while(isspace(*p)) p++;
	}
	q=p;
    }
    list = (char **) realloc(list, (++num)*sizeof(char*));
    list[num-1]=NULL;

    return list;
}
#endif

int parseFileDesc(char *fileName,s_ticfile *tic)
{
   FILE *filehandle, *dizhandle;
   char *line, *dizfile;
   int  j, found;
   unsigned int  i;
   signed int cmdexit;
   char cmd[256];
#ifdef __WATCOMC__
    const char * const *list;
#endif

    // find what unpacker to use
    for (i = 0, found = 0; (i < config->unpackCount) && !found; i++) {
	filehandle = fopen(fileName, "rb");
	if (filehandle == NULL) return 2;
	// is offset is negative we look at the end
	fseek(filehandle, config->unpack[i].offset, config->unpack[i].offset >= 0 ? SEEK_SET : SEEK_END);
	if (ferror(filehandle)) { fclose(filehandle); continue; };
	for (found = 1, j = 0; j < config->unpack[i].codeSize; j++) {
	    if ((getc(filehandle) & config->unpack[i].mask[j]) != config->unpack[i].matchCode[j])
		found = 0;
	}
	fclose(filehandle);
    }

    // unpack file_id.diz (config->fileDescName)
    if (found) {
	fillCmdStatement(cmd,config->unpack[i-1].call,fileName,config->fileDescName,config->tempInbound);
	w_log( '6', "file %s: unpacking with \"%s\"", fileName, cmd);
        if( hpt_stristr(config->unpack[i-1].call, "zipInternal") )
        {
            cmdexit = 1;
#ifdef USE_HPT_ZLIB
            cmdexit = UnPackWithZlib(fileName, config->tempInbound);
#endif
        }
        else
        {
#ifdef __WATCOMC__
            list = mk_lst(cmd);
            cmdexit = spawnvp(P_WAIT, cmd, list);
            free((char **)list);
            if (cmdexit != 0) {
            w_log( '9', "exec failed: %s, return code: %d", strerror(errno), cmdexit);
            return 3;
        }
#else
        if ((cmdexit = system(cmd)) != 0) {
            w_log( '9', "exec failed, code %d", cmdexit);
            return 3;
        }
#endif
    }
    } else {
	w_log( '9', "file %s: cannot find unpacker", fileName);
	return 3;
    };

   dizfile=malloc(strlen(config->tempInbound)+strlen(config->fileDescName)+1);
   sprintf(dizfile, "%s%s", config->tempInbound, config->fileDescName);
   if ((dizhandle=fopen(dizfile,"r")) == NULL) {
      w_log('9',"File %s not found or not moveable",dizfile);
      return 3;
   };

   for (i=0;i<(unsigned int)tic->anzldesc;i++)
      nfree(tic->ldesc[i]);
   tic->anzldesc=0;

   while ((line = readLine(dizhandle)) != NULL) {
      tic->ldesc=
         srealloc(tic->ldesc,(tic->anzldesc+1)*sizeof(*tic->ldesc));
      tic->ldesc[tic->anzldesc]=sstrdup(line);
      tic->anzldesc++;
      nfree(line);
   } /* endwhile */

  fclose(dizhandle);
  remove(dizfile);

  nfree(dizfile);

  return(1);
}

int autoCreate(char *c_area, s_addr pktOrigAddr, char *desc)
{
   FILE *f;
   char *NewAutoCreate;
   char *fileName;

   char *fileechoFileName = NULL;
   char buff[255], myaddr[20], hisaddr[20];
   s_link *creatingLink;
   s_addr *aka;
   s_message *msg;
   s_filearea *area;
   FILE *echotosslog;

   fileechoFileName = (char *) smalloc(strlen(c_area)+1);
   strcpy(fileechoFileName, c_area);

   creatingLink = getLinkFromAddr(config, pktOrigAddr);

   fileechoFileName = makeMsgbFileName(config, c_area);

    // translating name of the area to lowercase/uppercase
    if (config->createAreasCase == eUpper) strUpper(c_area);
    else strLower(c_area);

    // translating filename of the area to lowercase/uppercase
    if (config->areasFileNameCase == eUpper) strUpper(fileechoFileName);
    else strLower(fileechoFileName);

    if (creatingLink->autoFileCreateSubdirs)
    {
        char *cp;
        for (cp = fileechoFileName; *cp; cp++)
        {
            if (*cp == '.')
            {
                *cp = PATH_DELIM;
            }
        }
    }

   if (creatingLink->autoFileCreateSubdirs &&
       stricmp(config->fileAreaBaseDir,"Passthrough"))
   {
       sprintf(buff,"%s%s",config->fileAreaBaseDir,fileechoFileName);
       if (_createDirectoryTree(buff))
       {
           if (!quiet) fprintf(stderr, "cannot make all subdirectories for %s\n",
                   fileechoFileName);
           return 1;
       }
   }


   fileName = creatingLink->autoFileCreateFile;
   if (fileName == NULL) fileName = getConfigFileName();

   f = fopen(fileName, "a");
   if (f == NULL) {
      f = fopen(getConfigFileName(), "a");
      if (f == NULL)
         {
            if (!quiet) fprintf(stderr,"autocreate: cannot open config file\n");
            return 1;
         }
   }

   aka = creatingLink->ourAka;

   /* making local address and address of uplink */
   strcpy(myaddr,aka2str(*aka));
   strcpy(hisaddr,aka2str(pktOrigAddr));

   /* write new line in config file */

   if (creatingLink->LinkGrp)
   {
     sprintf(buff, "FileArea %s %s%s -a %s -g %s ",
	     c_area,
	     config->fileAreaBaseDir,
	     (stricmp(config->fileAreaBaseDir,"Passthrough") == 0) ? "" : fileechoFileName,
	     myaddr,
	     creatingLink->LinkGrp);
   }
   else
   {
     sprintf(buff, "FileArea %s %s%s -a %s ",
	     c_area,
	     config->fileAreaBaseDir,
	     (stricmp(config->fileAreaBaseDir,"Passthrough") == 0) ? "" : fileechoFileName,
	     myaddr);
   }

   if (creatingLink->autoFileCreateDefaults) {
      NewAutoCreate=(char *) scalloc (strlen(creatingLink->autoFileCreateDefaults)+1, sizeof(char));
      strcpy(NewAutoCreate,creatingLink->autoFileCreateDefaults);
   } else NewAutoCreate = (char*)scalloc(1, sizeof(char));

   if ((fileName=strstr(NewAutoCreate,"-d "))==NULL) {
     if (desc[0] != 0) {
       char *tmp;
       tmp=(char *) scalloc (strlen(NewAutoCreate)+strlen(desc)+7,sizeof(char));
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       nfree (NewAutoCreate);
       NewAutoCreate=tmp;
     }
   } else {
     if (desc[0] != 0) {
       char *tmp;
       tmp=(char *) scalloc (strlen(NewAutoCreate)+strlen(desc)+7,sizeof(char));
       fileName[0]='\0';
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       fileName++;
       fileName=strrchr(fileName,'\"')+1; //"
       strcat(tmp,fileName);
       nfree(NewAutoCreate);
       NewAutoCreate=tmp;
     }
   }

   if ((NewAutoCreate != NULL) && (strlen(buff)+strlen(NewAutoCreate))<255) {
      strcat(buff, NewAutoCreate);
   }

   nfree(NewAutoCreate);

   sprintf (buff+strlen(buff)," %s",hisaddr);

   fprintf(f, "%s\n", buff);

   fclose(f);

   /* add new created echo to config in memory */
   parseLine(buff,config);

   w_log( '8', "FileArea '%s' autocreated by %s", c_area, hisaddr);

   /* report about new filearea */
   if (config->ReportTo && !cmAnnNewFileecho && (area = getFileArea(config, c_area)) != NULL) {
      if (getNetMailArea(config, config->ReportTo) != NULL) {
         msg = makeMessage(area->useAka, area->useAka, versionStr, config->sysop, "Created new fileareas", 1);
         msg->text = createKludges(NULL, area->useAka, area->useAka);
      } else {
         msg = makeMessage(area->useAka, area->useAka, versionStr, "All", "Created new fileareas", 0);
         msg->text = createKludges(config->ReportTo, area->useAka, area->useAka);
      } /* endif */
      sprintf(buff, "\r \rNew filearea: %s\r\rDescription : %s\r", area->areaName,
          (area->description) ? area->description : "");
      msg->text = (char*)srealloc(msg->text, strlen(msg->text)+strlen(buff)+1);
      strcat(msg->text, buff);
      writeMsgToSysop(msg, config->ReportTo);
      freeMsgBuff(msg);
      nfree(msg);
      if (config->echotosslog != NULL) {
         echotosslog = fopen (config->echotosslog, "a");
         fprintf(echotosslog,"%s\n",config->ReportTo);
         fclose(echotosslog);
      }
   } else {
   } /* endif */

   if (cmAnnNewFileecho) announceNewFileecho (announcenewfileecho, c_area, hisaddr);

   return 0;
}

int readCheck(s_filearea *echo, s_link *link)
{

  /* rc == '\x0000' access o'k
   rc == '\x0001' no access group
   rc == '\x0002' no access level
   rc == '\x0003' no access export
   rc == '\x0004' not linked
   rc == '\x0005' pause
   */

  unsigned int i;

  for (i=0; i<echo->downlinkCount; i++)
  {
    if (link == echo->downlinks[i]->link) break;
  }

  if (i == echo->downlinkCount) return 4;

  /* pause */
  if ((link->Pause & FPAUSE) == FPAUSE && !echo->noPause) return 5;

/* Do not check for groupaccess here, use groups only (!) for Filefix */
/*  if (strcmp(echo->group,"0")) {
      if (link->numAccessGrp) {
          if (config->numPublicGroup) {
              if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp) &&
                  !grpInArray(echo->group,config->PublicGroup,config->numPublicGroup))
                  return 1;
          } else if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp)) return 1;
      } else if (config->numPublicGroup) {
          if (!grpInArray(echo->group,config->PublicGroup,config->numPublicGroup)) return 1;
      } else return 1;
  }*/

  if (echo->levelread > link->level) return 2;

  if (i < echo->downlinkCount)
  {
    if (echo->downlinks[i]->export == 0) return 3;
  }

  return 0;
}

int writeCheck(s_filearea *echo, s_addr *aka)
{

  /* rc == '\x0000' access o'k
   rc == '\x0001' no access group
   rc == '\x0002' no access level
   rc == '\x0003' no access import
   rc == '\x0004' not linked
   */

  unsigned int i;

  s_link *link;

  if (!addrComp(*aka,*echo->useAka)) return 0;

  link = getLinkForFileArea (config, aka2str(*aka), echo);
  if (link == NULL) return 4;

  for (i=0; i<echo->downlinkCount; i++)
  {
    if (link == echo->downlinks[i]->link) break;
  }
  if (i == echo->downlinkCount) return 4;

/* Do not check for groupaccess here, use groups only (!) for Filefix */
/*  if (strcmp(echo->group,"0")) {
      if (link->numAccessGrp) {
         if (config->numPublicGroup) {
            if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp) &&
                !grpInArray(echo->group,config->PublicGroup,config->numPublicGroup))
               return 1;
         } else
            if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp)) return 1;
      } else
         if (config->numPublicGroup) {
            if (!grpInArray(echo->group,config->PublicGroup,config->numPublicGroup)) return 1;
         } else return 1;
   }*/

  if (echo->levelwrite > link->level) return 2;

  if (i < echo->downlinkCount)
  {
    if (link->import == 0) return 3;
  }

  return 0;
}

/* FIXME:
 i don't know for what reasont this function
 to developers: use createOutboundFileName and delete this shit
 (this function doesn't support ASO) -- ml (5-10-2001).
int createFlo(s_link *link, e_prio prio)
{

    FILE *f; // bsy file for current link
    char name[13], bsyname[13], zoneSuffix[6], pntDir[14];

   if (link->hisAka.point != 0) {
      sprintf(pntDir, "%04x%04x.pnt%c", link->hisAka.net, link->hisAka.node, PATH_DELIM);
      sprintf(name, "%08x.flo", link->hisAka.point);
   } else {
      pntDir[0] = 0;
      sprintf(name, "%04x%04x.flo", link->hisAka.net, link->hisAka.node);
   }

   if (link->hisAka.zone != config->addr[0].zone) {
      // add suffix for other zones
      sprintf(zoneSuffix, ".%03x%c", link->hisAka.zone, PATH_DELIM);
   } else {
      zoneSuffix[0] = 0;
   }

   switch (prio) {
      case CRASH :    name[9] = 'c';
                      break;
      case HOLD  :    name[9] = 'h';
                      break;
      case DIRECT:    name[9] = 'd';
                      break;
      case IMMEDIATE: name[9] = 'i';
                      break;
      case NORMAL:    break;
   }

   // create floFile
   link->floFile = (char *) smalloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+strlen(name)+1);
   link->bsyFile = (char *) smalloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+strlen(name)+1);
   strcpy(link->floFile, config->outbound);
   if (zoneSuffix[0] != 0) strcpy(link->floFile+strlen(link->floFile)-1, zoneSuffix);
   strcat(link->floFile, pntDir);
   createDirectoryTree(link->floFile); // create directoryTree if necessary
   strcpy(link->bsyFile, link->floFile);
   strcat(link->floFile, name);

   // create bsyFile
   strcpy(bsyname, name);
   bsyname[9]='b';bsyname[10]='s';bsyname[11]='y';
   strcat(link->bsyFile, bsyname);

   // maybe we have session with this link?
   if (fexist(link->bsyFile)) {
      nfree (link->bsyFile);
      return 1;
   } else {
      if ((f=fopen(link->bsyFile,"a")) == NULL) {
         if (!quiet) fprintf(stderr,"cannot create *.bsy file for %s\n",addr2string(&link->hisAka));
         remove(link->bsyFile);
         nfree(link->bsyFile);
         nfree(link->floFile);
         w_log( LL_CRIT, "cannot create *.bsy file, exit.");
         closeLog();
         if (config->lockfile != NULL) remove(config->lockfile);
         disposeConfig(config);
         exit(1);
      }
      fclose(f);
   }
   return 0;
}
*/

void doSaveTic(char *ticfile,s_ticfile *tic)
{
   char filename[256];
   unsigned int i;
   s_savetic *savetic;

   for (i = 0; i< config->saveTicCount; i++)
     {
       savetic = &(config->saveTic[i]);
       if (patimat(tic->area,savetic->fileAreaNameMask)==1) {
          w_log('6',"Saving Tic-File %s to %s",strrchr(ticfile,PATH_DELIM) + 1,savetic->pathName);
          strcpy(filename,savetic->pathName);
          strcat(filename,strrchr(ticfile, PATH_DELIM) + 1);
          //strLower(filename);
          if (copy_file(ticfile,filename)!=0) {
             w_log('9',"File %s not found or not moveable",ticfile);
          };
          break;
       };

     };

   return;
}

int sendToLinks(int isToss, s_filearea *filearea, s_ticfile *tic,
                char *filename)
/*
   isToss == 1 - tossing
   isToss == 0 - hatching
*/
{
   unsigned int i, z;
   char descr_file_name[256], newticedfile[256], fileareapath[256];
   char *linkfilepath=NULL;
   char hlp[100],timestr[40];
   time_t acttime;
   s_addr *old_seenby = NULL;
   s_addr old_from, old_to;
   int old_anzseenby = 0;
   int readAccess;
   int cmdexit;
   char *comm;
   char *p;
   int busy;
   char *newticfile;
   FILE *flohandle;
   int minLinkCount;


   if (isToss == 1) minLinkCount = 2; // uplink and downlink
   else minLinkCount = 1;             // only downlink

   if (!filearea->pass)
      strcpy(fileareapath,filearea->pathName);
   else
      strcpy(fileareapath,config->passFileAreaDir);
   p = strrchr(fileareapath,PATH_DELIM);
   if(p) strLower(p+1);

   _createDirectoryTree(fileareapath);

   if (tic->replaces!=NULL && !filearea->pass && !filearea->noreplace) {
      /* Delete old file[s] */
      int num_files;
      char *repl;

      repl = strrchr(tic->replaces,PATH_DELIM);
      if (repl==NULL) repl = tic->replaces;
      else repl++;
      num_files = removeFileMask(fileareapath,repl);
      if (num_files>0) {
         w_log('6',"Removed %d file[s]. Filemask: %s",num_files,repl);
      }
   }

   //if (!filearea->pass || (filearea->pass && filearea->downlinkCount>=minLinkCount)) {

      strcpy(newticedfile,fileareapath);
      strcat(newticedfile,MakeProperCase(tic->file));

      if(!filearea->pass && filearea->noreplace && fexist(newticedfile)) {
         w_log('9',"File %s already exist in filearea %s. Can't replace it",tic->file,tic->area);
         return(3);
      }

      if (isToss == 1) {
         if (!filearea->sendorig) {
            if (move_file(filename,newticedfile)!=0) {
                w_log('9',"File %s not found or not moveable",filename);
                return(2);
            } else {
               w_log('6',"Moved %s to %s",filename,newticedfile);
            }
         } else {
            if (copy_file(filename,newticedfile)!=0) {
               w_log('9',"File %s not found or not moveable",filename);
               return(2);
            } else {
              w_log('6',"Put %s to %s",filename,newticedfile);
            }
            strcpy(newticedfile,config->passFileAreaDir);
            strcat(newticedfile,MakeProperCase(tic->file));
            if (move_file(filename,newticedfile)!=0) {
               w_log('9',"File %s not found or not moveable",filename);
               return(2);
            } else {
               w_log('6',"Moved %s to %s",filename,newticedfile);
            }
         }
      } else
         if (copy_file(filename,newticedfile)!=0) {
            w_log('9',"File %s not found or not moveable",filename);
            return(2);
         } else {
            w_log('6',"Put %s to %s",filename,newticedfile);
         }
   //}

   if (tic->anzldesc==0)
      if (config->fileDescName)
         parseFileDesc(newticedfile, tic);

   if (!filearea->pass) {
      strcpy(descr_file_name, filearea->pathName);
      strcat(descr_file_name, "files.bbs");
      adaptcase(descr_file_name);
      removeDesc(descr_file_name,tic->file);
      if (tic->anzldesc>0)
         add_description (descr_file_name, tic->file, tic->ldesc, tic->anzldesc);
      else
         add_description (descr_file_name, tic->file, tic->desc, tic->anzdesc);
   }

   if (cmAnnFile && !filearea->hide)
   {
      if (tic->anzldesc>0)
         announceInFile (announcefile, tic->file, tic->size, tic->area, tic->origin, tic->ldesc, tic->anzldesc);
      else
         announceInFile (announcefile, tic->file, tic->size, tic->area, tic->origin, tic->desc, tic->anzdesc);
   }

   if (filearea->downlinkCount>=minLinkCount) {
      /* Adding path & seenbys */
      time(&acttime);
      strcpy(timestr,asctime(gmtime(&acttime)));
      timestr[strlen(timestr)-1]=0;
      if (timestr[8]==' ') timestr[8]='0';

      sprintf(hlp,"%s %lu %s UTC %s",
              aka2str(*filearea->useAka), (unsigned long) time(NULL), timestr,versionStr);

      tic->path=srealloc(tic->path,(tic->anzpath+1)*sizeof(*tic->path));
      tic->path[tic->anzpath]=sstrdup(hlp);
      tic->anzpath++;
   }

   if (isToss == 1) {
      /* Save seenby structure */
      old_seenby = smalloc(tic->anzseenby*sizeof(s_addr));
      memcpy(old_seenby,tic->seenby,tic->anzseenby*sizeof(s_addr));
      old_anzseenby = tic->anzseenby;
      memcpy(&old_from,&tic->from,sizeof(s_addr));
      memcpy(&old_to,&tic->to,sizeof(s_addr));
   }

   for (i=0;i<filearea->downlinkCount;i++) {
      s_link* downlink = filearea->downlinks[i]->link;
      if ( addrComp(tic->from,downlink->hisAka)!=0 &&
           ( (isToss==1 && addrComp(tic->to,downlink->hisAka)!=0) || isToss==0 ) &&
           addrComp(tic->origin,downlink->hisAka)!=0 &&
           ( (isToss==1 && seenbyComp (tic->seenby, tic->anzseenby,downlink->hisAka)!=0) || isToss==0) 
         ) {
         /* Adding Downlink to Seen-By */
         tic->seenby=srealloc(tic->seenby,(tic->anzseenby+1)*sizeof(s_addr));
         memcpy(&tic->seenby[tic->anzseenby],&downlink->hisAka,sizeof(s_addr));
         tic->anzseenby++;
      }
   }

   /* Checking to whom I shall forward */
   for (i=0;i<filearea->downlinkCount;i++) {
      s_link* downlink = filearea->downlinks[i]->link;
      if (addrComp(old_from,downlink->hisAka)!=0 &&
            addrComp(old_to,downlink->hisAka)!=0 &&
            addrComp(tic->origin,downlink->hisAka)!=0) {
         /* Forward file to */

         readAccess = readCheck(filearea, downlink);
         switch (readAccess) {
         case 0: break;
         case 5:
            w_log('7',"Link %s paused",
                    aka2str(downlink->hisAka));
            break;
         case 4:
            w_log('7',"Link %s not subscribe to File Area %s",
                    aka2str(old_from), tic->area);
            break;
         case 3:
            w_log('7',"Not export to link %s",
                    aka2str(downlink->hisAka));
            break;
         case 2:
            w_log('7',"Link %s no access level",
                    aka2str(downlink->hisAka));
            break;
         case 1:
            w_log('7',"Link %s no access group",
                    aka2str(downlink->hisAka));
            break;
         }

         if (readAccess == 0) {
            if (isToss == 1 && seenbyComp(old_seenby, old_anzseenby, downlink->hisAka) == 0) {
               w_log('7',"File %s already seenby %s",
                       tic->file,
                       aka2str(downlink->hisAka));
            } else {
//               memcpy(&tic->from,filearea->useAka,sizeof(s_addr));
               memcpy(&tic->from,downlink->ourAka,sizeof(s_addr));
               memcpy(&tic->to,&downlink->hisAka,
                      sizeof(s_addr));
               if (downlink->ticPwd!=NULL)
                  strncpy(tic->password,downlink->ticPwd,sizeof(tic->password));
               else tic->password[0]='\0';

               busy = 0;

               if (createOutboundFileName(downlink,
                     downlink->fileEchoFlavour,
                     FLOFILE)==1)
                  busy = 1;

               if (busy) {
                  w_log( '7', "Save TIC in temporary dir");
                  /* Create temporary directory */
                  xstrcat(&linkfilepath,config->busyFileDir);
               } else {
                  if (config->separateBundles) {
                     xstrcat(&linkfilepath, downlink->floFile);
                     sprintf(strrchr(linkfilepath, '.'), ".sep%c", PATH_DELIM);
                  } else {
                      // fileBoxes support
                      if (needUseFileBoxForLink(config,downlink)) {
                          if (!downlink->fileBox) 
                              downlink->fileBox = makeFileBoxName (config,downlink);
                          xstrcat(&linkfilepath, downlink->fileBox);
                      } else {
                          xstrcat(&linkfilepath, config->ticOutbound);
                      }
                  }
               }
               _createDirectoryTree(linkfilepath);

               if (!busy) {
                   /* Don't create TICs for everybody */
                   if (!downlink->noTIC) 
                       newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
                   else 
                       newticfile = NULL;
                  
                   if (needUseFileBoxForLink(config,downlink)) {
                       xstrcat(&linkfilepath, tic->file);
                       if (link_file(newticedfile,linkfilepath ) == 0)
                       {
                          if(copy_file(newticedfile,linkfilepath )==0)
                             w_log('6',"Copied %s to %s",newticedfile,linkfilepath);
                          else 
                             w_log('9',"File %s not found or not copyable",newticedfile);
                       } else {
                             w_log('6',"Linked %s to %s",newticedfile,linkfilepath);
                       }
                   } else {
                       flohandle=fopen(downlink->floFile,"a");
                       fprintf(flohandle,"%s\n",newticedfile);
                       if (newticfile != NULL) fprintf(flohandle,"^%s\n",newticfile);
                       fclose(flohandle);
                   }
                   
                   if (newticfile != NULL) writeTic(newticfile,tic);

                   remove(downlink->bsyFile);
                   w_log('6',"Forwarding %s for %s",
                       tic->file,
                       aka2str(downlink->hisAka));
                   
               }
               nfree(downlink->bsyFile);
               nfree(downlink->floFile);
               nfree(linkfilepath);
            } /* if Seenby */
         } /* if readAccess == 0 */
      } /* Forward file */
   }

   if (!filearea->hide) {
   /* report about new files - if filearea not hidden */
      newFileReport = (s_newfilereport**)srealloc(newFileReport, (newfilesCount+1)*sizeof(s_newfilereport*));
      newFileReport[newfilesCount] = (s_newfilereport*)scalloc(1, sizeof(s_newfilereport));
      newFileReport[newfilesCount]->useAka = filearea->useAka;
      newFileReport[newfilesCount]->areaName = filearea->areaName;
      newFileReport[newfilesCount]->areaDesc = filearea->description;
      newFileReport[newfilesCount]->fileName = sstrdup(tic->file);
      if (config->originInAnnounce) {
         newFileReport[newfilesCount]->origin.zone = tic->origin.zone;
         newFileReport[newfilesCount]->origin.net = tic->origin.net;
         newFileReport[newfilesCount]->origin.node = tic->origin.node;
         newFileReport[newfilesCount]->origin.point = tic->origin.point;
      }

      if (tic->anzldesc>0) {
         newFileReport[newfilesCount]->fileDesc = (char**)scalloc(tic->anzldesc, sizeof(char*));
         for (i = 0; i < tic->anzldesc; i++) {
            newFileReport[newfilesCount]->fileDesc[i] = sstrdup(tic->ldesc[i]);
            if (config->intab != NULL) recodeToInternalCharset(newFileReport[newfilesCount]->fileDesc[i]);
         } /* endfor */
         newFileReport[newfilesCount]->filedescCount = tic->anzldesc;
      } else {
         newFileReport[newfilesCount]->fileDesc = (char**)scalloc(tic->anzdesc, sizeof(char*));
         for (i = 0; i < (unsigned int)tic->anzdesc; i++) {
            newFileReport[newfilesCount]->fileDesc[i] = sstrdup(tic->desc[i]);
            if (config->intab != NULL) recodeToInternalCharset(newFileReport[newfilesCount]->fileDesc[i]);
         } /* endfor */
         newFileReport[newfilesCount]->filedescCount = tic->anzdesc;
      }

      newFileReport[newfilesCount]->fileSize = tic->size;

      newfilesCount++;
   }

   if (isToss == 0) reportNewFiles();

   /* execute external program */
   for (z = 0; z < config->execonfileCount; z++) {
     if (stricmp(filearea->areaName,config->execonfile[z].filearea) != 0) continue;
       if (patimat(tic->file,config->execonfile[z].filename) == 0) continue;
       else {
         comm = (char *) smalloc(strlen(config->execonfile[z].command)+1
                                +(!filearea->pass ? strlen(filearea->pathName) : strlen(config->passFileAreaDir))
                                +strlen(tic->file)+1);
         if (comm == NULL) {
            w_log( '9', "Exec failed - not enough memory");
            continue;
         }
         sprintf(comm,"%s %s%s",config->execonfile[z].command,
                                (!filearea->pass ? filearea->pathName : config->passFileAreaDir),
                                tic->file);
         w_log( '6', "Executing %s", comm);
         if ((cmdexit = system(comm)) != 0) {
           w_log( '9', "Exec failed, code %d", cmdexit);
         }
         nfree(comm);
       }
   }

   if (isToss == 1) nfree(old_seenby);
   return(0);
}

#if !defined(UNIX)

/* FIXME: This code is nonportable and should therefore really be part
          of a porting library like huskylib or fidoconf!!!
*/

int hidden (char *filename)
{
#if (defined(__TURBOC__) && !defined(OS2)) || defined(__DJGPP__)
   unsigned fattrs;
   _dos_getfileattr(filename, &fattrs);
   return fattrs & _A_HIDDEN;
#elif defined (WINNT) || defined(__NT__)
   unsigned fattrs;
   fattrs = (GetFileAttributes(filename) & 0x2) ? _A_HIDDEN : 0;
   return fattrs & _A_HIDDEN;
#elif defined (OS2)
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
   s_filearea *filearea;
   s_link *from_link, *to_link;
   int busy;
   unsigned long crc;
   struct stat stbuf;
   int writeAccess;
   int fileisfound = 0;
   int rc = 0;


#ifdef DEBUG_HPT
   printf("Ticfile %s\n",ticfile);
#endif

   w_log('6',"Processing Tic-File %s",ticfile);

   parseTic(ticfile,&tic);

   w_log('6',"File: %s Area: %s From: %s Orig: %u:%u/%u.%u",
           tic.file, tic.area, aka2str(tic.from),
           tic.origin.zone,tic.origin.net,tic.origin.node,tic.origin.point);

   if (tic.to.zone!=0) {
      if (!isOurAka(config,tic.to)) {
         /* Forwarding tic and file to other link? */
         to_link=getLinkFromAddr(config,tic.to);
         if ( (to_link != NULL) && (to_link->forwardPkts != fOff) ) {
            if ( (to_link->forwardPkts==fSecure) && (sec != secProtInbound) && (sec != secLocalInbound) );
            else { /* Forwarding */
                busy = 0;
                if (createOutboundFileName(to_link,
                     to_link->fileEchoFlavour, FLOFILE)==0) {
                  strcpy(linkfilepath,to_link->floFile);
                  if (!busy) { // FIXME: it always not busy!!!
                     *(strrchr(linkfilepath,PATH_DELIM))=0;
                     newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
                     if (move_file(ticfile,newticfile)==0) {
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
                        remove(to_link->bsyFile);
                        nfree(to_link->bsyFile);
                        nfree(to_link->floFile);
                     }
                  }
               }
               doSaveTic(ticfile,&tic);
               disposeTic(&tic);
               return(0);
            }
         }
         /* not to us and no forward */
         w_log('9',"Tic File adressed to %s, not to us", aka2str(tic.to));
         disposeTic(&tic);
         return(4);
      }
   }

   /* Security Check */
   from_link=getLinkFromAddr(config,tic.from);
   if (from_link == NULL) {
      w_log('9',"Link for Tic From Adress '%s' not found",
              aka2str(tic.from));
      disposeTic(&tic);
      return(1);
   }

   if (tic.password[0]!=0 && ((from_link->ticPwd==NULL) ||
       (stricmp(tic.password,from_link->ticPwd)!=0))) {
      w_log('9',"Wrong Password %s from %s",
              tic.password,aka2str(tic.from));
      disposeTic(&tic);
      return(1);
   }

   strcpy(ticedfile,ticfile);
   *(strrchr(ticedfile,PATH_DELIM)+1)=0;
   j = strlen(ticedfile);
   strcat(ticedfile,tic.file);
   adaptcase(ticedfile);
   strcpy(tic.file, ticedfile + j);

   /* Recieve file? */
   if (!fexist(ticedfile)) {
      if (from_link->delNotRecievedTIC) {
         w_log('6',"File %s from filearea %s not received, remove his TIC",tic.file,tic.area);
         disposeTic(&tic);
         return(0);
      } else {
         w_log('6',"File %s from filearea %s not received, wait",tic.file,tic.area);
         disposeTic(&tic);
         return(5);
      }
   }

#if !defined(UNIX)
   if(hidden(ticedfile)) {
      w_log('6',"File %s from filearea %s not completely received, wait",tic.file,tic.area);
      disposeTic(&tic);
      return(5);
   }
#endif


   filearea=getFileArea(config,tic.area);

   if (filearea==NULL && from_link->autoFileCreate) {
      autoCreate(tic.area,tic.from,tic.areadesc);
      filearea=getFileArea(config,tic.area);
   }

   if (filearea == NULL) {
      w_log('9',"Cannot open or create File Area %s",tic.area);
      if (!quiet) fprintf(stderr,"Cannot open or create File Area %s !\n",tic.area);
      disposeTic(&tic);
      return(2);
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
                  return(3);
              }
              if (rename(realfile, ticedfile) != 0) {
                  w_log('9',"Can't file %s rename to %s", realfile, ticedfile);
                  nfree(findfile);
                  nfree(realfile);
                  disposeTic(&tic);
                  return(3);
              }
              if (rename(findfile, realfile) != 0) {
                  remove(findfile);
              }
              nfree(findfile);
              nfree(realfile);
          } else {
              w_log('9',"Wrong CRC for file %s - in tic:%08lx, need:%08lx",tic.file,tic.crc,crc);
              disposeTic(&tic);
              return(3);
          }
      }
   }


   stat(ticedfile,&stbuf);
   tic.size = stbuf.st_size;
   if (!tic.size) {
      w_log('6',"File %s from filearea %s has zero size",tic.file,tic.area);
      disposeTic(&tic);
      return(5);
   }

   writeAccess = writeCheck(filearea,&tic.from);

   switch (writeAccess) {
   case 0: break;
   case 4:
      w_log('9',"Link %s not subscribed to File Area %s",
              aka2str(tic.from), tic.area);
      disposeTic(&tic);
      return(3);
   case 3:
      w_log('9',"Not import from link %s",
              aka2str(from_link->hisAka));
      disposeTic(&tic);
      return(3);
   case 2:
      w_log('9',"Link %s no access level",
      aka2str(from_link->hisAka));
      disposeTic(&tic);
      return(3);
   case 1:
      w_log('9',"Link %s no access group",
      aka2str(from_link->hisAka));
      disposeTic(&tic);
      return(3);
   }

   rc = sendToLinks(1, filearea, &tic, ticedfile);

   doSaveTic(ticfile,&tic);
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

#if !defined(UNIX)
         if (!hidden(dummy)) {
#endif
            rc=processTic(dummy,sec);

            switch (rc) {
               case 1:  /* pktpwd problem */
                  changeFileSuffix(dummy, "sec");
                  break;
               case 2:  /* could not open file */
                  changeFileSuffix(dummy, "acs");
                  break;
               case 3:  /* not/wrong pkt */
                  changeFileSuffix(dummy, "bad");
                  break;
               case 4:  /* not to us */
                  changeFileSuffix(dummy, "ntu");
                  break;
               case 5:  /* file not recieved */
                  break;
               default:
                  remove (dummy);
                  break;
            } /* switch */
#if !defined(UNIX)
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
    DIR            *dir;
    struct dirent  *file;
    char           *ticfile;
    s_link         *link;
    s_ticfile      tic;
    s_filearea *filearea;
    FILE *flohandle;
    int error = 0;

   w_log('6',"Checking tmp dir");
   strcpy(tmpdir, config->busyFileDir);
   dir = opendir(tmpdir);
   if (dir == NULL) return;

   while ((file = readdir(dir)) != NULL) {
      if (strlen(file->d_name) != 12) continue;
      //if (!file->d_size) continue;
      ticfile = (char *) smalloc(strlen(tmpdir)+strlen(file->d_name)+1);
      strcpy(ticfile, tmpdir);
      strcat(ticfile, file->d_name);
      if (stricmp(file->d_name+8, ".TIC") == 0) {
         memset(&tic,0,sizeof(tic));
         parseTic(ticfile,&tic);
         link = getLinkFromAddr(config, tic.to);
	 // createFlo doesn't  support ASO!!!
         //if (createFlo(link,cvtFlavour2Prio(link->fileEchoFlavour))==0) {
        if (createOutboundFileName(link, link->fileEchoFlavour, FLOFILE)==0) {
	     filearea=getFileArea(config,tic.area);
	     if (filearea!=NULL) {
               if (!filearea->pass && !filearea->sendorig) strcpy(newticedfile,filearea->pathName);
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
                  if (move_file(ticfile,newticfile)!=0) {
                     w_log('9',"File %s not found or not moveable",ticfile);
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
         remove(link->bsyFile);
         nfree(link->bsyFile);
         nfree(link->floFile);
         disposeTic(&tic);
      } /* if ".TIC" */
      nfree(ticfile);
   } /* while */
   closedir(dir);
}

int foundInArray(char **filesInTic, unsigned int filesCount, char *name)
{
    unsigned int i, rc = 0;

    for (i = 0; i < filesCount; i++)
        if (patimat(filesInTic[i],name) == 1) {
            rc = 1;
            break;
        }
    return rc;
}

void cleanPassthroughDir(void)
{
    DIR            *dir;
    struct dirent  *file;
    char           *ticfile;
    s_ticfile tic;
    char **filesInTic = NULL;
    unsigned int filesCount = 0, i;
    char tmpdir[256];

   w_log( LL_INFO, "Start clean (purging files in passthrough dir)...");

   /* check busyFileDir */
   strcpy(tmpdir, config->busyFileDir);
   *(strrchr(tmpdir,PATH_DELIM))=0;
   if (direxist(tmpdir)) {
      strcpy(tmpdir, config->busyFileDir);
      dir = opendir(tmpdir);
      if (dir != NULL) {
         while ((file = readdir(dir)) != NULL) {
            if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
            if (patimat(file->d_name, "*.TIC") == 1) {
               ticfile = (char *) smalloc(strlen(tmpdir)+strlen(file->d_name)+1);
               strcpy(ticfile, tmpdir);
               strcat(ticfile, file->d_name);
               memset(&tic,0,sizeof(tic));
               parseTic(ticfile,&tic);
               if (filesCount == 0 || (filesCount > 0 && !foundInArray(filesInTic,filesCount,tic.file))) {
                  filesInTic = srealloc(filesInTic, sizeof(char *)*(filesCount+1));
                  filesInTic[filesCount] = (char *) smalloc(strlen(tic.file)+1);
                  strcpy(filesInTic[filesCount], tic.file);
                  filesCount++;
               }
               disposeTic(&tic);
               nfree(ticfile);
            }
         }
         closedir(dir);
      }
   }

   /* check separateBundles dirs (does anybody use this?) */
   if (config->separateBundles) {
      for (i = 0; i < config->linkCount; i++) {
         
         if (createOutboundFileName(&(config->links[i]), normal, FLOFILE) == 0)  {

         strcpy(tmpdir, config->links[i].floFile);
         sprintf(strrchr(tmpdir, '.'), ".sep");
         if (direxist(tmpdir)) {
            sprintf(tmpdir+strlen(tmpdir), "%c", PATH_DELIM);
            dir = opendir(tmpdir);
            if (dir == NULL) continue;

            while ((file = readdir(dir)) != NULL) {
               if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
               ticfile = (char *) smalloc(strlen(tmpdir)+strlen(file->d_name)+1);
               strcpy(ticfile, tmpdir);
               strcat(ticfile, file->d_name);
               if (patimat(file->d_name, "*.TIC") == 1) {
                  memset(&tic,0,sizeof(tic));
                  parseTic(ticfile,&tic);
                  if (filesCount == 0 || (filesCount > 0 && !foundInArray(filesInTic,filesCount,tic.file))) {
                     filesInTic = srealloc(filesInTic, sizeof(char *)*(filesCount+1));
                     filesInTic[filesCount] = (char *) smalloc(strlen(tic.file)+1);
                     strcpy(filesInTic[filesCount], tic.file);
                     filesCount++;
                  }
                  disposeTic(&tic);
               }
               nfree(ticfile);
            } /* while */
            closedir(dir);
            remove(config->links[i].bsyFile);
            nfree(config->links[i].bsyFile);
            nfree(config->links[i].floFile);
         }
         }
      }
   }

   /* check ticOutbound */
   dir = opendir(config->ticOutbound);
   if (dir != NULL) {
      while ((file = readdir(dir)) != NULL) {
         if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
         if (patimat(file->d_name, "*.TIC") == 1) {
            ticfile = (char *) smalloc(strlen(config->passFileAreaDir)+strlen(file->d_name)+1);
            strcpy(ticfile, config->ticOutbound);
            strcat(ticfile, file->d_name);
            memset(&tic,0,sizeof(tic));
            parseTic(ticfile,&tic);
            if (filesCount == 0 || (filesCount > 0 && !foundInArray(filesInTic,filesCount,tic.file))) {
               filesInTic = srealloc(filesInTic, sizeof(char *)*(filesCount+1));
               filesInTic[filesCount] = (char *) smalloc(strlen(tic.file)+1);
               strcpy(filesInTic[filesCount], tic.file);
               filesCount++;
            }
            disposeTic(&tic);
            nfree(ticfile);
         }
      }
      closedir(dir);
   }

   /* purge passFileAreaDir */
   dir = opendir(config->passFileAreaDir);
   if (dir != NULL) {
      while ((file = readdir(dir)) != NULL) {
         if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
         if (patimat(file->d_name, "*.TIC") != 1) {
            ticfile = (char *) smalloc(strlen(config->passFileAreaDir)+strlen(file->d_name)+1);
            strcpy(ticfile, config->passFileAreaDir);
            strcat(ticfile, file->d_name);
            if (filesCount == 0) {
               w_log('6',"Remove file %s from passthrough dir", ticfile);
               remove(ticfile);
               nfree(ticfile);
               continue;
            }
            if (!foundInArray(filesInTic,filesCount,file->d_name)) {
               w_log('6',"Remove file %s from passthrough dir", ticfile);
               remove(ticfile);
            }
            nfree(ticfile);
         }
      }
      closedir(dir);
   }

   if (filesCount > 0) {
      for (i=0; i<filesCount; i++)
        nfree(filesInTic[i]);
      nfree(filesInTic);
   }
}

int putMsgInArea(s_area *echo, s_message *msg, int strip)
{
   char *ctrlBuff, *textStart, *textWithoutArea;
   UINT textLength = (UINT) msg->textLength;
   HAREA harea;
   HMSG  hmsg;
   XMSG  xmsg;
   int rc = 0;


   harea = MsgOpenArea((UCHAR *) echo->fileName, MSGAREA_CRIFNEC, echo->msgbType | MSGTYPE_ECHO);
   if (harea != NULL) {
      hmsg = MsgOpenMsg(harea, MOPEN_CREATE, 0);
      if (hmsg != NULL) {

         /* recode from TransportCharset to internal Charset */
         if (msg->recode == 0 && config->intab != NULL) {
            recodeToInternalCharset(msg->subjectLine);
            recodeToInternalCharset(msg->text);
            recodeToInternalCharset(msg->toUserName);
            recodeToInternalCharset(msg->fromUserName);
                        msg->recode = 1;
         }

         textWithoutArea = msg->text;

         if ((strncmp(msg->text, "AREA:", 5) == 0) && (strip==1)) {
            /* jump over AREA:xxxxx\r */
            while (*(textWithoutArea) != '\r') textWithoutArea++;
            textWithoutArea++;
         }

         ctrlBuff = (char *) CopyToControlBuf((UCHAR *) textWithoutArea,
                                              (UCHAR **) &textStart,
                                              &textLength);
         /* textStart is a pointer to the first non-kludge line */
         xmsg = createXMSG(msg);

         MsgWriteMsg(hmsg, 0, &xmsg, (byte *) textStart, (dword) strlen(textStart), (dword) strlen(textStart), (dword)strlen(ctrlBuff), (byte *)ctrlBuff);

         MsgCloseMsg(hmsg);
         nfree(ctrlBuff);
         rc = 1;

      } else {
         w_log( '9', "Could not create new msg in %s!", echo->fileName);
      } /* endif */
      MsgCloseArea(harea);
   } else {
      w_log( '9', "Could not open/create EchoArea %s!", echo->fileName);
   } /* endif */
   return rc;
}

void writeMsgToSysop(s_message *msg, char *areaName)
{
   s_area       *echo;

   xstrscat(&(msg->text), " \r--- ", versionStr,"\r * Origin: ", (config->name) ? config->name : "", " (", aka2str(msg->origAddr), ")\r", NULL);
   msg->textLength = strlen(msg->text);
   if (msg->netMail == 1) writeNetmail(msg, areaName);
   else {
      echo = getArea(config, areaName);
      if (echo != &(config->badArea)) {
         if (echo->msgbType != MSGTYPE_PASSTHROUGH) {
            msg->recode = 1;
            putMsgInArea(echo, msg,1);
            echo->imported = 1;  /* area has got new messages */
            w_log( '7', "Post report message to %s area", echo->areaName);
         }
      } else {
/*         putMsgInBadArea(msgToSysop[i], msgToSysop[i]->origAddr, 0); */
      }
   }

}

char *formDescStr(char *desc)
{
   char *keepDesc, *newDesc, *tmp, *ch, *buff=NULL;

   keepDesc = sstrdup(desc);

   if (strlen(desc) <= 50) {
      return keepDesc;
   }

   newDesc = (char*)scalloc(1, sizeof(char));

   tmp = keepDesc;

   ch = strtok(tmp, " \t\r\n");
   while (ch) {
      if (strlen(ch) > 54 && !buff) {
		  newDesc = (char*)srealloc(newDesc, strlen(newDesc)+55);
		  strncat(newDesc, ch, 54);
		  xstrscat(&newDesc, "\r", print_ch(24, ' '), NULL);
		  ch += 54;
      } else {
		  if (buff && strlen(buff)+strlen(ch) > 54) {
			  xstrscat(&newDesc, buff, "\r", print_ch(24, ' '), NULL);
			  nfree(buff);
		  } else {
			  xstrscat(&buff, ch, " ", NULL);
			  ch = strtok(NULL, " \t\r\n");
		  }
      }
   }
   if (buff && strlen(buff) != 0) {
	   xstrcat(&newDesc, buff);
   } nfree (buff);

   nfree(keepDesc);

   return newDesc;
}

char *formDesc(char **desc, int count)
{
	char *buff=NULL, *tmp;
	int i;

	for (i = 0; i < count; i++ ) {
		tmp = formDescStr(desc[i]);
		if (i == 0) {
			xstrscat(&buff, tmp , "\r", NULL);
		} else {
			xstrscat(&buff, print_ch(24, ' '), tmp, "\r", NULL);
		}
		nfree(tmp);
	}
	return buff;
}

void freeFReport(s_newfilereport *report)
{
   unsigned int i;
   nfree(report->fileName);
   for (i = 0; i < report->filedescCount; i++) {
      nfree(report->fileDesc[i]);
   } /* endfor */
   nfree(report->fileDesc);
}

static int compare_filereport(const void *a, const void *b)
{
   const s_newfilereport* r1 = *(s_newfilereport**)a;
   const s_newfilereport* r2 = *(s_newfilereport**)b;

   if( ( r1->useAka->zone  > r2->useAka->zone ) &&
      ( r1->useAka->net   > r2->useAka->net   ) &&
      ( r1->useAka->node  > r2->useAka->node  ) &&
      ( r1->useAka->point > r2->useAka->point )     )
      return 1;
   else if( ( r1->useAka->zone  < r2->useAka->zone  ) &&
      ( r1->useAka->net   < r2->useAka->net   ) &&
      ( r1->useAka->node  < r2->useAka->node  ) &&
      ( r1->useAka->point < r2->useAka->point )    )
      return -1;
   else if( stricmp( r1->areaName, r2->areaName ) > 0)
      return 1;
   else if( stricmp( r1->areaName, r2->areaName ) < 0)
      return -1;
   else if( stricmp( r1->fileName, r2->fileName ) > 0)
      return 1;
   else if( stricmp( r1->fileName, r2->fileName ) < 0)
      return -1;
   return 0;
}

void reportNewFiles()
{
   int       b,  fileCount;
   unsigned  int i, c;
   UINT32    fileSize;
   s_message *msg;
   char      buff[256], *tmp;
   FILE      *echotosslog;
   char      *annArea;
   int       areaLen;

   if (cmAnnounce) annArea = announceArea;
   else if (config->ReportTo != NULL && (!cmAnnFile)) annArea = config->ReportTo;
        else return;

   // sort newFileReport
   qsort( (void*)newFileReport, newfilesCount, sizeof(s_newfilereport*), 
       compare_filereport); 
   /* post report about new files to annArea */
   for (c = 0; c < config->addrCount && newfilesCount != 0; c++) {
      msg = NULL;
      for (i = 0; i < newfilesCount; i++) {
         if (newFileReport[i] != NULL) {
            if (newFileReport[i]->useAka == &(config->addr[c])) {
               if (msg == NULL) {
                  if (getNetMailArea(config, annArea) != NULL) {
                     msg = makeMessage(newFileReport[i]->useAka,
                        newFileReport[i]->useAka,
                        versionStr, config->sysop, "New Files", 1);
                     msg->text = createKludges(NULL, newFileReport[i]->useAka, newFileReport[i]->useAka);
                  } else {
                     msg = makeMessage(newFileReport[i]->useAka,
                        newFileReport[i]->useAka,
                        versionStr, "All", "New Files", 0);
                     msg->text = createKludges(annArea, newFileReport[i]->useAka, newFileReport[i]->useAka);
                  } /* endif */
                  xstrcat(&(msg->text), " ");
               } /* endif */

               fileCount = fileSize = 0;
               areaLen = strlen(newFileReport[i]->areaName);
               sprintf(buff, "\r>Area : %s%s", newFileReport[i]->areaName,
                                               print_ch(((areaLen<=15) ? 25 : areaLen+10), ' '));
               sprintf(buff+((areaLen<=15) ? 25 : areaLen+10), "Desc : %s\r %s\r",
                    (newFileReport[i]->areaDesc) ? newFileReport[i]->areaDesc : "",
                    print_ch(77, '-'));
               msg->text = (char*)srealloc(msg->text, strlen(msg->text)+strlen(buff)+1);
               strcat(msg->text, buff);
               sprintf(buff, " %s%s", strUpper(newFileReport[i]->fileName), print_ch(25, ' '));
               tmp = formDesc(newFileReport[i]->fileDesc, newFileReport[i]->filedescCount);
               sprintf(buff+14, "% 9i", newFileReport[i]->fileSize);
               msg->text = (char*)srealloc(msg->text, strlen(msg->text)+strlen(buff)+strlen(tmp)+2);
               sprintf(msg->text+strlen(msg->text), "%s %s", buff, tmp);
               if (config->originInAnnounce) {
                  msg->text = (char*)srealloc(msg->text, strlen(msg->text)+75);
                  sprintf(msg->text+strlen(msg->text), "%sOrig: %u:%u/%u.%u\r",print_ch(24, ' '),
                          newFileReport[i]->origin.zone,newFileReport[i]->origin.net,
                          newFileReport[i]->origin.node,newFileReport[i]->origin.point);
               }
               if (tmp == NULL || tmp[0] == 0) strcat(msg->text,"\r");
               nfree(tmp);
               fileCount++;
               fileSize += newFileReport[i]->fileSize;
               for (b = i+1; b < newfilesCount; b++) {
                  if (newFileReport[b] != NULL && newFileReport[i] != NULL &&
                      newFileReport[b]->useAka == &(config->addr[c]) &&
                      stricmp(newFileReport[i]->areaName,
                      newFileReport[b]->areaName) == 0) {
                      sprintf(buff, " %s%s", strUpper(newFileReport[b]->fileName), print_ch(25, ' '));
                      tmp = formDesc(newFileReport[b]->fileDesc, newFileReport[b]->filedescCount);
                      sprintf(buff+14, "% 9i", newFileReport[b]->fileSize);
                      msg->text = (char*)srealloc(msg->text, strlen(msg->text)+strlen(buff)+strlen(tmp)+2);
                      sprintf(msg->text+strlen(msg->text), "%s %s", buff, tmp);
                      if (config->originInAnnounce) {
                         msg->text = (char*)srealloc(msg->text, strlen(msg->text)+75);
                         sprintf(msg->text+strlen(msg->text), "%sOrig: %u:%u/%u.%u\r",print_ch(24, ' '),
                                 newFileReport[b]->origin.zone,newFileReport[b]->origin.net,
                                 newFileReport[b]->origin.node,newFileReport[b]->origin.point);
                      }
                      if (tmp == NULL || tmp[0] == 0) strcat(msg->text,"\r");
                      nfree(tmp);
                      fileCount++;
                      fileSize += newFileReport[b]->fileSize;
                      freeFReport(newFileReport[b]); newFileReport[b] = NULL;
                  } else {
                  } /* endif */
               } /* endfor */
               sprintf(buff, " %s\r", print_ch(77, '-'));
               msg->text = (char*)srealloc(msg->text, strlen(msg->text)+strlen(buff)+80);
               sprintf(msg->text+strlen(msg->text), "%s %u bytes in %u file(s)\r", buff, fileSize, fileCount);
               freeFReport(newFileReport[i]); newFileReport[i] = NULL;
            } else {
            } /* endif */
         } /* endif */
      } /* endfor */
      if (msg) {
         writeMsgToSysop(msg, annArea);
         freeMsgBuff(msg);
         nfree(msg);
      } else {
      } /* endif */
   } /* endfor */
   if (newFileReport) {
      nfree(newFileReport);
      if (config->echotosslog != NULL) {
         echotosslog = fopen (config->echotosslog, "a");
         if (echotosslog != NULL) {
            fprintf(echotosslog,"%s\n",annArea);
            fclose(echotosslog);
         }
      }
   }
}

void toss()
{

   w_log( LL_INFO, "Start tossing...");

   processDir(config->localInbound, secLocalInbound);
   processDir(config->protInbound, secProtInbound);
   processDir(config->inbound, secInbound);

   reportNewFiles();
}
