/*****************************************************************************
 * AreaFix for HTICK (FTN Ticker / Request Processor)
 *****************************************************************************
 * Copyright (C) 1998-2000
 *
 * Max Levenkov
 *
 * Fido:     2:5000/117
 * Internet: sackett@mail.ru
 *
 * Novosibirsk, West Siberia, Russia
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#if ((!(defined(_MSC_VER) && (_MSC_VER >= 1200)) ) && (!defined(__TURBOC__)))
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <fidoconf/xstr.h>
#include <fidoconf/afixcmd.h>
#include <fidoconf/arealist.h>
#include <fidoconf/areatree.h>

#include <smapi/prog.h>
#include <smapi/patmat.h>

#include <fcommon.h>
#include <global.h>
#include <version.h>
#include <toss.h>
#include <areafix.h>
#include <hatch.h>
#include <scan.h>
#include <add_desc.h>

unsigned char RetFix;

int limitCheck(s_link *link, s_message *msg);

char *errorRQ(char *line)
{
   char *report = NULL, err[] = "Error line";
   xscatprintf(&report,"%s %s\r", line, err);
   return report;
}

int subscribeCheck(s_filearea area, s_message *msg, s_link *link)
{
  unsigned int i;
  int found = 0;

  for (i = 0; i<area.downlinkCount;i++)
  {
    if (addrComp(msg->origAddr, area.downlinks[i]->link->hisAka)==0) return 0;
  }

  if (area.group)
  {
    if (link->numAccessGrp > 0)
    {
      for (i = 0; i < link->numAccessGrp; i++)
	if (strcmp(area.group, link->AccessGrp[i]) == 0) found = 1;
    }

    if (config->numPublicGroup > 0)
    {
      for (i = 0; i < config->numPublicGroup; i++)
	if (strcmp(area.group, config->PublicGroup[i]) == 0) found = 1;
    }
  }
  else found = 1;

  if (found == 0) return 2;
  if (area.hide) return 3;
  return 1;
}


int subscribeAreaCheck(s_filearea *area, s_message *msg, char *areaname, s_link *link) {
	int rc=4;
	
	if (!areaname) return rc;
	
	if (patimat(area->areaName,areaname)==1) {
		rc=subscribeCheck(*area, msg, link);
		/*  0 - already subscribed */
		/*  1 - need subscribe */
		/*  2 - no access group */
		/*  3 - area is hidden */
		if (area->manual) rc = 2;
	} else rc = 4; /*  this is another area */
	
	return rc;
}

int unsubscribeAreaCheck(s_filearea *area, s_message *msg, char *areaname, s_link *link) {
	int rc=4;
	
	if (!areaname) return rc;
	
	if (patimat(area->areaName,areaname)==1) {
		rc=subscribeCheck(*area, msg, link);
		/*  0 - already subscribed */
		/*  1 - need subscribe */
		/*  2 - no access group */
		/*  3 - area is hidden */
		if (area->mandatory) rc = 2;
	} else rc = 4; /*  this is another area */
	
	return rc;
}

char *unlinked(s_message *msg, s_link *link)
{
    unsigned int i, rc, n;
    char *report=NULL;
    s_filearea *area;

    area=config->fileAreas;

    xscatprintf(&report, "Unlinked areas to %s\r\r", aka2str(link->hisAka));

    for (i=n=0; i<config->fileAreaCount; i++) {
	rc=subscribeCheck(area[i], msg, link);
	if (rc == 1) {
        xscatprintf(&report, " %s\r", area[i].areaName);
        n++;
	}
    }
    xscatprintf(&report, "\r%u areas unlinked\r", n);
    w_log( '8', "FileFix: unlinked fileareas list sent to %s", aka2str(link->hisAka));

    return report;
}

char *list(s_message *msg, s_link *link) {

    unsigned int i,j,active,avail,rc,desclen;
    unsigned int *areaslen;
    unsigned int maxlen;
    char *report=NULL;
    int readdeny, writedeny;

   areaslen = smalloc(config->fileAreaCount * sizeof(int));

   maxlen = 0;
   for (i=0; i< config->fileAreaCount; i++) {
      areaslen[i]=strlen(config->fileAreas[i].areaName);
      if (areaslen[i]>maxlen) maxlen = areaslen[i];
   }
   xscatprintf(&report, "Available fileareas for %s\r\r", aka2str(link->hisAka));

   for (i=active=avail=0; i< config->fileAreaCount; i++) {

      rc=subscribeCheck(config->fileAreas[i],msg, link);
      if (rc < 2) {
         if (config->fileAreas[i].description!=NULL)
            desclen=strlen(config->fileAreas[i].description);
         else
           desclen=0;

         if (rc==0) {
            readdeny = readCheck(&config->fileAreas[i], link);
            writedeny = writeCheck(&config->fileAreas[i], &(link->hisAka));
            if (!readdeny && !writedeny)
               xstrcat(&report,"& ");
            else if (writedeny)
               xstrcat(&report,"+ ");
            else xstrcat(&report,"* ");
            active++;
            avail++;
         } else {
            xstrcat(&report,"  ");
            avail++;
         }
         xstrcat(&report, config->fileAreas[i].areaName);
         if (desclen!=0) {
            xstrcat(&report," ");
            for (j=0;j<(maxlen)-areaslen[i];j++)
                xstrcat(&report,".");
            xstrcat(&report," ");
            xstrcat(&report,config->fileAreas[i].description);
         }
         xstrcat(&report,"\r");
      }
   }

   xscatprintf(&report, "\r '+'  You are receiving files from this area.\r '*'  You can send files to this file echo.\r '&'  You can send and receive files.\r\r%i areas available for %s, %i areas active\r", avail, aka2str(link->hisAka), active);
   if (link->ffixEchoLimit) xscatprintf(&report, "\rYour limit is %u areas for subscribe\r", link->ffixEchoLimit);

   w_log( LL_AREAFIX, "FileFix: list sent to %s", aka2str(link->hisAka));

   nfree(areaslen);

   return report;
}

