#ifndef _FILELIST_H
#define _FILELIST_H

#include <fidoconfig/fidoconfig.h>

void formatDesc(char **desc, int *count);
void putFileInFilelist(FILE *f, char *filename, off_t size, int day, int month, int year, int countdesc, char **desc);
void printFileArea(char *area_areaName, char *area_pathName, char *area_description, FILE *f, int bbs);
void filelist();

#endif
