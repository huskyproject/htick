CC      = gcc
CFLAGS  = -c -I../h -I../.. -Wall -O3 -DUNIX
SRC_DIR = ../src/
LIBS = ../../fidoconf/libfidoconfigsun.a ../../smapi/libsmapisun.a

OBJS    = htick.o global.o toss.o fcommon.o scan.o areafix.o add_desc.o seenby.o hatch.o filelist.o filecase.o report.o clean.o

all: $(OBJS) \
     htick

%.o: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(SRC_DIR)$*.c

htick: $(OBJS)
	$(CC) -s -o htick $(OBJS) $(LIBS)

clean:
	-rm -f *.o
	-rm core
	-rm htick

distclean: clean
	-rm htick
	-rm *.log

