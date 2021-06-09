# $Id$
#
# include Husky-Makefile-Config
ifeq ($(DEBIAN), 1)
# Every Debian-Source-Package has one included.
include /usr/share/husky/huskymak.cfg
else ifdef RPM_BUILD_ROOT
# RPM build requires all files to be in the source directory
include huskymak.cfg
else
include ../huskymak.cfg
endif

OBJS     = htick$(_OBJ) global$(_OBJ) toss$(_OBJ) fcommon$(_OBJ) \
           scan$(_OBJ) htickafix$(_OBJ) add_desc$(_OBJ) seenby$(_OBJ) \
           hatch$(_OBJ) filelist$(_OBJ) filecase$(_OBJ) report$(_OBJ) \
           clean$(_OBJ)
MAN1PAGE = htick.1.gz
MAN1DIR  = $(MANDIR)$(DIRSEP)man1
SRC_DIR  = src/

ifeq ($(DEBUG), 1)
  CFLAGS = $(DEBCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(DEBLFLAGS)
else
  CFLAGS = $(OPTCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(OPTLFLAGS)
endif

ifeq ($(SHORTNAME), 1)
  LIBS=-L$(LIBDIR) -lareafix -lfidoconf -lsmapi -lhusky
else
  LIBS=-L$(LIBDIR) -lareafix -lfidoconfig -lsmapi -lhusky
endif

ifeq ($(USE_HPTZIP), 1)
  LIBS+= -lhptzip -lz
  CFLAGS += -DUSE_HPTZIP
endif

CDEFS=-D$(OSTYPE) $(ADDCDEFS)

.PHONY: all

all: $(OBJS) htick$(EXE) htick.1.gz

htick.1.gz: man/htick.1
	gzip -c man/htick.1 > htick.1.gz

htick$(EXE): $(OBJS)
	$(CC) $(LFLAGS) -o htick$(EXE) $(OBJS) $(LIBS)

%$(_OBJ): $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(CDEFS) $(SRC_DIR)$*.c

gen-doc:
	-(cd doc; $(MAKE) all)

clean-doc:
	-(cd doc; $(MAKE) clean)

distclean-doc:
	-(cd doc; $(MAKE) distclean)

install-doc:
	-(cd doc; $(MAKE) install)

uninstall-doc:
	-(cd doc; $(MAKE) uninstall)

clean:
	-$(RM) $(RMOPT) *$(_OBJ)
	-$(RM) $(RMOPT) core

distclean: clean
	-$(RM) $(RMOPT) htick$(EXE)
	-$(RM) $(RMOPT) *.1.gz
	-$(RM) $(RMOPT) *.log

install: htick$(EXE)
	$(MKDIR) $(MKDIROPT) $(DESTDIR)$(BINDIR) $(DESTDIR)$(MAN1DIR)
	$(INSTALL) $(IBOPT) htick$(EXE) $(DESTDIR)$(BINDIR)
	$(INSTALL) $(IMOPT) $(MAN1PAGE) $(DESTDIR)$(MAN1DIR)

uninstall:
	$(RM) $(RMOPT) $(DESTDIR)$(BINDIR)$(DIRSEP)htick$(EXE)
	$(RM) $(RMOPT) $(DESTDIR)$(MAN1DIR)$(DIRSEP)$(MAN1PAGE)

