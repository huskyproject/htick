CC      = cc
INSTDIR = /usr/local/bin
MANDIR  = /usr/local/man
LIBDIR  = /home/fido/lib

OBJS    = htick.o global.o log.o toss.o fcommon.o scan.o areafix.o strsep.o add_descr.o seenby.o recode.o crc32.o hatch.o filelist.o
SRC_DIR = ../src/

CFLAGS  = -c -I../h -I../../fidoconfig -I../../smapi -g  -DUNIX
LFLAGS  = -L$(LIBDIR)


all: $(OBJS) \
     htick 

%.o: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(SRC_DIR)$*.c

htick: $(OBJS)
	$(CC) $(LFLAGS) -o htick $(OBJS) -lsmapiunix -lfidoconfig

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
