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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#ifdef __EMX__
#include <sys/types.h>
#endif
#include <sys/stat.h>
#if !(defined(_MSC_VER) && (_MSC_VER >= 1200))
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

#include <smapi/msgapi.h>
#include <smapi/typedefs.h>
#include <smapi/compiler.h>
#include <pkt.h>
#include <areafix.h>
#include <version.h>
#include <smapi/ffind.h>

#include <smapi/stamp.h>
#include <smapi/progprot.h>

#include <add_descr.h>
#include <seenby.h>
#include <recode.h>
#include <crc32.h>
#include <filecase.h>

s_newfilereport **newFileReport = NULL;
unsigned newfilesCount = 0;

void changeFileSuffix(char *fileName, char *newSuffix) {

   int   i = 1;
   char  buff[200];

   char *beginOfSuffix = strrchr(fileName, '.')+1;
   char *newFileName;
   int  length = strlen(fileName)-strlen(beginOfSuffix)+strlen(newSuffix);

   newFileName = (char *) calloc(length+1+2, 1);
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
      writeLogEntry(htick_log, '9', "Could not change suffix for %s. File already there and the 255 files after", fileName);
   }
}

int to_us(const s_addr destAddr)
{
   int i = 0;

   while (i < config->addrCount)
     if (addrComp(destAddr, config->addr[i++]) == 0)
       return 0;
   return !0;
}

void createKludges(char *buff, const char *area, const s_addr *ourAka, const s_addr *destAka) 
{                                   
                                                                                
   *buff = 0;                                                                   
   if (area != NULL)                                                            
      sprintf(buff + strlen(buff), "AREA:%s\r", area);                          
   else {                                                                       
      if (ourAka->point) sprintf(buff + strlen(buff), "\1FMPT %d\r",
          ourAka->point);                                                                            
      if (destAka->point) sprintf(buff + strlen(buff), "\1TOPT %d\r",
          destAka->point);                                                                          
   };                                                                           
                                                                                
   if (ourAka->point)                                                           
      sprintf(buff + strlen(buff),"\1MSGID: %u:%u/%u.%u %08lx\r",               
          ourAka->zone,ourAka->net,ourAka->node,ourAka->point,(unsigned long) time(NULL));  
   else                                                                         
      sprintf(buff + strlen(buff),"\1MSGID: %u:%u/%u %08lx\r",                  
              ourAka->zone,ourAka->net,ourAka->node,(unsigned long) time(NULL));                
                                                                                
   sprintf(buff + strlen(buff), "\1PID: %s\r", versionStr);                     
}                                                                               


XMSG createXMSG(s_message *msg)
{
	XMSG  msgHeader;
	struct tm *date;
	time_t    currentTime;
	union stamp_combo dosdate;
	int i,remapit;
        char *subject;
	
	if (msg->netMail == 1) {
		/* attributes of netmail must be fixed */
		msgHeader.attr = msg->attributes;
		
		if (to_us(msg->destAddr)==0) {
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
       subject = (char *) malloc (size);
       sprintf (subject,"%s%s",config->inbound,msg->subjectLine);
     }
   }
   strcpy((char *) msgHeader.subj,subject);
   if (subject != msg->subjectLine)
     free(subject);
       
   msgHeader.orig.zone  = msg->origAddr.zone;
   msgHeader.orig.node  = msg->origAddr.node;
   msgHeader.orig.net   = msg->origAddr.net;
   msgHeader.orig.point = msg->origAddr.point;
   msgHeader.dest.zone  = msg->destAddr.zone;
   msgHeader.dest.node  = msg->destAddr.node;
   msgHeader.dest.net   = msg->destAddr.net;
   msgHeader.dest.point = msg->destAddr.point;

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
   char *slash;
   s_area *nmarea;
#ifdef UNIX
   char limiter = '/';
#else
   char limiter = '\\';
#endif

   if ((nmarea=getNetMailArea(config, areaName))==NULL) nmarea = &(config->netMailAreas[0]);
   /* create Directory Tree if necessary */
   if (nmarea->msgbType == MSGTYPE_SDM)
      createDirectoryTree(nmarea->fileName);
   else {
      /* squish area */
      slash = strrchr(nmarea->fileName, limiter);
      *slash = '\0';
      createDirectoryTree(nmarea->fileName);
      *slash = limiter;
   }

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

         writeLogEntry(htick_log, '6', "Wrote Netmail to: %u:%u/%u.%u",
                 msg->destAddr.zone, msg->destAddr.net, msg->destAddr.node, msg->destAddr.point);
      } else {
         writeLogEntry(htick_log, '9', "Could not write message to %s", areaName);
      } /* endif */

      MsgCloseArea(netmail);
   } else {
/*     printf("%u\n", msgapierr); */
      writeLogEntry(htick_log, '9', "Could not open netmailarea %s", areaName);
   } /* endif */
}


char *addr2string(s_addr *addr)
{
   static char hlp[50];

   if (addr->point==0)
      sprintf(hlp,"%u:%u/%u",addr->zone,addr->net,addr->node);
     else
    sprintf(hlp,"%u:%u/%u.%u",addr->zone,addr->net,addr->node,addr->point);

   return(hlp);
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
      fprintf(tichandle,"From %s\r\n",addr2string(&tic->from));
   if (tic->to.zone!=0)
      fprintf(tichandle,"To %s\r\n",addr2string(&tic->to));
   if (tic->origin.zone!=0)
      fprintf(tichandle,"Origin %s\r\n",addr2string(&tic->origin));
   if (tic->size!=0)
      fprintf(tichandle,"Size %u\r\n",tic->size);
   if (tic->date!=0)
      fprintf(tichandle,"Date %lu\r\n",tic->date);
   if (tic->crc!=0)
      fprintf(tichandle,"Crc %lX\r\n",tic->crc);

   for (i=0;i<tic->anzpath;i++)
       fprintf(tichandle,"Path %s\r\n",tic->path[i]);
  
   for (i=0;i<tic->anzseenby;i++)
       fprintf(tichandle,"Seenby %s\r\n",addr2string(&tic->seenby[i]));
  
   if (tic->password[0]!=0)
      fprintf(tichandle,"Pw %s\r\n",tic->password);

     fclose(tichandle);
}