char *linked(s_message *msg, s_link *link, int action)
{
    unsigned int i, n, rc;
    char *report=NULL;
    int readdeny, writedeny;

    if ((link->Pause & FPAUSE) == FPAUSE)
        xscatprintf(&report, "\rPassive fileareas on %s\r\r", aka2str(link->hisAka));
    else
        xscatprintf(&report, "\rActive fileareas on %s\r\r", aka2str(link->hisAka));
							

    for (i=n=0; i<config->fileAreaCount; i++) {
	rc=subscribeCheck(config->fileAreas[i], msg, link);
	if (rc==0) {
        if (action == 1) {
            readdeny = readCheck(&config->fileAreas[i], link);
            writedeny = writeCheck(&config->fileAreas[i], &(link->hisAka));
            if (!readdeny && !writedeny)
                xstrcat(&report,"& ");
            else if (writedeny)
                xstrcat(&report,"+ ");
            else
                xstrcat(&report,"* ");
        } else {
            xstrcat(&report," ");
        }
	    xstrcat(&report, config->fileAreas[i].areaName);
	    xstrcat(&report, "\r");
	    n++;
	}
    }
    if (action == 1)
        xscatprintf(&report, "\r '+'  You are receiving files from this area.\r '*'  You can send files to this file echo.\r '&'  You can send and receive files.\r\r%u areas linked for %s\r", n, aka2str(link->hisAka));
    else
        xscatprintf(&report, "\r%u areas linked\r", n);
    if (link->ffixEchoLimit) xscatprintf(&report, "\rYour limit is %u areas for subscribe\r", link->ffixEchoLimit);
    return report;
}

char *help(s_link *link) {
	FILE *f;
	int i=1;
	char *help = NULL;
	long endpos;

	if (config->filefixhelp!=NULL) {
		if ((f=fopen(config->filefixhelp,"r")) == NULL)
		  {
		    w_log( LL_ERR,"FileFix: cannot open help file '%s'",
						config->filefixhelp);
		    return NULL;
		  }
		
		fseek(f,0l,SEEK_END);
		endpos=ftell(f);
		
		help=(char*) scalloc((size_t) endpos + 1,sizeof(char));

		fseek(f,0l,SEEK_SET);
		endpos = fread(help,1,(size_t) endpos,f);
		
		for (i=0; i<endpos; i++) {
		   if (help[i]=='\r' && (i+1)<endpos && help[i+1]=='\n') help[i]=' ';
		   if (help[i]=='\n') help[i]='\r';
		}

		fclose(f);

		w_log( LL_AREAFIX, "FileFix: help sent to %s", aka2str(link->hisAka));

		return help;
	}

	return NULL;
}

char *available(s_link *link) {
    FILE *f;
    unsigned int j=0, found;
    unsigned int k;
    char *report = NULL, *line, *token, *running, linkAka[SIZE_aka2str];
    s_link *uplink=NULL;
    ps_arealist al;


    for (j = 0; j < config->linkCount; j++) {
	uplink = &(config->links[j]);

	found = 0;
	for (k = 0; k < link->numAccessGrp && uplink->LinkGrp; k++)
	    if (strcmp(link->AccessGrp[k], uplink->LinkGrp) == 0)
		found = 1;

	if ((uplink->forwardFileRequests && uplink->forwardFileRequestFile) &&
	    ((uplink->LinkGrp == NULL) || (found != 0))) {
	    if ((f=fopen(uplink->forwardFileRequestFile,"r")) == NULL) {
		w_log(LL_AREAFIX, "Filefix: cannot open forwardFileRequestFile \"%s\": %s",
		      uplink->forwardFileRequestFile, strerror(errno));
		return report;
	    }

	    xscatprintf(&report, "Available File List from %s:\r",
			aka2str(uplink->hisAka));

	    al = newAreaList();
        while ((line = readLine(f)) != NULL) {
            line = trimLine(line);
            if (line[0] != '\0') {
                running = line;
                token = strseparate(&running, " \t\r\n");
                addAreaListItem(al,0,0,token,running);

            }
		nfree(line);
	    }
	    fclose(f);

	    if(al->count) {
		sortAreaList(al);
		line = formatAreaList(al,78,NULL);
		xstrcat(&report,"\r");
		xstrcat(&report,line);
		nfree(line);
	    }

	    freeAreaList(al);

	    xscatprintf(&report, " %s\r\r",print_ch(77,'-'));

	    /*  warning! do not ever use aka2str twice at once! */
	    sprintf(linkAka, "%s", aka2str(link->hisAka));
	    w_log(LL_AREAFIX, "Filefix: Available File List from %s sent to %s", aka2str(uplink->hisAka), linkAka);
	}
    }

    if (report==NULL) {
	xstrcat(&report, "\r  no links for creating Available File List\r");
	w_log(LL_AREAFIX, "Filefix: no links for creating Available File List");
    }
    return report;
}

