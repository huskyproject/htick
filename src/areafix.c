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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MSDOS
#include <fidoconfig.h>
#else
#include <fidoconf.h>
#endif

#include <common.h>
#include <fcommon.h>
#include <global.h>
#include <pkt.h>
#include <version.h>
#include <toss.h>
#include <patmat.h>
#include <ctype.h>
#include <progprot.h>
#include <strsep.h>
#include <areafix.h>
#include <hatch.h>

unsigned char RetFix;

void freeMsgBuff(s_message *msg)
{
    free(msg->subjectLine);
    free(msg->toUserName);
    free(msg->fromUserName);
    free(msg->text);
}

int strncasesearch(char *strL, char *strR, int len)
{
    char *str;
    int ret;
    
    str = (char*)calloc(strlen(strL)+1, sizeof(char));
    strcpy(str, strL);
    if (strlen(str) > len) str[len] = 0;
    ret = stricmp(str, strR);
    free(str);
    return ret;
}

char *print_ch(int len, char ch)
{
    static char tmp[256];
    
    memset(tmp, ch, len);
    tmp[len]=0;
    return tmp;
}

char *errorRQ(char *line)
{
   char *report, err[] = "Error line";

   report = (char*)calloc(strlen(line)+strlen(err)+3, sizeof(char));
   sprintf(report, "%s %s\r", line, err);

   return report;
}

s_message *makeMessage(s_addr *origAddr, s_addr *destAddr, char *fromName, char *toName, char *subject, char netmail)
{
    // netmail == 0 - echomail
    // netmail == 1 - netmail
    time_t time_cur;
    s_message *msg;
    
    time_cur = time(NULL);
    
    msg = (s_message*)calloc(1, sizeof(s_message));
    
    msg->origAddr.zone = origAddr->zone;
    msg->origAddr.net = origAddr->net;
    msg->origAddr.node = origAddr->node;
    msg->origAddr.point = origAddr->point;

    msg->destAddr.zone = destAddr->zone;
    msg->destAddr.net = destAddr->net;
    msg->destAddr.node = destAddr->node;
    msg->destAddr.point = destAddr->point;

	

    msg->fromUserName = (char*)calloc(strlen(fromName)+1, sizeof(char));
    strcpy(msg->fromUserName, fromName);
    
    msg->toUserName = (char*)calloc(strlen(toName)+1, sizeof(char));
    strcpy(msg->toUserName, toName);
    
    msg->subjectLine = (char*)calloc(strlen(subject)+1, sizeof(char));
    strcpy(msg->subjectLine, subject);

    msg->attributes = MSGLOCAL;
    if (netmail) {
       msg->attributes |= MSGPRIVATE;
       msg->netMail = 1;
    }
    if (config->areafixKillReports) msg->attributes |= MSGKILL;
    
    strftime(msg->datetime, 21, "%d %b %y  %T", localtime(&time_cur));
    
    
    return msg;
}

char *aka2str(s_addr aka) {

    static char straka[24];
    if (aka.point) sprintf(straka,"%u:%u/%u.%u",aka.zone,aka.net,aka.node,aka.point);
    else sprintf(straka,"%u:%u/%u",aka.zone,aka.net,aka.node);
	
    return straka;
}	


int subscribeCheck(s_filearea area, s_message *msg, s_link *link) {
	int i;
	for (i = 0; i<area.downlinkCount;i++) {
		if (addrComp(msg->origAddr, area.downlinks[i]->link->hisAka)==0) return 0;
	}
	if (area.group != '\060')
	    if (link->AccessGrp) {
			if (config->PublicGroup) {
				if (strchr(link->AccessGrp, area.group) == NULL &&
					strchr(config->PublicGroup, area.group) == NULL) return 2;
			} else if (strchr(link->AccessGrp, area.group) == NULL) return 2;
	    } else if (config->PublicGroup) {
			if (strchr(config->PublicGroup, area.group) == NULL) return 2;
		} else return 2;
	if (area.hide) return 3;
	return 1;
}


int subscribeAreaCheck(s_filearea *area, s_message *msg, char *areaname, s_link *link) {
	int rc=4;
	
	if (!areaname) return rc;
	
	if (patimat(area->areaName,areaname)==1) {
		rc=subscribeCheck(*area, msg, link);
		// 0 - already subscribed
		// 1 - need subscribe
		// 2 - no access group
		// 3 - area is hidden
		if (area->manual) rc = 2;
	} else rc = 4; // this is another area
	
	return rc;
}

int delLinkFromArea(FILE *f, char *fileName, char *str) {
    long curpos, endpos, linelen=0, len;
    char *buff, *sbuff, *ptr, *tmp, *line;
	
    curpos = ftell(f);
    buff = readLine(f);
    buff = trimLine(buff);
    len = strlen(buff);

	sbuff = buff;

	while ( (ptr = strstr(sbuff, str)) ) {
		if (isspace(ptr[strlen(str)]) || ptr[strlen(str)]=='\000') break;
		sbuff = ptr+1;
	}
	
	line = ptr;
	
    if (ptr) {
	curpos += (ptr-buff-1);
	while (ptr) {
	    tmp = strseparate(&ptr, " \t");
	    if (ptr == NULL) {
		linelen = (buff+len+1)-line;
		break;
	    }
	    if (*ptr != '-') {
		linelen = ptr-line;
		break;
	    }
	    else {
		if (strncasesearch(ptr, "-r", 2)) {
		    if (strncasesearch(ptr, "-w", 2)) {
			if (strncasesearch(ptr, "-mn", 3)) {
			    linelen = ptr-line;
			    break;
			}
		    }
		}
	    }
	}
	fseek(f, 0L, SEEK_END);
	endpos = ftell(f);
	len = endpos-(curpos+linelen);
	buff = (char*)realloc(buff, len+1);
	memset(buff, 0, len+1);
	fseek(f, curpos+linelen, SEEK_SET);
	len = fread(buff, sizeof(char), (size_t) len, f);
 	fseek(f, curpos, SEEK_SET);
//	fputs(buff, f);
	fwrite(buff, sizeof(char), (size_t) len, f);
#ifdef __WATCOMC__
	fflush( f );
	fTruncate( fileno(f), endpos-linelen );
	fflush( f );
#else
	truncate(fileName, endpos-linelen);
#endif
    }
    free(buff);
    return 0;
}

