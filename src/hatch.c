#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hatch.h>
#include <global.h>
#include <fcommon.h>
#include <toss.h>
#include <fidoconf/log.h>
#include <crc32.h>
#include <fidoconf/common.h>
#include <add_desc.h>
#include <fidoconf/recode.h>
#include <version.h>
#include <smapi/progprot.h>
#include <fidoconf/adcase.h>
#include <filecase.h>

void hatch()
{
   s_ticfile tic;
   s_filearea *filearea;
   char *newticfile;
   struct stat stbuf;
   extern s_newfilereport **newFileReport;
   extern int newfilesCount;

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

   MakeProperCase(tic.file);

   strcpy(tic.area,hatcharea);
   filearea=getFileArea(config,tic.area);

   if (config->outtab != NULL) recodeToTransportCharset(hatchdesc);
   tic.desc=srealloc(tic.desc,(tic.anzdesc+1)*sizeof(&tic.desc));
   tic.desc[tic.anzdesc]=sstrdup(hatchdesc);
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

   sendToLinks(0, filearea, &tic, hatchfile);

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
   link = getLinkFromAddr(config, address);
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
   tic.path=srealloc(tic.path,(tic.anzpath+1)*sizeof(*tic.path));
   tic.path[tic.anzpath]=sstrdup(tmp);
   tic.anzpath++;

   // Adding Downlink to Seen-By
   tic.seenby=srealloc(tic.seenby,(tic.anzseenby+1)*sizeof(s_addr));
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
      nfree(link->bsyFile);
   }
   nfree(link->floFile);
   disposeTic(&tic);
   return 0;
}