int changeconfig(char *fileName, s_filearea *area, s_link *link, int action) {
    char *cfgline = NULL, *line = NULL, *token = NULL, *areaName = NULL, *tmpPtr =NULL;
    long strbeg = 0, strend = -1;

    areaName = area->areaName;

    if (init_conf(fileName))
        return 1;

    while ((cfgline = configline()) != NULL) {
        line = sstrdup(cfgline);
        line = trimLine(line);
        line = stripComment(line);
        if (line[0] != 0) {
            tmpPtr = line = shell_expand(line);
            token = strseparate(&tmpPtr, " \t");
            if (stricmp(token, "filearea")==0) {
                token = strseparate(&tmpPtr, " \t");
                if (stricmp(token, areaName)==0) {
                    fileName = sstrdup(getCurConfName());
                    strend = get_hcfgPos();
                    if(strbeg > strend) strbeg = 0;
                    break;
                }
            }
        }
        strbeg = get_hcfgPos();
        nfree(cfgline);
        nfree(line);
    }
    close_conf();
    nfree(line);
    if (strend == -1) {
        nfree(cfgline);
        nfree(fileName);
        return 1; /*  impossible */
    }
    switch (action) {
    case 0:
        xstrscat(&cfgline, " ", aka2str(link->hisAka), NULL);
        break;
    case 1:
        DelLinkFromString(cfgline, link->hisAka);
        break;
    default: break;
    }
    InsertCfgLine(fileName, cfgline, strbeg, strend);
    nfree(cfgline);
    nfree(fileName);
    
    return 0;
}

/*  subscribe if (act==0),  unsubscribe if (act==1) */
int forwardRequestToLink( char *areatag,  char *descr,
                          s_link *uplink, s_link *dwlink,
                          int act)
{
    s_message *msg;
    char *base, pass[]="passthrough";

    if (uplink->msg == NULL) {
        msg = makeMessage(uplink->ourAka, &(uplink->hisAka), config->sysop,
            uplink->RemoteFileRobotName ? uplink->RemoteFileRobotName : "filefix",
            uplink->fileFixPwd ? uplink->fileFixPwd : "\x00", 1,
            config->filefixReportsAttr);
        msg->text = createKludges(config, NULL,
            uplink->ourAka, &(uplink->hisAka),
            versionStr);
	if (config->filefixReportsFlags)
	    xstrscat(&(msg->text), "\001FLAGS ", config->filefixReportsFlags, "\r", NULL);
        uplink->msg = msg;
    } else msg = uplink->msg;

    if (act==0) {
        if (getFileArea(areatag) == NULL) {
            base = uplink->fileBaseDir;
            if (config->createFwdNonPass == 0) uplink->fileBaseDir = pass;
            /*  create from own address */
            if (isOurAka(config,dwlink->hisAka)) {
                uplink->fileBaseDir = base;
            }
            autoCreate(areatag, descr, &(uplink->hisAka), &(dwlink->hisAka));
            uplink->fileBaseDir = base;
        }
        xstrscat(&msg->text, "+", areatag, "\r", NULL);
    } else  {
        xscatprintf(&(msg->text), "-%s\r", areatag);
    }
    return 0;
}


static int compare_links_priority(const void *a, const void *b) {
    int ia = *((int*)a);
    int ib = *((int*)b);
    if(config->links[ia].forwardFilePriority < config->links[ib].forwardFilePriority) return -1;
    else if(config->links[ia].forwardFilePriority > config->links[ib].forwardFilePriority) return 1;
    else return 0;
}

int forwardRequest(char *areatag, s_link *dwlink) {
    unsigned int i, rc = 1;
    s_link *uplink;
    int *Indexes;
    unsigned int Requestable = 0;

    /* From Lev Serebryakov -- sort Links by priority */
    Indexes = smalloc(sizeof(int)*config->linkCount);
    for (i = 0; i < config->linkCount; i++) {
	if (config->links[i].forwardFileRequests) Indexes[Requestable++] = i;
    }
    qsort(Indexes,Requestable,sizeof(Indexes[0]),compare_links_priority);
    for (i = 0; i < Requestable; i++) {
	uplink = &(config->links[Indexes[i]]);

    if (uplink->forwardFileRequests && (uplink->LinkGrp) ?
        grpInArray(uplink->LinkGrp,dwlink->AccessGrp,dwlink->numAccessGrp) : 1)
    {
        if (uplink->forwardFileRequestFile!=NULL) {
            char *descr = NULL;
            /* first try to find the areatag in forwardRequestFile */
            if (IsAreaAvailable(areatag,uplink->forwardFileRequestFile,&descr,1))
            {
                forwardRequestToLink(areatag,descr,uplink,dwlink,0);
                rc = 0;
            }
            else
            { rc = 2; }/* found link with freqfile, but there is no areatag */
            nfree(descr);
        } else {
            rc = 1;   /* link is anabled for forward requests but 
                         has no forwardFileRequestFile defined 
                      */
        }/* (uplink->forwardRequestFile!=NULL) */
        if (rc==0) {
            nfree(Indexes);
            return rc;
        }

    }   /*  if (uplink->forwardFileRequests && (uplink->LinkGrp) ? */
    }   /*  for (i = 0; i < Requestable; i++) { */
    /*  link with "forwardFileRequests on" not found */
    nfree(Indexes);
    return rc;
}


