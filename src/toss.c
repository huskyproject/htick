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

   if (!fexist(newFileName))
      rename(fileName, newFileName);
   else {
      sprintf(buff, "Could not change suffix for %s. File already there and the 255 files after", fileName);
      writeLogEntry(log, '9', buff);
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


XMSG createXMSGNetmail(s_message *msg)
{
   XMSG  msgHeader;
   struct tm *date;
   time_t    currentTime;
   union stamp_combo dosdate;

      msgHeader.attr = msg->attributes;
      msgHeader.attr &= ~(MSGCRASH | MSGREAD | MSGSENT | MSGKILL | MSGLOCAL | MSGHOLD | MSGFRQ | MSGSCANNED | MSGLOCKED); // kill these flags
      msgHeader.attr |= MSGPRIVATE; // set this flags

   strcpy((char *) msgHeader.from,msg->fromUserName);
   strcpy((char *) msgHeader.to, msg->toUserName);
   strcpy((char *) msgHeader.subj,msg->subjectLine);
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
//   ASCII_Date_To_Binary(msg->datetime, (union stamp_combo *)&(msgHeader.date_written));

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

         msgHeader = createXMSGNetmail(msg);
         /* Create CtrlBuf for SMAPI */
         ctrlBuf = (char *) CopyToControlBuf((UCHAR *) msg->text, (UCHAR **) &bodyStart, &len);
         /* write message */
         MsgWriteMsg(msgHandle, 0, &msgHeader, (UCHAR *) bodyStart, len, len, strlen(ctrlBuf)+1, (UCHAR *) ctrlBuf);
         free(ctrlBuf);
         MsgCloseMsg(msgHandle);

         sprintf(buff, "Worte Netmail to: %u:%u/%u.%u",
                 msg->destAddr.zone, msg->destAddr.net, msg->destAddr.node, msg->destAddr.point);
         writeLogEntry(log, '6', buff);
      } else {
         writeLogEntry(log, '9', "Could not write message to NetmailArea");
      } /* endif */

      MsgCloseArea(netmail);
   } else {
      printf("%u\n", msgapierr);
      writeLogEntry(log, '9', "Could not open NetmailArea");
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
   
   fprintf(tichandle,"Created by HTick/Linux, written by Gabriel Plutzar\r\n");
   fprintf(tichandle,"File %s\r\n",tic->file);
   if (tic->desc[0]!=0)
      fprintf(tichandle,"Desc %s\r\n",tic->desc);
   fprintf(tichandle,"Area %s\r\n",tic->area);
   if (tic->areadesc[0]!=0)
      fprintf(tichandle,"Areadesc %s\r\n",tic->areadesc);
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
   if (tic->crc[0]!=0)
      fprintf(tichandle,"Crc %s\r\n",tic->crc);

   for (i=0;i<tic->anzldesc;i++)
       fprintf(tichandle,"LDesc %s\r\n",tic->ldesc[i]);
  
   for (i=0;i<tic->anzseenby;i++)
       fprintf(tichandle,"Seenby %s\r\n",addr2string(&tic->seenby[i]));

   for (i=0;i<tic->anzpath;i++)
       fprintf(tichandle,"Path %s\r\n",tic->path[i]);
  
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
   char hlp[200];
   char *token,*param;

   tichandle=fopen(ticfile,"r");
   memset(tic,0,sizeof(*tic));

   while (!feof(tichandle))
         {
         fgets(hlp,200,tichandle);

         if (hlp[0]==0 || hlp[0]==10 || hlp[0]==13 || hlp[0]==';' ||
             hlp[0]=='#')
            continue;

         if (hlp[strlen(hlp)-1]=='\n')
            hlp[strlen(hlp)-1]=0;

         if (hlp[strlen(hlp)-1]=='\r')
            hlp[strlen(hlp)-1]=0;


         token=strtok(hlp, " \t");
         param=stripLeadingChars(strtok(NULL, "\0"), "\t");

         if (token==NULL || param==NULL)
            continue;

         if (stricmp(token,"created")==0) 
            continue;

         if (stricmp(token,"file")==0)
            strcpy(tic->file,param); else
         if (stricmp(token,"areadesc")==0) 
            strcpy(tic->areadesc,param); else
         if (stricmp(token,"area")==0)
            strcpy(tic->area,param); else
         if (stricmp(token,"desc")==0)
            strcpy(tic->desc,param); else
         if (stricmp(token,"replaces")==0)
            strcpy(tic->replaces,param); else
         if (stricmp(token,"pw")==0)
            strcpy(tic->password,param); else
         if (stricmp(token,"size")==0)
            tic->size=atoi(param); else
         if (stricmp(token,"crc")==0) 
            strcpy(tic->crc,param); else
         if (stricmp(token,"date")==0)
            tic->date=atoi(param); else
         if (stricmp(token,"from")==0)
            string2addr(param,&tic->from); else
         if (stricmp(token,"to")==0 || stricmp(token,"Destination")==0)
            string2addr(param,&tic->to); else
         if (stricmp(token,"origin")==0) 
            string2addr(param,&tic->origin); else
         if (stricmp(token,"magic")==0) 
            {;} else
         if (stricmp(token,"seenby")==0) 
            {
            tic->seenby=
               realloc(tic->seenby,(tic->anzseenby+1)*sizeof(s_addr));
            string2addr(param,&tic->seenby[tic->anzseenby]);
            tic->anzseenby++;
            }
           else
            if (stricmp(token,"path")==0) 
               {
               tic->path=
                  realloc(tic->path,(tic->anzpath+1)*sizeof(*tic->path));
               tic->path[tic->anzpath]=strdup(param);
               tic->anzpath++;
               }
              else
               if (stricmp(token,"ldesc")==0) 
                  {
                  tic->ldesc=
                    realloc(tic->ldesc,(tic->anzldesc+1)*sizeof(*tic->ldesc));
                  tic->ldesc[tic->anzldesc]=strdup(param);
                  tic->anzldesc++;
                  }
                 else
                  {    
                  printf("Unknown Keyword %s in Tic File\n",token);
                  sprintf(hlp,"Unknown Keyword %s in Tic File\n",token);
                  writeLogEntry(log, '7', hlp);  
                  }
            }

  fclose(tichandle);

  return(1);
}

