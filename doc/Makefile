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
html: htick.html
else
html:
	@echo
endif

ifdef DVIDIR
dvi: htick.dvi
else
dvi:
	@echo
endif

ifdef INFODIR
info: htick.info
else
info:
	@echo
endif

htick.html: htick.texi
	export LC_ALL=C; makeinfo --html --no-split htick.texi

htick.dvi: htick.texi
	texi2dvi htick.texi

htick.info: htick.texi
	makeinfo --no-split htick.texi

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
	-$(MKDIR) $(MKDIROPT) $(INFODIR)
	$(INSTALL) $(IMOPT) htick.info $(INFODIR)
	-install-info --info-dir=$(INFODIR)  $(INFODIR)$(DIRSEP)htick.info
endif
ifdef HTMLDIR
	-$(MKDIR) $(MKDIROPT) $(HTMLDIR)
	$(INSTALL) $(IMOPT) htick*html $(HTMLDIR)
endif
ifdef DVIDIR
	-$(MKDIR) $(MKDIROPT) $(DVIDIR)
	$(INSTALL) $(IMOPT) htick.dvi $(DVIDIR)
endif

uninstall:
	-install-info --remove --info-dir=$(INFODIR) $(INFODIR)$(DIRSEP)htick.info
	-$(RM) $(RMOPT) $(INFODIR)$(DIRSEP)htick.info
	-$(RM) $(RMOPT) $(HTMLDIR)$(DIRSEP)htick*html
	-$(RM) $(RMOPT) $(DVIDIR)$(DIRSEP)htick.dvi
