# include Husky-Makefile-Config
ifeq ($(DEBIAN), 1)
# Every Debian-Source-Paket has one included.
include /usr/share/husky/huskymak.cfg
else ifdef RPM_BUILD_ROOT
# RPM build requires all files to be in the source directory
include ../huskymak.cfg
else
include ../../huskymak.cfg
endif

.PHONY: all
all: info html ruhtml txt rutxt dvi rudvi pdf

ifdef INFODIR
info: htick.info
else ifdef RUINFODIR
info: htick.info
else
info: ;
endif

ifdef HTMLDIR
html: htick.html
else
html: ;
endif

ifdef RUHTMLDIR
ruhtml: htick.ru.html
else
ruhtml: ;
endif

ifdef TXTDIR
txt: htick.txt
else
txt: ;
endif

ifdef RUTXTDIR
rutxt: htick.ru.txt
else
rutxt: ;
endif

ifdef DVIDIR
dvi: htick.dvi
else
dvi: ;
endif

ifdef RUDVIDIR
rudvi: htick.ru.dvi
else
rudvi: ;
endif

ifdef PDFDIR
pdf: htick.pdf
else
pdf: ;
endif

ifdef INFODIR
htick.info: htick.texi
	-makeinfo --no-split htick.texi -o htick.info
else ifdef RUINFODIR
htick.info: htick.rus.texi
	-makeinfo --no-split -o htick.info htick.rus.texi
endif

htick.html: htick.texi
	-LC_ALL=C makeinfo --html --no-split htick.texi -o htick.html

htick.ru.html: htick.rus.texi
	-LC_ALL=C makeinfo --html --no-split -o htick.ru.html htick.rus.texi

htick.txt: htick.texi
	-LC_ALL=C makeinfo --plaintext --no-split htick.texi -o htick.txt

htick.ru.txt: htick.rus.texi
	-LC_ALL=C makeinfo --plaintext --no-split htick.rus.texi -o htick.ru.txt

htick.dvi: htick.texi
	-texi2dvi htick.texi

htick.ru.dvi: htick.rus.texi
	-LC_ALL=C texi2dvi htick.rus.texi -o htick.ru.dvi

htick.pdf: htick.texi
	-LC_ALL=C makeinfo --pdf --no-split htick.texi -o htick.pdf

clean:
	-$(RM) $(RMOPT) htick.fns
	-$(RM) $(RMOPT) htick.aux
	-$(RM) $(RMOPT) htick.toc
	-$(RM) $(RMOPT) htick.cp
	-$(RM) $(RMOPT) htick.fn
	-$(RM) $(RMOPT) htick.vr
	-$(RM) $(RMOPT) htick.kr
	-$(RM) $(RMOPT) htick.ke
	-$(RM) $(RMOPT) htick.tp
	-$(RM) $(RMOPT) htick.kes
	-$(RM) $(RMOPT) htick.krs
	-$(RM) $(RMOPT) htick.ky
	-$(RM) $(RMOPT) htick.pg
	-$(RM) $(RMOPT) htick.cps
	-$(RM) $(RMOPT) htick.log
	-$(RM) $(RMOPT) *~

distclean: clean
	-$(RM) $(RMOPT) htick*dvi
	-$(RM) $(RMOPT) htick.inf
	-$(RM) $(RMOPT) htick*html
	-$(RM) $(RMOPT) htick.ps
	-$(RM) $(RMOPT) htick.doc
	-$(RM) $(RMOPT) htick*txt
	-$(RM) $(RMOPT) htick.pdf
	-$(RM) $(RMOPT) htick.info

install: info html ruhtml txt rutxt dvi rudvi pdf
ifdef INFODIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(INFODIR)
	$(INSTALL) $(IMOPT) htick.info $(DESTDIR)$(INFODIR)
	-install-info --info-dir=$(INFODIR)  $(DESTDIR)$(INFODIR)$(DIRSEP)htick.info