void disposeTic(s_ticfile *tic)
{
  int i;

  if (tic->seenby!=NULL)
     free(tic->seenby);

  if (tic->path!=NULL)
     { 
     for (i=0;i<tic->anzpath;i++)
         free(tic->path[i]);
     free(tic->path);
     }

  if (tic->desc!=NULL)
     { 
     for (i=0;i<tic->anzdesc;i++)
         free(tic->desc[i]);
     free(tic->desc);
     }

  if (tic->ldesc!=NULL)
     { 
     for (i=0;i<tic->anzldesc;i++)
         free(tic->ldesc[i]);
     free(tic->ldesc);
     }
}

int parseTic(char *ticfile,s_ticfile *tic)
{
   FILE *tichandle;   
   char *line, *token, *param, *linecut = "";

   tichandle=fopen(ticfile,"r");
   memset(tic,0,sizeof(*tic));

   while ((line = readLine(tichandle)) != NULL) {
      line = trimLine(line);

      if (*line!=0 || *line!=10 || *line!=13 || *line!=';' || *line!='#') {
         if (config->MaxTicLineLength) {
            linecut = (char *) malloc(config->MaxTicLineLength+1);
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
               realloc(tic->desc,(tic->anzdesc+1)*sizeof(*tic->desc));
               tic->desc[tic->anzdesc]=strdup(param);
               tic->anzdesc++;
            }
            else if (stricmp(token,"replaces")==0) strcpy(tic->replaces,param);
            else if (stricmp(token,"pw")==0) strcpy(tic->password,param);
            else if (stricmp(token,"size")==0) tic->size=atoi(param);
            else if (stricmp(token,"crc")==0) tic->crc = strtoul(param,NULL,16);
            else if (stricmp(token,"date")==0) tic->date=atoi(param);
            else if (stricmp(token,"from")==0) string2addr(param,&tic->from);
            else if (stricmp(token,"to")==0 || stricmp(token,"Destination")==0)
               string2addr(param,&tic->to);
            else if (stricmp(token,"origin")==0) string2addr(param,&tic->origin);
            else if (stricmp(token,"magic")==0);
            else if (stricmp(token,"seenby")==0) {
               tic->seenby=
                  realloc(tic->seenby,(tic->anzseenby+1)*sizeof(s_addr));
               string2addr(param,&tic->seenby[tic->anzseenby]);
               tic->anzseenby++;
            }
            else if (stricmp(token,"path")==0) {
               tic->path=
                  realloc(tic->path,(tic->anzpath+1)*sizeof(*tic->path));
               tic->path[tic->anzpath]=strdup(param);
               tic->anzpath++;
            }
            else if (stricmp(token,"ldesc")==0) {
               tic->ldesc=
                  realloc(tic->ldesc,(tic->anzldesc+1)*sizeof(*tic->ldesc));
               tic->ldesc[tic->anzldesc]=strdup(param);
               tic->anzldesc++;
            }
            else {
/*               printf("Unknown Keyword %s in Tic File\n",token); */
               writeLogEntry(htick_log, '7', "Unknown Keyword %s in Tic File",token);
            }
         } /* endif */
         if (config->MaxTicLineLength) free(linecut);
      } /* endif */
      free(line);
   } /* endwhile */

  fclose(tichandle);

  return(1);
}

