#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <filelist.h>
#include <toss.h>
#include <log.h>
#include <global.h>
#include <string.h>
#include <fcommon.h>
#include <common.h>
#include <progprot.h>
#include <add_descr.h>

unsigned long totalfilessize = 0;
unsigned int totalfilesnumber = 0;

void printFileArea(s_filearea area, FILE *f) {
    FILE *fbbs;
    char fileareapath[256], fbbsname[256], fbbsline[264],
	tmp[264], filename[256];
    char *token = "";
    unsigned int flag = 0, totalnumber = 0;
    unsigned long fsize = 0, totalsize = 0;
    struct stat stbuf;
    time_t fileTime;
    struct tm *locTime;
    char **removeFiles = NULL;
    unsigned int removeCount = 0, i;

   strcpy(fileareapath,area.pathName);
   strLower(fileareapath);
   createDirectoryTree(fileareapath);
   strcpy(fbbsname,fileareapath);
   strcat(fbbsname,"files.bbs");
   if ( (fbbs = fopen(fbbsname,"r")) == NULL ) return;
   if (area.description!=NULL)
     fprintf(f,"FileArea: %s (%s)\n", area.areaName, area.description);
   else
     fprintf(f,"FileArea: %s\n", area.areaName);
   fprintf(f,"-----------------------------------------------------------------------------\n");

   while (!feof(fbbs)) {
         fgets(fbbsline,sizeof fbbsline,fbbs);

         if (fbbsline[0]==0 || fbbsline[0]==10 || fbbsline[0]==13)
            continue;

         if (fbbsline[strlen(fbbsline)-1]=='\n')
            fbbsline[strlen(fbbsline)-1]=0;

         if (fbbsline[strlen(fbbsline)-1]=='\r')
            fbbsline[strlen(fbbsline)-1]=0;

	 if (flag && (*fbbsline == '\t' || *fbbsline == ' ' || *fbbsline == '>')) {
	    if (flag == 1) {
	       fprintf(f,"                              ");
	       token=stripLeadingChars(fbbsline, " >");
	       if (*token == '>') token++;
	       fprintf(f," %s\n",token);
	    }
	 }
	 else {
	    flag = 0;

	    strcpy(tmp,fbbsline);
            token = strtok(tmp, " \t\0");

            if (token==NULL)
               continue;

	    strLower(token);
	 }

         if (!flag) {
            strcpy(filename,fileareapath);
            strcat(filename,token);
	    if (fexist(filename)) {
	       fprintf(f,"%-12s",token);
	       stat(filename,&stbuf);
	       fsize = stbuf.st_size;
	       fprintf(f,"%8lu ",fsize);
	       totalsize += fsize;
	       totalnumber++;

	       fileTime = stbuf.st_mtime;
               locTime = localtime(&fileTime);
	       fprintf(f, "%02u-", locTime->tm_mday);
               switch (locTime->tm_mon) {
               case 0: fprintf(f, "Jan");
	          break;
	       case 1: fprintf(f, "Feb");
                  break;
               case 2: fprintf(f, "Mar");
                  break;
               case 3: fprintf(f, "Apr");
                  break;
               case 4: fprintf(f, "May");
                  break;
               case 5: fprintf(f, "Jun");
                  break;
               case 6: fprintf(f, "Jul");
                  break;
               case 7: fprintf(f, "Aug");
                  break;
               case 8: fprintf(f, "Sep");
                  break;
               case 9: fprintf(f, "Oct");
                  break;
               case 10: fprintf(f, "Nov");
                  break;
               case 11: fprintf(f, "Dec");
                  break;
               default:
                 break;
               }
	       fprintf(f, "-%02u", locTime->tm_year % 100);

               token=stripLeadingChars(strtok(NULL, "\0"), " ");
	       if (token == NULL) token="Description not avaliable";
	       fprintf(f," %s\n",token);
	       flag = 1;
	    } else {
               removeFiles = realloc(removeFiles, sizeof(char *)*(removeCount+1));
	       removeFiles[removeCount] = (char *) malloc(strlen(strrchr(filename,PATH_DELIM)+1)+1);
               strcpy(removeFiles[removeCount], strrchr(filename,PATH_DELIM)+1);
	       removeCount++;
	       flag = 2;
	    }
	 }

	 fbbsline[0] = '\0';
   }
   fclose(fbbs);
   fprintf(f,"-----------------------------------------------------------------------------\n");
   fprintf(f,"Total files in area: %4d, total size: %10lu bytes\n\n",totalnumber,totalsize);
   if (removeCount > 0) {
      for (i=0; i<removeCount; i++) {
        removeDesc(fbbsname,removeFiles[i]);
	free(removeFiles[i]);
      }
      free(removeFiles);
   }
   totalfilessize += totalsize;
   totalfilesnumber += totalnumber;
}

void filelist()
{
    FILE *f;
    char logstr[200];
    int i;

   if (strlen(flistfile) == 0) {
      writeLogEntry(htick_log,'6',"Not found output file");
      return;
   }

   if ( (f = fopen(flistfile,"w")) == NULL ) {
         sprintf(logstr,"Could not open for write file %s",flistfile);
         writeLogEntry(htick_log,'6',logstr);
         return;
   }

   for (i=0; i<config->fileAreaCount; i++) {
      if (config->fileAreas[i].pass != 1)
         printFileArea(config->fileAreas[i], f);
   }

   fprintf(f,"=============================================================================\n");
   fprintf(f,"Total files in filelist: %4d, total size: %10lu bytes\n",totalfilesnumber,totalfilessize);
   fprintf(f,"=============================================================================\n\n");

   fclose(f);
}
