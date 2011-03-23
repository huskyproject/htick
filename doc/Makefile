# $Id$
#
# makefile for build and install htick documentation
# Require huskymak.cfg
#
# make html  generate one htick.html file
# make dvi   generate htick.dvi
# make info  generate htick.info file (for texinfo documentation system)
# make all   generate all files for your platform
#

ifeq ($(DEBIAN), 1)
# Every Debian-Source-Paket has one included.
include /usr/share/husky/huskymak.cfg
else
include ../../huskymak.cfg
endif

all: html info dvi

ifdef HTMLDIR
html: htick.html htick.rus.html
else
html:
	@echo
endif

ifdef DVIDIR
dvi: htick.dvi htick.rus.dvi
else
dvi:
	@echo
endif

ifdef INFODIR
info: htick.info htick.rus.info
else
info:
	@echo
endif

htick.html: htick.texi
	export LC_ALL=C; makeinfo --html --no-split $<

htick.rus.html: htick.rus.texi
	export LC_ALL=C; makeinfo --html --no-split $<

htick.dvi: htick.texi
	texi2dvi $<

htick.info: htick.texi
	makeinfo --no-split $<

clean:
	-$(RM) $(RMOPT) htick.aux
	-$(RM) $(RMOPT) htick.cp
	-$(RM) $(RMOPT) htick.fn
	-$(RM) $(RMOPT) htick.fns
	-$(RM) $(RMOPT) htick.ky
	-$(RM) $(RMOPT) htick.log
	-$(RM) $(RMOPT) htick.pg
	-$(RM) $(RMOPT) htick.toc
	-$(RM) $(RMOPT) htick.tp
	-$(RM) $(RMOPT) htick.vr

distclean: clean
	-$(RM) $(RMOPT) htick.dvi
	-$(RM) $(RMOPT) htick.info*
	-$(RM) $(RMOPT) htick.inf
	-$(RM) $(RMOPT) htick*html

install: info html dvi
	@echo .
ifdef INFODIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(INFODIR)
	$(INSTALL) $(IMOPT) htick.info htick.rus.info $(DESTDIR)$(INFODIR)
	-install-info --info-dir=$(DESTDIR)$(INFODIR)  $(DESTDIR)$(INFODIR)$(DIRSEP)htick.info
endif
ifdef HTMLDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(HTMLDIR)
	$(INSTALL) $(IMOPT) htick*html $(DESTDIR)$(HTMLDIR)
endif
ifdef DVIDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(DVIDIR)
	$(INSTALL) $(IMOPT) htick.dvi htick.rus.dvi $(DESTDIR)$(DVIDIR)
endif

uninstall:
	-install-info --remove --info-dir=$(DESTDIR)$(INFODIR) $(DESTDIR)$(INFODIR)$(DIRSEP)htick.info
	-$(RM) $(RMOPT) $(DESTDIR)$(INFODIR)$(DIRSEP)htick.info
	-$(RM) $(RMOPT) $(DESTDIR)$(HTMLDIR)$(DIRSEP)htick*html
	-$(RM) $(RMOPT) $(DESTDIR)$(DVIDIR)$(DIRSEP)htick.dvi
