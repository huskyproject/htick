#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hatch.h>
#include <global.h>
#include <fcommon.h>
#include <common.h>
#include <toss.h>
#include <log.h>
#include <crc32.h>
#include <common.h>
#include <add_descr.h>
#include <recode.h>
#include <version.h>
#include <progprot.h>
#include <adcase.h>

void hatch()
{
   s_ticfile tic;
   s_filearea *filearea;
   FILE *flohandle;
   char filename[256], fileareapath[256], descr_file_name[256],
         linkfilepath[256];
   char *newticfile, sepname[13], *sepDir;
   time_t acttime;
   char tmp[100],timestr[40];
   char logstr[200];
   int i, busy;
   struct stat stbuf;
   extern s_newfilereport **newFileReport;
   extern int newfilesCount;

   newFileReport = NULL;
   newfilesCount = 0;


   memset(&tic,0,sizeof(tic));

   // Exist file?
   adaptcase(hatchfile);
   if (!fexist(hatchfile)) {
       sprintf(logstr,"File %s, not found",hatchfile);
       writeLogEntry(htick_log,'6',logstr);
       disposeTic(&tic);
       return;
   }

   sepDir = strrchr(hatchfile,PATH_DELIM);
   if (sepDir) strcpy(tic.file, sepDir+1);
   else strcpy(tic.file,hatchfile);
   strLower(tic.file); /* Finally, we want it in lower case */

   strcpy(tic.area,hatcharea);
   filearea=getFileArea(config,tic.area);

   if (config->outtab != NULL) recodeToTransportCharset(hatchdesc);
   tic.desc=realloc(tic.desc,(tic.anzdesc+1)*sizeof(&tic.desc));
   tic.desc[tic.anzdesc]=strdup(hatchdesc);
   tic.anzdesc++;
   if (hatchReplace) strcpy(tic.replaces,tic.file);
/*
   if (filearea==NULL) {
      autoCreate(tic.area,tic.from,tic.areadesc);
      filearea=getFileArea(config,tic.area);
   }
*/
   if (filearea!=NULL) {
      strcpy(fileareapath,filearea->pathName);
   } else {
      sprintf(logstr,"Cannot open or create File Area %s",tic.area);
      writeLogEntry(htick_log,'9',logstr);
      fprintf(stderr,"Cannot open or create File Area %s !",tic.area);
      disposeTic(&tic);
      return;
   } 

   strLower(fileareapath);
   createDirectoryTree(fileareapath);

   stat(hatchfile,&stbuf);
   tic.size = stbuf.st_size;

   tic.origin = tic.from = *filearea->useAka;

   // Adding crc
   tic.crc = filecrc32(hatchfile);

   strcpy(filename,fileareapath);
   strcat(filename,tic.file);
   strLower(filename);
   if (copy_file(hatchfile,filename)!=0) {
      sprintf(logstr,"File %s not found or moveable",hatchfile);
      writeLogEntry(htick_log,'9',logstr);
      disposeTic(&tic);
      return;
   } else {
      sprintf(logstr,"Put %s to %s",hatchfile,filename);
      writeLogEntry(htick_log,'6',logstr);
   }

   strcpy(descr_file_name, fileareapath);
   strcat(descr_file_name, "files.bbs");
   adaptcase(descr_file_name);

   removeDesc(descr_file_name,tic.file);
   add_description (descr_file_name, tic.file, tic.desc, tic.anzdesc);
   if (cmAnnFile) announceInFile (announcefile, tic.file, tic.size, tic.area, tic.origin, tic.desc, tic.anzdesc);

   if (filearea->downlinkCount > 0) {

      // Adding path
      time(&acttime);
      strcpy(timestr,asctime(localtime(&acttime)));
      timestr[strlen(timestr)-1]=0;
      sprintf(tmp,"%s %lu %s %s",
              addr2string(filearea->useAka), (unsigned long) time(NULL), timestr,versionStr);
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
	 }
	 else *(strrchr(linkfilepath,PATH_DELIM))=0;

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
            fprintf(flohandle,"%s\n",filename);
            fprintf(flohandle,"^%s\n",newticfile);
            fclose(flohandle);

            remove(filearea->downlinks[i]->link->bsyFile);

            sprintf(logstr,"Forwarding %s for %s, %s",
                    tic.file,
                    filearea->downlinks[i]->link->name,
                    addr2string(&filearea->downlinks[i]->link->hisAka));
            writeLogEntry(htick_log,'6',logstr);
	    free(filearea->downlinks[i]->link->bsyFile);
	 }
	 free(filearea->downlinks[i]->link->floFile);
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
      if (config->intab != NULL) recodeToInternalCharset(newFileReport[newfilesCount]->fileDesc[i]);
   } /* endfor */
   newFileReport[newfilesCount]->filedescCount = tic.anzdesc;

   newFileReport[newfilesCount]->fileSize = tic.size;

   newfilesCount++;

   disposeTic(&tic);
   
   reportNewFiles();
}

