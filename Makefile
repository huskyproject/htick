# include Husky-Makefile-Config
include ../huskymak.cfg

OBJS    = htick$(OBJ) global$(OBJ) log$(OBJ) toss$(OBJ) fcommon$(OBJ) \
          scan$(OBJ) areafix$(OBJ) strsep$(OBJ) add_desc$(OBJ) seenby$(OBJ) \
          recode$(OBJ) crc32$(OBJ) hatch$(OBJ) filelist$(OBJ) filecase$(OBJ)
SRC_DIR = src/

ifeq ($(DEBUG), 1)
  CFLAGS = $(DEBCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(DEBLFLAGS)
else
  CFLAGS = $(OPTCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(OPTLFLAGS)
endif

ifeq ($(SHORTNAME), 1)
  LIBS  = -L$(LIBDIR) -lfidoconf -lsmapi
else
  LIBS  = -L$(LIBDIR) -lfidoconfig -lsmapi
endif

CDEFS=-D$(OSTYPE) $(ADDCDEFS)

all: $(OBJS) htick$(EXE)

%$(OBJ): $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(CDEFS) $(SRC_DIR)$*.c

htick$(EXE): $(OBJS)
	$(CC) $(LFLAGS) -o htick$(EXE) $(OBJS) $(LIBS)

clean:
	-$(RM) *$(OBJ)
	-$(RM) *~
	-$(RM) core
	-$(RM) htick$(EXE)

distclean: clean
	-$(RM) htick$(EXE)
	-$(RM) *.1.gz
	-$(RM) *.log

install: htick$(EXE)
	$(INSTALL) $(IBOPT) htick$(EXE) $(BINDIR)

uninstall:
	$(RM) $(BINDIR)$(DIRSEP)htick$(EXE)

