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
#include <unistd.h>

#include <fidoconfig.h>
#include <common.h>
#include <fcommon.h>
#include <global.h>

#include <toss.h>
#include <patmat.h>

#include <dir.h>

#include <msgapi.h>
#include <typedefs.h>
#include <compiler.h>
#include <pkt.h>
#include <areafix.h>
#include <version.h>

#include <stamp.h>
#include <progprot.h>

#include <add_descr.h>
#include <seenby.h>
#include <recode.h>
#include <crc32.h>

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
      sprintf(buff, "Could not change suffix for %s. File already there and the 255 files after", fileName);
      writeLogEntry(htick_log, '9', buff);
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
          ourAka->zone,ourAka->net,ourAka->node,ourAka->point,time(NULL));  
   else                                                                         
      sprintf(buff + strlen(buff),"\1MSGID: %u:%u/%u %08lx\r",                  
              ourAka->zone,ourAka->net,ourAka->node,time(NULL));                
                                                                                
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
		// attributes of netmail must be fixed
		msgHeader.attr = msg->attributes;
		
		if (to_us(msg->destAddr)==0) {
			msgHeader.attr &= ~(MSGCRASH | MSGREAD | MSGSENT | MSGKILL | MSGLOCAL | MSGHOLD
			  | MSGFRQ | MSGSCANNED | MSGLOCKED | MSGFWD); // kill these flags
			msgHeader.attr |= MSGPRIVATE; // set this flags
		} //else if (header!=NULL) msgHeader.attr |= MSGFWD; // set TRS flag, if the mail is not to us

      // Check if we must remap
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
       msgHeader.attr &= ~(MSGCRASH | MSGREAD | MSGSENT | MSGKILL | MSGLOCAL | MSGHOLD | MSGFRQ | MSGSCANNED | MSGLOCKED); // kill these flags
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

   memset(&(msgHeader.date_written), 0, 8);    // date to 0

   msgHeader.utc_ofs = 0;
   msgHeader.replyto = 0;
   memset(msgHeader.replies, 0, MAX_REPLY * sizeof(UMSGID));   // no replies
   strcpy((char *) msgHeader.__ftsc_date, msg->datetime);
   ASCII_Date_To_Binary(msg->datetime, (union stamp_combo *) &(msgHeader.date_written));

   currentTime = time(NULL);
   date = localtime(&currentTime);
   TmDate_to_DosDate(date, &dosdate);
   msgHeader.date_arrived = dosdate.msg_st;

   return msgHeader;
}