char *subscribe(s_link *link, s_message *msg, char *cmd) {
	unsigned int i, c, rc=4,found=0;
	char *line, *report = NULL;
	s_filearea *area;

	line = cmd;
	
	if (line[0]=='+') line++;
	
	report=(char*)scalloc(1, sizeof(char));

	for (i=0; i<config->fileAreaCount; i++) {
		rc=subscribeAreaCheck(&(config->fileAreas[i]),msg,line, link);
		if (rc == 4) continue;
 		if (rc!=0 && limitCheck(link,msg)) rc = 6; /* areas limit exceed for link */
		
		area = &(config->fileAreas[i]);
		
		for (c = 0; c<area->downlinkCount; c++) {
		    if (link == area->downlinks[c]->link) {
			if (area->downlinks[c]->manual) rc=5;
			break;
		    }
		}
        found = 1;
		switch (rc) {
		    case 0:
			xscatprintf(&report, "%s Already linked\r", area->areaName);
			w_log( LL_AREAFIX, "FileFix: %s already linked to %s", aka2str(link->hisAka), area->areaName);
    			break;
		    case 1:
                changeconfig (getConfigFileName(), area, link, 0);
                Addlink(link, NULL, area);
                xscatprintf(&report, "%s Added\r",area->areaName);
                w_log( LL_AREAFIX, "FileFix: %s subscribed to %s",aka2str(link->hisAka),area->areaName);
                break;
            case 3: /* report that area not found for hidden areas */
                break;
            case 5:
                xscatprintf(&report, "%s Link is not possible\r", area->areaName);
                w_log( LL_AREAFIX, "FileFix: area %s -- link is not possible for %s", area->areaName, aka2str(link->hisAka));
                break;
            default :
                xscatprintf(&report, "%s No access\r", area->areaName);
                w_log( LL_AREAFIX, "FileFix: filearea %s -- no access for %s", area->areaName, aka2str(link->hisAka));
                continue;
		}
    }
    if (rc!=0 && limitCheck(link,msg)) rc = 6; /* areas limit exceed for link */
    if(rc == 4 && link->denyFRA==0 && !found)
    {
        /*  try to forward request */
        if (forwardRequest(line, link) > 0) {
            xscatprintf(&report, "%s no uplinks to forward\r", line);
            w_log( LL_AREAFIX, "Filefix: %s - no uplinks to forward", line);
        }
        else {
            xscatprintf(&report, "%s request forwarded\r", line);
            w_log( LL_AREAFIX, "Filefix: %s - request forwarded", line);
        }
    }
    if (rc == 6) {   /* areas limit exceed for link */
    	xscatprintf(&report," %s - no access (full limit)\r",line);
	w_log( LL_AREAFIX,"Filefix: %s - no access (full limit)",line);
    }        
    if (*report == '\0') {
        xscatprintf(&report,"%s Not found\r",line);
        w_log( LL_AREAFIX, "FileFix: filearea %s is not found",line);
    }
    return report;
}

char *unsubscribe(s_link *link, s_message *msg, char *cmd) {
	unsigned int i, c, rc = 2;
	char *line;
	char *report=NULL;
	s_filearea *area;
	
	line = cmd;
	
	if (line[1]=='-') return NULL;
	line++;
	
	for (i = 0; i< config->fileAreaCount; i++) {
		rc=unsubscribeAreaCheck(&(config->fileAreas[i]),msg,line, link);
		if ( rc==4 ) continue;
	
		area = &(config->fileAreas[i]);
		
		for (c = 0; c<area->downlinkCount; c++) {
		    if (link == area->downlinks[c]->link) {
			if (area->downlinks[c]->mandatory) rc=5;
			break;
		    }
		}
		
		switch (rc) {
        case 0: RemoveLink(link, NULL, area);
			changeconfig (getConfigFileName(),  area, link, 1);
			xscatprintf(&report, "%s Unlinked\r",area->areaName);
			w_log( '8', "FileFix: %s unlinked from %s",aka2str(link->hisAka),area->areaName);
			break;
		case 1: if (strstr(line, "*")) continue;
			xscatprintf(&report, "%s Not linked\r",line);
			w_log( '8', "FileFix: area %s is not linked to %s", area->areaName, aka2str(link->hisAka));
			break;
		case 5:
            xscatprintf(&report, "%s Unlink is not possible\r", area->areaName);
			w_log( '8', "FileFix: area %s -- unlink is not possible for %s", area->areaName, aka2str(link->hisAka));
			break;
		default: w_log( '8', "FileFix: area %s -- no access for %s", area->areaName, aka2str(link->hisAka));
			continue;
		}
	}
	if (!report) {
		xscatprintf(&report, "%s Not found\r",line);
		w_log( '8', "FileFix: area %s is not found", line);
	}
	return report;
}

