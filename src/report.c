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
#include <fidoconf/log.h>
#include <fidoconf/xstr.h>
#include <fidoconf/common.h>
#include <fidoconf/afixcmd.h>
#include <fidoconf/dirlayer.h>

#include "toss.h"
#include "global.h"
#include "report.h"
#include "version.h"

typedef struct {
    s_filearea *farea;
    UINT        begin; 
    UINT32      fSize;
    UINT        fCount;
} s_FAreaRepInfo;

s_ticfile* Report = NULL;
UINT rCount = 0;

s_FAreaRepInfo*   aList = NULL;
UINT aCount = 0;


void doSaveTic4Report(s_ticfile *tic)
{
    FILE *tichandle;
    unsigned int i;
    
    char *rpTicName = NULL;
    
    rpTicName = makeUniqueDosFileName(config->announceSpool,"tic",config);

    tichandle = fopen(rpTicName,"wb");
    
    if(tichandle == NULL){
        w_log(LL_CRIT, "Can't create file %s for file %s",rpTicName,tic->file);
        return;
    } else {
        w_log(LL_CREAT, "Report file %s created for file %s",rpTicName,tic->file);
    }
    
    fprintf(tichandle,"File %s\r\n",tic->file);
    fprintf(tichandle,"Area %s\r\n",tic->area);

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
    char* fname = NULL;

    dir = opendir(config->announceSpool);
    
    while ((file = readdir(dir)) != NULL) {
        if (patimat(file->d_name, "*.TIC") == 0)
            continue;
        xstrscat(&fname,config->announceSpool,file->d_name,NULL);
        Report = srealloc( Report, (rCount+1)*sizeof(s_ticfile) );
        w_log(LL_DEBUG, "Parsing Report file %s",file->d_name);
        parseTic( fname, &(Report[rCount]) );
        rCount++;
        nfree(fname);
    }
    closedir(dir);
    w_log(LL_DEBUG, "Sortin report information. number of entries: %d",rCount);
    qsort( (void*)Report, rCount, sizeof(s_ticfile), cmp_reportEntry ); 
}

void buildAccessList()
{
    UINT i = 0;
    s_filearea *currFArea = NULL;    

    for (i = 0; i < rCount; i++) {
        if( currFArea == NULL || stricmp(Report[i].area, currFArea->areaName))
        {
            aCount++;
            aList = srealloc( aList, (aCount)*sizeof(s_FAreaRepInfo));
            currFArea            = getFileArea(config,Report[i].area);
            aList[aCount-1].farea  = currFArea;
            aList[aCount-1].begin  = i;
            aList[aCount-1].fCount = 0;
            aList[aCount-1].fSize  = 0;
        }
        aList[aCount-1].fCount++;
        aList[aCount-1].fSize += Report[i].size; 
    }
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

    for(i = 0; i < rCount; i++)
        disposeTic(&(Report[i]));
    nfree(Report);
    nfree(aList);
    
    dir = opendir(config->announceSpool);
    w_log( LL_INFO, "Deleting report files");
    while ((file = readdir(dir)) != NULL) {
        if (patimat(file->d_name, "*.TIC") == 0)
            continue;
        xstrscat(&fname,config->announceSpool,file->d_name,NULL);
        remove(fname);
        w_log(LL_DELETE,"Removed file: %s",fname);
        nfree(fname);
    }
    closedir(dir);
}

void MakeDefaultRepDef()
{
    char* annArea;

    if(config->ADCount == 0)
    {
        config->AnnDefs = scalloc(1, sizeof(s_anndef));
        if (config->ReportTo) {
            annArea = config->ReportTo;
        } else {
            annArea = config->netMailAreas[0].areaName;
        }
        config->ADCount = 1;
        config->AnnDefs[0].annAreaTag = sstrdup(annArea);
    }
}

s_message* MakeReportMessage(ps_anndef pRepDef)
{
    s_message *msg = NULL;
    int netmail = 0;

    if (stricmp(pRepDef->annAreaTag,"netmail")==0)
        netmail=1;
    else if (getNetMailArea(config, pRepDef->annAreaTag) != NULL) 
        netmail=1;
    
    msg = makeMessage(
        pRepDef->annadrfrom ? pRepDef->annadrfrom : &(config->addr[0]),
        pRepDef->annadrto   ? pRepDef->annadrto   : &(config->addr[0]),
        pRepDef->annfrom    ? pRepDef->annfrom    : versionStr, 
        pRepDef->annto      ? pRepDef->annto      : (netmail ? config->sysop : "All"),
        pRepDef->annsubj    ? pRepDef->annsubj    : "New Files", 
        netmail,
        config->filefixKillReports);
    
    msg->text = createKludges(  
        config->disableTID,
        netmail ? NULL : pRepDef->annAreaTag, 
        pRepDef->annadrfrom ? pRepDef->annadrfrom : &(config->addr[0]),
        pRepDef->annadrto   ? pRepDef->annadrto   : &(config->addr[0]), 
        versionStr);
    xstrcat(&(msg->text), "\001FLAGS NPD\r");
    return msg;
}

