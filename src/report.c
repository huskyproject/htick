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

#include <fidoconf/log.h>
#include <fidoconf/xstr.h>
#include <fidoconf/common.h>
#include <fidoconf/afixcmd.h>
#include <fidoconf/dirlayer.h>

#include "toss.h"
#include "global.h"
#include "report.h"
#include "version.h"


s_ticfile* Report = NULL;
unsigned   rCount = 0;

void doSaveTic4Report(char *ticfile,s_ticfile *tic)
{
    FILE *tichandle;
    unsigned int i;
    
    char *rpTicName = NULL;
    
    xstrscat(&rpTicName,config->announceSpool,GetFilenameFromPathname(ticfile),NULL);
    
    tichandle = fopen(rpTicName,"wb");
    
    if(tichandle == NULL){
        w_log(LL_CRIT, "Can't create file %s",rpTicName);
        return;
    } else {
        w_log(LL_CREAT, "Report file %s created  ",rpTicName);
    }
    
    fprintf(tichandle,"File %s\r\n",tic->file);
    fprintf(tichandle,"Area %s\r\n",tic->area);

    if (tic->areadesc)
        fprintf(tichandle,"Areadesc %s\r\n",tic->areadesc);
    if (tic->anzldesc>0) {
        for (i=0;i<tic->anzldesc;i++)
            fprintf(tichandle,"LDesc %s\r\n",tic->ldesc[i]);
    } else {
        for (i=0;i<tic->anzdesc;i++)
            fprintf(tichandle,"Desc %s\r\n",tic->desc[i]);
    }
    if (tic->origin.zone!=0)
        fprintf(tichandle,"Origin %s\r\n",aka2str(tic->origin));
    if (tic->size!=0)
        fprintf(tichandle,"Size %u\r\n",tic->size);
    
    fclose(tichandle);
}

static int cmp_reportEntry(const void *a, const void *b)
{
   const s_ticfile* r1 = (s_ticfile*)a;
   const s_ticfile* r2 = (s_ticfile*)b;

   if( stricmp( r1->area, r2->area ) > 0)
      return 1;
   else if( stricmp( r1->area, r2->area ) < 0)
      return -1;
   else if( stricmp( r1->file, r2->file ) > 0)
      return 1;
   else if( stricmp( r1->file, r2->file ) < 0)
      return -1;
   return 0;
}


void getReportInfo()
{
    DIR            *dir;
    struct dirent  *file;
    s_ticfile *tic = NULL;
    char* fname = NULL;

    dir = opendir(config->announceSpool);
    
    while ((file = readdir(dir)) != NULL) {
        if (patimat(file->d_name, "*.TIC") == 0)
            continue;
        xstrscat(&fname,config->announceSpool,file->d_name,NULL);
        Report = srealloc( Report, (rCount+1)*sizeof(s_ticfile) );
        parseTic( fname, &(Report[rCount]) );
        rCount++;
        nfree(fname);
    }
    closedir(dir);
    qsort( (void*)Report, rCount, sizeof(s_ticfile), cmp_reportEntry ); 
}

/* report generation */


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

void freeReportInfo()
{
    unsigned i = 0;
    DIR           *dir;
    struct dirent *file;
    char* fname   = NULL;

    for(i; i < rCount; i++)
        disposeTic(&(Report[i]));
    nfree(Report);
    
    dir = opendir(config->announceSpool);
    
    while ((file = readdir(dir)) != NULL) {
        if (patimat(file->d_name, "*.TIC") == 0)
            continue;
        xstrscat(&fname,config->announceSpool,file->d_name,NULL);
        remove(fname);
        w_log('6',"Removed file: %s",fname);
        nfree(fname);
    }
    closedir(dir);
}