char *resend(s_link *link, s_message *msg, char *cmd)
{
    unsigned int rc, i;
    char *line;
    char *report=NULL, *token = NULL, *filename=NULL, *filearea=NULL;
    s_filearea *area = NULL;

    line = cmd;
    line=stripLeadingChars(line, " \t");
    token = strtok(line, " \t\0");
    if (token == NULL)
    {
        xscatprintf(&report, "Error in line! Format: %%Resend <file> <filearea>\r");
        return report;
    }
    filename = sstrdup(token);
    token=stripLeadingChars(strtok(NULL, "\0"), " ");
    if (token == NULL)
    {
        nfree(filename);
        xscatprintf(&report, "Error in line! Format: %%Resend <file> <filearea>\r");
        return report;
    }
    filearea = sstrdup(token);
    area = getFileArea(filearea);
    if (area != NULL) {
        rc = 1;
        for (i = 0; i<area->downlinkCount;i++)
            if (addrComp(msg->origAddr, area->downlinks[i]->link->hisAka)==0)
                rc = 0;
            if (rc == 1 && area->manual == 1) rc = 5;
            else rc = send(filename,filearea,aka2str(link->hisAka));
            switch (rc) {
            case 0: xscatprintf(&report, "Send %s from %s for %s\r",
                        filename,filearea,aka2str(link->hisAka));
                break;
            case 1: xscatprintf(&report, "Error: Passthrough filearea %s!\r",filearea);
                w_log( '8', "FileFix %%Resend: Passthrough filearea %s", filearea);
                break;
            case 2: xscatprintf(&report, "Error: Filearea %s not found!\r",filearea);
                w_log( '8', "FileFix %%Resend: Filearea %s not found", filearea);
                break;
            case 3: xscatprintf(&report, "Error: File %s not found!\r",filename);
                w_log( '8', "FileFix %%Resend: File %s not found", filename);
                break;
            case 5: xscatprintf(&report, "Error: You don't have access for filearea %s!\r",filearea);
                w_log( '8', "FileFix %%Resend: Link don't have access for filearea %s", filearea);
                break;
            }
    } else {
        xscatprintf(&report, "Error: filearea %s not found!\r",filearea);
        w_log( '8', "FileFix %%Resend: Filearea %s not found", filearea);
    }

    nfree(filearea);
    nfree(filename);
    return report;
}


char *pause_link(s_message *msg, s_link *link)
{
    char *tmp = NULL;
    char *report=NULL;

    if ((link->Pause & FPAUSE) != FPAUSE) {
        if (Changepause(getConfigFileName(), link,0,FPAUSE) == 0)
            return NULL;
    }
    xstrcat(&report, " System switched to passive\r");
    tmp = linked (msg, link, 0);
    xstrcat(&report, tmp);
    nfree(tmp);

    return report;
}

char *resume_link(s_message *msg, s_link *link)
{
    char *tmp = NULL, *report=NULL;

    if ((link->Pause & FPAUSE) == FPAUSE) {
       if (Changepause(getConfigFileName(), link,0,FPAUSE) == 0)
          return NULL;
    }
    xstrcat(&report, " System switched to active\r");
    tmp = linked(msg, link, 0);
    xstrcat(&report, tmp);
    nfree(tmp);
    return report;
}

int tellcmd(char *cmd) {
	char *line = NULL;

	if (strncasecmp(cmd, "* Origin:", 9) == 0) return NOTHING;
	line = strpbrk(cmd, " \t");
	if (line && *cmd != '%') *line = 0;

	line = cmd;

	switch (line[0]) {
	case '%':
		line++;
		if (*line == 0) return FFERROR;
		if (stricmp(line,"list")==0) return LIST;
		if (stricmp(line,"help")==0) return HELP;
		if (stricmp(line,"unlinked")==0) return UNLINK;
		if (stricmp(line,"linked")==0) return LINKED;
        if (stricmp(line,"avail")==0) return AVAIL;
		if (stricmp(line,"query")==0) return LINKED;
		if (stricmp(line,"pause")==0) return PAUSE;
		if (stricmp(line,"resume")==0) return RESUME;
		if (stricmp(line,"info")==0) return INFO;
		if (strncasecmp(line, "resend", 6)==0)
		   if (line[6] != 0) return RESEND;
		return FFERROR;
	case '\001': return NOTHING;
	case '\000': return NOTHING;
	case '-'  : return DEL;
	case '+': line++; if (line[0]=='\000') return FFERROR;
	default: return ADD;
	}
	
	return 0;
}

char *processcmd(s_link *link, s_message *msg, char *line, int cmd) {
	
	char *report = NULL;
	
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
	case AVAIL: report = available (link);
		RetFix=AVAIL;
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
	case FFERROR: report = errorRQ(line);
		RetFix=FFERROR;
		break;
	default: return NULL;
	}
	
	return report;
}