// add string to file
// add string to file
int addstring(FILE *f, char *aka) {
	char *cfg;
	long areapos,endpos,cfglen,len;

	//current position
#ifndef UNIX
	/* in dos and win32 by default \n translates into 2 chars */
	fseek(f,-2,SEEK_CUR);
#else                                                   
	fseek(f,-1,SEEK_CUR);
#endif
	areapos=ftell(f);
	
	// end of file
	fseek(f,0l,SEEK_END);
	endpos=ftell(f);
	cfglen=endpos-areapos;
	
	// storing end of file...
	cfg = (char*) calloc((size_t) cfglen+1, sizeof(char));
	fseek(f,-cfglen,SEEK_END);
	len = fread(cfg,sizeof(char),(size_t) cfglen,f);
	
	// write config
	fseek(f,-cfglen,SEEK_END);
	fputs(" ",f);
	fputs(aka,f);
//	fputs(cfg,f);
	fwrite(cfg,sizeof(char),(size_t) len,f);
	fflush(f);
	
	free(cfg);
	return 0;
}

int delstring(FILE *f, char *fileName, char *straka, int before_str) {
	int al,i=1;
	char *cfg, c, j='\040';
	long areapos,endpos,cfglen;

	al=strlen(straka);

	// search for the aka string
	while ((i!=0) && ((j!='\040') || (j!='\011'))) {
		for (i=al; i>0; i--) {
			fseek(f,-2,SEEK_CUR);
			c=fgetc(f);
			if (straka[i-1]!=tolower(c)) {j = c; break;}
		}
	}
	
	//current position
	areapos=ftell(f);

	// end of file
	fseek(f,0l,SEEK_END);
	endpos=ftell(f);
	cfglen=endpos-areapos-al;
	
	// storing end of file...
	cfg=(char*) calloc((size_t) cfglen+1,sizeof(char));
	fseek(f,-cfglen-1,SEEK_END);
	fread(cfg,sizeof(char),(size_t) (cfglen+1),f);
	
	// write config
	fseek(f,-cfglen-al-1-before_str,SEEK_END);
	fputs(cfg,f);

	truncate(fileName,endpos-al-before_str);
	
	fseek(f,areapos-1,SEEK_SET);

	free(cfg);
	return 0;
}

void addlink(s_link *link, s_filearea *area) {
    char *test = NULL;
    
    area->downlinks = realloc(area->downlinks, sizeof(s_arealink*)*(area->downlinkCount+1));
    area->downlinks[area->downlinkCount] = (s_arealink*)calloc(1, sizeof(s_arealink));
    area->downlinks[area->downlinkCount]->link = link;
    
    if (link->optGrp) test = strchr(link->optGrp, area->group);
    area->downlinks[area->downlinkCount]->export = 1;
    area->downlinks[area->downlinkCount]->import = 1;
    area->downlinks[area->downlinkCount]->mandatory = 0;
    if (link->export) if (*link->export==0) {
	    if (link->optGrp == NULL || (link->optGrp && test)) {
		area->downlinks[area->downlinkCount]->export = 0;
	    }
	} 
    if (link->import) if (*link->import==0) {
	    if (link->optGrp == NULL ||  (link->optGrp && test)) {
		area->downlinks[area->downlinkCount]->import = 0;
	    }
	} 
    if (link->mandatory) if (*link->mandatory==1) {
	    if (link->optGrp == NULL || (link->optGrp && test)) {
		area->downlinks[area->downlinkCount]->mandatory = 1;
	    }
	} 
    area->downlinkCount++;
}

void removelink(s_link *link, s_filearea *area) {
	int i;
	s_link *links;

	for (i=0; i < area->downlinkCount; i++) {
           links = area->downlinks[i]->link;
           if (addrComp(link->hisAka, links->hisAka)==0) break;
	}
	
	free(area->downlinks[i]);
	area->downlinks[i] = area->downlinks[area->downlinkCount-1];
	area->downlinkCount--;
}

char *unlinked(s_message *msg, s_link *link)
{
    int i, rc, n;
    char *report, addline[256];
    s_filearea *area;
    
    area=config->fileAreas;
    
    sprintf(addline, "Unlinked fileareas to %s\r\r", aka2str(link->hisAka));
    report=(char*)calloc(strlen(addline)+1, sizeof(char));
    strcpy(report, addline);
    
    for (i=n=0; i<config->fileAreaCount; i++) {
	rc=subscribeCheck(area[i], msg, link);
	if (rc == 1) {
	    report=(char*)realloc(report, strlen(report)+
				strlen(area[i].areaName)+3);
	    strcat(report, " ");
	    strcat(report, area[i].areaName);
	    strcat(report, "\r");
	    n++;
	}
    }
    sprintf(addline, "\r%u areas unlinked\r", n);
    report=(char*)realloc(report, strlen(report)+strlen(addline)+1);
    strcat(report, addline);

    sprintf(addline,"FileFix: unlinked fileareas list sent to %s", aka2str(link->hisAka));
    writeLogEntry(htick_log, '8', addline);

    return report;
}

