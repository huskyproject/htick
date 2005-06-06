/*****************************************************************************
 * AreaFix for HTICK (FTN Ticker / Request Processor)
 *****************************************************************************
 * Copyright (C) 2004
 *
 * val khokhlov (Fido: 2:550/180), Kiev, Ukraine
 *
 * Based on original HTICK code by:
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
#include <sys/types.h>
#include <sys/stat.h>

/* compiler.h */
#include <huskylib/compiler.h>

#ifdef HAS_UNISTD_H
#include <unistd.h>
#endif

/* fidoconf */
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <huskylib/xstr.h>
#include <fidoconf/afixcmd.h>
#include <fidoconf/arealist.h>
#include <fidoconf/areatree.h>

/* areafix */
#include <areafix/areafix.h>
#include <areafix/afglobal.h>
#include <areafix/callback.h>
#include <areafix/query.h>

/* htick */
#include <fcommon.h>
#include <global.h>
#include <toss.h>
#include <htickafix.h>
#include <hatch.h>
#include <scan.h>
#include <add_desc.h>
extern char      *versionStr;

int e_readCheck(s_fidoconfig *config, s_area *echo, s_link *link) {

    /*  rc == '\x0000' access o'k */
    /*  rc == '\x0001' no access group */
    /*  rc == '\x0002' no access level */
    /*  rc == '\x0003' no access export */
    /*  rc == '\x0004' not linked */

    unsigned i, rc = 0;
    unsigned Pause = echo->areaType;
    /* check for OurAka */
    if(!isOurAka(config,link->hisAka))
    {
        for (i=0; i<echo->downlinkCount; i++) {
            if (link == echo->downlinks[i]->link) break;
        }
        if (i == echo->downlinkCount) return 4;
    } else if ( echo->msgbType  == MSGTYPE_PASSTHROUGH ) {
        return 4;
    }
    /*  pause */
    if (((link->Pause & Pause) == Pause) && echo->noPause==0) return 3;

    if (echo->group) {
		if (link->numAccessGrp) {
			if (config->numPublicGroup) {
				if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp) &&
					!grpInArray(echo->group,config->PublicGroup,config->numPublicGroup))
					rc = 1;
			} else if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp)) rc = 1;
		} else if (config->numPublicGroup) {
			if (!grpInArray(echo->group,config->PublicGroup,config->numPublicGroup)) rc = 1;
		} else if (link->numOptGrp==0) return 1;
		
		if (link->numOptGrp) {
			if (grpInArray(echo->group,link->optGrp,link->numOptGrp)) {
				if (link->export==0) return 3; else rc = 0;
			}
		}
	}
	
    if (echo->levelread > link->level) return 2;
    /* check for 'access export' for arealink set up by WriteOnly keyword */
    if (echo->downlinks[i]->export==0) return 3;

    return rc;
}

int e_writeCheck(s_fidoconfig *config, s_area *echo, s_link *link) {

    /*  rc == '\x0000' access o'k */
    /*  rc == '\x0001' no access group */
    /*  rc == '\x0002' no access level */
    /*  rc == '\x0003' no access import */
    /*  rc == '\x0004' not linked */

    unsigned int i=0, rc = 0;
    /* check for OurAka */
    if(!isOurAka(config,link->hisAka))
    {
        for (i=0; i<echo->downlinkCount; i++) {
            if (link == echo->downlinks[i]->link) break;
        }
        if (i == echo->downlinkCount) return 4;
    } else if ( echo->msgbType  == MSGTYPE_PASSTHROUGH ) {
        return 4;
    } else if (echo->downlinkCount == 0) {
        /* always OK for nolinks (need for hpucode's tics) */      
        return 0;
    }

    if (echo->group) {
		if (link->numAccessGrp) {
			if (config->numPublicGroup) {
				if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp) &&
					!grpInArray(echo->group,config->PublicGroup,config->numPublicGroup))
					rc = 1;
			} else if (!grpInArray(echo->group,link->AccessGrp,link->numAccessGrp)) rc = 1;
		} else if (config->numPublicGroup) {
			if (!grpInArray(echo->group,config->PublicGroup,config->numPublicGroup)) rc = 1;
		} else if (link->numOptGrp==0) return 1;

		if (link->numOptGrp) {
			if (grpInArray(echo->group,link->optGrp,link->numOptGrp)) {
				if (link->import==0) return 3; else rc = 0;
			}
		}
    }
	
    if (echo->levelwrite > link->level) return 2;
    /* check for 'access import' for arealink set up by ReadOnly keyword */
    if (echo->downlinks[i]->import==0) return 3;

    return rc;
}