int autoCreate(char *c_area, s_addr pktOrigAddr, char *desc)
{
   FILE *f;
   char *NewAutoCreate;
   char *fileName;
   char *fileechoFileName;
   char buff[255], myaddr[20], hisaddr[20];
   int i=0;
   s_link *creatingLink;
   s_addr *aka;
   s_message *msg;
   s_filearea *area;
   FILE *echotosslog;

   fileechoFileName = (char *) malloc(strlen(c_area)+1);
   strcpy(fileechoFileName, c_area);

   /* translating path of the area to lowercase, much better imho. */
   while (*fileechoFileName != '\0') {
      *fileechoFileName=tolower(*fileechoFileName);
      if ((*fileechoFileName=='/') || (*fileechoFileName=='\\')) *fileechoFileName = '_'; /* convert any path elimiters to _ */
      fileechoFileName++;
      i++;
   }

   while (i>0) {
      fileechoFileName--;
      i--;
   }

   creatingLink = getLinkFromAddr(*config, pktOrigAddr);

   fileName = creatingLink->autoFileCreateFile;
   if (fileName == NULL) fileName = getConfigFileName();

   f = fopen(fileName, "a");
   if (f == NULL) {
      f = fopen(getConfigFileName(), "a");
      if (f == NULL)
         {
            fprintf(stderr,"autocreate: cannot open config file\n");
            return 1;
         }
   }

   aka = creatingLink->ourAka;

   /* making local address and address of uplink */
   strcpy(myaddr,addr2string(aka));
   strcpy(hisaddr,addr2string(&pktOrigAddr));

   /* write new line in config file */
                            
   sprintf(buff, "FileArea %s %s%s -a %s ",
           c_area,
           config->fileAreaBaseDir,
           (stricmp(config->fileAreaBaseDir,"Passthrough") == 0) ? "" : fileechoFileName,
           myaddr);

   if (creatingLink->autoFileCreateDefaults) {
      NewAutoCreate=(char *) calloc (strlen(creatingLink->autoFileCreateDefaults)+1, sizeof(char));
      strcpy(NewAutoCreate,creatingLink->autoFileCreateDefaults);
   } else NewAutoCreate = (char*)calloc(1, sizeof(char));

   if ((fileName=strstr(NewAutoCreate,"-d "))==NULL) {
     if (desc[0] != 0) {
       char *tmp;
       tmp=(char *) calloc (strlen(NewAutoCreate)+strlen(desc)+6,sizeof(char));
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       free (NewAutoCreate);
       NewAutoCreate=tmp;
     }
   } else {
     if (desc[0] != 0) {
       char *tmp;
       tmp=(char *) calloc (strlen(NewAutoCreate)+strlen(desc)+6,sizeof(char));
       fileName[0]='\0';
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       fileName++;
       fileName=strrchr(fileName,'\"')+1; //"
       strcat(tmp,fileName);
       free(NewAutoCreate);
       NewAutoCreate=tmp;
     }
   }

   if ((NewAutoCreate != NULL) && (strlen(buff)+strlen(NewAutoCreate))<255) {
      strcat(buff, NewAutoCreate);
   }

   free(NewAutoCreate);

   sprintf (buff+strlen(buff)," %s",hisaddr);

   fprintf(f, "%s\n", buff);

   fclose(f);

   /* add new created echo to config in memory */
   parseLine(buff,config);

   writeLogEntry(htick_log, '8', "FileArea '%s' autocreated by %s", c_area, hisaddr);

   /* report about new filearea */
   if (config->ReportTo && !cmAnnNewFileecho && (area = getFileArea(config, c_area)) != NULL) {
      if (getNetMailArea(config, config->ReportTo) != NULL) {
         msg = makeMessage(area->useAka, area->useAka, versionStr, config->sysop, "Created new fileareas", 1);
         msg->text = (char *)calloc(300, sizeof(char));
         createKludges(msg->text, NULL, area->useAka, area->useAka);
      } else {
         msg = makeMessage(area->useAka, area->useAka, versionStr, "All", "Created new fileareas", 0);
         msg->text = (char *)calloc(300, sizeof(char));
         createKludges(msg->text, config->ReportTo, area->useAka, area->useAka);
      } /* endif */
      sprintf(buff, "\r \rNew filearea: %s\r\rDescription : %s\r", area->areaName, 
          (area->description) ? area->description : "");
      msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+1);
      strcat(msg->text, buff);
      writeMsgToSysop(msg, config->ReportTo);
      freeMsgBuff(msg);
      free(msg);
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

  int i;

  for (i=0; i<echo->downlinkCount; i++)
  {
    if (link == echo->downlinks[i]->link) break;
  }

  if (i == echo->downlinkCount) return 4;

  /* pause */
  if (link->Pause && !echo->noPause) return 5;

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

  int i;

  s_link *link;

  if (!addrComp(*aka,*echo->useAka)) return 0;

  link = getLinkFromAddr (*config,*aka);
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

int createFlo(s_link *link, e_prio prio)
{

    FILE *f; /* bsy file for current link */
    char name[13], bsyname[13], zoneSuffix[6], pntDir[14];

   if (link->hisAka.point != 0) {
      sprintf(pntDir, "%04x%04x.pnt%c", link->hisAka.net, link->hisAka.node, PATH_DELIM);
      sprintf(name, "%08x.flo", link->hisAka.point);
   } else {
      pntDir[0] = 0;
      sprintf(name, "%04x%04x.flo", link->hisAka.net, link->hisAka.node);
   }

   if (link->hisAka.zone != config->addr[0].zone) {
      /* add suffix for other zones */
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

   /* create floFile */
   link->floFile = (char *) malloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+strlen(name)+1);
   link->bsyFile = (char *) malloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+strlen(name)+1);
   strcpy(link->floFile, config->outbound);
   if (zoneSuffix[0] != 0) strcpy(link->floFile+strlen(link->floFile)-1, zoneSuffix);
   strcat(link->floFile, pntDir);
   createDirectoryTree(link->floFile); /* create directoryTree if necessary */
   strcpy(link->bsyFile, link->floFile);
   strcat(link->floFile, name);

   /* create bsyFile */
   strcpy(bsyname, name);
   bsyname[9]='b';bsyname[10]='s';bsyname[11]='y';
   strcat(link->bsyFile, bsyname);

   /* maybe we have session with this link? */
   if (fexist(link->bsyFile)) {
      free (link->bsyFile); link->bsyFile = NULL;
      return 1;
   } else {
      if ((f=fopen(link->bsyFile,"a")) == NULL) {
         fprintf(stderr,"cannot create *.bsy file for %s\n",addr2string(&link->hisAka));
         remove(link->bsyFile);
         free(link->bsyFile);
         link->bsyFile=NULL;
         free(link->floFile);
         if (config->lockfile != NULL) remove(config->lockfile);
         writeLogEntry(htick_log, '9', "cannot create *.bsy file");
         writeLogEntry(htick_log, '1', "End");
         closeLog(htick_log);
         disposeConfig(config);
         exit(1);
      }
      fclose(f);
   }
   return 0;
}

void doSaveTic(char *ticfile,s_ticfile *tic)
{
   char filename[256];
   int i;
   s_savetic *savetic;

   for (i = 0; i< config->saveTicCount; i++)
     {
       savetic = &(config->saveTic[i]);
       if (patimat(tic->area,savetic->fileAreaNameMask)==1) {
          writeLogEntry(htick_log,'6',"Saving Tic-File %s to %s",strrchr(ticfile,PATH_DELIM) + 1,savetic->pathName);
          strcpy(filename,savetic->pathName);
          strcat(filename,strrchr(ticfile, PATH_DELIM) + 1);
          strLower(filename);
          if (copy_file(ticfile,filename)!=0) {
             writeLogEntry(htick_log,'9',"File %s not found or moveable",ticfile);
          };
          break;
       };

     };

   return;
}

int sendToLinks(int isToss, s_filearea *filearea, s_ticfile tic,
                char *filename)
