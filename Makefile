# htick/Makefile
#
# This file is part of htick, part of the Husky fidonet software project
# Use with GNU make v.3.82 or later
# Requires: husky enviroment
#

# Version
htick_g1:=$(GREP) -Po 'define\s+VER_MAJOR\s+\K\d+'
htick_g2:=$(GREP) -Po 'define\s+VER_MINOR\s+\K\d+'
htick_g3:=$(GREP) -Po 'define\s+VER_PATCH\s+\K\d+'
htick_g4:=$(GREP) -Po 'char\s+cvs_date\[\]\s*=\s*"\K\d+-\d+-\d+'
htick_VERMAJOR := $(shell $(htick_g1) $(htick_ROOTDIR)$(htick_H_DIR)version.h)
htick_VERMINOR := $(shell $(htick_g2) $(htick_ROOTDIR)$(htick_H_DIR)version.h)
htick_VERPATCH := $(shell $(htick_g3) $(htick_ROOTDIR)$(htick_H_DIR)version.h)
htick_cvsdate  := $(shell $(htick_g4) $(htick_ROOTDIR)cvsdate.h)
htick_reldate  := $(subst -,,$(htick_cvsdate))
htick_VER      := $(htick_VERMAJOR).$(htick_VERMINOR).$(htick_reldate)

htick_LIBS := $(areafix_TARGET_BLD) $(fidoconf_TARGET_BLD) \
              $(smapi_TARGET_BLD) $(huskylib_TARGET_BLD)

htick_CFLAGS = $(CFLAGS)
htick_CDEFS := $(CDEFS) -I$(areafix_ROOTDIR) \
                        -I$(fidoconf_ROOTDIR) \
                        -I$(smapi_ROOTDIR) \
                        -I$(huskylib_ROOTDIR) \
                        -I$(htick_ROOTDIR)$(htick_H_DIR)

ifneq ($(USE_HPTZIP), 0)
    ifeq ($(DYNLIBS), 1)
        htick_LIBZ = -lz
    else
        htick_LIBZ = -Xlinker -l:libz.a
    endif
    htick_LIBS += $(hptzip_TARGET_BLD)
    htick_CFLAGS += -DUSE_HPTZIP
    htick_CDEFS  += -I$(hptzip_ROOTDIR)
endif

htick_SRC  = $(wildcard $(htick_SRCDIR)*.c)
htick_OBJS = $(addprefix $(htick_OBJDIR),$(notdir $(htick_SRC:.c=$(_OBJ))))
htick_DEPS = $(addprefix $(htick_DEPDIR),$(notdir $(htick_SRC:.c=$(_DEP))))

htick_TARGET     = htick$(_EXE)
htick_TARGET_BLD = $(htick_BUILDDIR)$(htick_TARGET)
htick_TARGET_DST = $(BINDIR_DST)$(htick_TARGET)

ifdef MAN1DIR
    htick_MAN1PAGES := htick.1
    htick_MAN1BLD := $(htick_BUILDDIR)$(htick_MAN1PAGES).gz
    htick_MAN1DST := $(DESTDIR)$(MAN1DIR)$(DIRSEP)$(htick_MAN1PAGES).gz
endif


.PHONY: htick_all htick_install htick_uninstall htick_clean htick_distclean \
        htick_depend htick_doc htick_doc_install htick_doc_uninstall \
        htick_doc_clean htick_doc_distclean htick_rmdir_DEP htick_rm_DEPS \
        htick_clean_OBJ htick_main_distclean

htick_all: $(htick_TARGET_BLD) $(htick_MAN1BLD) htick_doc

ifneq ($(MAKECMDGOALS), depend)
    include $(htick_DOCDIR)Makefile
ifneq ($(MAKECMDGOALS), distclean)
ifneq ($(MAKECMDGOALS), uninstall)
    include $(htick_DEPS)
endif
endif
endif


# Build application
$(htick_TARGET_BLD): $(htick_OBJS) $(htick_LIBS) | do_not_run_make_as_root
	$(CC) $(LFLAGS) $(EXENAMEFLAG) $@ $^ $(htick_LIBZ)

# Compile .c files
$(htick_OBJS): $(htick_OBJDIR)%$(_OBJ): $(htick_SRCDIR)%.c | $(htick_OBJDIR)
	$(CC) $(htick_CFLAGS) $(htick_CDEFS) -o $(htick_OBJDIR)$*$(_OBJ) $(htick_SRCDIR)$*.c

$(htick_OBJDIR): | $(htick_BUILDDIR) do_not_run_make_as_root
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@


# Build man pages
ifdef MAN1DIR
    $(htick_MAN1BLD): $(htick_MANDIR)$(htick_MAN1PAGES)
	@[ $$($(ID) $(IDOPT)) -eq 0 ] && echo "DO NOT run \`make\` from root" && exit 1 || true
	gzip -c $< > $@
else
    $(htick_MAN1BLD): ;
endif


# Install
ifneq ($(MAKECMDGOALS), install)
    htick_install: ;
else
    htick_install: $(htick_TARGET_DST) htick_install_man htick_doc_install ;
endif

$(htick_TARGET_DST): $(htick_TARGET_BLD) | $(DESTDIR)$(BINDIR)
	$(INSTALL) $(IBOPT) $< $(DESTDIR)$(BINDIR); \
	$(TOUCH) "$@"

ifndef MAN1DIR
    htick_install_man: ;
else
    htick_install_man: $(htick_MAN1DST)

    $(htick_MAN1DST): $(htick_MAN1BLD) | $(DESTDIR)$(MAN1DIR)
	$(INSTALL) $(IMOPT) $< $(DESTDIR)$(MAN1DIR); $(TOUCH) "$@"
endif


# Clean
htick_clean: htick_clean_OBJ htick_doc_clean
	-[ -d "$(htick_OBJDIR)" ] && $(RMDIR) $(htick_OBJDIR) || true

htick_clean_OBJ:
	-$(RM) $(RMOPT) $(htick_OBJDIR)*

# Distclean
htick_distclean: htick_doc_distclean htick_main_distclean htick_rmdir_DEP
	-[ -d "$(htick_BUILDDIR)" ] && $(RMDIR) $(htick_BUILDDIR) || true

htick_rmdir_DEP: htick_rm_DEPS
	-[ -d "$(htick_DEPDIR)" ] && $(RMDIR) $(htick_DEPDIR) || true

htick_rm_DEPS:
	-$(RM) $(RMOPT) $(htick_DEPDIR)*

htick_main_distclean: htick_clean
	-$(RM) $(RMOPT) $(htick_TARGET_BLD)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(htick_MAN1BLD)
endif


# Uninstall
htick_uninstall: htick_doc_uninstall
	-$(RM) $(RMOPT) $(htick_TARGET_DST)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(htick_MAN1DST)
endif


# Depend
ifeq ($(MAKECMDGOALS),depend)
htick_depend: $(htick_DEPS) ;

# Build a dependency makefile for every source file
$(htick_DEPS): $(htick_DEPDIR)%$(_DEP): $(htick_SRCDIR)%.c | $(htick_DEPDIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(htick_CFLAGS) $(htick_CDEFS) $< > $@.$$$$; \
	sed 's,\($*\)$(_OBJ)[ :]*,$(htick_OBJDIR)\1$(_OBJ) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(htick_DEPDIR): | $(htick_BUILDDIR) do_not_run_depend_as_root
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@
endif

$(htick_BUILDDIR):
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@
