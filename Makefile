# htick/Makefile
#
# This file is part of htick, part of the Husky fidonet software project
# Use with GNU make v.3.82 or later
# Requires: husky enviroment
#

htick_LIBS := $(areafix_TARGET_BLD) $(fidoconf_TARGET_BLD) \
              $(smapi_TARGET_BLD) $(huskylib_TARGET_BLD)

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

htick_TARGET     = htick$(_EXE)
htick_TARGET_BLD = $(htick_BUILDDIR)$(htick_TARGET)
htick_TARGET_DST = $(BINDIR_DST)$(htick_TARGET)

ifdef MAN1DIR
    htick_MAN1PAGES := htick.1
    htick_MAN1BLD := $(htick_BUILDDIR)$(htick_MAN1PAGES).gz
    htick_MAN1DST := $(DESTDIR)$(MAN1DIR)$(DIRSEP)$(htick_MAN1PAGES).gz
endif


.PHONY: htick_build htick_install htick_uninstall htick_clean htick_distclean \
        htick_depend htick_doc htick_doc_install htick_doc_uninstall \
        htick_doc_clean htick_doc_distclean htick_rmdir_DEP htick_rm_DEPS \
        htick_clean_OBJ htick_main_distclean

htick_build: $(htick_TARGET_BLD) $(htick_MAN1BLD) htick_doc

ifneq ($(MAKECMDGOALS), depend)
    include $(htick_DOCDIR)Makefile
    ifneq ($(MAKECMDGOALS), distclean)
        ifneq ($(MAKECMDGOALS), uninstall)
            include $(htick_DEPS)
        endif
    endif
endif


# Build application
$(htick_TARGET_BLD): $(htick_ALL_OBJS) $(htick_LIBS) | do_not_run_make_as_root
	$(CC) $(LFLAGS) $(EXENAMEFLAG) $@ $^ $(htick_LIBZ)

# Compile .c files
$(htick_ALL_OBJS): $(htick_OBJDIR)%$(_OBJ): $(htick_SRCDIR)%.c | $(htick_OBJDIR)
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
