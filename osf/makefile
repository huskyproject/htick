CC      = cc
INSTDIR = /usr/local/bin
MANDIR  = /usr/local/man
LIBDIR  = /home/fido/lib

OBJS    = htick.o global.o toss.o fcommon.o scan.o areafix.o add_descr.o seenby.o hatch.o filelist.o filecase.o
SRC_DIR = ../src/

CFLAGS  = -c -I../h -I../../fidoconfig -I../../smapi -g  -DUNIX -DNOSEMAPHORES
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