/*
   if isToss == 1 - tossing
   else           - hatching
*/
{
   int i, z;
   char descr_file_name[256], newticedfile[256], fileareapath[256],
        linkfilepath[256];
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


   if (isToss) minLinkCount = 2; // uplink and downlink
   else minLinkCount = 1;        // only downlink

   if (!filearea->pass && !filearea->sendorig)
      strcpy(fileareapath,filearea->pathName);
   else
      strcpy(fileareapath,config->passFileAreaDir);
   p = strrchr(fileareapath,PATH_DELIM); 
   if(p) strLower(p+1);

   createDirectoryTree(fileareapath);

   if (tic.replaces && !filearea->pass ) {
      /* Delete old file */
      /* TODO: File Masks */
      strcpy(newticedfile,fileareapath);
      strcat(newticedfile,tic.replaces);
      adaptcase(newticedfile);

      if (remove(newticedfile)==0) {
         writeLogEntry(htick_log,'6',"Removed file %s one request",newticedfile);
      }
   }

   if (!filearea->pass || (filearea->pass && filearea->downlinkCount>=minLinkCount)) {

      strcpy(newticedfile,fileareapath);
      p = strrchr(newticedfile,PATH_DELIM); 
      if(p) strLower(p+1);
      strcat(newticedfile,MakeProperCase(tic.file));

      if (isToss)
         if (move_file(filename,newticedfile)!=0) {
             writeLogEntry(htick_log,'9',"File %s not found or moveable",filename);
             disposeTic(&tic);
             return(2);
         } else {
             writeLogEntry(htick_log,'6',"Moved %s to %s",filename,newticedfile);
         }
      else
         if (copy_file(filename,newticedfile)!=0) {
            writeLogEntry(htick_log,'9',"File %s not found or moveable",filename);
            disposeTic(&tic);
            return(2);
         } else {
            writeLogEntry(htick_log,'6',"Put %s to %s",filename,newticedfile);
         }
   }

   if (!filearea->pass) {
      strcpy(descr_file_name, filearea->pathName);
      strcat(descr_file_name, "files.bbs");
      adaptcase(descr_file_name);
      removeDesc(descr_file_name,tic.file);
      if (tic.anzldesc>0)
         add_description (descr_file_name, tic.file, tic.ldesc, tic.anzldesc);
      else
         add_description (descr_file_name, tic.file, tic.desc, tic.anzdesc);
   }

   if (cmAnnFile && !filearea->hide)
   {
      if (tic.anzldesc>0)
         announceInFile (announcefile, tic.file, tic.size, tic.area, tic.origin, tic.ldesc, tic.anzldesc);
      else
         announceInFile (announcefile, tic.file, tic.size, tic.area, tic.origin, tic.desc, tic.anzdesc);
   }

   if (filearea->downlinkCount>=minLinkCount) {
      /* Adding path & seenbys */
      time(&acttime);
      strcpy(timestr,asctime(gmtime(&acttime)));
      timestr[strlen(timestr)-1]=0;
      if (timestr[8]==' ') timestr[8]='0';

      sprintf(hlp,"%s %lu %s UTC %s",
              addr2string(filearea->useAka), (unsigned long) time(NULL), timestr,versionStr);

      tic.path=realloc(tic.path,(tic.anzpath+1)*sizeof(*tic.path));
      tic.path[tic.anzpath]=strdup(hlp);
      tic.anzpath++;

      if (isToss) {
         /* Save seenby structure */
         old_seenby = malloc(tic.anzseenby*sizeof(s_addr));
         memcpy(old_seenby,tic.seenby,tic.anzseenby*sizeof(s_addr));
         old_anzseenby = tic.anzseenby;
         memcpy(&old_from,&tic.from,sizeof(s_addr));
         memcpy(&old_to,&tic.to,sizeof(s_addr));
      }
   }

   for (i=0;i<filearea->downlinkCount;i++) {
      if (addrComp(tic.from,filearea->downlinks[i]->link->hisAka)!=0 && 
            (isToss && addrComp(tic.to,filearea->downlinks[i]->link->hisAka)!=0) &&
            addrComp(tic.origin,filearea->downlinks[i]->link->hisAka)!=0 &&
            seenbyComp (tic.seenby, tic.anzseenby,
            filearea->downlinks[i]->link->hisAka) != 0) {
         /* Adding Downlink to Seen-By */
         tic.seenby=realloc(tic.seenby,(tic.anzseenby+1)*sizeof(s_addr));
         memcpy(&tic.seenby[tic.anzseenby],
            &filearea->downlinks[i]->link->hisAka,
            sizeof(s_addr));
         tic.anzseenby++;
      }
   }

   /* Checking to whom I shall forward */
   for (i=0;i<filearea->downlinkCount;i++) {
      if (addrComp(old_from,filearea->downlinks[i]->link->hisAka)!=0 && 
            addrComp(old_to,filearea->downlinks[i]->link->hisAka)!=0 &&
            addrComp(tic.origin,filearea->downlinks[i]->link->hisAka)!=0) {
         /* Forward file to */

         readAccess = readCheck(filearea, filearea->downlinks[i]->link);
         switch (readAccess) {
         case 0: break;
         case 5:
            writeLogEntry(htick_log,'7',"Link %s paused",
                    addr2string(&filearea->downlinks[i]->link->hisAka));
            break;
         case 4:
            writeLogEntry(htick_log,'7',"Link %s not subscribe to File Area %s",
                    addr2string(&old_from), tic.area);
	    break;
         case 3:
            writeLogEntry(htick_log,'7',"Not export to link %s",
                    addr2string(&filearea->downlinks[i]->link->hisAka));
	    break;
         case 2:
            writeLogEntry(htick_log,'7',"Link %s no access level",
            addr2string(&filearea->downlinks[i]->link->hisAka));
	    break;
         case 1:
            writeLogEntry(htick_log,'7',"Link %s no access group",
            addr2string(&filearea->downlinks[i]->link->hisAka));
	    break;
         }

         if (readAccess == 0) {
            if (isToss && seenbyComp(old_seenby, old_anzseenby, filearea->downlinks[i]->link->hisAka) == 0) {
               writeLogEntry(htick_log,'7',"File %s already seenby %s",
                       tic.file,
                       addr2string(&filearea->downlinks[i]->link->hisAka));
            } else {
               memcpy(&tic.from,filearea->useAka,sizeof(s_addr));
               memcpy(&tic.to,&filearea->downlinks[i]->link->hisAka,
                      sizeof(s_addr));
               if (filearea->downlinks[i]->link->ticPwd!=NULL)
                  strcpy(tic.password,filearea->downlinks[i]->link->ticPwd);

               busy = 0;

               if (createOutboundFileName(filearea->downlinks[i]->link,
                   cvtFlavour2Prio(filearea->downlinks[i]->link->fileEchoFlavour),
                   FLOFILE)==1)
                  busy = 1;

               if (busy) {
                  writeLogEntry(htick_log, '7', "Save TIC in temporary dir");
                  /* Create temporary directory */
                  strcpy(linkfilepath,config->busyFileDir);
               } else {
                  if (config->separateBundles) {
                     strcpy(linkfilepath, filearea->downlinks[i]->link->floFile);
                     sprintf(strrchr(linkfilepath, '.'), ".sep%c", PATH_DELIM);
                  } else {
                     strcpy(linkfilepath, config->passFileAreaDir);
                  }
	       }
               createDirectoryTree(linkfilepath);

	       /* Don't create TICs for everybody */
	       if (!filearea->downlinks[i]->link->noTIC || busy) {
                 newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
                 writeTic(newticfile,&tic);   
	       } else newticfile = NULL;

               if (!busy) {
                  flohandle=fopen(filearea->downlinks[i]->link->floFile,"a");
                  fprintf(flohandle,"%s\n",newticedfile);
	       
	          if (newticfile != NULL) fprintf(flohandle,"^%s\n",newticfile);

                  fclose(flohandle);

                  remove(filearea->downlinks[i]->link->bsyFile);

                  writeLogEntry(htick_log,'6',"Forwarding %s for %s",
                          tic.file,
                          addr2string(&filearea->downlinks[i]->link->hisAka));
               }
               free(filearea->downlinks[i]->link->bsyFile);
	       filearea->downlinks[i]->link->bsyFile=NULL;
               free(filearea->downlinks[i]->link->floFile);
	       filearea->downlinks[i]->link->floFile=NULL;
            } /* if Seenby */
         } /* if readAccess == 0 */
      } /* Forward file */
   }

   if (!filearea->hide) {
   /* report about new files - if filearea not hidden */
      newFileReport = (s_newfilereport**)realloc(newFileReport, (newfilesCount+1)*sizeof(s_newfilereport*));
      newFileReport[newfilesCount] = (s_newfilereport*)calloc(1, sizeof(s_newfilereport));
      newFileReport[newfilesCount]->useAka = filearea->useAka;
      newFileReport[newfilesCount]->areaName = filearea->areaName;
      newFileReport[newfilesCount]->areaDesc = filearea->description;
      newFileReport[newfilesCount]->fileName = strdup(tic.file);
      if (config->originInAnnounce) {
         newFileReport[newfilesCount]->origin.zone = tic.origin.zone;
         newFileReport[newfilesCount]->origin.net = tic.origin.net;
         newFileReport[newfilesCount]->origin.node = tic.origin.node;
         newFileReport[newfilesCount]->origin.point = tic.origin.point;
      }

      if (tic.anzldesc>0) {
         newFileReport[newfilesCount]->fileDesc = (char**)calloc(tic.anzldesc, sizeof(char*));
         for (i = 0; i < tic.anzldesc; i++) {
            newFileReport[newfilesCount]->fileDesc[i] = strdup(tic.ldesc[i]);
            if (config->intab != NULL) recodeToInternalCharset(newFileReport[newfilesCount]->fileDesc[i]);
         } /* endfor */
         newFileReport[newfilesCount]->filedescCount = tic.anzldesc;
      } else {
         newFileReport[newfilesCount]->fileDesc = (char**)calloc(tic.anzdesc, sizeof(char*));
         for (i = 0; i < tic.anzdesc; i++) {
            newFileReport[newfilesCount]->fileDesc[i] = strdup(tic.desc[i]);
            if (config->intab != NULL) recodeToInternalCharset(newFileReport[newfilesCount]->fileDesc[i]);
         } /* endfor */
         newFileReport[newfilesCount]->filedescCount = tic.anzdesc;
      }

      newFileReport[newfilesCount]->fileSize = tic.size;

      newfilesCount++;
   }

   if (!isToss) reportNewFiles();

   /* execute external program */
   for (z = 0; z < config->execonfileCount; z++) {
     if (stricmp(filearea->areaName,config->execonfile[z].filearea) != 0) continue;
       if (patimat(tic.file,config->execonfile[z].filename) == 0) continue;
       else {
         comm = (char *) malloc(strlen(config->execonfile[z].command)+1
                                +(!filearea->pass ? strlen(filearea->pathName) : strlen(config->passFileAreaDir))
                                +strlen(tic.file)+1);
         if (comm == NULL) {
            writeLogEntry(htick_log, '9', "Exec failed - not enough memory");
            continue;
         }
         sprintf(comm,"%s %s%s",config->execonfile[z].command,
                                (!filearea->pass ? filearea->pathName : config->passFileAreaDir),
                                tic.file);
         writeLogEntry(htick_log, '6', "Executing %s", comm);
         if ((cmdexit = system(comm)) != 0) {
           writeLogEntry(htick_log, '9', "Exec failed, code %d", cmdexit);
         }
         free(comm);
       }                                         
   }

   if (isToss) free(old_seenby);
   return(0);
}

