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
#include <fidoconf/common.h>
#include <add_descr.h>
#include <recode.h>
#include <version.h>
#include <smapi/progprot.h>
#include <fidoconf/adcase.h>

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
   int i, busy;
   struct stat stbuf;
   extern s_newfilereport **newFileReport;
   extern int newfilesCount;
   int readAccess;

   newFileReport = NULL;
   newfilesCount = 0;


   memset(&tic,0,sizeof(tic));

   // Exist file?
   adaptcase(hatchfile);
   if (!fexist(hatchfile)) {
       writeLogEntry(htick_log,'6',"File %s, not found",hatchfile);
       disposeTic(&tic);
       return;
   }

   newticfile = strrchr(hatchfile,PATH_DELIM);
   if (newticfile) strcpy(tic.file, newticfile+1);
   else strcpy(tic.file,hatchfile);
#ifdef UNIX
   strLower(tic.file); /* Finally, we want it in lower case */
#endif

   strcpy(tic.area,hatcharea);
   filearea=getFileArea(config,tic.area);

   if (config->outtab != NULL) recodeToTransportCharset(hatchdesc);
   tic.desc=realloc(tic.desc,(tic.anzdesc+1)*sizeof(&tic.desc));
   tic.desc[tic.anzdesc]=strdup(hatchdesc);
   tic.anzdesc++;
   if (hatchReplace) strcpy(tic.replaces,replaceMask);