char *resend(s_link *link, s_message *msg, char *cmd)
{
    unsigned int rc, i;
    char *line;
    char *report=NULL, *token = NULL, *filename=NULL, *filearea=NULL;
    s_area *area = NULL;

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


int filefix_tellcmd(char *cmd) {
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
		if (stricmp(line,"unlinked")==0) return UNLINKED;
		if (stricmp(line,"linked")==0) return QUERY;
        if (stricmp(line,"avail")==0) return AVAIL;
		if (stricmp(line,"query")==0) return QUERY;
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

char *filefix_processcmd(s_link *link, s_message *msg, char *line, int cmd) {
	
	char *report = NULL;
	
	switch (cmd) {

	case NOTHING: return NULL;

	case LIST: report = list (lt_all, link, line);
		RetFix=LIST;
		break;
	case HELP: report = help (link);
		RetFix=HELP;
		break;
	case ADD: report = subscribe (link, line);
		RetFix=ADD;
		break;
	case DEL: report = unsubscribe (link, line);
		RetFix=DEL;
		break;
	case UNLINKED: report = list (lt_unlinked, link, line);
		RetFix=UNLINKED;
		break;
	case QUERY: report = list (lt_linked, link, line);
		RetFix=QUERY;
		break;
	case AVAIL: report = available (link, line);
		RetFix=AVAIL;
		break;
	case PAUSE: report = pause_link (link);
		RetFix=PAUSE;
		break;
	case RESUME: report = resume_link (link);
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


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */


int processFileFix(s_message *msg)
{
	int security=1, notforme = 0;
	s_link *link = NULL;
	s_link *tmplink = NULL;
	char *textBuff = NULL, *report=NULL, *preport = NULL, *token = NULL;
	
	/*  find link */
	link=getLinkFromAddr(config, msg->origAddr);

	if (link)
	{
		w_log(LL_AREAFIX, "FileFix: request from %s", aka2str(link->hisAka));
	} else {
		w_log(LL_ERR, "FileFix: No such link in config: %s!", aka2str(msg->origAddr));
	}
	
	/*  this is for me? */
	if (link!=NULL)	notforme=addrComp(msg->destAddr, *link->ourAka);
	
	/*  ignore msg for other link (maybe this is transit...) */
	if (notforme) {
		w_log(LL_AREAFIX, "Destination address (%s) of the message belongs to set of our addresses but differs from ourAka setting for link",
			  aka2str(msg->destAddr));
		return 2;
	}


	/*  security ckeck. link,araefixing & password. */
    if (link != NULL) {
        if (link->filefix.on) {
            if (link->filefix.pwd) {
                if (stricmp(link->filefix.pwd, msg->subjectLine)==0) security=0;
                else security=3;
            }
            else security = 0;
        } else security=2;
    } else security=1;

	if (!security) {
        /*  remove kluges */
        char *tmp = msg->text;
        token = strseparate (&tmp,"\n\r");

        while(token != NULL) {
            if( !strcmp(token,"---") || !strncmp(token,"--- ",4) )
                /*  stop on tearline ("---" or "--- text") */
                break;
            if( token[0] != '\001' )
                xstrscat(&textBuff,token,"\r",NULL);
            token = strseparate (&tmp,"\n\r");
        }
        nfree(msg->text);
        msg->text = textBuff;

    	textBuff = tmp = sstrdup(msg->text);
		token = strseparate (&textBuff, "\n\r");

		while(token != NULL) {
			preport = filefix_processcmd( link, msg,  stripLeadingChars(token, " \t"), filefix_tellcmd (token) );
			if (preport != NULL) {
				switch (RetFix) {
				case LIST:
					RetMsg(msg, link, preport, "Filefix reply: list request");
					break;
				case HELP:
					RetMsg(msg, link, preport, "Filefix reply: help request");
					break;
				case ADD: case DEL:
					//if (report == NULL) report = textHead();
					report = areaStatus(report, preport);
					break;
				case UNLINKED:
					RetMsg(msg, link, preport, "Filefix reply: unlinked request");
					break;
				case QUERY:
    				RetMsg(msg, link, preport, "Filefix reply: linked request");
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
					report = areaStatus(report, preport);
					break;
				default: break;
				}
				
			}
			token = strseparate (&textBuff, "\n\r");
		}
        nfree(tmp);
		
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
		/* free(report); */ 
		
		w_log( '8', "FileFix: security violation from %s", aka2str(link->hisAka));
		
		free(tmplink);
		
		return 0;
	}
	

	if ( report != NULL ) {
        if (af_robot->queryReports) {
		    preport=list(lt_linked, link, NULL);
		    xstrcat(&report, preport);
		    nfree(preport);
		}
		RetMsg(msg, link, report, "Filefix reply: node change request");
	}

	w_log( '8', "FileFix: successfully done for %s",aka2str(link->hisAka));
    /*  send msg to the links (forward requests to areafix) */
    sendAreafixMessages();
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
				 link->filefix.name ? link->filefix.name : "Filefix",
				 link->filefix.pwd ? link->filefix.pwd : "", 1,
                 af_robot->reportsAttr);
	    tmpmsg->text = cmd;
        processFileFix(tmpmsg);
	    tmpmsg->text=NULL;
	    freeMsgBuffers(tmpmsg);
	} else w_log(LL_ERR, "FileFix: no such link in config: %s!", aka2str(addr));
    }
    else scan();
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

int afSendMsg(s_message *tmpmsg) {
  writeNetmail(tmpmsg, config->robotsArea);
  return 1;
}

int afWriteMsgToSysop(s_message *msg) {
  if (config->ReportTo)
    writeMsgToSysop(msg, config->ReportTo, config->origin ? config->origin : config->name);
  return 1;  
}

void afReportAutoCreate(char *c_area, char *descr, hs_addr pktOrigAddr, ps_addr forwardAddr) {
    s_area *area;
    s_message *msg;
    FILE *echotosslog;

    /* report about new filearea */
    if (config->ReportTo && !cmAnnNewFileecho && (area = getFileArea(c_area)) != NULL) {
        if (getNetMailArea(config, config->ReportTo) != NULL) {
            msg = makeMessage(area->useAka,
                area->useAka,
                af_robot->name,
                config->sysop,
                "Created new fileareas", 1,
                af_robot->reportsAttr);
            msg->text = createKludges(config, NULL, area->useAka, area->useAka, af_versionStr);
        } else {
            msg = makeMessage(area->useAka,
                area->useAka,
                af_robot->name,
                "All", "Created new fileareas", 0,
                af_robot->reportsAttr);
            msg->text = createKludges(config, config->ReportTo, area->useAka, area->useAka, af_versionStr);
        } /* endif */
	if (af_robot->reportsFlags)
	    xstrscat(&(msg->text), "\001FLAGS ", af_robot->reportsFlags, "\r", NULL);
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

    if (cmAnnNewFileecho) announceNewFileecho (announcenewfileecho, c_area, aka2str(pktOrigAddr));
}

s_link_robot *getLinkRobot(s_link *link) {
    return &(link->filefix);
}

int init_htickafix(void) {
  /* vars */
  af_config      = config;
  af_cfgFile     = cfgFile;
  af_app         = &theApp;
  af_versionStr  = versionStr;
  af_quiet       = quiet;
  af_silent_mode = 0; /*silent_mode;*/
  af_report_changes = 0; /*report_changes;*/
  af_send_notify = cmNotifyLink;
  af_pause = FILEAREA;
  /* callbacks and hooks */
  call_getArea  = &getFileArea;
  call_sendMsg  = &afSendMsg;
  call_writeMsgToSysop = &afWriteMsgToSysop;
  call_getLinkRobot = &getLinkRobot;
  hook_onAutoCreate = &afReportAutoCreate;
  robot = getRobot(config, "filefix", 0); /* !!! val: change this later !!! */
  return init_areafix("filefix");
}