int send(char *filename, char *area, char *addr)
//0 - OK
//1 - Passthrough filearea
//2 - filearea not found
//3 - file not found
//4 - link not found
{
    s_ticfile tic;
    s_link *link = NULL;
    s_filearea *filearea;
    s_addr address;
    char sendfile[256], logstr[200], descr_file_name[256];
    char tmp[100], timestr[40], linkfilepath[256];
    char sepname[13], *sepDir, *newticfile;
    struct stat stbuf;
    time_t acttime;
    int busy;
    FILE *flohandle;

   filearea=getFileArea(config,area);
   if (filearea == NULL) {
      fprintf(stderr,"Error: Filearea not found\n");
      return 2;
   }
   if (filearea->pass == 1) {
      fprintf(stderr,"Error: Passthrough filearea\n");
      return 1;
   }

   string2addr(addr, &address);
   link = getLinkFromAddr(*config, address);
   if (link == NULL) {
      fprintf(stderr,"Error: Link not found\n");
      return 4;
   }

   memset(&tic,0,sizeof(tic));

   strcpy(sendfile,filearea->pathName);
   strLower(sendfile);
   createDirectoryTree(sendfile);
   strcat(sendfile,filename);

   // Exist file?
   adaptcase(sendfile);
   if (!fexist(sendfile)) {
         fprintf(stderr,"Error: File not found\n");
         sprintf(logstr,"File %s, not found",sendfile);
         writeLogEntry(htick_log,'6',logstr);
         disposeTic(&tic);
         return 3;
   }

   sepDir = strrchr(sendfile,PATH_DELIM);
   if (sepDir) strcpy(tic.file, sepDir+1);
   else strcpy(tic.file,sendfile);

   strcpy(tic.area,area);

   stat(sendfile,&stbuf);
   tic.size = stbuf.st_size;

   tic.origin = tic.from = *filearea->useAka;

   // Adding crc
   tic.crc = filecrc32(sendfile);

   strcpy(descr_file_name, filearea->pathName);
   strcat(descr_file_name, "files.bbs");
   adaptcase(descr_file_name);

   getDesc(descr_file_name, tic.file, &tic);

   // Adding path
   time(&acttime);
   strcpy(timestr,asctime(localtime(&acttime)));
   timestr[strlen(timestr)-1]=0;
   sprintf(tmp,"%s %lu %s %s",
           addr2string(filearea->useAka), (unsigned long) time(NULL), timestr,versionStr);
   tic.path=realloc(tic.path,(tic.anzpath+1)*sizeof(*tic.path));
   tic.path[tic.anzpath]=strdup(tmp);
   tic.anzpath++;

   // Adding Downlink to Seen-By
   tic.seenby=realloc(tic.seenby,(tic.anzseenby+1)*sizeof(s_addr));
   memcpy(&tic.seenby[tic.anzseenby], &link->hisAka, sizeof(s_addr));
   tic.anzseenby++;

   // Forward file to
   memcpy(&tic.from,filearea->useAka, sizeof(s_addr));
   memcpy(&tic.to,&link->hisAka, sizeof(s_addr));
   if (link->ticPwd != NULL) strcpy(tic.password,link->ticPwd);

   busy = 0;

   if (createOutboundFileName(link,
       cvtFlavour2Prio(link->echoMailFlavour), FLOFILE)==1)
      busy = 1;

   strcpy(linkfilepath,link->floFile);
   if (busy) {
      writeLogEntry(htick_log, '7', "Save TIC in temporary dir");
      //Create temporary directory
      *(strrchr(linkfilepath,'.'))=0;
      strcat(linkfilepath,".htk");
      createDirectoryTree(linkfilepath);
   } else *(strrchr(linkfilepath,PATH_DELIM))=0;

   // separate bundles
   if (config->separateBundles && !busy) {
          
      if (link->hisAka.point != 0)
         sprintf(sepname,"%08x.sep",link->hisAka.point);
      else
         sprintf(sepname,"%04x%04x.sep",link->hisAka.net,link->hisAka.node);

      sepDir = (char*) malloc(strlen(linkfilepath)+1+strlen(sepname)+1+1);
      sprintf(sepDir,"%s%c%s%c",linkfilepath,PATH_DELIM,sepname,PATH_DELIM);
      strcpy(linkfilepath,sepDir);

      createDirectoryTree(sepDir);
      free(sepDir);
   }

   newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
   writeTic(newticfile,&tic);

   if (!busy) {
      flohandle=fopen(link->floFile,"a");
      fprintf(flohandle,"%s\n",sendfile);
      fprintf(flohandle,"^%s\n",newticfile);
      fclose(flohandle);

      remove(link->bsyFile);

      sprintf(logstr,"Send %s from %s for %s, %s",
              tic.file,tic.area,link->name,addr2string(&link->hisAka));
      writeLogEntry(htick_log,'6',logstr);
      free(link->bsyFile);
   }
   free(link->floFile);
   disposeTic(&tic);
   return 0;
}