void writeNetmail(s_message *msg)
{
   HAREA  netmail;
   HMSG   msgHandle;
   UINT   len = msg->textLength;
   char   *bodyStart;             // msg-body without kludgelines start
   char   *ctrlBuf;               // Kludgelines
   XMSG   msgHeader;
   char   buff[36];               // buff for sprintf
   char *slash;
#ifdef UNIX
   char limiter = '/';
#else
   char limiter = '\\';
#endif

   // create Directory Tree if necessary
   if (config->netMailArea.msgbType == MSGTYPE_SDM)
      createDirectoryTree(config->netMailArea.fileName);
   else {
      // squish area
      slash = strrchr(config->netMailArea.fileName, limiter);
      *slash = '\0';
      createDirectoryTree(config->netMailArea.fileName);
      *slash = limiter;
   }

   netmail = MsgOpenArea((unsigned char *) config->netMailArea.fileName, MSGAREA_CRIFNEC, config->netMailArea.msgbType);

   if (netmail != NULL) {
      msgHandle = MsgOpenMsg(netmail, MOPEN_CREATE, 0);

      if (msgHandle != NULL) {
         config->netMailArea.imported = 1; // area has got new messages

         if (config->intab != NULL) {
            recodeToInternalCharset(msg->text);
            recodeToInternalCharset(msg->subjectLine);
         }

         msgHeader = createXMSG(msg);
         /* Create CtrlBuf for SMAPI */
         ctrlBuf = (char *) CopyToControlBuf((UCHAR *) msg->text, (UCHAR **) &bodyStart, &len);
         /* write message */
         MsgWriteMsg(msgHandle, 0, &msgHeader, (UCHAR *) bodyStart, len, len, strlen(ctrlBuf)+1, (UCHAR *) ctrlBuf);
         free(ctrlBuf);
         MsgCloseMsg(msgHandle);

         sprintf(buff, "Wrote Netmail to: %u:%u/%u.%u",
                 msg->destAddr.zone, msg->destAddr.net, msg->destAddr.node, msg->destAddr.point);
         writeLogEntry(htick_log, '6', buff);
      } else {
         writeLogEntry(htick_log, '9', "Could not write message to NetmailArea");
      } /* endif */

      MsgCloseArea(netmail);
   } else {
//      printf("%u\n", msgapierr);
      writeLogEntry(htick_log, '9', "Could not open NetmailArea");
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

void strLower(char *s)
{
  while (*s!=0)
        {
        *s=tolower(*s);
        s++;
        }
}

void writeTic(char *ticfile,s_ticfile *tic)
{
   FILE *tichandle;
   int i;

   tichandle=fopen(ticfile,"w");
   
   fprintf(tichandle,"Created by HTick, written by Gabriel Plutzar\n");
   fprintf(tichandle,"File %s\n",tic->file);
   fprintf(tichandle,"Area %s\n",tic->area);
   if (tic->areadesc[0]!=0)
      fprintf(tichandle,"Areadesc %s\n",tic->areadesc);

   for (i=0;i<tic->anzdesc;i++)
      fprintf(tichandle,"Desc %s\n",tic->desc[i]);

   for (i=0;i<tic->anzldesc;i++)
       fprintf(tichandle,"LDesc %s\n",tic->ldesc[i]);

   if (tic->replaces[0]!=0)
      fprintf(tichandle,"Replaces %s\n",tic->replaces);
   if (tic->from.zone!=0)
      fprintf(tichandle,"From %s\n",addr2string(&tic->from));
   if (tic->to.zone!=0)
      fprintf(tichandle,"To %s\n",addr2string(&tic->to));
   if (tic->origin.zone!=0)
      fprintf(tichandle,"Origin %s\n",addr2string(&tic->origin));
   if (tic->size!=0)
      fprintf(tichandle,"Size %u\n",tic->size);
   if (tic->date!=0)
      fprintf(tichandle,"Date %lu\n",tic->date);
   if (tic->crc!=0)
      fprintf(tichandle,"Crc %lX\n",tic->crc);

   for (i=0;i<tic->anzpath;i++)
       fprintf(tichandle,"Path %s\n",tic->path[i]);
  
   for (i=0;i<tic->anzseenby;i++)
       fprintf(tichandle,"Seenby %s\n",addr2string(&tic->seenby[i]));
  
   if (tic->password[0]!=0)
      fprintf(tichandle,"Pw %s\n",tic->password);

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
   char hlp[80];
   char *line, *token, *param;

   tichandle=fopen(ticfile,"r");
   memset(tic,0,sizeof(*tic));

   while ((line = readLine(tichandle)) != NULL) {
      line = trimLine(line);

      if (*line!=0 || *line!=10 || *line!=13 || *line!=';' || *line!='#') {
         token=strtok(line, " \t");
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
//               printf("Unknown Keyword %s in Tic File\n",token);
               sprintf(hlp,"Unknown Keyword %s in Tic File",token);
               writeLogEntry(htick_log, '7', hlp);  
            }
         } /* endif */
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
                                                                               
   fileechoFileName = (char *) malloc(strlen(c_area)+1);
   strcpy(fileechoFileName, c_area);

   //translating path of the area to lowercase, much better imho.              
   while (*fileechoFileName != '\0') {                                                   
      *fileechoFileName=tolower(*fileechoFileName);                                                
      if ((*fileechoFileName=='/') || (*fileechoFileName=='\\')) *fileechoFileName = '_'; // convert any path elimiters to _                           
      fileechoFileName++;                                                                
      i++;                                                                     
   }                                                                           
                                                                               
   while (i>0) {fileechoFileName--;i--;};                                                
                                                                               
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
                                                                      
   // making local address and address of uplink                      
   strcpy(myaddr,addr2string(aka));
   strcpy(hisaddr,addr2string(&pktOrigAddr));
                                                                                
   //write new line in config file                                              
                            
/*   sprintf(buff, "FileArea %s %s%s%s -a %s %s ", 
           c_area,
           config->fileAreaBaseDir, 
           config->fileAreaBaseDir[strlen(config->fileAreaBaseDir)-1]=='/'?
                  "":"/",
           c_area, 
           myaddr, 
           hisaddr); */

   sprintf(buff, "FileArea %s %s%s -a %s ", 
           c_area,
           config->fileAreaBaseDir, 
           c_area, 
           myaddr);
           
   if (creatingLink->autoFileCreateDefaults) {
      NewAutoCreate=(char *) calloc (strlen(creatingLink->autoFileCreateDefaults)+1, sizeof(char));
      strcpy(NewAutoCreate,creatingLink->autoFileCreateDefaults);
   } else NewAutoCreate = (char*)calloc(1, sizeof(char));
   
   if ((fileName=strstr(NewAutoCreate,"-d "))==NULL) {
     if (desc!=NULL) {
       char *tmp;
       tmp=(char *) calloc (strlen(NewAutoCreate)+strlen(desc)+6,sizeof(char));
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       free (NewAutoCreate);
       NewAutoCreate=tmp;
     }
   } else {
     if (desc!=NULL) {
       char *tmp;
       tmp=(char *) calloc (strlen(NewAutoCreate)+strlen(desc)+6,sizeof(char));
       fileName[0]='\0';
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       fileName++;
       fileName=strrchr(fileName,'\"')+1;
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
                                                                    
   // add new created echo to config in memory                      
   parseLine(buff,config);                                          
                                                                    
   sprintf(buff, "FileArea '%s' autocreated by %s", c_area, hisaddr);   
   writeLogEntry(htick_log, '8', buff);                                   

   // report about new filearea
   if (config->ReportTo && (area = getFileArea(config, c_area)) != NULL) {
      if (stricmp(config->ReportTo, "netmail")==0) {
         msg = makeMessage(area->useAka, area->useAka, versionStr, config->sysop, "Created new fileareas", 1);
         msg->text = (char *)calloc(300, sizeof(char));
         createKludges(msg->text, NULL, area->useAka, area->useAka);
      } else {
         msg = makeMessage(area->useAka, area->useAka, versionStr, "All", "Created new fileareas", 0);
         msg->text = (char *)calloc(300, sizeof(char));
         createKludges(msg->text, config->ReportTo, area->useAka, area->useAka);
      } /* endif */
      sprintf(buff, "\r \rNew filearea: %s\r\rDescription : %s\r", area->areaName, area->description);
      msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+1);
      strcat(msg->text, buff);
      writeMsgToSysop(msg);
      freeMsgBuff(msg);
      free(msg);
   } else {
   } /* endif */

   if (cmAnnNewFileecho) announceNewFileecho (announcenewfileecho, c_area, hisaddr);

   return 0;                                                        
}              

int processTic(char *ticfile, e_tossSecurity sec)                     
{
   s_ticfile tic;
   int i;
   FILE *flohandle;

   char ticedfile[256],fileareapath[256],
        newticedfile[256],linkfilepath[256],descr_file_name[256];
   char *newticfile, sepname[13], *sepDir;
   char logstr[200];
   s_filearea *filearea;
   char hlp[100],timestr[40];
   time_t acttime;
   s_link *from_link;
   s_addr *old_seenby;
   int old_anzseenby;
   int busy;
   unsigned long crc;
   struct stat stbuf;

//   printf("Ticfile %s\n",ticfile);

   sprintf(logstr,"Processing Tic-File %s",ticfile);
   writeLogEntry(htick_log,'6',logstr);

   parseTic(ticfile,&tic);

   // Security Check
   from_link=getLinkFromAddr(*config,tic.from);
   if (from_link==NULL) {
      sprintf(logstr,"Link for Tic From Adress '%s' not found",
              addr2string(&tic.from));
      writeLogEntry(htick_log,'9',logstr);
      disposeTic(&tic);
      return(1);
   }

   if (tic.password[0]!=0 && ((from_link->ticPwd==NULL) ||
							  (stricmp(tic.password,from_link->ticPwd)!=0))) {
      sprintf(logstr,"Wrong Password %s from %s",
              tic.password,addr2string(&tic.from));
      writeLogEntry(htick_log,'9',logstr);
      disposeTic(&tic);
      return(1);
   }

   if (tic.to.zone!=0) {
      if (to_us(tic.to)) {
         sprintf(logstr,"Tic File adressed to %s, not to us",
                 addr2string(&tic.to));
         writeLogEntry(htick_log,'9',logstr);
         disposeTic(&tic);
         return(4);
      }
   }

   strcpy(ticedfile,ticfile);
   *(strrchr(ticedfile,PATH_DELIM)+1)=0;
   strcat(ticedfile,tic.file);

   // Recieve file?
   if (!fexist(ticedfile)) {
      strLower(ticedfile);
      if (!fexist(ticedfile)) {
         sprintf(logstr,"File %s, not resieved, wait",tic.file);
         writeLogEntry(htick_log,'6',logstr);
         disposeTic(&tic);
         return(5);
      }
   }

   //Calculate file size
   stat(ticedfile,&stbuf);
   tic.size = stbuf.st_size;

   crc = filecrc32(ticedfile);
   if (tic.crc != crc) {
      sprintf(logstr,"Wrong CRC - in tic:%lx, need:%lx",tic.crc,crc);
      writeLogEntry(htick_log,'9',logstr);
      disposeTic(&tic);
      return(3);
   }

   filearea=getFileArea(config,tic.area);

   if (filearea==NULL && from_link->autoFileCreate) {
      autoCreate(tic.area,tic.from,tic.areadesc);
      filearea=getFileArea(config,tic.area);
   }

   if (filearea!=NULL) {
      strcpy(fileareapath,filearea->pathName);
      if (filearea->pathName[strlen(filearea->pathName)-1]!=PATH_DELIM) {
         filearea->pathName = (char*)realloc(filearea->pathName, strlen(filearea->pathName)+2);
         sprintf(hlp, "%c", PATH_DELIM);
         strcat(filearea->pathName,hlp);
      }
   } else {
      sprintf(logstr,"Cannot open oder create File Area %s",tic.area);
      writeLogEntry(htick_log,'9',logstr);
      fprintf(stderr,"Cannot open oder create File Area %s !",tic.area);
      disposeTic(&tic);
      return(2);
   } 

   if (isLinkOfFileArea(from_link,filearea)==0) {
      sprintf(logstr,"Link %s not subscribe to File Area %s, rename tic to bad",
              addr2string(&tic.from), tic.area);
      writeLogEntry(htick_log,'9',logstr);
      disposeTic(&tic);
      return(3);
   }

   strLower(fileareapath);
   createDirectoryTree(fileareapath);

   if (tic.replaces[0]!=0) {
      // Delete old file
      strcpy(newticedfile,fileareapath);
      if (fileareapath[strlen(fileareapath)-1]!=PATH_DELIM) {
         sprintf(hlp, "%c", PATH_DELIM);
         strcat(newticedfile,hlp);
      }
      strcat(newticedfile,tic.replaces);
      strLower(newticedfile);

      if (remove(newticedfile)==0) {
         sprintf(logstr,"Removed file %s one request",newticedfile);
         writeLogEntry(htick_log,'6',logstr);
      }
   }

   strcpy(newticedfile,fileareapath);
   if (fileareapath[strlen(fileareapath)-1]!=PATH_DELIM) {
      sprintf(hlp, "%c", PATH_DELIM);
      strcat(newticedfile,hlp);
   }
   strcat(newticedfile,tic.file);
   strLower(newticedfile);

   if (move_file(ticedfile,newticedfile)!=0) {
      strLower(ticedfile);
      if (move_file(ticedfile,newticedfile)==0) {
         sprintf(logstr,"Moved %s to %s",ticedfile,newticedfile);
         writeLogEntry(htick_log,'6',logstr);
      } else {
         sprintf(logstr,"File %s not found or moveable",ticedfile);
         writeLogEntry(htick_log,'9',logstr);
         disposeTic(&tic);
         return(2);
      }
   } else {
      sprintf(logstr,"Moved %s to %s",ticedfile,newticedfile);
      writeLogEntry(htick_log,'6',logstr);
   }

   strcpy(descr_file_name, fileareapath);
   if (fileareapath[strlen(fileareapath)-1]!=PATH_DELIM) {
      sprintf(hlp, "%c", PATH_DELIM);
      strcat(descr_file_name, hlp);
   }
   strcat(descr_file_name, "files.bbs");
   strLower(descr_file_name);
     
   removeDesc(descr_file_name,tic.file);
   add_description (descr_file_name, tic.file, tic.desc, tic.anzdesc);
   if (cmAnnFile) announceInFile (announcefile, tic.file, tic.size, tic.area, tic.origin, tic.desc, tic.anzdesc);

   // Adding path & seenbys
   time(&acttime);
   strcpy(timestr,asctime(localtime(&acttime)));
   timestr[strlen(timestr)-1]=0;

   sprintf(hlp,"%s %lu %s %s",
              addr2string(filearea->useAka), time(NULL), timestr,versionStr);

   tic.path=realloc(tic.path,(tic.anzpath+1)*sizeof(*tic.path));
   tic.path[tic.anzpath]=strdup(hlp);
   tic.anzpath++;

   //Save seenby structure
   old_seenby = malloc(tic.anzseenby*sizeof(s_addr));
   memcpy(old_seenby,tic.seenby,sizeof(s_addr));
   old_anzseenby = tic.anzseenby;

   for (i=0;i<filearea->downlinkCount;i++) {
      if (addrComp(tic.from,filearea->downlinks[i]->link->hisAka)!=0 && 
            addrComp(tic.to,filearea->downlinks[i]->link->hisAka)!=0 &&
            addrComp(tic.origin,filearea->downlinks[i]->link->hisAka)!=0 &&
          //addrComp(tic.to, *filearea->downlinks[i]->ourAka)!=0 &&
            seenbyComp (tic.seenby, tic.anzseenby,
            filearea->downlinks[i]->link->hisAka) != 0) {
         // Adding Downlink to Seen-By
         tic.seenby=realloc(tic.seenby,(tic.anzseenby+1)*sizeof(s_addr));
         memcpy(&tic.seenby[tic.anzseenby],
            &filearea->downlinks[i]->link->hisAka,
            sizeof(s_addr));
         tic.anzseenby++;
      }
   }

   // Checking to whom I shall forward

   for (i=0;i<filearea->downlinkCount;i++) {
      if (addrComp(tic.from,filearea->downlinks[i]->link->hisAka)!=0 && 
            addrComp(tic.to,filearea->downlinks[i]->link->hisAka)!=0 &&
          //addrComp(tic.to, *filearea->downlinks[i]->ourAka)!=0 &&
            addrComp(tic.origin,filearea->downlinks[i]->link->hisAka)!=0) {
         // Forward file to
         if (seenbyComp (old_seenby, old_anzseenby, filearea->downlinks[i]->link->hisAka) == 0) {
            sprintf(logstr,"File %s already seenby %s, %s",
                        tic.file,
                        filearea->downlinks[i]->link->name,
                        addr2string(&filearea->downlinks[i]->link->hisAka));
            writeLogEntry(htick_log,'6',logstr);
         } else {
            memcpy(&tic.from,filearea->useAka,sizeof(s_addr));
            memcpy(&tic.to,&filearea->downlinks[i]->link->hisAka,
                    sizeof(s_addr));
            strcpy(tic.password,filearea->downlinks[i]->link->ticPwd);

            busy = 0;

            if (createOutboundFileName(filearea->downlinks[i]->link,
                 cvtFlavour2Prio(filearea->downlinks[i]->link->echoMailFlavour),
                 FLOFILE)==1)
                busy = 1;

            strcpy(linkfilepath,filearea->downlinks[i]->link->floFile);
            if (busy) {
               writeLogEntry(htick_log, '7', "Save TIC in temporary dir");
               //Create temporary directory
               *(strrchr(linkfilepath,'.'))=0;
               strcat(linkfilepath,".htk");
               createDirectoryTree(linkfilepath);
            } else *(strrchr(linkfilepath,PATH_DELIM))=0;

            // separate bundles
            if (config->separateBundles && !busy) {
          
               if (filearea->downlinks[i]->link->hisAka.point != 0)
                  sprintf(sepname,
                        "%08x.sep",
                        filearea->downlinks[i]->link->hisAka.point);
               else
                  sprintf(sepname,
                         "%04x%04x.sep",
                         filearea->downlinks[i]->link->hisAka.net,
                         filearea->downlinks[i]->link->hisAka.node);

               sepDir = (char*) malloc(strlen(linkfilepath)+1+strlen(sepname)+1+1);
               sprintf(sepDir,"%s%c%s%c",linkfilepath,PATH_DELIM,sepname,PATH_DELIM);
               strcpy(linkfilepath,sepDir);

               createDirectoryTree(sepDir);
               free(sepDir);
            }

            newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
            writeTic(newticfile,&tic);   

            if (!busy) {
               flohandle=fopen(filearea->downlinks[i]->link->floFile,"a");
               fprintf(flohandle,"%s\n",newticedfile);
               fprintf(flohandle,"^%s\n",newticfile);
               fclose(flohandle);

               remove(filearea->downlinks[i]->link->bsyFile);

               sprintf(logstr,"Forwarding %s for %s, %s",
                       tic.file,
                       filearea->downlinks[i]->link->name,
                       addr2string(&filearea->downlinks[i]->link->hisAka));

               writeLogEntry(htick_log,'6',logstr);
            }

         } // if Seenby
      } // Forward file
   }

   // report about new files
   newFileReport = (s_newfilereport**)realloc(newFileReport, (newfilesCount+1)*sizeof(s_newfilereport*));
   newFileReport[newfilesCount] = (s_newfilereport*)calloc(1, sizeof(s_newfilereport));
   newFileReport[newfilesCount]->useAka = filearea->useAka;
   newFileReport[newfilesCount]->areaName = filearea->areaName;
   newFileReport[newfilesCount]->areaDesc = filearea->description;
   newFileReport[newfilesCount]->fileName = strdup(tic.file);

   newFileReport[newfilesCount]->fileDesc = (char**)calloc(tic.anzdesc, sizeof(char*));
   for (i = 0; i < tic.anzdesc; i++) {
      newFileReport[newfilesCount]->fileDesc[i] = strdup(tic.desc[i]);
   } /* endfor */
   newFileReport[newfilesCount]->filedescCount = tic.anzdesc;

   newFileReport[newfilesCount]->fileSize = tic.size;

   newfilesCount++;

   free(old_seenby);
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
                                                                
      if (patimat(file->d_name, "*.TIC") == 1)
         {
         rc=processTic(dummy,sec);
                                                                
         switch (rc) {                                          
            case 1:   // pktpwd problem                         
               changeFileSuffix(dummy, "sec");                  
               break;                                           
            case 2:  // could not open file
               changeFileSuffix(dummy, "acs");                  
               break;                                           
            case 3:  // not/wrong pkt                           
               changeFileSuffix(dummy, "bad");                  
               break;         
            case 4:  // not to us                    
               changeFileSuffix(dummy, "ntu");       
               break;                                
            case 5:  // file not recieved
               break;                                
            default:                                 
               remove (dummy);                       
               break;                                
         }       
         }                                          
      free(dummy);                                   
   }                                                 
   closedir(dir);                                    
}                                                    

int createFlo(s_link *link, e_prio prio)
{

    FILE *f; // bsy file for current link
    char name[13], bsyname[13], zoneSuffix[6], pntDir[14];

#ifdef UNIX
    char limiter='/';
#else
    char limiter='\\';
#endif

   if (link->hisAka.point != 0) {
      sprintf(pntDir, "%04x%04x.pnt%c", link->hisAka.net, link->hisAka.node, limiter);
      sprintf(name, "%08x.flo", link->hisAka.point);
   } else {
      pntDir[0] = 0;
      sprintf(name, "%04x%04x.flo", link->hisAka.net, link->hisAka.node);
   }

   if (link->hisAka.zone != config->addr[0].zone) {
      // add suffix for other zones
      sprintf(zoneSuffix, ".%03x%c", link->hisAka.zone, limiter);
   } else {
      zoneSuffix[0] = 0;
   }

   switch (prio) {
      case CRASH : name[9] = 'c';
                   break;
      case HOLD  : name[9] = 'h';
                   break;
      case NORMAL: break;
   } /* endswitch */

   // create floFile
   link->floFile = (char *) malloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+strlen(name)+1);
   link->bsyFile = (char *) malloc(strlen(config->outbound)+strlen(pntDir)+strlen(zoneSuffix)+strlen(name)+1);
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
      free (link->bsyFile); link->bsyFile = NULL;
      return 1;
   } else {
      if ((f=fopen(link->bsyFile,"a")) == NULL) {
         fprintf(stderr,"cannot create *.bsy file for %s\n",link->name);
         if (config->lockfile != NULL) {
            remove(link->bsyFile);
            free(link->bsyFile);
            link->bsyFile=NULL;
         }
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

void checkTmpDir(s_link link)
{
    char tmpdir[256], fileareapath[256], newticedfile[256], newticfile[256];
    char logstr[200];
    DIR            *dir;
    struct dirent  *file;
    char           *ticfile;
    s_ticfile tic;
    s_filearea *filearea;
    FILE *flohandle;

   if (createFlo(&link, cvtFlavour2Prio(link.echoMailFlavour))==0) {
      strcpy(tmpdir,link.floFile);
      *(strrchr(tmpdir,'.'))=0;
      sprintf(tmpdir+strlen(tmpdir), ".htk%c", PATH_DELIM);

      dir = opendir(tmpdir);
      if (dir == NULL) {
         remove(link.bsyFile);
         return;
      }

      while ((file = readdir(dir)) != NULL) {
         ticfile = (char *) malloc(strlen(tmpdir)+strlen(file->d_name)+1);
         strcpy(ticfile, tmpdir);
         strcat(ticfile, file->d_name);
         if (patimat(file->d_name, "*.TIC") == 1) {
            memset(&tic,0,sizeof(tic));
            parseTic(ticfile,&tic);
            filearea=getFileArea(config,tic.area);
            if (filearea!=NULL) {
               strcpy(fileareapath,filearea->pathName);
//               if (filearea->pathName[strlen(filearea->pathName)-1]!='/')
//                  strcat(filearea->pathName,"/");
               strcpy(newticedfile,filearea->pathName);
               strcat(newticedfile,tic.file);
               strLower(newticedfile);

               strcpy(newticfile,link.floFile);
               if (config->separateBundles) {
                  *(strrchr(newticfile, '.')) = 0;
                  sprintf(newticfile+strlen(newticfile), ".sep%c", PATH_DELIM);
                  createDirectoryTree(newticfile);
               } else {
                  *(strrchr(newticfile,PATH_DELIM)+1)=0;
               } /* endif */
      
               strcat(newticfile,strrchr(ticfile,PATH_DELIM)+1);
               move_file(ticfile,newticfile);

               flohandle=fopen(link.floFile,"a");
               fprintf(flohandle,"%s\n",newticedfile);
               fprintf(flohandle,"^%s\n",newticfile);
               fclose(flohandle);

               sprintf(logstr,"Forwarding save file %s for %s, %s",
                       tic.file,
                       link.name,
                       addr2string(&link.hisAka));

               writeLogEntry(htick_log,'6',logstr);
	    }
	 }
         free(ticfile);
      } //while
   closedir(dir);
   remove(link.bsyFile);
   if (tmpdir[strlen(tmpdir)-1] == PATH_DELIM) {
      tmpdir[strlen(tmpdir)-1] = 0;
   } else {
   } /* endif */
   rmdir(tmpdir);
   } // if
}

void processTmpDir()
{
    int i;

   for (i = 0; i < config->linkCount; i++) checkTmpDir(config->links[i]);
}

int putMsgInArea(s_area *echo, s_message *msg, int strip)
{
   char buff[70], *ctrlBuff, *textStart, *textWithoutArea;
   UINT textLength = (UINT) msg->textLength;
   HAREA harea;
   HMSG  hmsg;
   XMSG  xmsg;
   char *slash;
   int rc = 0;

   // create Directory Tree if necessary
   if (echo->msgbType == MSGTYPE_SDM)
      createDirectoryTree(echo->fileName);
   else {
      // squish area
      slash = strrchr(echo->fileName, PATH_DELIM);
      *slash = '\0';
      createDirectoryTree(echo->fileName);
      *slash = PATH_DELIM;
   }

   harea = MsgOpenArea((UCHAR *) echo->fileName, MSGAREA_CRIFNEC, echo->msgbType | MSGTYPE_ECHO);
   if (harea != NULL) {
      hmsg = MsgOpenMsg(harea, MOPEN_CREATE, 0);
      if (hmsg != NULL) {

         // recode from TransportCharset to internal Charset
         if (msg->recode == 0 && config->intab != NULL) {
            recodeToInternalCharset(msg->subjectLine);
            recodeToInternalCharset(msg->text);
            recodeToInternalCharset(msg->toUserName);
            recodeToInternalCharset(msg->fromUserName);
                        msg->recode = 1;
         }

         textWithoutArea = msg->text;

         if ((strncmp(msg->text, "AREA:", 5) == 0) && (strip==1)) {
            // jump over AREA:xxxxx\r
            while (*(textWithoutArea) != '\r') textWithoutArea++;
            textWithoutArea++;
         }

         ctrlBuff = (char *) CopyToControlBuf((UCHAR *) textWithoutArea,
				              (UCHAR **) &textStart,
				              &textLength);
         // textStart is a pointer to the first non-kludge line
         xmsg = createXMSG(msg);

         MsgWriteMsg(hmsg, 0, &xmsg, (byte *) textStart, (dword) strlen(textStart), (dword) strlen(textStart), (dword)strlen(ctrlBuff), (byte *)ctrlBuff);

         MsgCloseMsg(hmsg);
         free(ctrlBuff);
	 rc = 1;

      } else {
         sprintf(buff, "Could not create new msg in %s!", echo->fileName);
         writeLogEntry(htick_log, '9', buff);
      } /* endif */
      MsgCloseArea(harea);
   } else {
      sprintf(buff, "Could not open/create EchoArea %s!", echo->fileName);
      writeLogEntry(htick_log, '9', buff);
   } /* endif */
   return rc;
}

void writeMsgToSysop(s_message *msg)
{
   char tmp[81], *ptr;
   s_area	*echo;

   sprintf(tmp, " \r--- %s\r * Origin: %s (%s)\r", versionStr, config->name, addr2string(&(msg->origAddr)));
   msg->text = (char*)realloc(msg->text, strlen(tmp)+strlen(msg->text)+1);
   strcat(msg->text, tmp);
   msg->textLength = strlen(msg->text);
   if (msg->netMail == 1) writeNetmail(msg);
   else {
      ptr = strchr(msg->text, '\r');
      strncpy(tmp, msg->text+5, (ptr-msg->text)-5);
      tmp[ptr-msg->text-5]=0;
      echo = getArea(config, tmp);
      if (echo != &(config->badArea)) {
         if (echo->msgbType != MSGTYPE_PASSTHROUGH) {
            putMsgInArea(echo, msg,1);
            echo->imported = 1;  // area has got new messages
            sprintf(tmp, "Post report message to %s area", echo->areaName);
            writeLogEntry(htick_log, '7', tmp);
         }
      } else {
//         putMsgInBadArea(msgToSysop[i], msgToSysop[i]->origAddr, 0);
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

   // post report about new files to config->ReportTo
   for (c = 0; c < config->addrCount && newfilesCount != 0; c++) {
      msg = NULL;
      for (i = 0; i < newfilesCount; i++) {
         if (newFileReport[i] != NULL) {
            if (newFileReport[i]->useAka == &(config->addr[c])) {
               if (msg == NULL) {
                  if (stricmp(config->ReportTo, "netmail") == 0) {
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
                     createKludges(msg->text, config->ReportTo, newFileReport[i]->useAka, newFileReport[i]->useAka);
                  } /* endif */
                  strcat(msg->text, " ");
               } /* endif */
               fileCount = fileSize = 0;
               sprintf(buff, "\r Area : %s%s", newFileReport[i]->areaName,
                                               print_ch(25, ' '));
               sprintf(buff+25, "Desc : %s\r %s\r", newFileReport[i]->areaDesc,
                                                    print_ch(77, '-'));
               msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+1);
               strcat(msg->text, buff);
               sprintf(buff, " %s%s", newFileReport[i]->fileName, print_ch(25, ' '));
               tmp = formDesc(newFileReport[i]->fileDesc, newFileReport[i]->filedescCount);
               sprintf(buff+14, "% 9i", newFileReport[i]->fileSize);
               msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+strlen(tmp)+2);
               sprintf(msg->text+strlen(msg->text), "%s %s", buff, tmp);
               free(tmp);
               fileCount++;
               fileSize += newFileReport[i]->fileSize;
               for (b = i+1; b < newfilesCount; b++) {
                  if (newFileReport[b] != NULL && newFileReport[i] != NULL &&
                      newFileReport[b]->useAka == &(config->addr[c]) &&
                      stricmp(newFileReport[i]->areaName,
                      newFileReport[b]->areaName) == 0) {
                      sprintf(buff, " %s%s", newFileReport[b]->fileName, print_ch(25, ' '));
                      tmp = formDesc(newFileReport[b]->fileDesc, newFileReport[b]->filedescCount);
                      sprintf(buff+14, "% 9i", newFileReport[b]->fileSize);
                      msg->text = (char*)realloc(msg->text, strlen(msg->text)+strlen(buff)+strlen(tmp)+2);
                      sprintf(msg->text+strlen(msg->text), "%s %s", buff, tmp);
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
            } else {
            } /* endif */
            freeFReport(newFileReport[i]); newFileReport[i] = NULL;
         } /* endif */
      } /* endfor */
      if (msg) {
         writeMsgToSysop(msg);
         freeMsgBuff(msg);
         free(msg);
      } else {
      } /* endif */
   } /* endfor */
   if (newFileReport) free(newFileReport);
}

void toss()
{

   writeLogEntry(htick_log, '4', "Start tossing...");

   processDir(config->localInbound, secLocalInbound);
   processDir(config->protInbound, secProtInbound);
   processDir(config->inbound, secInbound);
   
   reportNewFiles();
}

