/* $Id$ */
#ifndef _ADD_DESCR_H_
#define _ADD_DESCR_H_

#include "global.h"

int add_description(char * descr_file_name,
                    char * file_name,
                    char ** description,
                    unsigned int count_desc);
int removeDesc(char * descr_file_name, char * file_name);
int announceNewFileecho(char * announcenewfileecho, char * c_area, char * hisaddr);
int GetDescFormBbsFile(char * descr_file_name, char * file_name, s_ticfile * tic);
int GetDescFormDizFile(char * fileName, s_ticfile * tic);
int GetDescFormFile(char * fileName, s_ticfile * tic);

#endif