void ReportOneFile(s_message* msg, s_ticfile* tic)
{
    char *tmp = NULL;
    if(strlen(tic->file) > 12)
        xscatprintf(&(msg->text)," %s\r%23ld ",
        tic->file,
        tic->size
        );
    else
        xscatprintf(&(msg->text)," %-12s %9ld ",
        tic->file,  
        tic->size
        );
    
    if (tic->anzldesc > 0) {
        tmp = formDesc(tic->ldesc, tic->anzldesc); 
    } else { 
        tmp = formDesc(tic->desc, tic->anzdesc); 
    }
    xstrcat(&(msg->text),tmp);
    if (config->originInAnnounce) {
        xscatprintf(&(msg->text), "%sOrig: %s\r",print_ch(24, ' '),aka2str(tic->origin));
    }
    if (tmp == NULL || tmp[0] == 0) xstrcat(&(msg->text),"\r");
    nfree(tmp);
}

int IsAreaMatched(char* areaname, ps_anndef pRepDef)
{
    int nRet = 0;
    UINT i   = 0;
    if(!pRepDef->annInclude)
    {
        nRet = 1;
    }
    for(i = 0; i < pRepDef->numbI; i++)
    {
        if(patimat(areaname,pRepDef->annInclude[i]) == 1)
        {
            nRet = 1;
            break;
        }
    }
    return nRet;
}

void reportNewFiles()
{
    UINT      fileCountTotal = 0;
    UINT32    fileSizeTotal = 0;

    UINT      i,j,ii;
    s_message *msg = NULL;
    char      *tmp = NULL;
    FILE      *echotosslog;
    ps_anndef RepDef;
    
    MakeDefaultRepDef();
    
    for(i = 0; i < config->ADCount; i++)    
    {
        RepDef = &(config->AnnDefs[i]);
        fileCountTotal = 0;
        fileSizeTotal = 0;
        for(j = 0; j < aCount; j++)
        {
            if(!IsAreaMatched(aList[j].farea->areaName,RepDef))
            {
                continue;
            }
            if(!msg) msg = MakeReportMessage(RepDef);
            
            xscatprintf(&(msg->text), "\r>Area : %s",strUpper(aList[j].farea->areaName));
            if(aList[j].farea->description)
            xscatprintf(&(msg->text), " : %s", aList[j].farea->description);
            xscatprintf(&(msg->text), "\r %s\r", print_ch(77, '-'));
            
            for(ii = aList[j].begin; ii < aList[j].begin+aList[j].fCount; ii++)
            {
                ReportOneFile(msg,&(Report[ii]));
            }
            xscatprintf(&(msg->text), " %s\r", print_ch(77, '-'));
            xscatprintf(&(msg->text), " %u bytes in %u file(s)\r", aList[j].fSize, aList[j].fCount);
            fileCountTotal += aList[j].fCount;
            fileSizeTotal  += aList[j].fSize;
        }
        if(msg)
        {
            xscatprintf(&(msg->text), "\r %s\r", print_ch(77, '='));
            xscatprintf(&(msg->text), ">Total %u bytes in %u file(s)\r", fileSizeTotal, fileCountTotal);
            
            writeMsgToSysop(msg, RepDef->annAreaTag);
            freeMsgBuffers(msg);
            nfree(msg);
            if (config->echotosslog != NULL) {
                echotosslog = fopen (config->echotosslog, "a");
                if (echotosslog != NULL) {
                    fprintf(echotosslog,"%s\n",RepDef->annAreaTag);
                    fclose(echotosslog);
                }
            }
        }
    }
}

void report()
{
    w_log( LL_INFO, "Start report generating...");
    getReportInfo();
    if(Report)
    {
        buildAccessList();
        reportNewFiles();
        freeReportInfo();
    }
    w_log( LL_INFO, "End report generating");
}