char *areastatus(char *preport, char *text)
{
    char *pth = NULL, *ptmp = NULL, *tmp = NULL, *report = NULL, tmpBuff[256];
    pth = (char*)scalloc(1, sizeof(char));
    tmp = preport;
    ptmp = strchr(tmp, '\r');
    while (ptmp) {
		*ptmp=0;
		ptmp++;
        report=strchr(tmp, ' ');
		*report=0;
		report++;
        if (strlen(tmp) > 50) tmp[50] = 0;
		if (50-strlen(tmp) == 0) sprintf(tmpBuff, " %s  %s\r", tmp, report);
        else if (50-strlen(tmp) == 1) sprintf(tmpBuff, " %s   %s\r", tmp, report);
		else sprintf(tmpBuff, " %s %s  %s\r", tmp, print_ch(50-strlen(tmp)-1, '.'), report);
        pth=(char*)srealloc(pth, strlen(tmpBuff)+strlen(pth)+1);
		strcat(pth, tmpBuff);
		tmp=ptmp;
		ptmp = strchr(tmp, '\r');
    }
    tmp = (char*)scalloc(strlen(pth)+strlen(text)+1, sizeof(char));
    strcpy(tmp, text);
    strcat(tmp, pth);
    free(text);
    free(pth);
    return tmp;
}

void preprocText(char *preport, s_message *msg, char *flags)
{
    msg->text = createKludges(config,
                              NULL, &msg->origAddr, &msg->destAddr,
                              versionStr);
    if (flags)
	xstrscat(&msg->text, "\001FLAGS ", flags, "\r", NULL);
    xstrcat(&msg->text, preport);
    xscatprintf(&msg->text, " \r--- %s FileFix\r", versionStr);
    msg->textLength=(int)strlen(msg->text);
}

char *textHead(void)
{
    char *text_head=NULL;

    xscatprintf(&text_head, " FileArea%sStatus\r",	print_ch(44,' '));
	xscatprintf(&text_head, " %s  -------------------------\r",print_ch(50, '-'));
    return text_head;
}

void RetMsg(s_message *msg, s_link *link, char *report, char *subj)
{
    s_message *tmpmsg;

    if (config->filefixFromName == NULL)
        tmpmsg = makeMessage(link->ourAka, &(link->hisAka), msg->toUserName, msg->fromUserName,
                             subj, 1,config->filefixReportsAttr);
    else
        tmpmsg = makeMessage(link->ourAka, &(link->hisAka),
                             config->filefixFromName, msg->fromUserName,
                             subj, 1,config->filefixReportsAttr);
    preprocText(report, tmpmsg, config->filefixReportsFlags);

    writeNetmail(tmpmsg, config->robotsArea);

    freeMsgBuffers(tmpmsg);
    free(tmpmsg);
}

void sendFilefixMessages()
{
    s_link *link = NULL;
    s_message *linkmsg;
    unsigned int i;

    for (i = 0; i < config->linkCount; i++) {
        if (config->links[i].msg == NULL) continue;
        link = &(config->links[i]);
        linkmsg = link->msg;

        xscatprintf(&(linkmsg->text), " \r--- %s Filefix\r", versionStr);
        linkmsg->textLength = strlen(linkmsg->text);

        w_log(LL_AREAFIX, "Filefix: write netmail msg for %s", aka2str(link->hisAka));

        writeNetmail(linkmsg, config->robotsArea);

        freeMsgBuffers(linkmsg);
        nfree(linkmsg);
        link->msg = NULL;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */


int processFileFix(s_message *msg)
{
	int security=1, notforme = 0;
	s_link *link = NULL;
	s_link *tmplink = NULL;
	char *textBuff = NULL, *report=NULL, *preport = NULL, *token = NULL;
	
	/*  find link */
	link=getLinkFromAddr(config, msg->origAddr);

	/*  this is for me? */
	if (link!=NULL)	notforme=addrComp(msg->destAddr, *link->ourAka);
	
	/*  ignore msg for other link (maybe this is transit...) */
	if (notforme) {
		return 2;
	}


	/*  security ckeck. link,araefixing & password. */
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
					RetMsg(msg, link, preport, "Filefix reply: list request");
					break;
				case HELP:
					RetMsg(msg, link, preport, "Filefix reply: help request");
					break;
				case ADD: case DEL:
					if (report == NULL) report = textHead();
					report = areastatus(preport, report);
					break;
				case UNLINK:
					RetMsg(msg, link, preport, "Filefix reply: unlinked request");
					break;
				case LINKED:
    					RetMsg(msg, link, preport, "Filefix reply: linked request");
					w_log( '8', "FileFix: linked fileareas list sent to %s", aka2str(link->hisAka));
					break;
				case AVAIL:
    			    RetMsg(msg, link, preport, "Filefix reply: avail request");
					break;
				case PAUSE: case RESUME:
					RetMsg(msg, link, preport, "Filefix reply: node change request");
					break;
				case INFO:
					RetMsg(msg, link, preport, "Filefix reply: link information");
					break;
				case RESEND:
					RetMsg(msg, link, preport, "Filefix reply: resend request");
 					break;
				case FFERROR:
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
			tmplink = (s_link*)scalloc(1, sizeof(s_link));
			tmplink->ourAka = &(msg->destAddr);
			tmplink->hisAka.zone = msg->origAddr.zone;
			tmplink->hisAka.net = msg->origAddr.net;
			tmplink->hisAka.node = msg->origAddr.node;
			tmplink->hisAka.point = msg->origAddr.point;
			link = tmplink;
		}
		/*  security problem */
		
		switch (security) {
		case 1:
            report = sstrdup(" \r your system is unknown\r");
			break;
		case 2:
            report = sstrdup(" \r filefix is turned off\r");
			break;
		case 3:
			report = sstrdup(" \r password error\r");
			break;
		default:
			report = sstrdup(" \r unknown error. mail to sysop.\r");
			break;
		}
		
	
		RetMsg(msg, link, report, "security violation");
		free(report);
		
		w_log( '8', "FileFix: security violation from %s", aka2str(link->hisAka));
		
		free(tmplink);
		
		return 0;
	}
	

	if ( report != NULL ) {
		preport=linked(msg, link, 0);
		xstrcat(&report, preport);
		free(preport);
		RetMsg(msg, link, report, "node change request");
		free(report);
	}

	w_log( '8', "FileFix: sucessfully done for %s",aka2str(link->hisAka));
    /*  send msg to the links (forward requests to areafix) */
    sendFilefixMessages();
	return 1;
}

