CC      = gcc
DEBUG   = 0

OBJS    = htick.o global.o log.o toss.o fcommon.o scan.o areafix.o strsep.o add_desc.o seenby.o recode.o crc32.o hatch.o filelist.o filecase.o
SRC_DIR = ../src/

ifeq ($(DEBUG), 1)
  CFLAGS  = -c -I../h -I../.. -Wall -ggdb -O2 -DUNIX
  LFLAGS  = -L../../smapi -L../../fidoconf
else
  CFLAGS  = -c -I../h -I../.. -Wall -O3 -s -DUNIX
  LFLAGS  = -L../../smapi -L../../fidoconf
endif

all: $(OBJS) \
     htick

%.o: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(SRC_DIR)$*.c

htick: $(OBJS)
	$(CC) $(LFLAGS) -o htick $(OBJS) -lsmapibe -lfidoconfigbe

clean:
	-rm -f *.o
	-rm *~
	-rm core
	-rm htick

distclean: clean
	-rm htick
	-rm *.1.gz
	-rm *.log