int processTic(char *ticfile, e_tossSecurity sec)                     
{
   s_ticfile tic;
   size_t j;
   FILE *flohandle;

   char ticedfile[256], linkfilepath[256];
   char *newticfile;
   s_filearea *filearea;
   s_link *from_link, *to_link;
   int busy;
   unsigned long crc;
   struct stat stbuf;
   int writeAccess;


#ifdef DEBUG_HPT
   printf("Ticfile %s\n",ticfile);
#endif

   writeLogEntry(htick_log,'6',"Processing Tic-File %s",ticfile);

   parseTic(ticfile,&tic);

   writeLogEntry(htick_log,'6',"File: %s Area: %s From: %s Orig: %u:%u/%u.%u",
           tic.file, tic.area, addr2string(&tic.from),
           tic.origin.zone,tic.origin.net,tic.origin.node,tic.origin.point);

   /* Security Check */
   from_link=getLinkFromAddr(*config,tic.from);
   if (from_link == NULL) {
      writeLogEntry(htick_log,'9',"Link for Tic From Adress '%s' not found",
              addr2string(&tic.from));
      disposeTic(&tic);
      return(1);
   }

   if (tic.password[0]!=0 && ((from_link->ticPwd==NULL) ||
       (stricmp(tic.password,from_link->ticPwd)!=0))) {
      writeLogEntry(htick_log,'9',"Wrong Password %s from %s",
              tic.password,addr2string(&tic.from));
      disposeTic(&tic);
      return(1);
   }

   if (tic.to.zone!=0) {
      if (to_us(tic.to)) {
         /* Forwarding tic and file to other link? */
         to_link=getLinkFromAddr(*config,tic.to);
         if ( (to_link != NULL) && (to_link->forwardPkts != fOff) ) {
	    if ( (to_link->forwardPkts==fSecure) && (sec != secProtInbound) && (sec != secLocalInbound) );
	    else { /* Forwarding */
	       busy = 0;
               if (createFlo(to_link, cvtFlavour2Prio(to_link->fileEchoFlavour))==0) {
        	  strcpy(linkfilepath,to_link->floFile);
        	  if (!busy) {
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
                        free(to_link->bsyFile);
                        free(to_link->floFile);
		     }
		  }
	       }
               doSaveTic(ticfile,&tic);
               disposeTic(&tic);
               return(0);
	    }
	 }
	 /* not to us and no forward */
         writeLogEntry(htick_log,'9',"Tic File adressed to %s, not to us",
                 addr2string(&tic.to));
         disposeTic(&tic);
         return(4);
      }
   }

   strcpy(ticedfile,ticfile);
   *(strrchr(ticedfile,PATH_DELIM)+1)=0;
   j = strlen(ticedfile);
   strcat(ticedfile,tic.file);
   adaptcase(ticedfile);
   strcpy(tic.file, ticedfile + j);

   /* Recieve file? */
   if (!fexist(ticedfile)) {
       writeLogEntry(htick_log,'6',"File %s, not received, wait",tic.file);
       disposeTic(&tic);
       return(5);
   }

   stat(ticedfile,&stbuf);
   tic.size = stbuf.st_size;

   filearea=getFileArea(config,tic.area);

   if (filearea==NULL && from_link->autoFileCreate) {
      autoCreate(tic.area,tic.from,tic.areadesc);
      filearea=getFileArea(config,tic.area);
   }

   if (filearea == NULL) {
      writeLogEntry(htick_log,'9',"Cannot open or create File Area %s",tic.area);
      fprintf(stderr,"Cannot open or create File Area %s !\n",tic.area);
      disposeTic(&tic);
      return(2);
   } 

   /* Check CRC Value and reject faulty files depending on noCRC flag */
   crc = filecrc32(ticedfile);
   if (filearea->noCRC) tic.crc = crc;
   else if (tic.crc != crc) {
        writeLogEntry(htick_log,'9',"Wrong CRC for file %s - in tic:%lx, need:%lx",tic.file,tic.crc,crc);
        disposeTic(&tic);
        return(3);
   }
     
   writeAccess = writeCheck(filearea,&tic.from);

   switch (writeAccess) {
   case 0: break;
   case 4:
      writeLogEntry(htick_log,'9',"Link %s not subscribed to File Area %s",
              addr2string(&tic.from), tic.area);
      disposeTic(&tic);
      return(3);
   case 3:
      writeLogEntry(htick_log,'9',"Not import from link %s",
              addr2string(&from_link->hisAka));
      disposeTic(&tic);
      return(3);
   case 2:
      writeLogEntry(htick_log,'9',"Link %s no access level",
      addr2string(&from_link->hisAka));
      disposeTic(&tic);
      return(3);
   case 1:
      writeLogEntry(htick_log,'9',"Link %s no access group",
      addr2string(&from_link->hisAka));
      disposeTic(&tic);
      return(3);
   }

   sendToLinks(1, filearea, tic, ticedfile);

   doSaveTic(ticfile,&tic);
   disposeTic(&tic);
   return(0);
}