/*
   if (filearea==NULL) {
      autoCreate(tic.area,tic.from,tic.areadesc);
      filearea=getFileArea(config,tic.area);
   }
*/
   if (filearea == NULL) {
      writeLogEntry(htick_log,'9',"Cannot open or create File Area %s",tic.area);
      fprintf(stderr,"Cannot open or create File Area %s !",tic.area);
      disposeTic(&tic);
      return;
   } 

   stat(hatchfile,&stbuf);
   tic.size = stbuf.st_size;

   tic.origin = tic.from = *filearea->useAka;

   // Adding crc
   tic.crc = filecrc32(hatchfile);

   if (filearea->pass != 1) {
      strcpy(fileareapath,filearea->pathName);
      strLower(fileareapath);
      createDirectoryTree(fileareapath);
      strcpy(filename,fileareapath);
      strcat(filename,tic.file);
      strLower(filename);
      if (copy_file(hatchfile,filename)!=0) {
         writeLogEntry(htick_log,'9',"File %s not found or moveable",hatchfile);
         disposeTic(&tic);
         return;
      } else {
         writeLogEntry(htick_log,'6',"Put %s to %s",hatchfile,filename);
      }
   } 

   if (filearea->sendorig || filearea->pass) {
      strcpy(fileareapath,config->passFileAreaDir);
      strLower(fileareapath);
      createDirectoryTree(fileareapath);
      strcpy(filename,fileareapath);
      strcat(filename,tic.file);
      strLower(filename);
      if (copy_file(hatchfile,filename)!=0) {
         writeLogEntry(htick_log,'9',"File %s not found or moveable",hatchfile);
         disposeTic(&tic);
         return;
      } else {
         writeLogEntry(htick_log,'6',"Put %s to %s",hatchfile,filename);
      }
   }

   if (filearea->pass != 1) {
      strcpy(descr_file_name, fileareapath);
      strcat(descr_file_name, "files.bbs");
      adaptcase(descr_file_name);
      removeDesc(descr_file_name,tic.file);
      add_description (descr_file_name, tic.file, tic.desc, tic.anzdesc);
   }

   if (cmAnnFile) announceInFile (announcefile, tic.file, tic.size, tic.area, tic.origin, tic.desc, tic.anzdesc);

   if (filearea->downlinkCount > 0) {

      // Adding path
      time(&acttime);
      strcpy(timestr,asctime(gmtime(&acttime)));
      timestr[strlen(timestr)-1]=0;
      if (timestr[8]==' ') timestr[8]='0';
      sprintf(tmp,"%s %lu %s UTC %s",
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

         readAccess = readCheck(filearea, filearea->downlinks[i]->link);
         switch (readAccess) {
         case 0: break;
         case 3:
            writeLogEntry(htick_log,'7',"Not export to link %s",
            addr2string(&filearea->downlinks[i]->link->hisAka));
	    break;
         case 2:
            writeLogEntry(htick_log,'7',"Link %s no access level",
            addr2string(&filearea->downlinks[i]->link->hisAka));
	    break;
         case 1:
            writeLogEntry(htick_log,'7',"Link %s no access group",
            addr2string(&filearea->downlinks[i]->link->hisAka));
	    break;
         }

         if (readAccess == 0) {

            memcpy(&tic.from,filearea->useAka,sizeof(s_addr));
            memcpy(&tic.to,&filearea->downlinks[i]->link->hisAka,
                   sizeof(s_addr));
            if (filearea->downlinks[i]->link->ticPwd!=NULL)
		strcpy(tic.password,filearea->downlinks[i]->link->ticPwd);

            busy = 0;

            if (createOutboundFileName(filearea->downlinks[i]->link,
                cvtFlavour2Prio(filearea->downlinks[i]->link->fileEchoFlavour),
                FLOFILE)==1)
               busy = 1;

            if (busy) {
               writeLogEntry(htick_log, '7', "Save TIC in temporary dir");
               //Create temporary directory
               strcpy(linkfilepath,config->busyFileDir);
	    } else {
               if (config->separateBundles) {
                  strcpy(linkfilepath, filearea->downlinks[i]->link->floFile);
                  sprintf(strrchr(linkfilepath, '.'), ".sep%c", PATH_DELIM);
               } else {
                  strcpy(linkfilepath, config->passFileAreaDir);
               }
	    }
            createDirectoryTree(linkfilepath);

	    /* Don't create TICs for everybody */
	    if (!filearea->downlinks[i]->link->noTIC) {
              newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
              writeTic(newticfile,&tic);   
	    } 
	    else newticfile = NULL;

            if (!busy) {
	       flohandle=fopen(filearea->downlinks[i]->link->floFile,"a");
               fprintf(flohandle,"%s\n",filename);
	       
	       if (newticfile != NULL)
                  fprintf(flohandle,"^%s\n",newticfile);

               fclose(flohandle);

               remove(filearea->downlinks[i]->link->bsyFile);

               writeLogEntry(htick_log,'6',"Forwarding %s for %s",
                       tic.file,
                       addr2string(&filearea->downlinks[i]->link->hisAka));
	    }
	    free(filearea->downlinks[i]->link->bsyFile);
	    filearea->downlinks[i]->link->bsyFile=NULL;
	    free(filearea->downlinks[i]->link->floFile);
	    filearea->downlinks[i]->link->floFile=NULL;
         } // if readAccess == 0
      } // Forward file

   }
   // report about new files - if not hidden
   if (!filearea->hide) {
      newFileReport = (s_newfilereport**)realloc(newFileReport, (newfilesCount+1)*sizeof(s_newfilereport*));
      newFileReport[newfilesCount] = (s_newfilereport*)calloc(1, sizeof(s_newfilereport));
      newFileReport[newfilesCount]->useAka = filearea->useAka;
      newFileReport[newfilesCount]->areaName = filearea->areaName;
      newFileReport[newfilesCount]->areaDesc = filearea->description;
      newFileReport[newfilesCount]->fileName = strdup(tic.file);
      if (config->originInAnnounce) {
         newFileReport[newfilesCount]->origin.zone = tic.origin.zone;
         newFileReport[newfilesCount]->origin.net = tic.origin.net;
         newFileReport[newfilesCount]->origin.node = tic.origin.node;
         newFileReport[newfilesCount]->origin.point = tic.origin.point;
      }

      newFileReport[newfilesCount]->fileDesc = (char**)calloc(tic.anzdesc, sizeof(char*));
      for (i = 0; i < tic.anzdesc; i++) {
         newFileReport[newfilesCount]->fileDesc[i] = strdup(tic.desc[i]);
         if (config->intab != NULL) recodeToInternalCharset(newFileReport[newfilesCount]->fileDesc[i]);
      } /* endfor */
      newFileReport[newfilesCount]->filedescCount = tic.anzdesc;

      newFileReport[newfilesCount]->fileSize = tic.size;

      newfilesCount++;

      reportNewFiles();
   }

   disposeTic(&tic);
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
    char sendfile[256], descr_file_name[256], tmpfile[256];
    char tmp[100], timestr[40], linkfilepath[256];
    char *newticfile;
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
         writeLogEntry(htick_log,'6',"File %s, not found",sendfile);
         disposeTic(&tic);
         return 3;
   }

   newticfile = strrchr(sendfile,PATH_DELIM);
   if (newticfile) strcpy(tic.file, newticfile+1);
   else strcpy(tic.file,sendfile);

   if (filearea->sendorig) {
      strcpy(tmpfile,config->passFileAreaDir);
      strcat(tmpfile,tic.file);
      adaptcase(tmpfile);

      if (copy_file(sendfile,tmpfile)!=0) {
         adaptcase(sendfile);
         if (copy_file(sendfile,tmpfile)==0) {
            writeLogEntry(htick_log,'6',"Copied %s to %s",sendfile,tmpfile);
         } else {
            writeLogEntry(htick_log,'9',"File %s not found or copyable",sendfile);
            disposeTic(&tic);
            return(2);
         }
      } else {
          writeLogEntry(htick_log,'6',"Copied %s to %s",sendfile,tmpfile);
          strcpy(sendfile,tmpfile);
      }
   }

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
   strcpy(timestr,asctime(gmtime(&acttime)));
   timestr[strlen(timestr)-1]=0;
   if (timestr[8]==' ') timestr[8]='0';
   sprintf(tmp,"%s %lu %s UTC %s",
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
       cvtFlavour2Prio(link->fileEchoFlavour), FLOFILE)==1)
      busy = 1;

   if (busy) {
      writeLogEntry(htick_log, '7', "Save TIC in temporary dir");
      //Create temporary directory
       strcpy(linkfilepath,config->busyFileDir);
   } else {
       if (config->separateBundles) {
          strcpy(linkfilepath, link->floFile);
          sprintf(strrchr(linkfilepath, '.'), ".sep%c", PATH_DELIM);
       } else {
           strcpy(linkfilepath, config->passFileAreaDir);
       }
   }
   createDirectoryTree(linkfilepath);

   newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
   writeTic(newticfile,&tic);

   if (!busy) {
      flohandle=fopen(link->floFile,"a");
      fprintf(flohandle,"%s\n",sendfile);
      fprintf(flohandle,"^%s\n",newticfile);
      fclose(flohandle);

      remove(link->bsyFile);

      writeLogEntry(htick_log,'6',"Send %s from %s for %s",
              tic.file,tic.area,addr2string(&link->hisAka));
      free(link->bsyFile);
   }
   free(link->floFile);
   disposeTic(&tic);
   return 0;
}
