#ifndef _ADD_DESCR_H_
#define _ADD_DESCR_H_

#include <fidoconfig.h>

int add_description (char *descr_file_name, char *file_name, char **description, int count_desc);
int removeDesc (char *descr_file_name, char *file_name);
int announceInFile (char *announcefile, char *file_name, int size, char *area, s_addr origin, char **description, int count_desc);
int announceNewFileecho (char *announcenewfileecho, char *c_area, char *hisaddr);

#endif
