# A makefile for htick documentation
# htick/doc/Makefile

.PHONY: htick_info htick_html htick_txt htick_dvi htick_pdf \
        htick_install_info htick_install_html htick_install_txt \
        htick_install_dvi htick_install_pdf htick_clean_DOC \
        htick_info_uninstall htick_docs_uninstall

ifdef DOCDIR
    htick_doc: htick_info htick_html htick_txt htick_dvi htick_pdf ;
else
    htick_doc: htick_info ;
endif

ifdef INFODIR
    htick_info: $(htick_BUILDDIR)htick.info.gz
else
    htick_info: ;
endif

ifeq ($(HTML), 1)
    htick_html: $(htick_BUILDDIR)htick.html
else
    htick_html: ;
endif

ifeq ($(TXT), 1)
    htick_txt: $(htick_BUILDDIR)htick.txt
else
    htick_txt: ;
endif

ifeq ($(DVI), 1)
    htick_dvi: $(htick_BUILDDIR)htick.dvi
else
    htick_dvi: ;
endif

ifeq ($(PDF), 1)
    htick_pdf: $(htick_BUILDDIR)htick.pdf
else
    htick_pdf: ;
endif

htick_DOCDIR_BLD=$(htick_BUILDDIR)doc$(DIRSEP)
htick_TEXI=$(htick_DOCDIR_BLD)htick.texi

$(htick_BUILDDIR)htick.info.gz: $(htick_BUILDDIR)htick.info
	gzip -c $? > $@

$(htick_BUILDDIR)htick.info: $(htick_TEXI)
	-$(MAKEINFO) $(MINOSPLIT) "$<" $(MIOUT) "$@"

$(htick_BUILDDIR)htick.html: $(htick_TEXI)
	-LC_ALL=C $(MAKEINFO) $(MIHTML) $(MINOSPLIT) "$<" $(MIOUT) "$@"

$(htick_BUILDDIR)htick.txt: $(htick_TEXI)
	-LC_ALL=C $(MAKEINFO) $(MIPTEXT) $(MINOSPLIT) "$<" $(MIOUT) "$@"

$(htick_BUILDDIR)htick.dvi: $(htick_DOCDIR_BLD)htick.dvi
	$(LN) $(LNHOPT) "$<" $(htick_BUILDDIR)

$(htick_DOCDIR_BLD)htick.dvi: $(htick_TEXI)
	-cd $(htick_DOCDIR_BLD); $(TEXI2DVI) ../../../"$<"

$(htick_BUILDDIR)htick.pdf: $(htick_DOCDIR_BLD)htick.pdf
	$(LN) $(LNHOPT) "$<" $(htick_BUILDDIR)

$(htick_DOCDIR_BLD)htick.pdf: $(htick_TEXI)
	-cd $(htick_DOCDIR_BLD); \
	LC_ALL=C $(MAKEINFO) $(MIPDF) $(MINOSPLIT) htick.texi $(MIOUT) htick.pdf

$(htick_TEXI): $(htick_DOCDIR)htick.texi | $(htick_DOCDIR_BLD)
	$(LN) $(LNHOPT) "$<" $(htick_DOCDIR_BLD)

$(htick_DOCDIR_BLD):
	[ -d "$(htick_BUILDDIR)" ] || $(MKDIR) $(MKDIROPT) "$(htick_BUILDDIR)"
	[ -d "$@" ] || $(MKDIR) $(MKDIROPT) "$@"


# Install
ifdef DOCDIR
# See "all_info_install" in the root Makefile
    htick_doc_install: all_info_install htick_install_html htick_install_txt \
                       htick_install_dvi htick_install_pdf ;
else
    htick_doc_install: all_info_install ;
endif

ifeq ($(HTML), 1)
    htick_install_html: $(DOCDIR_DST)htick.html ;

    $(DOCDIR_DST)htick.html: \
    $(htick_BUILDDIR)htick.html | $(DOCDIR_DST)
		$(INSTALL) $(IMOPT) "$<" "$|"; \
		$(TOUCH) "$@"
else
    htick_install_html: ;
endif

ifeq ($(TXT), 1)
    htick_install_txt: $(DOCDIR_DST)htick.txt ;

    $(DOCDIR_DST)htick.txt: \
    $(htick_BUILDDIR)htick.txt | $(DOCDIR_DST)
		$(INSTALL) $(IMOPT) "$<" "$|"; \
		$(TOUCH) "$@"
else
    htick_install_txt: ;
endif

ifeq ($(DVI), 1)
    htick_install_dvi: $(DOCDIR_DST)htick.dvi ;

    $(DOCDIR_DST)htick.dvi: \
    $(htick_BUILDDIR)htick.dvi | $(DOCDIR_DST)
		$(INSTALL) $(IMOPT) "$<" "$|"; \
		$(TOUCH) "$@"
else
    htick_install_dvi: ;
endif

ifeq ($(PDF), 1)
    htick_install_pdf: $(DOCDIR_DST)htick.pdf ;

    $(DOCDIR_DST)htick.pdf: \
    $(htick_BUILDDIR)htick.pdf | $(DOCDIR_DST)
		$(INSTALL) $(IMOPT) "$<" "$|"; \
		$(TOUCH) "$@"
else
    htick_install_pdf: ;
endif


# Clean
htick_doc_clean: htick_clean_DOC
	-[ -d "$(htick_DOCDIR_BLD)" ] && $(RMDIR) $(htick_DOCDIR_BLD) || true

htick_clean_DOC:
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.fns
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.aux
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.toc
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.cp
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.fn
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.vr
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.kr
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.ke
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.tp
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.kes
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.krs
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.ky
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.pg
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.cps
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)*.log
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.texi
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.dvi
	-$(RM) $(RMOPT) $(htick_DOCDIR_BLD)htick.pdf

# Distclean
htick_doc_distclean: htick_doc_clean
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick*dvi
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick.inf
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick*html
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick.ps
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick.doc
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick*txt
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick.pdf
	-$(RM) $(RMOPT) $(htick_BUILDDIR)htick.info*


# Uninstall
ifdef DOCDIR
    htick_doc_uninstall: htick_info_uninstall htick_docs_uninstall ;
else
    htick_doc_uninstall: htick_info_uninstall ;
endif

ifdef INFODIR
# See "all_info_uninstall" in the root Makefile
    htick_info_uninstall: all_info_uninstall
		$(RM) $(RMOPT) $(INFODIR_DST)htick.info.gz
else
    htick_info_uninstall: ;
endif

htick_docs_uninstall:
ifeq ($(HTML), 1)
	-$(RM) $(RMOPT) $(DOCDIR_DST)htick.html
endif
ifeq ($(TXT), 1)
	-$(RM) $(RMOPT) $(DOCDIR_DST)htick.txt
endif
ifeq ($(DVI), 1)
	-$(RM) $(RMOPT) $(DOCDIR_DST)htick.dvi
endif
ifeq ($(PDF), 1)
	-$(RM) $(RMOPT) $(DOCDIR_DST)htick.pdf
endif