int autoCreate(char *c_area, s_addr pktOrigAddr, char *desc)
{                                                                              
   FILE *f;
   char *NewAutoCreate;
   char *fileName;                                                             
   char buff[255], myaddr[20], hisaddr[20];                                    
   int i=0;                                                                    
   s_link *creatingLink;                                                       
   s_addr *aka;                                                                
                                                                               
   //translating name of the area to lowercase, much better imho.              
   while (*c_area != '\0') {                                                   
      *c_area=tolower(*c_area);                                                
      if ((*c_area=='/') || (*c_area=='\\')) *c_area = '_'; // convert any path elimiters to _                           
      c_area++;                                                                
      i++;                                                                     
   }                                                                           
                                                                               
   while (i>0) {c_area--;i--;};                                                
                                                                               
   creatingLink = getLinkFromAddr(*config, pktOrigAddr);                       
                                                                               
   fileName = creatingLink->autoCreateFile;                                    
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

   sprintf(buff, "FileArea %s %s%s%s -a %s ", 
           c_area,
           config->fileAreaBaseDir, 
           config->fileAreaBaseDir[strlen(config->fileAreaBaseDir)-1]=='/'?
                  "":"/",
           c_area, 
           myaddr);
           
   NewAutoCreate=(char *) calloc (strlen(config->autoFileCreateDefaults)+1, sizeof(char));
   strcpy(NewAutoCreate,config->autoFileCreateDefaults);
   
   if ((fileName=strstr(NewAutoCreate,"-d "))==NULL) {
     if (desc!=NULL) {
       char *tmp;
       tmp=(char *) calloc (strlen(NewAutoCreate)+strlen(desc)+6,sizeof(char));
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       free (NewAutoCreate);
       NewAutoCreate=tmp;
     }
   }
   else {
     if (desc!=NULL) {
       char *tmp;
       tmp=(char *) calloc (strlen(NewAutoCreate)+strlen(desc)+6,sizeof(char));
       fileName[0]='\0';
       sprintf(tmp,"%s -d \"%s\"", NewAutoCreate, desc);
       fileName++;
       fileName=rindex(fileName,'\"')+1;
       strcat(tmp,fileName);
       free(NewAutoCreate);
       NewAutoCreate=tmp;
     }
   }   

   if ((NewAutoCreate != NULL) &&
      (strlen(buff)+strlen(NewAutoCreate))<255) 
      {     
      strcat(buff, NewAutoCreate);                     
      }                            
   
   free(NewAutoCreate);
   
   sprintf (buff+strlen(buff)," %s",hisaddr);
   
   fprintf(f, "%s\n", buff);                                                
                                                                    
   fclose(f);                                                       
                                                                    
   // add new created echo to config in memory                      
   parseLine(buff,config);                                          
                                                                    
   sprintf(buff, "FileArea '%s' autocreated by %s", c_area, hisaddr);   
   writeLogEntry(log, '8', buff);                                   
   return 0;                                                        
}              