void processDir(char *directory, e_tossSecurity sec)
{
   DIR            *dir;
   struct dirent  *file;
   char           *dummy;
   int            rc;
   int            ticFile;

   if (directory==NULL) return;

   dir = opendir(directory);

   while ((file = readdir(dir)) != NULL) {
#ifdef DEBUG_HPT
      printf("testing %s\n", file->d_name);
#endif

      ticFile = 0;

      dummy = (char *) malloc(strlen(directory)+strlen(file->d_name)+1);
      strcpy(dummy, directory);
      strcat(dummy, file->d_name);

      if (patimat(file->d_name, "*.TIC") == 1) {
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
      } /* if */
      free(dummy);
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
    s_ticfile tic;
    s_filearea *filearea;
    FILE *flohandle;
    int error = 0;

   writeLogEntry(htick_log,'6',"Checking tmp dir");
   strcpy(tmpdir, config->busyFileDir);
   dir = opendir(tmpdir);
   if (dir == NULL) return;

   while ((file = readdir(dir)) != NULL) {
      if (strlen(file->d_name) != 12) continue;
      ticfile = (char *) malloc(strlen(tmpdir)+strlen(file->d_name)+1);
      strcpy(ticfile, tmpdir);
      strcat(ticfile, file->d_name);
      if (stricmp(file->d_name+8, ".TIC") == 0) {
         memset(&tic,0,sizeof(tic));
         parseTic(ticfile,&tic);
         link = getLinkFromAddr(*config, tic.to);
         if (createFlo(link, cvtFlavour2Prio(link->fileEchoFlavour))==0) {
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
                     strcpy(newticfile,config->passFileAreaDir);
                  }
                  createDirectoryTree(newticfile);
                  strcat(newticfile,strrchr(ticfile,PATH_DELIM)+1);
                  if (move_file(ticfile,newticfile)!=0) {
                     writeLogEntry(htick_log,'9',"File %s not found or moveable",ticfile);
		     error = 1;
		  }
               } else remove(ticfile);
	       if (!error) {
                  flohandle=fopen(link->floFile,"a");
                  fprintf(flohandle,"%s\n",newticedfile);
                  if (!link->noTIC)
                     fprintf(flohandle,"^%s\n",newticfile);
                  fclose(flohandle);

                  writeLogEntry(htick_log,'6',"Forwarding save file %s for %s",
                     tic.file, addr2string(&link->hisAka));
               }
            } /* if filearea */
         } /* if createFlo */
         remove(link->bsyFile);
         free(link->bsyFile);
         free(link->floFile);
      } /* if ".TIC" */
      disposeTic(&tic);
      free(ticfile);
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
    unsigned int filesCount = 0, i, busy;
    char tmpdir[256];

   writeLogEntry(htick_log,'6',"Purging files in passthrough dir");

/* check busyFileDir */
   strcpy(tmpdir, config->busyFileDir);
   *(strrchr(tmpdir,PATH_DELIM))=0;
   if (direxist(tmpdir)) {
     strcpy(tmpdir, config->busyFileDir);
     dir = opendir(tmpdir);
     if (dir == NULL) return;
  
     while ((file = readdir(dir)) != NULL) {
        if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
        ticfile = (char *) malloc(strlen(tmpdir)+strlen(file->d_name)+1);
        strcpy(ticfile, tmpdir);
        strcat(ticfile, file->d_name);
        if (patimat(file->d_name, "*.TIC") == 1) {
           memset(&tic,0,sizeof(tic));
           parseTic(ticfile,&tic);
           if (filesCount == 0 || (filesCount > 0 && !foundInArray(filesInTic,filesCount,tic.file))) {
             filesInTic = realloc(filesInTic, sizeof(char *)*(filesCount+1));
 	     filesInTic[filesCount] = (char *) malloc(strlen(tic.file)+1);
             strcpy(filesInTic[filesCount], tic.file);
	     filesCount++;
  	   }
	   disposeTic(&tic);
        }
        free(ticfile);
     }
     closedir(dir);
   }