void ffix(hs_addr addr, char *cmd)
{
    s_link          *link   = NULL;
    s_message	    *tmpmsg = NULL;

    if (cmd) {
	link = getLinkFromAddr(config, addr);
	if (link) {
	    tmpmsg = makeMessage(&addr, link->ourAka, link->name,
				 link->RemoteRobotName ?
				 link->RemoteRobotName : "Filefix",
				 link->fileFixPwd ?
				 link->fileFixPwd : "", 1,
                 config->areafixReportsAttr);
	    tmpmsg->text = cmd;
        processFileFix(tmpmsg);
	    tmpmsg->text=NULL;
	    freeMsgBuffers(tmpmsg);
	} else w_log(LL_ERR, "FileFix: no such link in config: %s!", aka2str(addr));
    }
    else scan();
}

/* file echo autocreation */

int   autoCreate(char *c_area, char *descr, ps_addr pktOrigAddr, ps_addr dwLink)
{
    FILE *f;
    char *NewAutoCreate = NULL;
    char *fileName = NULL;
    char *bDir = NULL;
    
    char *fileechoFileName = NULL;
    char *buff= NULL;
    char CR;
    s_link *creatingLink;
    s_message *msg;
    s_filearea *area;
    FILE *echotosslog;
    size_t configlen=0;

    w_log( LL_FUNC, "%s::autoCreate() begin", __FILE__ );
    
    creatingLink = getLinkFromAddr(config, *pktOrigAddr);
    
    fileechoFileName = makeMsgbFileName(config, c_area);
    
    /*  translating name of the area to lowercase/uppercase */
    if (config->createAreasCase == eUpper) strUpper(c_area);
    else strLower(c_area);
    
    /*  translating filename of the area to lowercase/uppercase */
    if (config->areasFileNameCase == eUpper) strUpper(fileechoFileName);
    else strLower(fileechoFileName);
    
    bDir = (creatingLink->fileBaseDir) ?
        creatingLink->fileBaseDir : config->fileAreaBaseDir;
    
    if( strcasecmp(bDir,"passthrough") )
    {
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
        xscatprintf(&buff,"%s%s",bDir,fileechoFileName);
        if (_createDirectoryTree(buff))
        {
            w_log(LL_ERROR, "cannot make all subdirectories for %s\n",
                fileechoFileName);
            nfree(buff);
            w_log( LL_FUNC, "%s::autoCreate() rc=1", __FILE__ );
            return 1;
        }
#if defined (UNIX)
        if(config->fileAreaCreatePerms && chmod(buff, config->fileAreaCreatePerms))
            w_log(LL_ERR, "Cannot chmod() for newly created filearea directory '%s': %s",
            sstr(buff), strerror(errno));
#endif
        nfree(buff);
    }
    

    fileName = creatingLink->autoFileCreateFile ? creatingLink->autoFileCreateFile : getConfigFileName();
    
    f = fopen(fileName, "a+b");
    if (f == NULL) {
        w_log( LL_ERR,"autocreate: cannot open config file %s", fileName);
        w_log( LL_FUNC, "%s::autoCreate() rc=9", __FILE__ );
        return 1;
    }
    
    /* write new line in config file */
    
    xscatprintf(&buff, "FileArea %s %s%s -a %s ",
        c_area, bDir,
        (strcasecmp(bDir,"passthrough") == 0) ? "" : fileechoFileName,
        aka2str(*(creatingLink->ourAka))
        );
    
    if ( creatingLink->LinkGrp &&
        !( creatingLink->autoFileCreateDefaults && fc_stristr(creatingLink->autoFileCreateDefaults, "-g ") )
        )
    {
        xscatprintf(&buff,"-g %s ",creatingLink->LinkGrp);
    }
    
    if (creatingLink->autoFileCreateDefaults) {
        NewAutoCreate = sstrdup(creatingLink->autoFileCreateDefaults);
        if ((fileName=strstr(NewAutoCreate,"-d ")) !=NULL ) {
            if (descr) {
                *fileName = '\0';
                xscatprintf(&buff,"%s -d \"%s\"",NewAutoCreate,descr);
            } else {
                xstrcat(&buff, NewAutoCreate);
            }
        } else {
            if (descr) 
                xscatprintf(&buff,"%s -d \"%s\"",NewAutoCreate,descr);
            else
                xstrcat(&buff, NewAutoCreate);
        }
        nfree(NewAutoCreate);
    }
    else if (descr)
    {
        xscatprintf(&buff,"-d \"%s\"",descr);
    }
    if(!isOurAka(config,*pktOrigAddr))
    {
        xscatprintf(&buff," %s",aka2str(*pktOrigAddr));
    }    
    if(dwLink && !isOurAka(config,*dwLink)) 
    {
        xscatprintf(&buff," %s",aka2str(*dwLink));
    }
    
    /* add new created echo to config in memory */
    parseLine(buff,config);
    RebuildFileAreaTree(config);

    if( fseek (f, -2L, SEEK_END) == 0) 
    {
        CR = getc (f); /*   may be it is CR aka '\r'  */
        if (getc(f) != '\n') {
            fseek (f, 0L, SEEK_END);  /*  not neccesary, but looks better ;) */
            fputs (cfgEol(), f);
        } else {
            fseek (f, 0L, SEEK_END);
        }
        configlen = ftell(f); /* config length */
        /* correct EOL in memory */
        if(CR == '\r')
            xstrcat(&buff,"\r\n"); /* DOS EOL */
        else
            xstrcat(&buff,"\n");   /* UNIX EOL */
    }
    else
    {
        xstrcat(&buff,cfgEol());   /* config depended EOL */
    }

    /*  add line to config */
    if ( fprintf(f, "%s", buff) != (int)(strlen(buff)) || fflush(f) != 0) 
    {
        w_log(LL_ERR, "Error creating area %s, config write failed: %s!",
            c_area, strerror(errno));
        fseek(f, configlen, SEEK_SET);
        setfsize(fileno(f), configlen);
    }
    fclose(f);
    nfree(buff);

    w_log( '8', "FileArea '%s' autocreated by %s", c_area, aka2str(*pktOrigAddr));
    
    /* report about new filearea */
    if (config->ReportTo && !cmAnnNewFileecho && (area = getFileArea(c_area)) != NULL) {
        if (getNetMailArea(config, config->ReportTo) != NULL) {
            msg = makeMessage(area->useAka,
                area->useAka,
                versionStr,
                config->sysop,
                "Created new fileareas", 1,
                config->filefixReportsAttr);
            msg->text = createKludges(config, NULL, area->useAka, area->useAka, versionStr);
        } else {
            msg = makeMessage(area->useAka,
                area->useAka,
                versionStr,
                "All", "Created new fileareas", 0,
                config->filefixReportsAttr);
            msg->text = createKludges(config, config->ReportTo, area->useAka, area->useAka, versionStr);
        } /* endif */
	if (config->filefixReportsFlags)
	    xstrscat(&(msg->text), "\001FLAGS ", config->filefixReportsFlags, "\r", NULL);
        xstrscat(&msg->text, "\r\rNew filearea: ",
                 area->areaName, "\r\rDescription : ",
                 area->description ? area->description : "", "\r", NULL);
        writeMsgToSysop(msg, config->ReportTo, NULL);
        freeMsgBuffers(msg);
        nfree(msg);
        if (config->echotosslog != NULL) {
            echotosslog = fopen (config->echotosslog, "a");
            fprintf(echotosslog,"%s\n",config->ReportTo);
            fclose(echotosslog);
        }
    }
    
    if (cmAnnNewFileecho) announceNewFileecho (announcenewfileecho, c_area, aka2str(*pktOrigAddr));

    if (config->afcFlag) {
        if (NULL == (f = fopen(config->afcFlag, "a")))
            w_log( LL_ERR, "Could not open autoAreaCreate flag '%s': %s",
                   config->afcFlag, strerror(errno) );
        else {
            w_log(LL_FLAG, "Created autoAreaCreate flag: %s", config->afcFlag);
            fclose(f);
        }
    }

    w_log( LL_FUNC, "%s::autoCreate() rc=0", __FILE__ );
    return 0;
}
 
/* test link for areas quantity limit exceed
 * return 0 if not limit exceed
 * else return not zero
 */
int limitCheck(s_link *link, s_message *msg) {
 register unsigned int i,n;

 w_log(LL_FUNC,"areafix.c::limitCheck()");

 if (link->ffixEchoLimit==0) return 0;

 for (i=n=0; i<config->fileAreaCount; i++)
	if (0==subscribeCheck(config->fileAreas[i], msg, link))	
	    n++;

 i = n >= link->ffixEchoLimit ;

 w_log(LL_FUNC,"areafix.c::limitCheck() rc=%u", i);
 return i;
}