else ifdef RUINFODIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(RUINFODIR)
	$(INSTALL) $(IMOPT) htick.info $(DESTDIR)$(RUINFODIR)
	-install-info --info-dir=$(RUINFODIR)  $(DESTDIR)$(RUINFODIR)$(DIRSEP)htick.info
endif

ifdef HTMLDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(HTMLDIR)
	$(INSTALL) $(IMOPT) htick.html $(DESTDIR)$(HTMLDIR)
endif
ifdef RUHTMLDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(RUHTMLDIR)
	$(INSTALL) $(IMOPT) htick.ru.html $(DESTDIR)$(RUHTMLDIR)
endif
ifdef TXTDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(TXTDIR)
	$(INSTALL) $(IMOPT) htick.txt $(DESTDIR)$(TXTDIR)
endif
ifdef RUTXTDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(RUTXTDIR)
	$(INSTALL) $(IMOPT) htick.ru.txt $(DESTDIR)$(RUTXTDIR)
endif
ifdef DVIDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(DVIDIR)
	$(INSTALL) $(IMOPT) htick.dvi $(DESTDIR)$(DVIDIR)
endif
ifdef RUDVIDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(RUDVIDIR)
	$(INSTALL) $(IMOPT) htick.ru.dvi $(DESTDIR)$(RUDVIDIR)
endif
ifdef PDFDIR
	-$(MKDIR) $(MKDIROPT) $(DESTDIR)$(PDFDIR)
	$(INSTALL) $(IMOPT) htick.pdf $(DESTDIR)$(PDFDIR)
endif

uninstall:
ifdef INFODIR
	-install-info --remove --info-dir=$(INFODIR) \
      $(DESTDIR)$(INFODIR)$(DIRSEP)htick.info >& /dev/null || true
	-$(RM) $(RMOPT) $(DESTDIR)$(INFODIR)$(DIRSEP)htick.info
else ifdef RUINFODIR
	-install-info --remove --info-dir=$(RUINFODIR) \
      $(DESTDIR)$(RUINFODIR)$(DIRSEP)htick.info >& /dev/null || true
	-$(RM) $(RMOPT) $(DESTDIR)$(RUINFODIR)$(DIRSEP)htick.info
endif
ifdef HTMLDIR
	-$(RM) $(RMOPT) $(DESTDIR)$(HTMLDIR)$(DIRSEP)htick.html
	-$(RMDIR) $(DESTDIR)$(HTMLDIR) >& /dev/null || true
endif
ifdef RUHTMLDIR
	-$(RM) $(RMOPT) $(DESTDIR)$(RUHTMLDIR)$(DIRSEP)htick.ru.html
	-$(RMDIR) $(DESTDIR)$(RUHTMLDIR) >& /dev/null || true
endif
ifdef TXTDIR
	-$(RM) $(RMOPT) $(DESTDIR)$(TXTDIR)$(DIRSEP)htick.txt
	-$(RMDIR) $(DESTDIR)$(TXTDIR) >& /dev/null || true
endif
ifdef RUTXTDIR
	-$(RM) $(RMOPT) $(DESTDIR)$(RUTXTDIR)$(DIRSEP)htick.ru.txt
	-$(RMDIR) $(DESTDIR)$(RUTXTDIR) >& /dev/null || true
endif
ifdef DVIDIR
	-$(RM) $(RMOPT) $(DESTDIR)$(DVIDIR)$(DIRSEP)htick.dvi
	-$(RMDIR) $(DESTDIR)$(DVIDIR) >& /dev/null || true
endif
ifdef RUDVIDIR
	-$(RM) $(RMOPT) $(DESTDIR)$(RUDVIDIR)$(DIRSEP)htick.ru.dvi
	-$(RMDIR) $(DESTDIR)$(RUDVIDIR) >& /dev/null || true
endif
ifdef PDFDIR
	-$(RM) $(RMOPT) $(DESTDIR)$(PDFDIR)$(DIRSEP)htick.pdf
	-$(RMDIR) $(DESTDIR)$(PDFDIR) >& /dev/null || true
endif