/* check separateBundles dirs (does anybody use this?) */
   if (config->separateBundles) {
      for (i = 0; i < config->linkCount; i++) {
         busy = 0;
         if (createOutboundFileName(&(config->links[i]), NORMAL, FLOFILE) == 1) 
            busy = 1;
         if (!busy) {
            strcpy(tmpdir, config->links[i].floFile);
            sprintf(strrchr(tmpdir, '.'), ".sep"); 
            if (direxist(tmpdir)) {
               sprintf(tmpdir+strlen(tmpdir), "%c", PATH_DELIM);
               dir = opendir(tmpdir);
               if (dir == NULL) continue;
   
               while ((file = readdir(dir)) != NULL) {
                  if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
                  ticfile = (char *) malloc(strlen(tmpdir)+strlen(file->d_name)+1);
                  strcpy(ticfile, tmpdir);
                  strcat(ticfile, file->d_name);
                  if (patimat(file->d_name, "*.TIC") == 1) {
                     memset(&tic,0,sizeof(tic));
                     parseTic(ticfile,&tic);
                     if (filesCount == 0 || (filesCount > 0 && !foundInArray(filesInTic,filesCount,tic.file))) {
                        filesInTic = realloc(filesInTic, sizeof(char *)*(filesCount+1));
                        filesInTic[filesCount] = (char *) malloc(strlen(tic.file)+1);
                        strcpy(filesInTic[filesCount], tic.file);
                        filesCount++;
                     }
                     disposeTic(&tic);
                  }
                  free(ticfile);
               }
               closedir(dir);
            }
            remove(config->links[i].bsyFile);
         } else 
            return;
      }
   }

/* check passFileAreaDir */
   dir = opendir(config->passFileAreaDir);
   if (dir == NULL) return;

   while ((file = readdir(dir)) != NULL) {
      if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
      ticfile = (char *) malloc(strlen(config->passFileAreaDir)+strlen(file->d_name)+1);
      strcpy(ticfile, config->passFileAreaDir);
      strcat(ticfile, file->d_name);
      if (patimat(file->d_name, "*.TIC") == 1) {
         memset(&tic,0,sizeof(tic));
         parseTic(ticfile,&tic);
         if (filesCount == 0 || (filesCount > 0 && !foundInArray(filesInTic,filesCount,tic.file))) {
	    filesInTic = realloc(filesInTic, sizeof(char *)*(filesCount+1));
	    filesInTic[filesCount] = (char *) malloc(strlen(tic.file)+1);
            strcpy(filesInTic[filesCount], tic.file);
	    filesCount++;
	 }
	 disposeTic(&tic);
      }
      free(ticfile);
   }
   closedir(dir);

/* purge passFileAreaDir */
   dir = opendir(config->passFileAreaDir);
   if (dir == NULL) return;

   while ((file = readdir(dir)) != NULL) {
      if (stricmp(file->d_name,".")==0 || stricmp(file->d_name,"..")==0) continue;
      ticfile = (char *) malloc(strlen(config->passFileAreaDir)+strlen(file->d_name)+1);
      strcpy(ticfile, config->passFileAreaDir);
      strcat(ticfile, file->d_name);
      if (patimat(file->d_name, "*.TIC") != 1) {
         if (filesCount == 0) {
            writeLogEntry(htick_log,'6',"Remove file %s from passthrough dir", ticfile);
            remove(ticfile);
	    free(ticfile);
	    continue;
         }
         if (!foundInArray(filesInTic,filesCount,file->d_name)) {
            writeLogEntry(htick_log,'6',"Remove file %s from passthrough dir", ticfile);
            remove(ticfile);
	 }
      }
      free(ticfile);
   }
   closedir(dir);

   if (filesCount > 0) {
      for (i=0; i<filesCount; i++)
	free(filesInTic[i]);
      free(filesInTic);
   }
}

int putMsgInArea(s_area *echo, s_message *msg, int strip)
{
   char *ctrlBuff, *textStart, *textWithoutArea;
   UINT textLength = (UINT) msg->textLength;
   HAREA harea;
   HMSG  hmsg;
   XMSG  xmsg;
   char *slash;
   int rc = 0;

   /* create Directory Tree if necessary */
   if (echo->msgbType == MSGTYPE_SDM)
      createDirectoryTree(echo->fileName);
   else {
      /* squish area */
      slash = strrchr(echo->fileName, PATH_DELIM);
      *slash = '\0';
      createDirectoryTree(echo->fileName);
      *slash = PATH_DELIM;
   }

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
         free(ctrlBuff);
	 rc = 1;

      } else {
         writeLogEntry(htick_log, '9', "Could not create new msg in %s!", echo->fileName);
      } /* endif */
      MsgCloseArea(harea);
   } else {
      writeLogEntry(htick_log, '9', "Could not open/create EchoArea %s!", echo->fileName);
   } /* endif */
   return rc;
}

void writeMsgToSysop(s_message *msg, char *areaName)
{
   char tmp[81];
   s_area	*echo;

   sprintf(tmp, " \r--- %s\r * Origin: %s (%s)\r", versionStr, config->name, addr2string(&(msg->origAddr)));
   msg->text = (char*)realloc(msg->text, strlen(tmp)+strlen(msg->text)+1);
   strcat(msg->text, tmp);
   msg->textLength = strlen(msg->text);
   if (msg->netMail == 1) writeNetmail(msg, areaName);
   else {
      echo = getArea(config, areaName);
      if (echo != &(config->badArea)) {
         if (echo->msgbType != MSGTYPE_PASSTHROUGH) {
	    msg->recode = 1;
            putMsgInArea(echo, msg,1);
            echo->imported = 1;  /* area has got new messages */
            writeLogEntry(htick_log, '7', "Post report message to %s area", echo->areaName);
         }
      } else {
/*         putMsgInBadArea(msgToSysop[i], msgToSysop[i]->origAddr, 0); */
      }
   }
    
}