int processTic(char *ticfile, e_tossSecurity sec)                     
{
   s_ticfile tic;
   int i;
   FILE *flohandle;

   char ticedfile[256],fileareapath[256],
        newticedfile[256],linkfilepath[256],descr_file_name[256];
   char *newticfile;
   char logstr[200];
   s_filearea *filearea;
   char hlp[100],timestr[40];
   time_t acttime;
   s_link *from_link;

   printf("Ticfile %s\n",ticfile);

   sprintf(logstr,"Processing Tic-File %s",ticfile);
   writeLogEntry(log,'6',logstr);

   parseTic(ticfile,&tic);

   // Security Check
   from_link=getLinkFromAddr(*config,tic.from);
   if (from_link==NULL)
      {
      sprintf(logstr,"Link for Tic From Adress '%s' not found",
              addr2string(&tic.from));
      writeLogEntry(log,'9',logstr);
      disposeTic(&tic);
      return(1);
      }

   if (tic.password[0]!=0 && stricmp(tic.password,from_link->ticPwd)!=0)
      {
      sprintf(logstr,"Wrong Password %s from %s",
              tic.password,addr2string(&tic.from));
      writeLogEntry(log,'9',logstr);
      disposeTic(&tic);
      return(1);
      }

   if (tic.to.zone!=0) 
      if (to_us(tic.to))
         {
         sprintf(logstr,"Tic File adressed to %s, not to us",
                 addr2string(&tic.to));
         writeLogEntry(log,'9',logstr);
         disposeTic(&tic);
         return(4);
         }

   strcpy(ticedfile,ticfile);
   *(strrchr(ticedfile,'/')+1)=0;
   strcat(ticedfile,tic.file);

   filearea=getFileArea(config,tic.area);

   if (filearea==NULL)
      {
      autoCreate(tic.area,tic.from,tic.areadesc);
      filearea=getFileArea(config,tic.area);
      }

   if (filearea!=NULL)
      {
      strcpy(fileareapath,filearea->pathName);
      if (filearea->pathName[strlen(filearea->pathName)-1]!='/')
         strcat(filearea->pathName,"/");

      }
     else
      {
      sprintf(logstr,"Cannot open oder create File Area %s",tic.area);
      writeLogEntry(log,'9',logstr);
      fprintf(stderr,"Cannot open oder create File Area %s !",tic.area);
      disposeTic(&tic);
      return(2);
      } 

   strLower(fileareapath);
   createDirectoryTree(fileareapath);

   if (tic.replaces[0]!=0)
      { // Delete old file
      strcpy(newticedfile,fileareapath);
      if (fileareapath[strlen(fileareapath)-1]!='/')
         strcat(newticedfile,"/");
      strcat(newticedfile,tic.replaces);
      strLower(newticedfile);
      if (remove(newticedfile)==0)
         {
         sprintf(logstr,"Removed file %s one request",newticedfile);
         writeLogEntry(log,'6',logstr);
         }
      }

   strcpy(newticedfile,fileareapath);
   if (fileareapath[strlen(fileareapath)-1]!='/')
      strcat(newticedfile,"/");
   strcat(newticedfile,tic.file);
   strLower(newticedfile);

   if (move_file(ticedfile,newticedfile)!=0)
      {
      strLower(ticedfile);
      if (move_file(ticedfile,newticedfile)==0)
         {
         sprintf(logstr,"Moved %s to %s",ticedfile,newticedfile);
         writeLogEntry(log,'6',logstr);
         }
        else
         {
         sprintf(logstr,"File %s not found or moveable",ticedfile);
         writeLogEntry(log,'9',logstr);
         disposeTic(&tic);
         return(2);
         }
      }
     else
      {
      sprintf(logstr,"Moved %s to %s",ticedfile,newticedfile);
      writeLogEntry(log,'6',logstr);
      }

     strcpy(descr_file_name, fileareapath);
     if (fileareapath[strlen(fileareapath)-1]!='/')
        strcat(descr_file_name, "/");
     strcat(descr_file_name, "files.bbs");
     strLower(descr_file_name);
     
     add_description (descr_file_name, tic.file, tic.desc);

    // Adding path & seenbys
      time(&acttime);
      strcpy(timestr,asctime(localtime(&acttime)));
      timestr[strlen(timestr)-1]=0;

      sprintf(hlp,"%s %s %s",
              addr2string(filearea->useAka),timestr,versionStr);

      tic.path=realloc(tic.path,(tic.anzpath+1)*sizeof(*tic.path));
      tic.path[tic.anzpath]=strdup(hlp);
      tic.anzpath++;

    for (i=0;i<filearea->downlinkCount;i++)
        if (addrComp(tic.from,filearea->downlinks[i]->hisAka)!=0 && 
            addrComp(tic.to,filearea->downlinks[i]->hisAka)!=0 &&
            addrComp(tic.origin,filearea->downlinks[i]->hisAka)!=0 &&
	    addrComp(tic.to, *filearea->downlinks[i]->ourAka)!=0 &&
            seenbyComp (&tic, filearea->downlinks[i]->hisAka) != 0)
           { // Adding Downlink to Seen-By
           tic.seenby=realloc(tic.seenby,(tic.anzseenby+1)*sizeof(s_addr));
           memcpy(&tic.seenby[tic.anzseenby],
                  &filearea->downlinks[i]->hisAka,
                  sizeof(s_addr));
           tic.anzseenby++;
           }


    // Checking to whom I shall forward

    for (i=0;i<filearea->downlinkCount;i++)
        if (addrComp(tic.from,filearea->downlinks[i]->hisAka)!=0 && 
            addrComp(tic.to,filearea->downlinks[i]->hisAka)!=0 &&
            addrComp(tic.origin,filearea->downlinks[i]->hisAka)!=0 &&
	    addrComp(tic.to, *filearea->downlinks[i]->ourAka)!=0)
            { // Forward file to
	     if (seenbyComp (&tic, filearea->downlinks[i]->hisAka) == 0) {
                sprintf(logstr,"File %s already seenby %s, %s",
                        tic.file,
                        filearea->downlinks[i]->name,
                        addr2string(&filearea->downlinks[i]->hisAka));
                writeLogEntry(log,'6',logstr);
	     } else {
             memcpy(&tic.from,filearea->useAka,sizeof(s_addr));
             memcpy(&tic.to,&filearea->downlinks[i]->hisAka,
                    sizeof(s_addr));
             strcpy(tic.password,filearea->downlinks[i]->ticPwd);

             if (createOutboundFileName(filearea->downlinks[i],
                 cvtFlavour2Prio(filearea->downlinks[i]->echoMailFlavour),
                 FLOFILE)==1)
                printf("busy \n");

             strcpy(linkfilepath,filearea->downlinks[i]->floFile);
             *(strrchr(linkfilepath,'/'))=0;

             newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
             writeTic(newticfile,&tic);   

             flohandle=fopen(filearea->downlinks[i]->floFile,"a");
             fprintf(flohandle,"^%s\r\n",newticfile);
             fprintf(flohandle,"%s\r\n",newticedfile);
             fclose(flohandle);   
       
             remove(filearea->downlinks[i]->bsyFile);

             sprintf(logstr,"Forwarding %s for %s, %s",
                     tic.file,
                     filearea->downlinks[i]->name,
                     addr2string(&filearea->downlinks[i]->hisAka));

             writeLogEntry(log,'6',logstr);
	     } // if Seenby
             } // Forward file


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
            default:                                 
               remove (dummy);                       
               break;                                
         }       
         }                                          
      free(dummy);                                   
   }                                                 
   closedir(dir);                                    
}                                                    


void toss()
{
//   writeLogEntry(log, '4', "Start tossing...");

   processDir(config->localInbound, secLocalInbound);
   processDir(config->protInbound, secProtInbound);
   processDir(config->inbound, secInbound);
}
