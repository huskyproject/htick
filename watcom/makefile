#
#  Makefile for Open Watcom 2.0
#
#  Use Watcom Make:
#
#  wmake NT=1
#
#    or
#
#  wmake OS2=1
#
#  Set DEBUG=1 to enable debugging.
#

CC=wcl386
WLIB=wlib -q -b

#  wcl386 in Windows & OS/2 will not accept forward slashes as path
#  separators with the -I parameter unless you put the path in
#  double quotes:

INC=-I"../h" -I"../../huskylib" -I"../../fidoconf" -I"../../smapi" -I"../../areafix"

SRC_DIR=../src

CFLAGS=-zq -bt=nt -l=nt
LIBS=../../huskylib/make/huskywcw.lib ../../fidoconf/make/fconfwcw.lib &
 ../../smapi/make/smapiwcw.lib ../../areafix/make/afixwcw.lib

!ifdef OS2
CFLAGS=-zq -bt=os2v2 -l=os2v2
LIBS=../../huskylib/make/huskywco.lib ../../fidoconf/make/fconfwco.lib &
 ../../smapi/make/smapiwco.lib ../../areafix/make/afixwco.lib
!endif

!ifdef DEBUG
CFLAGS+= -d2
!endif

BIN1=htick.exe
SRC1=$(SRC_DIR)/htick.c &
 $(SRC_DIR)/global.c &
 $(SRC_DIR)/toss.c &
 $(SRC_DIR)/fcommon.c &
 $(SRC_DIR)/scan.c &
 $(SRC_DIR)/htickafix.c &
 $(SRC_DIR)/add_desc.c &
 $(SRC_DIR)/seenby.c &
 $(SRC_DIR)/hatch.c &
 $(SRC_DIR)/filelist.c &
 $(SRC_DIR)/filecase.c &
 $(SRC_DIR)/report.c &
 $(SRC_DIR)/clean.c

BINS=$(BIN1)

{$(SRC_DIR)}.c.obj:
	$(CC) $(CFLAGS) $(INC) -fo=$@ -c $<

all: $(BINS)

$(BIN1): $(SRC1)
	$(CC) $(CFLAGS) $(INC) -Fe=$(BIN1) $(SRC1) $(LIBS)

.IGNORE
clean: .SYMBOLIC
	rm *.o
	rm *.obj
	rm *.err
	rm $(BINS)