char *formDescStr(char *desc)
{
   char *keepDesc, *newDesc, *tmp, *ch, buff[100];

   keepDesc = strdup(desc);

   if (strlen(desc) <= 50) {
      return keepDesc;
   } /* endif */

   newDesc = (char*)calloc(1, sizeof(char));

   tmp = keepDesc;

   ch = strtok(tmp, " \t\r\n");
   memset(buff, 0, sizeof(buff));
   while (ch) {
      if (strlen(ch) > 50) {
         newDesc = (char*)realloc(newDesc, 50+27);
         strncat(newDesc, ch, 50);
         sprintf(newDesc+strlen(newDesc), "\r%s", print_ch(24, ' '));
         ch += 50;
      } else {
         if (strlen(buff)+strlen(ch) > 50) {
            newDesc = (char*)realloc(newDesc, strlen(newDesc)+strlen(buff)+27);
            sprintf(newDesc+strlen(newDesc), "%s\r%s", buff, print_ch(24, ' '));
            memset(buff, 0, sizeof(buff));
         } else {
            sprintf(buff+strlen(buff), "%s ", ch);
            ch = strtok(NULL, " \t\r\n");
         } /* endif */
      } /* endif */
   } /* endwhile */
   if (strlen(buff) != 0) {
      newDesc = (char*)realloc(newDesc, strlen(newDesc)+strlen(buff)+1);
      strcat(newDesc, buff);
   } /* endif */

   free(keepDesc);

   return newDesc;
}

char *formDesc(char **desc, int count)
{
   char *buff, *tmp;
   int i;

   buff = (char*)calloc(1, sizeof(char));
   for (i = 0; i < count; i++ ) {
      tmp = formDescStr(desc[i]);
      buff = (char*)realloc(buff, strlen(buff)+strlen(tmp)+26);
      if (i == 0) {
         sprintf(buff+strlen(buff), "%s\r", tmp);
      } else {
         sprintf(buff+strlen(buff), "%s%s\r", print_ch(24, ' '), tmp);
      } /* endif */
      free(tmp);
   } /* endfor */

   return buff;
}

void freeFReport(s_newfilereport *report)
{
   int i;
   free(report->fileName);
   for (i = 0; i < report->filedescCount; i++) {
      free(report->fileDesc[i]);
   } /* endfor */
   free(report->fileDesc);
}

void reportNewFiles()
{
   int       b, c, i, fileCount;
   UINT32    fileSize;
   s_message *msg;
   char      buff[256], *tmp;
   FILE      *echotosslog;
   char      *annArea;
   int       areaLen;

   if (cmAnnounce) annArea = announceArea;
   else if (config->ReportTo != NULL && (!cmAnnFile)) annArea = config->ReportTo;
        else return;

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
                     msg->text = (char*)calloc(300, sizeof(char));
                     createKludges(msg->text, NULL, newFileReport[i]->useAka, newFileReport[i]->useAka);
                  } else {
                     msg = makeMessage(newFileReport[i]->useAka,
                        newFileReport[i]->useAka,
                        versionStr, "All", "New Files", 0);
                     msg->text = (char*)calloc(300, sizeof(char));
                     createKludges(msg->text, annArea, newFileReport[i]->useAka, newFileReport[i]->useAka);
                  } /* endif */
                  strcat(msg->text, " ");
               } /* endif */

               fileCount = fileSize = 0;
	       areaLen = strlen(newFileReport[i]->areaName);
               sprintf(buff, "\r Area : %s%s", newFileReport[i]->areaName,
                                               print_ch(((areaLen<=15) ? 25 : areaLen+10), ' '));
               sprintf(buff+((areaLen<=15) ? 25 : areaLen+10), "Desc : %s\r %s\r",
	    	    (newFileReport[i]->areaDesc) ? newFileReport[i]->areaDesc : "",
            	    print_ch(77, '-'));
               msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+1);
               strcat(msg->text, buff);
               sprintf(buff, " %s%s", strUpper(newFileReport[i]->fileName), print_ch(25, ' '));
               tmp = formDesc(newFileReport[i]->fileDesc, newFileReport[i]->filedescCount);
               sprintf(buff+14, "% 9i", newFileReport[i]->fileSize);
               msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+strlen(tmp)+2);
               sprintf(msg->text+strlen(msg->text), "%s %s", buff, tmp);
               if (config->originInAnnounce) {
                  msg->text = (char*)realloc(msg->text, strlen(msg->text)+75);
                  sprintf(msg->text+strlen(msg->text), "%sOrig: %u:%u/%u.%u\r",print_ch(24, ' '),
                          newFileReport[i]->origin.zone,newFileReport[i]->origin.net,
                          newFileReport[i]->origin.node,newFileReport[i]->origin.point);
	       }
	       if (tmp == NULL || tmp[0] == 0) strcat(msg->text,"\r");
               free(tmp);
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
                      msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+strlen(tmp)+2);
                      sprintf(msg->text+strlen(msg->text), "%s %s", buff, tmp);
                      if (config->originInAnnounce) {
                         msg->text = (char*)realloc(msg->text, strlen(msg->text)+75);
                         sprintf(msg->text+strlen(msg->text), "%sOrig: %u:%u/%u.%u\r",print_ch(24, ' '),
                                 newFileReport[b]->origin.zone,newFileReport[b]->origin.net,
                                 newFileReport[b]->origin.node,newFileReport[b]->origin.point);
	              }
		      if (tmp == NULL || tmp[0] == 0) strcat(msg->text,"\r");
                      free(tmp);
                      fileCount++;
                      fileSize += newFileReport[b]->fileSize;
                      freeFReport(newFileReport[b]); newFileReport[b] = NULL;
                  } else {
                  } /* endif */
               } /* endfor */
               sprintf(buff, " %s\r", print_ch(77, '-'));
               msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+80);
               sprintf(msg->text+strlen(msg->text), "%s %u bytes in %u file(s)\r", buff, fileSize, fileCount);
               freeFReport(newFileReport[i]); newFileReport[i] = NULL;
            } else {
            } /* endif */
         } /* endif */
      } /* endfor */
      if (msg) {
         writeMsgToSysop(msg, annArea);
         freeMsgBuff(msg);
         free(msg);
      } else {
      } /* endif */
   } /* endfor */
   if (newFileReport) {
      free(newFileReport);
      if (config->echotosslog != NULL) {
         echotosslog = fopen (config->echotosslog, "a");
         fprintf(echotosslog,"%s\n",annArea);
         fclose(echotosslog);
      }
   }
}

void toss()
{

   writeLogEntry(htick_log, '4', "Start tossing...");

   processDir(config->localInbound, secLocalInbound);
   processDir(config->protInbound, secProtInbound);
   processDir(config->inbound, secInbound);
   
   reportNewFiles();
}

