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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <smapi/msgapi.h>

#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>

#include <pkt.h>

#include <fcommon.h>
#include <smapi/patmat.h>
#include <scan.h>
#include <log.h>
#include <global.h>
#include <version.h>
#include <areafix.h>

#include <toss.h>
#include <strsep.h>


void cvtAddr(const NETADDR aka1, s_addr *aka2)
{
  aka2->zone = aka1.zone;
  aka2->net  = aka1.net;
  aka2->node = aka1.node;
  aka2->point = aka1.point;
}


void convertMsgHeader(XMSG xmsg, s_message *msg)
{
   // convert header
   msg->attributes  = xmsg.attr;

   msg->origAddr.zone  = xmsg.orig.zone;
   msg->origAddr.net   = xmsg.orig.net;
   msg->origAddr.node  = xmsg.orig.node;
   msg->origAddr.point = xmsg.orig.point;
   msg->origAddr.domain =  NULL;

   msg->destAddr.zone  = xmsg.dest.zone;
   msg->destAddr.net   = xmsg.dest.net;
   msg->destAddr.node  = xmsg.dest.node;
   msg->destAddr.point = xmsg.dest.point;
   msg->destAddr.domain = NULL;

   strcpy(msg->datetime, (char *) xmsg.__ftsc_date);
   msg->subjectLine = (char *) malloc(strlen((char *)xmsg.subj)+1);
   msg->toUserName  = (char *) malloc(strlen((char *)xmsg.to)+1);
   msg->fromUserName = (char *) malloc(strlen((char *)xmsg.from)+1);
   strcpy(msg->subjectLine, (char *) xmsg.subj);
   strcpy(msg->toUserName, (char *) xmsg.to);
   strcpy(msg->fromUserName, (char *) xmsg.from);
}

void convertMsgText(HMSG SQmsg, s_message *msg, s_addr ourAka)
{
   char    *kludgeLines, viaLine[100];
   UCHAR   *ctrlBuff;
   UINT32  ctrlLen;
   time_t  tm;
   struct tm *dt;

   // get kludge lines
   ctrlLen = MsgGetCtrlLen(SQmsg);
   ctrlBuff = (unsigned char *) malloc(ctrlLen+1);
   MsgReadMsg(SQmsg, NULL, 0, 0, NULL, ctrlLen, ctrlBuff);
   kludgeLines = (char *) CvtCtrlToKludge(ctrlBuff);
   free(ctrlBuff);

   // make text
   msg->textLength = MsgGetTextLen(SQmsg);

   time(&tm);
   dt = gmtime(&tm);
   sprintf(viaLine, "\001Via %u:%u/%u.%u @%04u%02u%02u.%02u%02u%02u.UTC %s",
           ourAka.zone, ourAka.net, ourAka.node, ourAka.point,
           dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec, versionStr);

   msg->text = (char *) calloc(1, msg->textLength+strlen(kludgeLines)+strlen(viaLine)+1);


   strcpy(msg->text, kludgeLines);
//   strcat(msg->text, "\001TID: ");
//   strcat(msg->text, versionStr);
//   strcat(msg->text, "\r");

   MsgReadMsg(SQmsg, NULL, 0, msg->textLength, (unsigned char *) msg->text+strlen(msg->text), 0, NULL);

   strcat(msg->text, viaLine);

   free(kludgeLines);
}

void scanNMArea(void)
{
   HAREA           netmail;
   HMSG            msg;
   unsigned long   highMsg, i, j;
   XMSG            xmsg;
   s_addr          dest;
   // s_addr  orig;
   int             for_us;
   // int from_us;
   s_message       filefixmsg;

   netmail = MsgOpenArea((unsigned char *) config->netMailAreas[0].fileName, MSGAREA_NORMAL, config->netMailAreas[0].msgbType);
   if (netmail != NULL) {

      highMsg = MsgGetHighMsg(netmail);
      writeLogEntry(htick_log, '1', "Scanning NetmailArea");

      // scan all Messages for filefix
      for (i=1; i<= highMsg; i++) {
         msg = MsgOpenMsg(netmail, MOPEN_RW, i);

         // msg does not exist
         if (msg == NULL) continue;

         MsgReadMsg(msg, &xmsg, 0, 0, NULL, 0, NULL);
         cvtAddr(xmsg.dest, &dest);
         for_us = 0;
         for (j=0; j < config->addrCount; j++)
            if (addrComp(dest, config->addr[j])==0) {for_us = 1; break;}
                
         // if for filefix - process it
         if ((stricmp((char*)xmsg.to,"filefix")==0 ||
              stricmp((char*)xmsg.to,"allfix")==0 ||
              stricmp((char*)xmsg.to,"filemgr")==0 ||
              stricmp((char*)xmsg.to,"htick")==0 ||
              stricmp((char*)xmsg.to,"filescan")==0)
             && for_us && (xmsg.attr & MSGREAD) != MSGREAD)
            {
            convertMsgHeader(xmsg, &filefixmsg);
            convertMsgText(msg, &filefixmsg, config->addr[j]);
            
            if (processFileFix(&filefixmsg) != 2) {
		xmsg.attr |= MSGREAD;
		MsgWriteMsg(msg, 0, &xmsg, NULL, 0, 0, 0, NULL);
	    }

	    freeMsgBuff(&filefixmsg);

            MsgCloseMsg(msg);
	    if (config->filefixKillRequests) MsgKillMsg(netmail, i);
//              i--;
            }
           else
            MsgCloseMsg(msg);
         
      } /* endfor */

      MsgCloseArea(netmail);
   } else {
      writeLogEntry(htick_log, '9', "Could not open NetmailArea");
   } /* endif */
}

void scan(void)
{
  scanNMArea();
}

