#ifndef _ADD_DESCR_H_
#define _ADD_DESCR_H_

#include <fcommon.h>

int add_description (char *descr_file_name, char *file_name, char **description, unsigned int count_desc);
int removeDesc (char *descr_file_name, char *file_name);
int announceInFile (char *announcefile, char *file_name, int size, char *area, s_addr origin, char **description, int count_desc);
int announceNewFileecho (char *announcenewfileecho, char *c_area, char *hisaddr);
int getDesc (char *descr_file_name, char *file_name, s_ticfile *tic);

#endif
