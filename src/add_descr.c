#include <stdio.h>
#include <add_descr.h>

int add_description (char *descr_file_name, char *file_name, char *description)
{
   FILE *descr_file;
   
   descr_file = fopen (descr_file_name, "a");
   if (descr_file == NULL) return 1;
   fprintf (descr_file, "%s\t%s\n", file_name, description);
   fclose (descr_file);
   return 0;
};