char *list(s_message *msg, s_link *link) {

	int i,j,active,avail,rc,desclen,len;
	int *areaslen;
	int maxlen;
	char *report, addline[256];
	
	areaslen = malloc(config->fileAreaCount * sizeof(int));

	maxlen = 0;
	for (i=0; i< config->fileAreaCount; i++) {
	   areaslen[i]=strlen(config->fileAreas[i].areaName);
	   if (areaslen[i]>maxlen) maxlen = areaslen[i];
	}
	
	sprintf(addline, "Available fileareas for %s\r\r", aka2str(link->hisAka));

	report=(char*)calloc(strlen(addline)+1,sizeof(char));
	strcpy(report, addline);

	for (i=active=avail=0; i< config->fileAreaCount; i++) {

	    rc=subscribeCheck(config->fileAreas[i],msg, link);
	    if (rc < 2) {
			if (config->fileAreas[i].description!=NULL)
			       desclen=strlen(config->fileAreas[i].description);
			else
			       desclen=0;

			len=strlen(report)+areaslen[i]+(maxlen-areaslen[i])+desclen+6;

			report=(char*) realloc(report, len);
				
			if (rc==0) {
				if (config->fileAreas[i].wgrp != NULL && atoi(config->fileAreas[i].wgrp) <= link->level
				    && config->fileAreas[i].rgrp != NULL && atoi(config->fileAreas[i].rgrp) <= link->level)
				    strcat(report,"& ");
				else if (config->fileAreas[i].rgrp != NULL && atoi(config->fileAreas[i].rgrp) <= link->level)
				    strcat(report,"+ ");
				else if (config->fileAreas[i].wgrp != NULL && atoi(config->fileAreas[i].wgrp) <= link->level)
				    strcat(report,"* ");
				else if (config->fileAreas[i].wgrp == NULL && config->fileAreas[i].rgrp == NULL)
				    strcat(report,"& ");
				else if (config->fileAreas[i].rgrp == NULL)
				    strcat(report,"+ ");
				else if (config->fileAreas[i].wgrp == NULL)
				    strcat(report,"* ");
				active++;
				avail++;
			} else {
				strcat(report,"  ");
				avail++;
			}
			strcat(report, config->fileAreas[i].areaName);
			if (desclen!=0)
			{
			       strcat(report," ");
			       for (j=0;j<(maxlen)-areaslen[i];j++) strcat(report,".");
                               strcat(report," ");
                               strcat(report,config->fileAreas[i].description);
			}
			strcat(report,"\r");
	    }
	}
	
	sprintf(addline,"\r '+'  You are receive files from this area.
	\r '*'  You can send files to this file echo.
	\r '&'  You can send and receive files.
	\r\r%i areas available for %s, %i areas active\r", avail, aka2str(link->hisAka), active);
	report=(char*) realloc(report, strlen(report)+strlen(addline)+1);
	strcat(report, addline);

	sprintf(addline,"FileFix: list sent to %s", aka2str(link->hisAka));
	writeLogEntry(htick_log, '8', addline);

        free(areaslen);
	
	return report;
}

char *linked(s_message *msg, s_link *link, int action)
{
    int i, n, rc;
    char *report, addline[256];

    if (link->Pause) 
        sprintf(addline, "\rPassive fileareas on %s\r\r", aka2str(link->hisAka));
    else
	sprintf(addline, "\rActive fileareas on %s\r\r", aka2str(link->hisAka));
							
    report=(char*)calloc(strlen(addline)+1, sizeof(char));
    strcpy(report, addline);
    
    for (i=n=0; i<config->fileAreaCount; i++) {
	rc=subscribeCheck(config->fileAreas[i], msg, link);
	if (rc==0) {
	    if (action == 1) {
	       report=(char*)realloc(report, strlen(report)+
			       strlen(config->fileAreas[i].areaName)+4);
	        if (config->fileAreas[i].wgrp != NULL && atoi(config->fileAreas[i].wgrp) <= link->level
				    && config->fileAreas[i].rgrp != NULL && atoi(config->fileAreas[i].rgrp) <= link->level)
		    strcat(report,"& ");
		else if (config->fileAreas[i].rgrp != NULL && atoi(config->fileAreas[i].rgrp) <= link->level)
		    strcat(report,"+ ");
		else if (config->fileAreas[i].wgrp != NULL && atoi(config->fileAreas[i].wgrp) <= link->level)
		    strcat(report,"* ");
		else if (config->fileAreas[i].wgrp == NULL && config->fileAreas[i].rgrp == NULL)
		    strcat(report,"& ");
		else if (config->fileAreas[i].rgrp == NULL)
		    strcat(report,"+ ");
		else if (config->fileAreas[i].wgrp == NULL)
		    strcat(report,"* ");
	    } else {
	    report=(char*)realloc(report, strlen(report)+
			    strlen(config->fileAreas[i].areaName)+3);
	       strcat(report," ");
	    }
	    strcat(report, config->fileAreas[i].areaName);
	    strcat(report, "\r");
	    n++;
	}
    }
    if (action == 1) sprintf(addline, "\r '+'  You are receive files from this area.
	\r '*'  You can send files to this file echo.
	\r '&'  You can send and receive files.
	\r\r%u areas linked for %s\r", n, aka2str(link->hisAka));
    else sprintf(addline, "\r%u areas linked\r", n);
    report=(char*)realloc(report, strlen(report)+strlen(addline)+1);
    strcat(report, addline);
    return report;
}

char *help(s_link *link) {
	FILE *f;
	int i=1;
	char *help, addline[256];
	long endpos;

	if (config->filefixhelp!=NULL) {
		if ((f=fopen(config->filefixhelp,"r")) == NULL)
			{
				fprintf(stderr,"FileFix: cannot open help file \"%s\"\n",
						config->filefixhelp);
				return NULL;
			}
		
		fseek(f,0l,SEEK_END);
		endpos=ftell(f);
		
		help=(char*) calloc((size_t) endpos,sizeof(char));

		fseek(f,0l,SEEK_SET);
		fread(help,1,(size_t) endpos,f);
		
		for (i=0; i<endpos; i++) {
		   if (help[i]=='\r' && (i+1)<endpos && help[i+1]=='\n') help[i]=' ';
		   if (help[i]=='\n') help[i]='\r';
		}

		fclose(f);

		sprintf(addline,"FileFix: help sent to %s",link->name);
		writeLogEntry(htick_log, '8', addline);

		return help;
	}

	return NULL;
}

char *available(s_link *link) {                                                 
/*        FILE *f;                                                                
        int i=1;                                                                
        char *avail, addline[256];                                              
        long endpos;                                                            
                                                                                
        if (config->available!=NULL) {                                          
                if ((f=fopen(config->available,"r")) == NULL)                   
                        {                                                       
                                fprintf(stderr,"areafix: cannot open Available Areas file \"%s\"\n",                                                            
                                                config->areafixhelp);           
                                return NULL;                                    
                        }                                                       
                                                                                
                fseek(f,0l,SEEK_END);                                           
                endpos=ftell(f);                                                
                                                                                
                avail=(char*) calloc((size_t) endpos,sizeof(char*));            
                                                                                
                fseek(f,0l,SEEK_SET);                                           
                fread(avail,1,(size_t) endpos,f);                               
                for (i=0; i<endpos; i++) if (avail[i]=='\n') avail[i]='\r';     
                                                                                
                fclose(f);                                                      
                                                                                
                sprintf(addline,"areafix: Available Area List sent to %s",link->name);                                                                          
                writeLogEntry(htick_log, '8', addline);                               
                                   
                return avail;                                                   
        }                                                                       
  */                                                                              
        return NULL;                                                            
}                                                                               

int changeconfig(char *fileName, s_filearea *area, s_link *link, int action) {
	FILE *f;
	char *cfgline, *token, *running, *areaName, work=1;
	long pos;
	
	areaName = area->areaName;

	if ((f=fopen(fileName,"r+")) == NULL)
		{
			fprintf(stderr,"FileFix: cannot open config file %s \n", fileName);
			return 1;
		}
	
	while (work) {
		pos = ftell(f);
		if ((cfgline = readLine(f)) == NULL) break;
		cfgline = trimLine(cfgline);
		if ((cfgline[0] != '#') && (cfgline[0] != 0)) {
			
			running = cfgline;
			token = strseparate(&running, " \t");
			
			if (stricmp(token, "include")==0) {
				while (*running=='\t') running++;
				token=strseparate(&running, " \t");
				changeconfig(token, area, link, action);
			}			
			else if (stricmp(token, "filearea")==0) {
				token = strseparate(&running, " \t"); 
				if (stricmp(token, areaName)==0) {
				    switch 	(action) {
				    case 0: 
					addstring(f,aka2str(link->hisAka));
					break;
 				    case 1:	fseek(f, pos, SEEK_SET);
					delLinkFromArea(f, fileName, aka2str(link->hisAka));
					break;
				    default: break;
				    }
				    work = 0;
				}
			}
			
		}
		free(cfgline);
	}
	
	fclose(f);
	return 0;
}

char *subscribe(s_link *link, s_message *msg, char *cmd) {
	int i, rc=4;
	char *line, *report, addline[256], logmsg[256];
	s_filearea *area;

	line = cmd;
	
	if (line[0]=='+') line++;
	
	report=(char*)calloc(1, sizeof(char));

	for (i=0; i<config->fileAreaCount; i++) {
		rc=subscribeAreaCheck(&(config->fileAreas[i]),msg,line, link);
		if (rc == 4) continue;
		
		area = &(config->fileAreas[i]);
		
		switch (rc) {
		    case 0: 
			sprintf(addline,"%s Already linked\r", area->areaName);
			sprintf(logmsg,"FileFix: %s already linked to %s",
					aka2str(link->hisAka), area->areaName);
			writeLogEntry(htick_log, '8', logmsg);
    			break;
		    case 1: 
		    case 3: 
			changeconfig (getConfigFileName(), area, link, 0);
			addlink(link, area);
			sprintf(addline,"%s Added\r",area->areaName);
			sprintf(logmsg,"FileFix: %s subscribed to %s",aka2str(link->hisAka),area->areaName);
			writeLogEntry(htick_log, '8', logmsg);
			break;
		    default :
			sprintf(logmsg,"FileFix: filearea %s -- no access for %s",
					area->areaName, aka2str(link->hisAka));
			writeLogEntry(htick_log, '8', logmsg);
			continue;
		}
		report=(char*)realloc(report, strlen(report)+strlen(addline)+1);
		strcat(report, addline);
	}
	
	if (*report == 0) {
	    sprintf(addline,"%s Not found\r",line);
	    sprintf(logmsg,"FileFix: filearea %s is not found",line);
	    writeLogEntry(htick_log, '8', logmsg);
	    report=(char*)realloc(report, strlen(addline)+1);
	    strcpy(report, addline);
	}
	return report;
}

char *unsubscribe(s_link *link, s_message *msg, char *cmd) {
	int i, c, rc = 2;
	char *line, addline[256], logmsg[256];
	char *report=NULL;
	s_filearea *area;
	
	line = cmd;
	
	if (line[1]=='-') return NULL;
	line++;
	
	report=(char*)calloc(1, sizeof(char));
	
	for (i = 0; i< config->fileAreaCount; i++) {
		rc=subscribeAreaCheck(&(config->fileAreas[i]),msg,line, link);
		if ( rc==4 ) continue;
	
		area = &(config->fileAreas[i]);
		
		for (c = 0; c<area->downlinkCount; c++) {
		    if (link == area->downlinks[c]->link) {
			if (area->downlinks[c]->mandatory) rc=5;
			break;
		    }
		}
		
		switch (rc) {
		case 0: removelink(link, area);
			changeconfig (getConfigFileName(),  area, link, 1);
			sprintf(addline,"%s Unlinked\r",area->areaName);
			sprintf(logmsg,"FileFix: %s unlinked from %s",aka2str(link->hisAka),area->areaName);
			writeLogEntry(htick_log, '8', logmsg);
			break;
		case 1: if (strstr(line, "*")) continue;
			sprintf(addline,"%s Not linked\r",line);
			sprintf(logmsg,"FileFix: area %s is not linked to %s",
					area->areaName, aka2str(link->hisAka));
			writeLogEntry(htick_log, '8', logmsg);
			break;
		case 5: sprintf(addline,"%s Unlink is not possible\r", area->areaName);
			sprintf(logmsg,"FileFix: area %s -- unlink is not possible for %s",
					area->areaName, aka2str(link->hisAka));
			writeLogEntry(htick_log, '8', logmsg);
			break;
		default: sprintf(logmsg,"FileFix: area %s -- no access for %s",
						 area->areaName, aka2str(link->hisAka));
			writeLogEntry(htick_log, '8', logmsg);
			continue;
		}
		
		report=(char*)realloc(report, strlen(report)+strlen(addline)+1);
		strcat(report, addline);
	}
	if (*report == 0) {
		sprintf(addline,"%s Not found\r",line);
		sprintf(logmsg,"FileFix: area %s is not found", line);
		writeLogEntry(htick_log, '8', logmsg);
		report=(char*)realloc(report, strlen(addline)+1);
		strcpy(report, addline);
	}
	return report;
}

char *resend(s_link *link, s_message *msg, char *cmd)
{
    int rc = 0, i;
    char *line, addline[256], logmsg[256];
    char *report=NULL, *token, filename[100], filearea[100];
    s_filearea *area = NULL;

   report=(char*)calloc(1, sizeof(char));
   line = cmd;
   line=stripLeadingChars(line, " \t");
   token = strtok(line, " \t\0");
   if (token == NULL)
      sprintf(addline,"Error in line! Format: %%Resend <file> <filearea>\r");
   else {
      strcpy(filename,token);
      token=stripLeadingChars(strtok(NULL, "\0"), " ");
      if (token == NULL)
         sprintf(addline,"Error in line! Format: %%Resend <file> <filearea>\r");
      else {
         strcpy(filearea,token);
	 rc = 1;
	 for (i = 0; i<area->downlinkCount;i++) {
	    if (addrComp(msg->origAddr, area->downlinks[i]->link->hisAka)==0)
	       rc = 0;
	 }
	 area = getFileArea(config,filearea);
	 if (area != NULL && rc == 1 && area->manual == 1) rc = 5;
	 else rc = send(filename,filearea,addr2string(&link->hisAka));
	 switch (rc) {
	 case 0: sprintf(addline,"Send %s from %s for %s, %s\r",
                         filename,filearea,link->name,addr2string(&link->hisAka));
		 break;
	 case 1: sprintf(addline,"Error: Passthrough filearea %s!\r",filearea);
		 sprintf(logmsg,"FileFix %%Resend: Passthrough filearea %s", filearea);
		 writeLogEntry(htick_log, '8', logmsg);
	         break;
	 case 2: sprintf(addline,"Error: Filearea %s not found!\r",filearea);
		 sprintf(logmsg,"FileFix %%Resend: Filearea %s not found", filearea);
		 writeLogEntry(htick_log, '8', logmsg);
	         break;
	 case 3: sprintf(addline,"Error: File %s not found!\r",filename);
		 sprintf(logmsg,"FileFix %%Resend: File %s not found", filename);
		 writeLogEntry(htick_log, '8', logmsg);
	         break;
	 case 5: sprintf(addline,"Error: You don't have access for filearea %s!\r",filearea);
		 sprintf(logmsg,"FileFix %%Resend: Link don't have access for filearea %s", filearea);
		 writeLogEntry(htick_log, '8', logmsg);
	         break;
	 }
      }
   }

   report=(char*)realloc(report, strlen(report)+strlen(addline)+1);
   strcat(report, addline);

   return report;
}

int testAddr(char *addr, s_addr hisAka)
{
    s_addr aka;
    string2addr(addr, &aka);
    if (addrComp(aka, hisAka)==0) return 1;
    return 0;
}

int changepause(char *confName, s_link *link)
{
    char *cfgline, *token;
    char *line, logmsg[256];
    long curpos, endpos, cfglen;
    int rc;
    FILE *f_conf;
    
	if ((f_conf=fopen(confName,"r+")) == NULL)
		{
			fprintf(stderr,"FileFix: cannot open config file %s \n", confName);
			return 1;
		}
	
    while ((cfgline = readLine(f_conf)) != NULL) {
		cfgline = trimLine(cfgline);
		if (*cfgline == 0 || *cfgline == '#') continue;
		line = cfgline;
		token = strseparate(&line, " \t");
		if (stricmp(token, "include")==0) {
			while (*line=='\t') line++;
			token=strseparate(&line, " \t");
			changepause(token, link);
		}			
		if (stricmp(token, "link") == 0) {
			free(cfgline);
			for (rc = 0; rc == 0; ) {
				fseek(f_conf, 0L, SEEK_CUR);
				curpos = ftell(f_conf);
				if ((cfgline = readLine(f_conf)) == NULL) { 
					rc = 2;
					break;
				}
				cfgline = trimLine(cfgline);
				if (!*cfgline || *cfgline == '#') continue;
				line = cfgline;
				token = strseparate(&line, " \t");
				if (stricmp(token, "link") == 0) {
					fseek(f_conf, curpos, SEEK_SET);
					rc = 1;
				}
				if (stricmp(token, "aka") == 0) break;
				free(cfgline);
			}
			if (rc == 2) return 0;
			if (rc == 1) continue;
			token = strseparate(&line, " \t");
			if (testAddr(token, link->hisAka)) {
				
				curpos = ftell(f_conf);
				
				fseek(f_conf, 0L, SEEK_END);
				endpos = ftell(f_conf);
				
				cfglen=endpos-curpos;
				
				line = (char*)calloc(cfglen+1, sizeof(char));
				fseek(f_conf, curpos, SEEK_SET);
				fread(line, sizeof(char), cfglen, f_conf);
		
				fseek(f_conf, curpos, SEEK_SET);
				fputs("Pause\n", f_conf);
				fputs(line, f_conf);
				free(line);
				free(cfgline);
				link->Pause = 1;
				sprintf(logmsg,"FileFix: system %s set passive", aka2str(link->hisAka));
				writeLogEntry(htick_log, '8', logmsg);
				break;
			}
		}
		free(cfgline);
    }
    fclose(f_conf);
    return 1;
}

char *pause_link(s_message *msg, s_link *link)
{
    char *tmp;
    char *report=NULL;
    
    if (link->Pause == 0) {
	if (changepause(getConfigFileName(), link) == 0) return NULL;    
    }

    report = linked(msg, link, 0);
    tmp = (char*)calloc(80, sizeof(char));
    strcpy(tmp, " System switched to passive\r");
    tmp = (char*)realloc(tmp, strlen(report)+strlen(tmp)+1);
    strcat(tmp, report);
    free(report);
    return tmp;
}

int changeresume(char *confName, s_link *link)
{
    FILE *f_conf;
    char *cfgline, *line, *token, logmsg[256];
    int rc=0;
    long curpos, remstr, endpos, cfglen;
    
	if ((f_conf=fopen(confName,"r+")) == NULL)
		{
			fprintf(stderr,"FileFix: cannot open config file %s \n", confName);
			return 1;
		}
    
    while ((cfgline = readLine(f_conf)) != NULL) {
		cfgline = trimLine(cfgline);
		if (*cfgline == 0 || *cfgline == '#') continue;
		line = cfgline;
		token = strseparate(&line, " \t");
		if (stricmp(token, "include")==0) {
			while (*line=='\t') line++;
			token=strseparate(&line, " \t");
			changeresume(token, link);
		}			
		if (stricmp(token, "link") == 0) {
			free(cfgline);
			for (rc = 0; rc == 0; ) {
				fseek(f_conf, 0L, SEEK_CUR);
				curpos = ftell(f_conf);
				if ((cfgline = readLine(f_conf)) == NULL) { 
					rc = 2;
					break;
				}
				if (*cfgline == 0 || *cfgline == '#') continue;
				line = cfgline;
				token = strseparate(&line, " \t");
				if (stricmp(token, "link") == 0) {
					fseek(f_conf, curpos, SEEK_SET);
					rc = 1;
				}
				if (stricmp(token, "aka") == 0) break;
				free(cfgline);
			}
			if (rc == 2) break;
			if (rc == 1) continue;
			token = strseparate(&line, " \t");
			if (testAddr(token, link->hisAka)) {
				free(cfgline);
				for (rc = 0; rc == 0; ) {
					fseek(f_conf, 0L, SEEK_CUR);
					curpos = ftell(f_conf);
					if ((cfgline = readLine(f_conf)) == NULL) {
						rc = 1;
						break;
					}
					cfgline = trimLine(cfgline);
					if (*cfgline == 0 || *cfgline == '#') continue;
					line = cfgline;
					token = strseparate(&line, " \t");
					if (stricmp(token, "link") == 0) {
						rc = 1;
						break;
					}
					if (stricmp(token, "pause") == 0) break;
					free(cfgline);
				}
				if (rc) break;
				
				remstr = ftell(f_conf);
				
				fseek(f_conf, 0L, SEEK_END);
				endpos = ftell(f_conf);
				
				cfglen=endpos-remstr;
				
				line = (char*)calloc(cfglen+1, sizeof(char));
				fseek(f_conf, remstr, SEEK_SET);
				cfglen = fread(line, sizeof(char), (size_t) cfglen, f_conf);
				
				fseek(f_conf, curpos, SEEK_SET);
//				fputs(line, f_conf);
				fwrite(line, sizeof(char), (size_t) cfglen, f_conf);
#ifdef __WATCOMC__
				fflush( f_conf );
				fTruncate( fileno(f_conf), endpos-(remstr-curpos) );
				fflush( f_conf );
#else
				truncate(confName, endpos-(remstr-curpos));
#endif
				free(line);
				free(cfgline);
				link->Pause = 0;
				sprintf(logmsg,"FileFix: system %s set active",	aka2str(link->hisAka));
				writeLogEntry(htick_log, '8', logmsg);
				break;
    	    }
		}
		free(cfgline);
    }
    fclose(f_conf);
    if (rc) return 0;
    return 1;
}

char *resume_link(s_message *msg, s_link *link)
{
    char *tmp, *report=NULL;
    
    if (link->Pause) {
		if (changeresume(getConfigFileName(), link) == 0) return NULL;
    }
	
    report = linked(msg, link, 0);
    tmp = (char*)calloc(80, sizeof(char));
    strcpy(tmp, " System switched to active\r");
    tmp = (char*)realloc(tmp, strlen(report)+strlen(tmp)+1);
    strcat(tmp, report);
    free(report);
    return tmp;
}

int tellcmd(char *cmd) {
	char *line;

	if (strncasesearch(cmd, "* Origin:", 9) == 0) return NOTHING;
	line = strpbrk(cmd, " \t");
	if (line && *cmd != '%') *line = 0;

	line = cmd;

	switch (line[0]) {
	case '%': 
		line++;
		if (*line == 0) return ERROR;
		if (stricmp(line,"list")==0) return LIST;
		if (stricmp(line,"help")==0) return HELP;
		if (stricmp(line,"unlinked")==0) return UNLINK;
		if (stricmp(line,"linked")==0) return LINKED;
		if (stricmp(line,"query")==0) return LINKED;
		if (stricmp(line,"pause")==0) return PAUSE;
		if (stricmp(line,"resume")==0) return RESUME;
		if (stricmp(line,"info")==0) return INFO;
		if (strncasesearch(line, "resend", 6)==0)
		   if (line[6] != 0) return RESEND;
		return ERROR;
	case '\001': return NOTHING;
	case '\000': return NOTHING;
	case '-'  : return DEL;
	case '+': line++; if (line[0]=='\000') return ERROR;
	default: return ADD;
	}
	
	return 0;
}

char *processcmd(s_link *link, s_message *msg, char *line, int cmd) {
	
	char *report;
	
	switch (cmd) {

	case NOTHING: return NULL;

	case LIST: report = list (msg, link);
		RetFix=LIST;
		break;
	case HELP: report = help (link);
		RetFix=HELP;
		break;
	case ADD: report = subscribe (link,msg,line);
		RetFix=ADD;
		break;
	case DEL: report = unsubscribe (link,msg,line);
		RetFix=DEL;
		break;
	case UNLINK: report = unlinked (msg, link);
		RetFix=UNLINK;
		break;
	case LINKED: report = linked (msg, link, 1);
		RetFix=LINKED;
		break;
	case PAUSE: report = pause_link (msg, link);
		RetFix=PAUSE;
		break;
	case RESUME: report = resume_link (msg, link);
		RetFix=RESUME;
		break;
/*	case INFO: report = info_link(msg, link);
		RetFix=INFO;
		break;*/
	case RESEND: report = resend(link, msg, line+7);
		RetFix=RESEND;
		break;
	case ERROR: report = errorRQ(line);
		RetFix=ERROR;
		break;
	default: return NULL;
	}
	
	return report;
}

char *areastatus(char *preport, char *text)
{
    char *pth, *ptmp, *tmp, *report, tmpBuff[256];
    pth = (char*)calloc(1, sizeof(char));
    tmp = preport;
    ptmp = strchr(tmp, '\r');
    while (ptmp) {
		*(ptmp++)=0;
        report=strchr(tmp, ' ');
		*(report++)=0;
        if (strlen(tmp) > 50) tmp[50] = 0;
		if (50-strlen(tmp) == 0) sprintf(tmpBuff, " %s  %s\r", tmp, report);
        else if (50-strlen(tmp) == 1) sprintf(tmpBuff, " %s   %s\r", tmp, report);
		else sprintf(tmpBuff, " %s %s  %s\r", tmp, print_ch(50-strlen(tmp)-1, '.'), report);
        pth=(char*)realloc(pth, strlen(tmpBuff)+strlen(pth)+1);
		strcat(pth, tmpBuff);
		tmp=ptmp;
		ptmp = strchr(tmp, '\r');
    }
    tmp = (char*)calloc(strlen(pth)+strlen(text)+1, sizeof(char));
    strcpy(tmp, text);
    strcat(tmp, pth);
    free(text);
    free(pth);
    return tmp;
}

void preprocText(char *preport, s_message *msg)
{
    char *text, tmp[80], kludge[100];
	
    sprintf(tmp, " \r--- %s FileFix\r", versionStr);
    createKludges(kludge, NULL, &msg->origAddr, &msg->destAddr);
    text=(char*) malloc(strlen(kludge)+strlen(preport)+strlen(tmp)+1);
    strcpy(text, kludge);
    strcat(text, preport);
    strcat(text, tmp);
    msg->textLength=(int)strlen(text);
    msg->text=text;
}

char *textHead()
{
    char *text_head, tmpBuff[256];
    
    sprintf(tmpBuff, " FileArea%sStatus\r",	print_ch(44,' '));
	sprintf(tmpBuff+strlen(tmpBuff)," %s  -------------------------\r",print_ch(50, '-')); 
    text_head=(char*)calloc(strlen(tmpBuff)+1, sizeof(char));
    strcpy(text_head, tmpBuff);
    return text_head;
}

void RetMsg(s_message *msg, s_link *link, char *report, char *subj)
{
    s_message *tmpmsg;
    
    tmpmsg = makeMessage(link->ourAka, &(link->hisAka), msg->toUserName, msg->fromUserName, subj, 1);
    preprocText(report, tmpmsg);
    
    writeNetmail(tmpmsg);
    
    freeMsgBuff(tmpmsg);
    free(tmpmsg);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


int processFileFix(s_message *msg)
{
	int security=1, notforme = 0;
	s_link *link = NULL;
	s_link *tmplink = NULL;
	char tmp[80], *textBuff, *report=NULL, *preport, *token;
	
	// find link
	link=getLinkFromAddr(*config, msg->origAddr);

	// this is for me?
	if (link!=NULL)	notforme=addrComp(msg->destAddr, *link->ourAka);
	
	// ignore msg for other link (maybe this is transit...)
	if (notforme) {
		return 1;
	}


	// security ckeck. link,araefixing & password.
	if (link != NULL) {
		if (link->FileFix==1) {
			if (link->fileFixPwd!=NULL) {
				if (stricmp(link->fileFixPwd,msg->subjectLine)==0) security=0;
                                else security=3;
                        }
		} else security=2;
	} else security=1;

	if (!security) {
		
		textBuff = msg->text;
		token = strseparate (&textBuff, "\n\r");

		while(token != NULL) {
			preport = processcmd( link, msg,  stripLeadingChars(token, " \t"), tellcmd (token) );
			if (preport != NULL) {
				switch (RetFix) {
				case LIST:
					RetMsg(msg, link, preport, "list request");
					break;
				case HELP:
					RetMsg(msg, link, preport, "help request");
					break;
				case ADD:
					if (report == NULL) report = textHead();
					report = areastatus(preport, report);
					break;
				case DEL:
					if (report == NULL) report = textHead();
					report = areastatus(preport, report);
					break;
				case UNLINK:
					RetMsg(msg, link, preport, "unlinked request");
					break;
				case LINKED:
					RetMsg(msg, link, preport, "linked request");
					sprintf(tmp,"FileFix: linked fileareas list sent to %s", aka2str(link->hisAka));
					writeLogEntry(htick_log, '8', tmp);
					break;
				case PAUSE:
					RetMsg(msg, link, preport, "node change request");
					break;
				case RESUME:
					RetMsg(msg, link, preport, "node change request");
					break;
				case INFO:
					RetMsg(msg, link, preport, "link information");
					break;
				case RESEND:
					RetMsg(msg, link, preport, "resend request");
 					break;
				case ERROR:
					if (report == NULL) report = textHead();
					report = areastatus(preport, report);
					break;
				default: break;
				}
				
				free(preport);
			}
			token = strseparate (&textBuff, "\n\r");
		}
		
	} else {
		
		if (link == NULL) {
			tmplink = (s_link*)calloc(1, sizeof(s_link));
			tmplink->ourAka = &(msg->destAddr);
			tmplink->hisAka.zone = msg->origAddr.zone;
			tmplink->hisAka.net = msg->origAddr.net;
			tmplink->hisAka.node = msg->origAddr.node;
			tmplink->hisAka.point = msg->origAddr.point;
			link = tmplink;
		}
		// security problem
		
		switch (security) {
		case 1:
			sprintf(tmp, " \r your system is unknown\r");
			break;
		case 2:
			sprintf(tmp, " \r filefix is turned off\r");
			break;
		case 3:
			sprintf(tmp, " \r password error\r");
			break;
		default:
			sprintf(tmp, " \r unknown error. mail to sysop.\r");
			break;
		}
		
		report=(char*) malloc(strlen(tmp)+1);
		strcpy(report,tmp);
		
		RetMsg(msg, link, report, "security violation");
		free(report);
		
		sprintf(tmp,"FileFix: security violation from %s", aka2str(link->hisAka));
		writeLogEntry(htick_log, '8', tmp);
		
		free(tmplink);
		
		return 0;
	}
	

	if ( report != NULL ) {
		preport=linked(msg, link, 0);
		report=(char*)realloc(report, strlen(report)+strlen(preport)+1);
		strcat(report, preport);
		free(preport);
		RetMsg(msg, link, report, "node change request");
		free(report);
	}


	sprintf(tmp,"FileFix: sucessfully done for %s",aka2str(link->hisAka));
	writeLogEntry(htick_log, '8', tmp);
	
	return 0;
}