void reportNewFiles()
{
   unsigned  int b,  fileCount = 0;
   unsigned  int i;
   UINT32    fileSize = 0;
   s_message *msg = NULL;
   char      *tmp = NULL;
   FILE      *echotosslog;
   char      *annArea;
   int netmail = 0;

   
   if (config->ReportTo == NULL)
       return;
   annArea = config->ReportTo;

   if (config->ReportTo) {
       if (stricmp(annArea,"netmail")==0)                netmail=1;
       else if (getNetMailArea(config, annArea) != NULL) netmail=1;
   } else netmail=1;

   msg = makeMessage(&(config->addr[0]),&(config->addr[0]), 
       versionStr, 
       netmail ? config->sysop : "All", "New Files", 
       netmail,
       config->filefixKillReports);

   msg->text = createKludges(  config->disableTID,
       netmail ? NULL : config->ReportTo, 
       &(config->addr[0]), &(config->addr[0]),
       versionStr);
   
   xstrcat(&(msg->text), "\001FLAGS NPD\r");
   
   
   for (i = 0; i < rCount; i++) {
       fileCount = 0;
       xscatprintf(&(msg->text), "\r>Area : %s", Report[i].area);
       if(Report[i].areadesc) xscatprintf(&(msg->text), " : %s", Report[i].areadesc);
       xscatprintf(&(msg->text), "\r %s\r", print_ch(77, '-'));
       
       if(strlen(Report[i].file) > 12)
           xscatprintf(&(msg->text)," %s\r%23ld ",
           Report[i].file,
           Report[i].size
           );
       else
           xscatprintf(&(msg->text)," %-12s %9ld ",
           Report[i].file, 
           Report[i].size
           );
       
       if (Report[i].anzldesc > 0) {
           tmp = formDesc(Report[i].ldesc, Report[i].anzldesc); 
       } else {
           tmp = formDesc(Report[i].desc, Report[i].anzdesc); 
       }
       xstrcat(&(msg->text),tmp);
       if (config->originInAnnounce) {
           xscatprintf(&(msg->text), "%sOrig: %u:%u/%u.%u\r",print_ch(24, ' '),
               Report[i].origin.zone,Report[i].origin.net,
               Report[i].origin.node,Report[i].origin.point);
       }
       if (tmp == NULL || tmp[0] == 0) xstrcat(&(msg->text),"\r");
       nfree(tmp);
       fileCount++;
       fileSize += Report[i].size;
       for (b = i+1; b < rCount; b++) {
           if (stricmp(Report[i].area,Report[b].area) == 0) {
               if(strlen(Report[b].file) > 12)
                   xscatprintf(&(msg->text)," %s\r%23ld ",
                   Report[b].file,
                   Report[b].size
                   );
               else
                   xscatprintf(&(msg->text)," %-12s %9ld ",
                   Report[b].file, 
                   Report[b].size
                   );
               
               if (Report[b].anzldesc > 0) {
                   tmp = formDesc(Report[b].ldesc, Report[b].anzldesc); 
               } else {
                   tmp = formDesc(Report[b].desc, Report[b].anzdesc); 
               }
               xstrcat(&(msg->text),tmp);
               if (config->originInAnnounce) {
                   xscatprintf(&(msg->text), "%sOrig: %u:%u/%u.%u\r",print_ch(24, ' '),
                       Report[b].origin.zone,Report[b].origin.net,
                       Report[b].origin.node,Report[b].origin.point);
               }
               if (tmp == NULL || tmp[0] == 0) xstrcat(&(msg->text),"\r");
               nfree(tmp);
               fileCount++;
               fileSize += Report[b].size;
           } else {
               break;
           } /* endif */
       } /* endfor */
       i = b;
       xscatprintf(&(msg->text), " %s\r", print_ch(77, '-'));
       xscatprintf(&(msg->text), " %u bytes in %u file(s)\r", fileSize, fileCount);
   } /* endfor */
   if (msg) {
       writeMsgToSysop(msg, annArea);
       freeMsgBuffers(msg);
       nfree(msg);
       if (config->echotosslog != NULL) {
           echotosslog = fopen (config->echotosslog, "a");
           if (echotosslog != NULL) {
               fprintf(echotosslog,"%s\n",annArea);
               fclose(echotosslog);
           }
       }
   }
}


void report()
{
    getReportInfo();
    if(Report)
    {
        reportNewFiles();
        freeReportInfo();
    }
}
