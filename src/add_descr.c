#include <stdio.h>
#include <stdlib.h>
#include <add_descr.h>
#include <fidoconfig.h>
#include <common.h>
#include <string.h>
#include <progprot.h>
#include <global.h>
#include <recode.h>
#include <toss.h>

int add_description (char *descr_file_name, char *file_name, char **description, int count_desc)
{
   FILE *descr_file;
   int i;
   char desc_line[263], namefile[50];
   
   descr_file = fopen (descr_file_name, "a");
   if (descr_file == NULL) return 1;
   strcpy(namefile,file_name);
   strLower(namefile);
   fprintf (descr_file, "%-12s", namefile);
   for (i=0;i<count_desc;i++) {
      strcpy(desc_line,description[i]);
      if (config->intab != NULL) recodeToInternalCharset(desc_line);
      if (i==0)
         fprintf(descr_file," %s\n",desc_line);
      else
         fprintf(descr_file,"             %s\n",desc_line);
   }
   if (count_desc == 0) fprintf(descr_file,"\n");
   fclose (descr_file);
   return 0;
};

int removeDesc (char *descr_file_name, char *file_name)
{
    FILE *f1, *f2;
    char hlp[264] = "", tmp[264], *token, descr_file_name_tmp[256];
    int flag = 0;

   f1 = fopen (descr_file_name, "r");
   if (f1 == NULL) return 1;

   strcpy(descr_file_name_tmp,descr_file_name);
   strcat(descr_file_name_tmp,".tmp");

   f2 = fopen (descr_file_name_tmp, "w");
   if (f2 == NULL) {
      fclose (f1);
      return 1;
   }

   while (!feof(f1)) {
         fgets(hlp,sizeof hlp,f1);

         if (hlp[0]==0 || hlp[0]==10 || hlp[0]==13 || hlp[0]==';')
            continue;

         if (hlp[strlen(hlp)-1]=='\r')
            hlp[strlen(hlp)-1]=0;

	 if (flag && (*hlp == '\t' || *hlp == ' ' || *hlp == '>'))
	    continue;
	 else
	    flag = 0;

	 strcpy(tmp,hlp);
         token = strtok(tmp, " \t\n\0");

         if (token==NULL)
            continue;

         if (stricmp(token,file_name) != 0)
	    fputs(hlp, f2);
	 else
	   flag = 1;
	 hlp[0] = '\0';
   }

   fclose (f1);
   fclose (f2);
   remove(descr_file_name);
   move_file(descr_file_name_tmp,descr_file_name);
   return 0;
}

int announceInFile (char *announcefile, char *file_name, int size, char *area, s_addr origin, char **description, int count_desc)
{
   FILE *ann_file;
   int i;
   char desc_line[256];
   
   if (!fexist(announcefile)) {
      ann_file = fopen (announcefile, "w");
      if (ann_file == NULL) return 1;
      fprintf (ann_file,"File             Size         Area                 Origin\n");
      fprintf (ann_file,"---------------- ------------ -------------------- ----------------\n");
   } else {
      ann_file = fopen (announcefile, "a");
      if (ann_file == NULL) return 1;
   }
   fprintf (ann_file, "%-16s %-12d %-20s %u:%u/%u.%u\n", file_name, size, area, origin.zone, origin.net, origin.node, origin.point);
   for (i=0;i<count_desc;i++) {
      strcpy(desc_line,description[i]);
      if (config->intab != NULL) recodeToInternalCharset(desc_line);
      fprintf(ann_file,"%s\n",desc_line);
   }
   fprintf (ann_file,"-------------------------------------------------------------------\n");
   fclose (ann_file);
   return 0;
}

int announceNewFileecho (char *announcenewfileecho, char *c_area, char *hisaddr)
{
   FILE *ann_file;
   
   if (!fexist(announcenewfileecho)) {
      ann_file = fopen (announcenewfileecho, "w");
      if (ann_file == NULL) return 1;
      fprintf (ann_file,"Action   Name                                                 By\n");
      fprintf (ann_file,"-------------------------------------------------------------------------------\n");
   } else {
      ann_file = fopen (announcenewfileecho, "a");
      if (ann_file == NULL) return 1;
   }
   fprintf (ann_file, "Created  %-52s %s\n", c_area, hisaddr);
   fclose (ann_file);
   return 0;
}

int getDesc (char *descr_file_name, char *file_name, s_ticfile *tic)
{
    FILE *f1;
    char hlp[264] = "", tmp[264], *token;
    int flag = 0;

   f1 = fopen (descr_file_name, "r");
   if (f1 == NULL) return 1;

   while (!feof(f1)) {
         fgets(hlp,sizeof hlp,f1);

         if (hlp[0]==0 || hlp[0]==10 || hlp[0]==13)
            continue;

         if (hlp[strlen(hlp)-1]=='\r')
            hlp[strlen(hlp)-1]=0;

         if (hlp[strlen(hlp)-1]=='\n')
            hlp[strlen(hlp)-1]=0;

	 if (flag && (*hlp == '\t' || *hlp == ' ' || *hlp == '>')) {
	    token=stripLeadingChars(hlp, " >");
	    if (*token == '>') token++;
            tic->desc=
            realloc(tic->desc,(tic->anzdesc+1)*sizeof(*tic->desc));
            tic->desc[tic->anzdesc]=strdup(token);
            tic->anzdesc++;
	    continue;
	 }
	 else
	    flag = 0;

	 strcpy(tmp,hlp);
         token = strtok(tmp, " \t\0");

         if (token==NULL)
            continue;

         if (stricmp(token,file_name) == 0) {
            token=stripLeadingChars(strtok(NULL, "\0"), " ");
	    if (token != NULL) {
               tic->desc=
               realloc(tic->desc,(tic->anzdesc+1)*sizeof(*tic->desc));
               tic->desc[tic->anzdesc]=strdup(token);
               tic->anzdesc++;
	    }
	    flag = 1;
	 }
	 hlp[0] = '\0';
   }

   fclose (f1);
   return 0;
}
