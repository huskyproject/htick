#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hatch.h>
#include <global.h>
#include <fcommon.h>
#include <toss.h>
#include <log.h>
#include <crc32.h>
#include <common.h>
#include <add_descr.h>
#include <recode.h>
#include <version.h>
#include <progprot.h>

void hatch()
{
    s_ticfile tic;
    s_filearea *filearea;
    FILE *flohandle;
    char filename[256], fileareapath[256], descr_file_name[256],
         linkfilepath[256];
    char *newticfile;
    time_t acttime;
    char tmp[100],timestr[40];
    char logstr[200];
    int i, busy = 0;
    struct stat stbuf;

   // Exist file?
   if (!fexist(hatchfile)) {
      strLower(hatchfile);
      if (!fexist(hatchfile)) {
         sprintf(logstr,"File %s, not found",hatchfile);
         writeLogEntry(htick_log,'6',logstr);
         disposeTic(&tic);
         return;
      }
   }

   memset(&tic,0,sizeof(tic));

   strcpy(tic.file,strrchr(hatchfile,'/')+1);
   if (tic.file == NULL) strcpy(tic.file,hatchfile);

   strcpy(tic.area,hatcharea);
   filearea=getFileArea(config,tic.area);

   if (config->outtab != NULL) recodeToTransportCharset(hatchdesc);
   tic.desc=realloc(tic.desc,(tic.anzdesc+1)*sizeof(&tic.desc));
   tic.desc[tic.anzdesc]=strdup(hatchdesc);
   tic.anzdesc++;
/*
   if (filearea==NULL) {
      autoCreate(tic.area,tic.from,tic.areadesc);
      filearea=getFileArea(config,tic.area);
   }
*/
   if (filearea!=NULL) {
      strcpy(fileareapath,filearea->pathName);
      if (filearea->pathName[strlen(filearea->pathName)-1]!='/')
         strcat(fileareapath,"/");
   } else {
      sprintf(logstr,"Cannot open oder create File Area %s",tic.area);
      writeLogEntry(htick_log,'9',logstr);
      fprintf(stderr,"Cannot open oder create File Area %s !",tic.area);
      disposeTic(&tic);
      return;
   } 

   strLower(fileareapath);
   createDirectoryTree(fileareapath);

   //Calculate file size
   stat(hatchfile,&stbuf);
   tic.size = stbuf.st_size;

   tic.origin = tic.from = config->addr[0];

   // Adding crc
   tic.crc = filecrc32(hatchfile);

   strcpy(filename,fileareapath);
   strcat(filename,tic.file);
   if (move_file(hatchfile,filename)!=0) {
      sprintf(logstr,"File %s not found or moveable",hatchfile);
      writeLogEntry(htick_log,'9',logstr);
      disposeTic(&tic);
      return;
   } else {
      sprintf(logstr,"Moved %s to %s",hatchfile,filename);
      writeLogEntry(htick_log,'6',logstr);
   }

   strcpy(descr_file_name, fileareapath);
   strcat(descr_file_name, "files.bbs");
   strLower(descr_file_name);

   add_description (descr_file_name, tic.file, tic.desc, tic.anzdesc);
   if (cmAnnFile) announceInFile (announcefile, tic.file, tic.size, tic.area, tic.origin, tic.desc, tic.anzdesc);

   if (filearea->downlinkCount > 0) {

      // Adding path
      time(&acttime);
      strcpy(timestr,asctime(localtime(&acttime)));
      timestr[strlen(timestr)-1]=0;
      sprintf(tmp,"%s %s %s",
              addr2string(filearea->useAka),timestr,versionStr);
      tic.path=realloc(tic.path,(tic.anzpath+1)*sizeof(*tic.path));
      tic.path[tic.anzpath]=strdup(tmp);
      tic.anzpath++;

      for (i=0;i<filearea->downlinkCount;i++) { 
         // Adding Downlink to Seen-By
         tic.seenby=realloc(tic.seenby,(tic.anzseenby+1)*sizeof(s_addr));
         memcpy(&tic.seenby[tic.anzseenby],
                &filearea->downlinks[i]->link->hisAka,
                sizeof(s_addr));
         tic.anzseenby++;
      }

      // Checking to whom I shall forward
      for (i=0;i<filearea->downlinkCount;i++) { 
         // Forward file to
         memcpy(&tic.from,filearea->useAka,sizeof(s_addr));
         memcpy(&tic.to,&filearea->downlinks[i]->link->hisAka,
         sizeof(s_addr));
         strcpy(tic.password,filearea->downlinks[i]->link->ticPwd);

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
	 }
	 else *(strrchr(linkfilepath,'/'))=0;

         newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
         writeTic(newticfile,&tic);

         if (!busy) {
	    flohandle=fopen(filearea->downlinks[i]->link->floFile,"a");
            fprintf(flohandle,"%s\r\n",filename);
            fprintf(flohandle,"^%s\r\n",newticfile);
            fclose(flohandle);

            remove(filearea->downlinks[i]->link->bsyFile);

            sprintf(logstr,"Forwarding %s for %s, %s",
                    tic.file,
                    filearea->downlinks[i]->link->name,
                    addr2string(&filearea->downlinks[i]->link->hisAka));
            writeLogEntry(htick_log,'6',logstr);
	 }
      } // Forward file

   }
   disposeTic(&tic);
}
