DEBUG   = 1
CC      = gcc
INSTDIR = /usr/local/bin
MANDIR  = /usr/local/man

OBJS    = htick.o global.o log.o toss.o fcommon.o scan.o areafix.o strsep.o add_descr.o seenby.o recode.o crc32.o hatch.o filelist.o
SRC_DIR = ../src/

FIDOCONFIG = ../../fidoconfig/libfidoconfig.a
SMAPI      = ../../smapi/libsmapisun.a

ifeq ($(DEBUG), 1)
  CFLAGS  = -c -I../h -I../../fidoconfig -I../../smapi -Wall -ggdb -O3 -s -DUNIX
  LFLAGS  = -L../../fidoconfig -L../../smapi
#  LFLAGS  =
else
  CFLAGS  = -c -I../h -I../../smapi -Wall -O3 -s -DUNIX
  LFLAGS  = -L../../smapi -L../../fidoconfig
# LFLAGS  =
endif

all: $(OBJS) \
     htick

%.o: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(SRC_DIR)$*.c

htick: $(OBJS)
	$(CC) $(LFLAGS) -o htick $(OBJS) $(FIDOCONFIG) $(SMAPI)

clean:
	-rm -f *.o
	-rm *~
	-rm core
	-rm htick

distclean: clean
	-rm htick
	-rm *.1.gz
	-rm *.log

install:
	install htick $(INSTDIR)
